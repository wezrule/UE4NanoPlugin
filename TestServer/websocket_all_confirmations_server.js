// This connects to the nano node and subscribes to all confirmation callback responses via websocket.
// For every block confirmed it will relay it to listening clients. Can be expensive if there are
// many clients, consider only using it for things like TPS visualisers. 
const config = require("./config");

const ReconnectingWebSocket = require("reconnecting-websocket");

// Create a reconnecting WebSocket to the node.
const WS = require("ws");
const ws = new ReconnectingWebSocket(config.node.ws_address, [], {
  WebSocket: WS,
  connectionTimeout: 1000,
  maxRetries: 100000,
  maxReconnectionDelay: 2000,
  minReconnectionDelay: 10, // if not set, initial connection will take a few seconds by default
});

// Act as a websocket server for Unreal Engine clients
const WebSocket = require("ws");
const ws_server = new WebSocket.Server({
  host: config.websocket_hostname,
  port: config.websocket.all_conf_host_port,
});

// Register subscription with the node
ws.onopen = () => {
  console.log("Connected with node");

  const confirmation_subscription = {
    action: "subscribe",
    topic: "confirmation",
  };

  ws.send(JSON.stringify(confirmation_subscription));

  // The node sent us a message
  ws.onmessage = (msg) => {
    data_json = JSON.parse(msg.data);

    // Check if this websocket connection is listening on this account
    if (data_json.topic === "confirmation") {
      // Send the whole thing we received to the client if they are listening
      for (let client of ws_server.clients) {
        if (client.readyState === WebSocket.OPEN) {
          client.send(msg.data);
        }
      }
    }
  };
};

ws.onerror = (e) => {
  console.log(e);
};

console.log(`[Expensive] All confirmation websocket server listening on: ws://${config.websocket_hostname}:${config.websocket.all_conf_host_port}`);

module.exports = ws;
