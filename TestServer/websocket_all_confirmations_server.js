const config = require('./config');

console.log("Do not use in production!!!!!!!!!!!!");

const ReconnectingWebSocket = require('reconnecting-websocket');

// Create a reconnecting WebSocket to the node.
const WS = require('ws');
console.log (config.node.ws_address);
const ws = new ReconnectingWebSocket(config.node.ws_address, [], {
    WebSocket: WS,
    connectionTimeout: 1000,
    maxRetries: 100000,
    maxReconnectionDelay: 2000,
    minReconnectionDelay: 10 // if not set, initial connection will take a few seconds by default
});

// Act as a websocket server for Unreal Engine clients
const WebSocket = require('ws');
const ws_server = new WebSocket.Server({
    port: config.websocket.all_conf_host_port
});

// Register subscription with the node
ws.onopen = () => {
	console.log ("Connected with node");

	const confirmation_subscription = {
		"action": "subscribe",
		"topic": "confirmation"
	}

     ws.send(JSON.stringify(confirmation_subscription ));

    // Listen for Unreal Engine clients connecting to us
    ws_server.on('connection', function connection(ws_server) {
	console.log ("connected to use");

        // The node sent us a message
        ws.onmessage = msg => {
		console.log ("Node sent up an email");
            data_json = JSON.parse(msg.data);

            // Check if this websocket connection is listening on this account
            if (data_json.topic === "confirmation") {
                // Send the whole thing we received to the client if they are listening
                ws_server.send(msg.data);
            }
        };
    });
}

ws.onerror = e => {
	console.log (e);
};

