# UE4NanoPlugin
Plugin for integrating Nano micropayments into Unreal Engine 4.

Features include:
- Functions for processing blocks, creating seeds + reading/saving them to disk in password-protected encrypted files. Generating qr codes, listening for websocket confirmation on the network. Demo nodejs servers

There are various helpful videos, I suggest watching them in this order:
- Shows the plugin integrated with the ActionRPG sample game https://www.youtube.com/watch?v=gMtzOkaNnXc
- Video tutorial showing the test level: TBD
- Video tutorial showing how to integrate in the ActionRPG shown above: TBD

Tested with the UE editor 4.25.3 with VS 2019 on Windows & Xcode on MacOSX 10.15.7
Also tested the plugin on Linux/Android/iOS
Other platforms like consoles may work but have not been tested.

Limitations
- The test servers should not be used in production due to lack of security/ddos protection. Likely canditates for a future version are the NanoRPCProxy.
- Only supports state blocks (v1)
- RawToNano blueprint function does not check locale and always outputs a decimal point (.), but NanoToRaw does accept both commar (,) and period (.).
- File permissions are always requested on Android/iOS. Also unsure if seed fields won't always remain after uninstalling the app.
- Not tested on other platforms like consoles

How to use:
1. Copy `Plugins` folder to an Unreal Engine project root directory.   
2. A node is required to be running which the websocket & http server (for RPC request) will talk with. websocket and rpc should be enabled in the config. 
3. A http server (for RPC requests) is definitely needed for communicating with the nano node via JSON-RPC, a test node.js server is supplied for this called `server.js`. A websocket server to receive notifications from nano network is highly recommended to make the most use of the plugin functionality. Another test server called `websocket_node.js` is supplied for this, both are found in the ./TestServer directory. The websocket script makes 2 connections to the node, 1 is filtered output and 1 gets all websocket events (usual for visualisers). If you only need filtered output (recommended!) then disable  sends 2 To make use of it:  
Modify `config.js` settings to be applicable for your system.
Run the nano_node after enabling rpc and websocket in `config-node.toml` file.
Run: `npm install`
`./server.js`

There are publically available server scripts found at 92.216.164.23 which are currently configured in the test level. Do not abuse this generiosity otherwise you will get IP banned!

which is useful for listening    to all websocket traffic (like for tps visualisers). A `config.js` file is where the server settings go for this. Run `npm install` to install dependencies.  

To check everything is set up, try the TestLevel Unreal engine map (see the second video above for details). `BP_PlayerState` contains `NanoWebsocket` & `NanoManager` variables which are set in the level blueprint and used throughout. NanoManager has the required http server details which need changing.

Implementation details:  
`NanoBlueprintLibrary` contains various generic functions such as creating seeds, encrypting/decrypting them using AES with a password, converting to accounts, converting between Raw and Nano and various other things.  
`NanoManager` is where the other functionality is located

// TODOS!
// Must haves for all setups:
- Connect to RPC

// If wanting websockets (recommended)
- Connect to Websocket
- Call SetupListener

Anything taking a Websocket argument doesn't require explicit listening, if it doesn't, then you must listen via websocket!



Recommendation setups for:
- Arcade machines

- Single player

- Multiplayer

// Convert Nano to raw

// Convert raw to nano

// Compare raw

// Create a seed

// Loop through  all seed files on the system

// Save a seed

// Generate a QRCode

// Listen for all websocket confirmations

// Hook into all filtered confirmation websocket events

// Listen for a payment on an account (only checks pending, and may return if there are any pending blocks >= to the amount listening for)

### Utility functions 
// Cancel a payment
// Listen for a payout (with expiry!) when all funds are taken from a QRCode
// Cancel a payout
// Send without waiting for confirmation
// Send waiting for confirmation
// Receive
// There is currently no ReceiveWaitConfirmation, this is intentional because any funds sent to you are considered irreversible and can always be pocketed by other means). This should not cause the player to wait for anything. If funds can be pocketed then use the below:
// Automatically pocket funds:

### Raw functions

// Create a block:

// Process a block

// Process a send block and wait for confirmation

// Process a receive 

// Hand off a block to a server which waits for validation:

### Where are AES encrypted files stored?  
**Windows**   
`C:\Users\<user>\AppData\Local\NanoGames`

**Linux**   
`/home/<user>/NanoGames`

**Mac**   
`/Users/<user>/Library/NanoGames`

**Other (e.g Android/iOS)**  
`<project folder>/NanoGames`
