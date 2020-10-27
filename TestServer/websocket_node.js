const WS = require("ws");
const ReconnectingWebSocket = require("reconnecting-websocket");

const config = require("./config");

// Create a reconnecting WebSocket to the node.
const ws = new ReconnectingWebSocket(config.node.ws_address, [], {
  WebSocket: WS,
  connectionTimeout: 1000,
  maxRetries: 100000,
  maxReconnectionDelay: 2000,
  minReconnectionDelay: 11, // if not set, initial connection will take a few seconds by default
});

// Act as a websocket server for Unreal Engine clients
const WebSocket = require("ws");
const ws_server = new WebSocket.Server({
  host: config.websocket_hostname,
  port: config.websocket.host_port,
});

  // This stores map<ws_connection, set<string>>. The set is accounts
  let ws_account_map = new Map();

  // This stores map<account, set<ws_connection>>;
  let account_ws_map = new Map();


  // Listen for Unreal Engine clients connecting to us
  ws_server.on("connection", (client) => {
    console.log("new client registered");
    // Received a message from an Unreal Engine client
    client.on("message", (message) => {
      let json;
      try {
        json = JSON.parse(message);
      } catch (err) {
        return;
      }

      if (json.action == "register_account") {
        if (ws_account_map.has(client)) {
          ws_account_map.get(client).add(json.account);
          console.log("111");
        } else {
          ws_account_map.set(client, new Set());
          ws_account_map.get(client).add(json.account);
          console.log("1122");
        }

        if (account_ws_map.has(json.account)) {
          account_ws_map.get(json.account).add(client);
          console.log("13232");
        } else {
          account_ws_map.set(json.account, new Set());
          account_ws_map.get(json.account).add(client);
          console.log("1111111");
        }

        const confirmation_subscription_account_add = {
          action: "update",
          topic: "confirmation",
          options: {
            accounts_add: [json.account],
          },
        };

        ws.send(JSON.stringify(confirmation_subscription_account_add));
      } else if (json.action == "unregister_account") {
        if (account_ws_map.has(json.account)) {
          account_ws_map.get(json.account).delete(client);
          if (account_ws_map.get(json.account).size == 0) {
            account_ws_map.delete(json.account);
            i;
          }

          ws_account_map.get(client).delete(json.account);
          if (ws_account_map.get(client).size == 0) {
            ws_account_map.delete(client);
          }

          // Is this the last reference to this account?
          if (account_ws_map.get(json.account).size == 0) {
            const confirmation_subscription_account_delete = {
              action: "update",
              topic: "confirmation",
              options: {
                accounts_del: [json.account],
              },
            };

            ws.send(JSON.stringify(confirmation_subscription_account_delete));
          }
        }
      }
    });

		// An UE client connection is lost
    ws_server.on("close", (client) => {
			// Loop through all accounts this connection was listening to and delete as appropriate. Unregister from node websocket subcription if there are no more listeners to this account
	if (ws_account_map.has(client)) {
        for (let account of ws_account_map.get(client)) {
          account_ws_map.get(account).delete(client);
          if (account_ws_map.get(account).size == 0) {
						account_ws_map.delete(account);
						
            const confirmation_subscription_account_delete = {
              action: "update",
              topic: "confirmation",
              options: {
                accounts_del: [json.account],
              },
            };

	console.log ("Unregister from client");

            ws.send(JSON.stringify(confirmation_subscription_account_delete));
          }

          ws_account_map.get(client).delete(account);
          if (ws_account_map.get(client).size == 0) {
            ws_account_map.delete(client);
          }
        }
      }
    });
  });

// Connected with the node
ws.onopen = () => {
  console.log("Connected with node (manual registering of account)");

  const confirmation_subscription = {
    action: "subscribe",
    topic: "confirmation",
    options: {
      accounts: "",
    },
  };
  // Send empty list of accoutns just to get the subscription
  ws.send(JSON.stringify(confirmation_subscription));

  // The node sent us a message
  ws.onmessage = (msg) => {
    data_json = JSON.parse(msg.data);
    // Check if this websocket connection is listening on this account
    if (data_json.topic === "confirmation") {
      // Send the whole thing we received to the client if they are listening
      let clients = new Set();
      if (account_ws_map.has(data_json.message.account)) {
        for (const client of account_ws_map.get(data_json.message.account)) {
          clients.add(client);
          console.log("client add");
        }
      }
      if (account_ws_map.has(data_json.message.block.link_as_account)) {
        for (const client of account_ws_map.get(
          data_json.message.block.link_as_account
        )) {
          clients.add(client);
          console.log("client add123");
        }
      }

      data_json.is_filtered = false;
      for (const client of clients) {
        client.send(JSON.stringify(data_json));
      }
    }
  };
};

ws.onerror = (e) => {
  // Error connecting to node
  console.log(e);
};

if (config.allow_listen_all) {
  // Create a reconnecting WebSocket to the node for listening to all confirmation requests
  const WS_listen_all = require("ws");
  const ws_listen_all = new ReconnectingWebSocket(config.node.ws_address, [], {
    WebSocket: WS_listen_all,
    connectionTimeout: 1000,
    maxRetries: 100000,
    maxReconnectionDelay: 2000,
    minReconnectionDelay: 10, // if not set, initial connection will take a few seconds by default
  });


    // This stores all UE websocket clients which have connected to us
    let clients = new Set();

  ws_listen_all.onopen = () => {
    console.log("Connected with node (all confirmations)");


    // Subscribe to all confirmation callbacks
    const confirmation_subscription = {
      action: "subscribe",
      topic: "confirmation",
    };

    ws_listen_all.send(JSON.stringify(confirmation_subscription));

    // The node sent us a message
    ws_listen_all.onmessage = (msg) => {
      data_json = JSON.parse(msg.data);
      // Check if this websocket connection is listening on this account
      if (data_json.topic === "confirmation") {
        // Send the whole thing we received to the client if they are listening.
        data_json.is_filtered = false;

        for (const client of clients) {
          client.send(JSON.stringify(data_json));
        }
      }
    };

    // Listen for Unreal Engine clients connecting to us
    ws_server.on("connection", (client) => {
      console.log("new client registered (all conf)");
      // Received a message from an Unreal Engine client
      client.on("message", (message) => {
        let json;
        try {
          json = JSON.parse(message);
        } catch (err) {
          return;
        }

        if (json.action == "listen_all") {
          if (!config.allow_listen_all) {
            return;
					}
					clients.add (client);
        } else if (json.action == "unlisten_all") {
          if (!config.allow_listen_all) {
            return;
					}
					if (clients.has (client)) {
						clients.delete (client);
					}
        }
      });

      ws_server.on("close", (client) => {
				if (clients.has (client)) {
					console.log ("cleared all conf client");
					clients.delete (client);
				}
      });
    });
  };

  ws_listen_all.onerror = (e) => {
    // Error connecting to node
    console.log(e);
  };
}

console.log(
  `Websocket server listening on: ws://${config.websocket_hostname}:${config.websocket.host_port}`
);

module.exports = ws;
