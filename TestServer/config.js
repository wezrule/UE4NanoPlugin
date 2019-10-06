var config = {};

// The port which calls from Unreal Engine plugin clients will call to access RPC commands indirectly and other things.
config.host_port = 6666;

config.websocket = {}
config.websocket.host_port = 6667;

config.node = {};
// Websocket address + port of the node
config.node.ws_address = "ws://192.168.1.8:7078"

// Wallet which can be used for requesting Nano, like a faucet
config.node.wallet = "5C9B72D16ACD64FFD49E420CF4CF962895C57FA679C589045B3B34C7CD98E59B";
config.node.source = "nano_191o3sb8a3qhnnxcmrbc1y7urei73e3z7m9cg3bzwz4u5j4m43j9p76pwkcc"; 
config.node.request_raw_amount = "10000000000000000000000000";

config.node.rpc = {};
config.node.rpc.address = "192.168.1.8";
config.node.rpc.port = 7076;

module.exports = config;
