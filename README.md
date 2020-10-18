# UE4NanoPlugin
Plugin for integrating Nano micropayments into Unreal Engine 4.

Features include:
- Functions for processing blocks, creating seeds + reading/saving them to disk in password-protected encrypted files. Generating qr codes, listening for websocket confirmation on the network. Demo nodejs servers

There are various helpful videos, I suggest watching them in this order:
- Shows the plugin integrated with the ActionRPG sample game https://www.youtube.com/watch?v=gMtzOkaNnXc
- Video tutorial showing the test level: TBD
- Video tutorial showing how to integrate in the ActionRPG shown above: TBD

Tested with UE 4.25.3 with VS 2019 & MacOSX 10.14. Cross-compiled from Windows onto Linux with Clang. Also ran on android, but iOS is TBD. 

Limitations
- QRCode supports only account, or account + amount
- Servers should not be used in production due to security implications.
- Only supports state blocks (v1)
- Supports only decimal point (.) in NanoToRaw/RawToNano blueprint functions
- File permissions are always requested on Android/iOS. Also unsure if seed fields won't always remain after uninstalling the app.
- Not tested on other platforms like consoles

How to use:
1. Copy `Plugins` folder to an Unreal Engine project root directory.   
2. A http server (for RPC requests) & a websocket server (to receive notifications from nano network) are needed. A sample node.js file is supplied (`server.js`) for showing how to communicate with a node and register/watch specific accounts via websocket. There is also another file `websocket_all_confirmations_server.js` which is useful for listening    to all websocket traffic (like for tps visualisers). A `config.js` file is where the server settings go for this. Run `npm install` to install dependencies.  
3. A node is required to be running which the websocket & http server (for RPC request) will talk with. websocket and rpc should be enabled in the config. 

To check everything is set up, try the TestLevel Unreal engine map (see the second video above for details). `BP_PlayerState` contains `NanoWebsocket` & `NanoManager` variables which are set in the level blueprint and used throughout. NanoManager has the required http server details which need changing.

Implementation details:  
`NanoBlueprintLibrary` contains various generic functions such as creating seeds, encrypting/decrypting them using AES with a password, converting to accounts, converting between Raw and Nano and various other things.  
`NanoManager` is where the other functionality is located

### Where are AES encrypted files stored?  
**Windows**   
`C:\Users\<user>\AppData\Local\Nano_Games`

**Linux**   
`/home/<user>/Nano_Games`

**Mac**   
`/Users/<user>/Library/Nano_Games`

**Other (e.g Android/iOS)**  
`<project folder>/Nano_Games`
