# UE4NanoPlugin
Plugin for integrating the Nano cryptocurrency into the Unreal Engine 4.

Tested on Windows/Linux/MacOS/Android/iOS with Unreal Enginge 4.25.3

Features include: functions for processing blocks, creating seeds + reading/saving them to disk in password-protected encrypted files. Generating qr codes, listening for payments to single accounts, private key payouts and many more

### How to test the plugin:
1 - `git clone https://github.com/wezrule/UE4NanoPlugin`  
2 - Open the project `UENano.uproject` (if there is an error then right click and generate project files, open up and build manually)  
3 - The rpc/websocket settings are located in NanoGameInstance blueprint (leave unchanged to use the publically available servers)  
4 - Run the map, first create a seed then top up the account and try all the other functions (a video is coming soon demonstrating this)  

The test level:  
![Windows1](https://user-images.githubusercontent.com/650038/97644668-d514b580-1a42-11eb-9e21-4c65283132c1.PNG)  

### How to add the plugin to your project
1. Copy `Plugins` folder to an Unreal Engine project root directory.  
2. Build your project   

### To run your own test servers (recommended for any intense testing)
1. Modify the `config.js` settings to be applicable for your system.
2. Run the nano_node after enabling rpc and websocket in `config-node.toml` file.
3. `nano_node --daemon`
4. `npm install`
5. `./server.js`

A nano node is required to be running which the websocket & http server (for RPC request) will talk with. Websocket & RPC should be enabled in the `node-config.toml` nano file. 
A http server (for RPC requests) is definitely needed for communicating with the nano node via JSON-RPC, a test node.js server is supplied for this called `server.js`. A websocket server to receive notifications from nano network is highly recommended to make the most use of the plugin functionality. Another test server called `websocket_node.js` is supplied for this, both are found in the ./TestServer directory. Running `server.js` will also run `websocket_node.js`. The websocket script makes 2 connections to the node, 1 is filtered output and 1 gets all websocket events (usual for visualisers). If you only need filtered output (recommended!) then disable `allow_listen_all=false` in `config.js`.  

`NanoBlueprintLibrary.cpp` contains various generic functions such as creating seeds, encrypting/decrypting them using AES with a password, converting to accounts, converting between Raw and Nano and various other things.  
`NanoWebsocket.cpp` maintains the websocket connection to the proxies.
`NanoManager.cpp` is where the other functionality is located

### Some notes when  hooking the plugin into your game
Anything with \*WaitForConfirmation in the name requires that the account being utilised has been `Watch`ed. This means that the websocket filtered connection is listening for events for this account, this creates a better user experience as websocket events are received as soon as the node processing them. All these functions have fallback methods which involve polling the node periodically (generally every 5 seconds). Anything taking a Websocket argument doesn't require explicit listening (such as the listening payment and payout functions). For watching (and unwatching an account):  
![WatchUnwatch](https://user-images.githubusercontent.com/650038/97642737-d0013780-1a3d-11eb-81a0-eea16d8d5547.PNG)

All setups should set the rpc url, the plugin is pretty useless without an RPC connection, set it after creating the object:
![NanoManagerConstruct](https://user-images.githubusercontent.com/650038/97642660-a6e0a700-1a3d-11eb-80a9-1e088555b3d5.PNG)

Following that construct the websocket and call `SetupFilteredConfirmationMessageWebsocketListener`, the last step is crucial as it listens to the websocket and fires off the various delegates which are bounded. The websocket connection will continuously attempt to reconnect if connection is lost, but will never call OnConnect again (this is to prevent some re-initialization errors seen in plugin implementations)
![NanoWebsocketConstruct](https://user-images.githubusercontent.com/650038/97642680-b2cc6900-1a3d-11eb-96df-87bea639d773.PNG)

Always check the `Error` boolean in all event responses, e.g:  
![errors](https://user-images.githubusercontent.com/650038/97644190-9d593e00-1a41-11eb-8547-c813d71d38e8.PNG)  

Recommendation setups for:
#### Arcade machines
Listen to payment - Have 1 seed per arcade machine, start at first index then increment each time a payment is needed. This only checks pending blocks, don't have anything else pocketing these funds automatically. Every time a new payment is needed, move to the next index. Only 1 payment can be listening at any 1 time!  
Create a QR code for the account/amount required:  
![CreateQRcodeAccount](https://user-images.githubusercontent.com/650038/97642695-b9f37700-1a3d-11eb-856c-a6f8ce0a4ce7.PNG)  
It looks like this:  
![qrcode](https://user-images.githubusercontent.com/650038/97644667-d3e38880-1a42-11eb-99dc-135c0a355ac9.PNG)  
Then listen for the payment:  
![ListenForPayment](https://user-images.githubusercontent.com/650038/97642704-be1f9480-1a3d-11eb-8f3b-dcefcdb9f189.PNG)  
If wanting to cancel it:  
![ListenPaymentCancel](https://user-images.githubusercontent.com/650038/97642706-bfe95800-1a3d-11eb-81e0-b70944e628be.PNG)  
For payouts do a similar process with showing a QR Code (use the variant taking a private key), and listen for payout:  
![ListenPayoutAndCancel](https://user-images.githubusercontent.com/650038/97642711-c11a8500-1a3d-11eb-850b-423874087fba.PNG)  

#### Single player  
Create seed for player and store it encrypted with password (also check for local seed files if they want to open them)  
![CreateSeed](https://user-images.githubusercontent.com/650038/97642697-bb24a400-1a3d-11eb-9e61-1af7f5b915b8.PNG)  

Loop through seed files:  
![LoopThroughSeedFiles](https://user-images.githubusercontent.com/650038/97642715-c4157580-1a3d-11eb-9401-49d031e6a768.PNG)  

Save seed:  
![Save seed](https://user-images.githubusercontent.com/650038/97642731-cf68a100-1a3d-11eb-8f77-22e0cabc01a3.PNG)  

Send and wait for confirmation:  
![sendwaitconfirmation](https://user-images.githubusercontent.com/650038/97644323-eb6e4180-1a41-11eb-9975-930305f6d581.PNG)  

#### Multiplayer  
Process seed (as above)
Create block locally and hand off to server  
![Step1Server](https://user-images.githubusercontent.com/650038/97642733-d0013780-1a3d-11eb-8dd1-c884e321253d.PNG)  
Server does validation (checks block is valid) then does appropriate action  
![Step2Server](https://user-images.githubusercontent.com/650038/97642734-d0013780-1a3d-11eb-9906-375d920412df.PNG)  

### Utility functions 
Automatically pocket any pending funds
![AutomaticallyPocketUnpocket](https://user-images.githubusercontent.com/650038/97642693-b829b380-1a3d-11eb-956f-c7b3ea57986a.PNG)

Listen to all websocket confirmations
![ListenUnlistenAllConfirmation](https://user-images.githubusercontent.com/650038/97642714-c24bb200-1a3d-11eb-9a53-735eb3d6f395.PNG)  

### Other functions

Create a block:  
![MakeBlock](https://user-images.githubusercontent.com/650038/97642721-c8419300-1a3d-11eb-9b0e-0b7efb522bdb.PNG)  

Generate work:  
![WorkGenerate](https://user-images.githubusercontent.com/650038/97642739-d099ce00-1a3d-11eb-89dc-b372e3c0c7b6.PNG)  

Get pending blocks:  
![Pending](https://user-images.githubusercontent.com/650038/97642727-ced00a80-1a3d-11eb-939d-e9c7bab832b0.png)  

Process a block:  
![ProcessWithBlock](https://user-images.githubusercontent.com/650038/97642728-cf68a100-1a3d-11eb-935d-be4ea96a21a8.PNG)  

Get the account frontier (Note: is the account doesn't exist yet, the frontier points to the account, if wanting to use this result as a "previous", need to check if it exists the account and if so branch and change to 0!:
![AccountFrontier](https://user-images.githubusercontent.com/650038/97642689-b5c75980-1a3d-11eb-8b2e-b5c91837f850.PNG)

Getting a wallet balance:  
![GetWalletBalance](https://user-images.githubusercontent.com/650038/97642701-bc55d100-1a3d-11eb-86af-d1a888440359.PNG)

Checking if a block is confirmed:  
![blockConfirmed](https://user-images.githubusercontent.com/650038/97644122-61be7400-1a41-11eb-81a1-d91ac51eece0.PNG)  

### Nano unit functions 
![NanoUnitFuncs](https://user-images.githubusercontent.com/650038/97642723-caa3ed00-1a3d-11eb-94cc-e9559f4744a3.PNG)  

### Debug only (unless making a faucet)  
Request nano:  
![requestnano](https://user-images.githubusercontent.com/650038/97642730-cf68a100-1a3d-11eb-9b0a-94b28561f7fa.PNG)  

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

Working on various other platforms:  
![AndroidPlay](https://user-images.githubusercontent.com/650038/97642692-b6f88680-1a3d-11eb-886e-aa5d97fa5bef.png)  
![Screenshot from 2020-10-29 17-42-33](https://user-images.githubusercontent.com/650038/97679453-2ea1d200-1a8d-11eb-9898-c1929fdac4c5.png)
![MacEditor](https://user-images.githubusercontent.com/650038/97642718-c677cf80-1a3d-11eb-82cc-9b660abb6ab7.png)  

You can of course also use this in C++ projects.

Any donation contributions are welcome: nano_15qry5fsh4mjbsgsqj148o8378cx5ext4dpnh5ex99gfck686nxkiuk9uouy
![download](https://user-images.githubusercontent.com/650038/97703969-70d90c80-1aa9-11eb-80b6-30bfad6dce31.png)
