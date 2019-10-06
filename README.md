# UE4NanoPlugin
Plugin for integrating Nano micropayments into Unreal Engine 4

**Currently in Beta. Only tested on Windows Desktop, do not use in production.**

How to use:
1. Copy `Plugins` folder to an Unreal Engine project root directory
2. A http server (for RPC requests) & a websocket server (to receive notifications from nano network) are needed. A sample node.js file is supplied (`server.js`) which sets this up. `config.js` is where the server settings go for this.  
3. A node is required to be running which the websocket & http server (for RPC request) will talk with. websocket and rpc should be enabled in the config. 

To check everything is set up, try the TestLevel Unreal engine map. `BP_PlayerState` contains `NanoWebsocket` & `NanoManager` components. NanoManager has the required http server details which need changing. A video demonstration is coming soon.

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
