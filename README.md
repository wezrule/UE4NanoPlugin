# UE4NanoPlugin
Plugin for integrating the Nano cryptocurrency into the Unreal Engine 4.

Tested on Windows/Linux/MacOS/Android/iOS with Unreal Enginge 4.25.3

Features include: functions for processing blocks, creating seeds + reading/saving them to disk in password-protected encrypted files. Generating qr codes, listening for payments to single accounts, private key payouts and many more

### How to test the plugin:
1 - `git clone https://github.com/wezrule/UE4NanoPlugin`  
2 - Open the project `UENano.uproject` (if there is an error then right click and generate project files, open up and build manually)  
3 - The rpc/websocket settings are located in NanoGameInstance blueprint (leave unchanged to use the publically available servers)  
4 - Run the map, first create a seed then top up the account and try all the other functions (a video is coming soon demonstrating this)  

### How to add the plugin to your project
1. Copy `Plugins` folder to an Unreal Engine project root directory.  
2. Build your project   

#### To run your own test servers (recommended for any intense testing)
1. Modify the `config.js` settings to be applicable for your system.
2. Run the nano_node after enabling rpc and websocket in `config-node.toml` file.
3. `nano_node --daemon`
4. `npm install`
5. `./server.js`

A nano node is required to be running which the websocket & http server (for RPC request) will talk with. Websocket & RPC should be enabled in the `node-config.toml` nano file. 
A http server (for RPC requests) is definitely needed for communicating with the nano node via JSON-RPC, a test node.js server is supplied for this called `server.js`. A websocket server to receive notifications from nano network is highly recommended to make the most use of the plugin functionality. Another test server called `websocket_node.js` is supplied for this, both are found in the ./TestServer directory. Running `server.js` will also run `websocket_node.js`. The websocket script makes 2 connections to the node, 1 is filtered output and 1 gets all websocket events (usual for visualisers). If you only need filtered output (recommended!) then disable `allow_listen_all=false` in `config.js`. T  

`NanoBlueprintLibrary.cpp` contains various generic functions such as creating seeds, encrypting/decrypting them using AES with a password, converting to accounts, converting between Raw and Nano and various other things.  
`NanoWebsocket.cpp` maintains the websocket connection to the proxies.
`NanoManager.cpp` is where the other functionality is located

### Some notes to hooking the plugin into your game
Anything with \*WaitForConfirmation in the name requires that the account being utilised has been `Watch`ed. This means that the websocket filtered connection is listening for events for this account, this creates a better user experience as websocket events are received as soon as the node processing them. All these functions have fallback methods which involve polling the node periodically (generally every 5 seconds). Anything taking a Websocket argument doesn't require explicit listening (such as the listening payment and payout functions).

All setups should set the rpc url, the plugin is pretty useless without an RPC connection, set it after creating the object:

Following that construct the websocket and call `SetupFilteredConfirmationMessageWebsocketListener`, the last step is cruical as it listens to the websocket and fires off the various delegates which are listening. OnConnect in this case will only be called once ever. The websocket connection will continuously attempt to reconnect if connection is lost, but will never call OnConnect again (this is to prevent some re-initialization errors seen in plugin implementations)

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

There are various helpful videos, I suggest watching them in this order:
- Shows the plugin integrated with the ActionRPG sample game https://www.youtube.com/watch?v=gMtzOkaNnXc  
- Video tutorial showing the test level: Coming soon  
- Video tutorial showing how to integrate into a game: Coming soon  

Limitations
- The test servers should not be used in production due to lack of security/ddos protection. Likely canditates for a future version are the NanoRPCProxy.
- Only supports state blocks (v1)
- RawToNano blueprint function does not check locale and always outputs a decimal point (.), but NanoToRaw does accept both commar (,) and period (.).
- File permissions are always requested on Android/iOS. Also unsure if seed fields won't always remain after uninstalling the app.
- Not tested on other platforms like consoles
- UNanoManager & UWebsocketManager must be GameInstance variables to maintain connections between levels.

### Where are AES encrypted files stored?  
**Windows**   
`C:\Users\<user>\AppData\Local\NanoGames`

**Linux**   
`/home/<user>/NanoGames`

**MacOS**   
`/Users/<user>/Library/NanoGames`

**Other (e.g Android/iOS)**  
`<project folder>/NanoGames`
