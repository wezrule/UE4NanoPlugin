console.log("Do not use in production!!!!!!!!!!!!");

const express = require("express");
const config = require("./config");
const fetch = require("node-fetch")
const bodyParser = require("body-parser");
const app = express();
const hostname = config.hostname;
const port = config.host_port;

const nodeWs = require ("./websocket_node"); // Comment out if you don't need to register specific accounts for websocket confirmations.

const nodeUrl = "http://" + config.node.rpc.address + ":" + config.node.rpc.port;

// Send request to the RPC server of the node and record response
const send = json_string => {
	return new Promise( (resolve) => {
    fetch(nodeUrl, {
        method: 'POST',
        body:    json_string,
        headers: { 'Content-Type': 'application/json', "Accepts":"application/json" },
	
    })
    .then(res => res.json())
    .then(json => {
        resolve (json);
    }).catch(error => {
	resolve({error: error});
    })
});
};

const WS = require("ws");
 const ReconnectingWebSocket = require("reconnecting-websocket");
const dpow_wss = new ReconnectingWebSocket(config.dpow.ws_address, [], {
  WebSocket: WS,
  connectionTimeout: 1000,
  maxRetries: 100000,
  maxReconnectionDelay: 2000,
  minReconnectionDelay: 10, // if not set, initial connection will take a few seconds by default
});

let id = 0;
let using_dpow = false;
let dpow_request_map = new Map();
dpow_wss.onopen = () => {
  using_dpow = true;

  // dPOW sent us a message
  dpow_wss.onmessage = (msg) => {
    let data_json = JSON.parse(msg.data);
    let value_l = dpow_request_map.get(data_json.id);
    if (typeof value_l !== 'undefined') {
    if (!data_json.hasOwnProperty("error")) {
      let new_response = {};
      new_response.work = data_json.work;
      new_response.hash = value_l.hash;
      value_l.res.json(new_response);
    } else {
      // dPOW failed to create us work, ask the node to generate it for us (TODO untested)
      let request = {};
      request.action = "work_generate";
      request.hash = value_l.hash;

      send(JSON.stringify(request))
        .then((rpc_response) => {
          value_l.res.json(JSON.stringify(rpc_response));
        })
        .catch((e) => {
          value_l.res.json({"error":"1"});
        });
    }
    delete dpow_request_map.get(data_json.id);
  };
  }
};

app.use(bodyParser.json());

app.post("/", (req, res) => {
  res.setHeader("Content-Type", "application/json");
  let obj;
  try {
	  obj = req.body;
  } catch (e) {
	  res.json({"error":"malformed json request"});
    return;
  }

  let action = obj.action;

  // The only RPC commands which are allowed. If dpow is not working, it will use the node to generate work.
  // May want to set up a GPU work server if not using dpow!!
  const allowed_actions = [
    "block_count",
    "account_info",
    "account_balance",
    "block_info",
    "pending",
    "process",
    "work_generate",
  ];
  // This is an optional action for development/faucet purposes
  if (action == "request_nano") {
    // The node generates this work so can be slow if calling before pre-generation is done
    let send_obj = {};
    send_obj.action = "send";
    send_obj.wallet = config.node.wallet;
    send_obj.source = config.node.source;
    send_obj.destination = obj.account;
    send_obj.amount = config.node.request_raw_amount;
    let send_rpc_response;
    send(JSON.stringify(send_obj))
      .then((rpc_response) => {
        send_rpc_response = JSON.parse(rpc_response);

        let account_info_obj = {};
        account_info_obj.action = "account_info";
        account_info_obj.account = obj.account;

        send(JSON.stringify(account_info_obj)).then(
          (account_info_response_str) => {
            let account_info_response = JSON.parse(account_info_response_str);
            let new_response = {};
            new_response.account = obj.account;
            new_response.amount = config.node.request_raw_amount;
            new_response.send_hash = send_rpc_response.block;
            if (account_info_response.hasOwnProperty("error")) {
              // account doesn't exist so hasn't been opened yet
              new_response.frontier = "0";
            } else {
              new_response.frontier = account_info_response.frontier;
            }

            res.json(new_response);
          }
        );
      })
      .catch((e) => {
        res.json({"error":"1"});
      });
  } else if (action == "work_generate" && config.dpow.enabled && using_dpow) {
    // Send to the dpow server and return the same request
    ++id;
    const dpow_request = {
      user: config.dpow.user,
      api_key: config.dpow.api_key,
      hash: obj.hash,
      id: id,
    };

    let value = {};
    value.res = res;
    value.hash = obj.hash;
    dpow_request_map.set(id, value);

    // Send a request to the dpow server
    dpow_wss.send(JSON.stringify(dpow_request));
  } else if (allowed_actions.includes(action)) {
    // Just forward to RPC server. TODO: Should probably check for validity
    send(JSON.stringify (obj))
	.then((rpc_response_json) => {
	res.json(rpc_response_json);
	})
      .catch((e) => {
	      res.json({"error":"1"});
      });
  } else {
    res.json({"error":"1"});
  }
});

app.listen(port, hostname, () => {
  console.log(`JSON-RPC server listening on port http://${hostname}:${port}`);
});

