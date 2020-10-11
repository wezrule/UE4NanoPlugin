#include "NanoManager.h"
#include "Engine.h"
#include "Runtime/Online/HTTP/Public/HttpModule.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "Json.h"
#include "JsonObjectConverter.h"
#include <nano/blocks.h>
#include <nano/numbers.h>
#include "NanoBlueprintLibrary.h"
#include <baseconverter/base_converter.hpp>

#include <cassert>

#include <ed25519-donna/ed25519.h>

#define RETURN_ERROR_IF_INVALID_RESPONSE(Data) \
	if (!RequestResponseIsValid (request, response, wasSuccessful)) { \
		Data.error = true; \
		return Data; \
	} \
\
	auto reqRespJson = GetResponseJson(request, response, wasSuccessful); \
	if (reqRespJson.response->HasField("error")) { \
		Data.error = true; \
		return Data; \
	}

#define RESPONSE_ARGUMENTS request, response, wasSuccessful

void UNanoManager::SetupReceiveMessageWebsocketListener(UNanoWebsocket* websocket) {
	websocket->onResponse.AddDynamic (this, &UNanoManager::OnReceiveMessage); // TODO: Should only call this once, but websocket needs to be initialized
}

void UNanoManager::GetWalletBalance(FGetBalanceResponseReceivedDelegate delegate, FString nanoAddress) {
	FGetBalanceRequestData getBalanceRequestData;
	getBalanceRequestData.account = nanoAddress;

	TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(getBalanceRequestData);
	MakeRequest(JsonObject, [this, delegate](RESPONSE_PARAMETERS) {
		delegate.ExecuteIfBound(GetBalanceResponseData(RESPONSE_ARGUMENTS));
	});
}

TSharedPtr<FJsonObject> UNanoManager::GetWorkGenerateJsonObject(FString hash) {
	FWorkGenerateRequestData workGenerateRequestData;
	workGenerateRequestData.hash = hash;
	return FJsonObjectConverter::UStructToJsonObject(workGenerateRequestData);
}

void UNanoManager::WorkGenerate(FWorkGenerateResponseReceivedDelegate delegate, FString hash) {
	WorkGenerate(hash, [delegate](RESPONSE_PARAMETERS) {
		delegate.ExecuteIfBound(GetWorkGenerateResponseData(RESPONSE_ARGUMENTS));
	});
}

void UNanoManager::WorkGenerate(FString hash, TFunction<void(RESPONSE_PARAMETERS)> const& d) {
	MakeRequest(GetWorkGenerateJsonObject(hash), d);
}

TSharedPtr<FJsonObject> UNanoManager::GetPendingJsonObject(FString account) {
	FPendingRequestData pendingRequestData;
	pendingRequestData.account = account;
	return FJsonObjectConverter::UStructToJsonObject(pendingRequestData);
}

void UNanoManager::Pending(FPendingResponseReceivedDelegate delegate, FString account) {
	Pending(account, [delegate](RESPONSE_PARAMETERS) {
		delegate.ExecuteIfBound(GetPendingResponseData(RESPONSE_ARGUMENTS));
	});
}

void UNanoManager::Pending(FString account, TFunction<void(RESPONSE_PARAMETERS)> const& d) {
	MakeRequest(GetPendingJsonObject(account), d);
}

TSharedPtr<FJsonObject> UNanoManager::GetAccountFrontierJsonObject(FString const& account) {
	FAccountFrontierRequestData accountFrontierRequestData;
	accountFrontierRequestData.account = account;
	return FJsonObjectConverter::UStructToJsonObject(accountFrontierRequestData);
}

TSharedPtr<FJsonObject> UNanoManager::GetBlockConfirmedJsonObject(FString const& hash) {
	FBlockConfirmedRequestData blockConfirmedRequestData;
	blockConfirmedRequestData.hash = hash;
	return FJsonObjectConverter::UStructToJsonObject(blockConfirmedRequestData);
}

void UNanoManager::AccountFrontier(FAccountFrontierResponseReceivedDelegate delegate, FString account) {
	AccountFrontier(account, [this, delegate](RESPONSE_PARAMETERS) {
		delegate.ExecuteIfBound(GetAccountFrontierResponseData(request, response, wasSuccessful));
	});
}

void UNanoManager::AccountFrontier(FString account, TFunction<void(RESPONSE_PARAMETERS)> const& delegate) {
	MakeRequest(GetAccountFrontierJsonObject(account), delegate);
}

void UNanoManager::Process(FProcessResponseReceivedDelegate delegate, FBlock block) {
	Process(block, [delegate](RESPONSE_PARAMETERS) {
		delegate.ExecuteIfBound(GetProcessResponseData(request, response, wasSuccessful));
	});
}

void UNanoManager::Process(FBlock block, TFunction<void(RESPONSE_PARAMETERS)> const& delegate) {

	nano::account account;
	account.decode_account(TCHAR_TO_UTF8(*block.account));

	nano::account representative;
	representative.decode_account(TCHAR_TO_UTF8(*block.representative));

	nano::amount balance;
	balance.decode_dec(TCHAR_TO_UTF8(*block.balance));

	nano::account link(TCHAR_TO_UTF8(*block.link));

	uint64_t work;
	nano::from_string_hex(TCHAR_TO_UTF8(*block.previous), work);

	nano::raw_key raw_key;
	raw_key.data = nano::uint256_union(TCHAR_TO_UTF8(*block.private_key));

	nano::block_hash previous(TCHAR_TO_UTF8(*block.previous));

	auto public_key (nano::pub_key (raw_key.data));

	nano::state_block state_block(account, previous, representative, balance, link, raw_key, public_key);

	FProcessRequestData processRequestData;
	FBlockRequestData blockProcessRequestData;
	blockProcessRequestData.account = block.account;
	blockProcessRequestData.balance = block.balance;
	blockProcessRequestData.previous = block.previous;
	blockProcessRequestData.representative = block.representative;
	blockProcessRequestData.link = link.to_string().c_str();
	blockProcessRequestData.signature = state_block.block_signature().to_string().c_str();
	blockProcessRequestData.work = block.work;

	processRequestData.block = blockProcessRequestData;

	TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(processRequestData);
	MakeRequest(JsonObject, delegate);
}

// Development only!
void UNanoManager::RequestNano(FReceivedNanoDelegate delegate, FString nanoAddress) {
	// Ask server for Nano
	FRequestNanoData requestNanoData;
	requestNanoData.account = nanoAddress;

	TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(requestNanoData);
	MakeRequest(JsonObject, [this, delegate](RESPONSE_PARAMETERS) {
		delegate.ExecuteIfBound(GetRequestNanoData(RESPONSE_ARGUMENTS));
	});
}

void UNanoManager::Watch(FAutomateResponseReceivedDelegate delegate, FString const& account, UNanoWebsocket* websocket) {

	// Check you haven't already added it
	assert (keyDelegateMap.find (TCHAR_TO_UTF8(*account)) == keyDelegateMap.cend ());

	websocket->RegisterAccount(account);

	// Keep a mapping of automatic listening delegates
	keyDelegateMap.emplace(std::piecewise_construct, std::forward_as_tuple(TCHAR_TO_UTF8(*account)), std::forward_as_tuple("", delegate)).first->second;
}

void UNanoManager::Unwatch(const FString& account, UNanoWebsocket* websocket) {
	websocket->UnregisterAccount(account);
	keyDelegateMap.erase(TCHAR_TO_UTF8(*account));
}

void UNanoManager::Automate(FAutomateResponseReceivedDelegate delegate, FString const& private_key, UNanoWebsocket* websocket) {
	assert (!private_key.IsEmpty ());
	nano::raw_key prv_key;
	prv_key.data = nano::uint256_union(TCHAR_TO_UTF8(*private_key));

	auto pub_key = nano::pub_key(prv_key.data);

	// Check you haven't already added it
	check (keyDelegateMap.find (pub_key.to_account()) == keyDelegateMap.cend ());

	websocket->RegisterAccount(pub_key.to_account().c_str());

	// Keep a mapping of automatic listening delegates
	auto prvKeyAutomateDelegate = &(keyDelegateMap.emplace(std::piecewise_construct, std::forward_as_tuple(pub_key.to_account()), std::forward_as_tuple(private_key, delegate)).first->second);

	// Set it up to check for pending blocks every few seconds in case the websocket connection has missed any
	GetWorld()->GetTimerManager().SetTimer(prvKeyAutomateDelegate->timerHandle, [this, pub_key, prvKeyAutomateDelegate]() {
		AutomatePocketPendingUtility(pub_key.to_account().c_str(), prvKeyAutomateDelegate);
	}, 5.0f, true, 0.f);
}

void UNanoManager::AutomateUnregister(const FString& account, UNanoWebsocket* websocket) {
	websocket->UnregisterAccount(account);

	// Remove timer (has to be the exact one, doesn't work if it's been copied.
	auto & prvKeyAutomateDelegate = keyDelegateMap[TCHAR_TO_UTF8(*account)];
	if (!prvKeyAutomateDelegate.prv_key.IsEmpty ()) {
		GetWorld()->GetTimerManager().ClearTimer(prvKeyAutomateDelegate.timerHandle);
	}
	keyDelegateMap.erase(TCHAR_TO_UTF8(*account));
}

namespace
{
	void fireAutomateDelegateError(const PrvKeyAutomateDelegate* prvKeyAutomateDelegate) {
		FAutomateResponseData data;
		data.error = true;
		prvKeyAutomateDelegate->delegate.ExecuteIfBound(data);
	}
}

void UNanoManager::AutomatePocketPendingUtility(const FString& account, const PrvKeyAutomateDelegate* prvKeyAutomateDelegate_receiver) {
	AccountFrontier(account, [this, account, prvKeyAutomateDelegate_receiver](RESPONSE_PARAMETERS) {

		auto frontierData = GetAccountFrontierResponseData(RESPONSE_ARGUMENTS);
		if (!frontierData.error) {
			Pending(account, [this, frontierData, prvKeyAutomateDelegate_receiver](RESPONSE_PARAMETERS) {
				auto pendingData = GetPendingResponseData(RESPONSE_ARGUMENTS);
				if (!pendingData.error) {
					if (pendingData.blocks.Num() > 0) {
						AutomateWorkGenerateLoop(frontierData, prvKeyAutomateDelegate_receiver, pendingData.blocks);
					}
				}else {
					fireAutomateDelegateError(prvKeyAutomateDelegate_receiver);
				}

			});
		} else {
			fireAutomateDelegateError(prvKeyAutomateDelegate_receiver);
		}
	});
}

void UNanoManager::AutomateWorkGenerateLoop(FAccountFrontierResponseData frontierData, const PrvKeyAutomateDelegate* prvKeyAutomateDelegate_receiver, TArray<FPendingBlock> pendingBlocks) {

	WorkGenerate(frontierData.hash, [this, frontierData, prvKeyAutomateDelegate_receiver, pendingBlocks](RESPONSE_PARAMETERS) mutable {
		auto workData = GetWorkGenerateResponseData(RESPONSE_ARGUMENTS);
		// prvKeyAutomateDelegate_receiver could be deleted, this is just a pointer
		if (!workData.error && pendingBlocks.Num() != 0) {
			// Create the receive block
			nano::account account;
			account.decode_account(TCHAR_TO_UTF8(*frontierData.account));

			// Use the first block
			auto pendingBlock = pendingBlocks[0];
			pendingBlocks.RemoveAt(0);	// Pop from front

			nano::amount amount;
			amount.decode_dec(TCHAR_TO_UTF8(*pendingBlock.amount));

			nano::amount balance;
			balance.decode_dec(TCHAR_TO_UTF8(*frontierData.balance));

			auto new_balance = balance.number() + amount.number();

			FBlock block;
			block.account = account.to_account().c_str();
			auto str = new_balance.ToString();
			str.RemoveFromStart(TEXT("0x"));
			block.balance = FString (BaseConverter::HexToDecimalConverter ().Convert(std::string (TCHAR_TO_UTF8 (*str.ToUpper()))).c_str ());
			block.link = pendingBlock.hash; // source hash

			// Need to check if this is the open block
			if (account == nano::account(TCHAR_TO_UTF8(*frontierData.hash))) {
				block.previous = "0";
			} else {
				block.previous = frontierData.hash;
			}

			block.private_key = prvKeyAutomateDelegate_receiver->prv_key;
			block.representative = TCHAR_TO_UTF8(*frontierData.representative);
			block.work = workData.work;

			// Form the output data
			FAutomateResponseData automateData;
			automateData.isSend = false;

			str = amount.number().ToString();
			str.RemoveFromStart(TEXT("0x"));
			automateData.amount = FString (BaseConverter::HexToDecimalConverter ().Convert(std::string (TCHAR_TO_UTF8 (*str.ToUpper()))).c_str ());
			automateData.balance =  block.balance;
			automateData.account = block.account;
			automateData.representative= block.representative;

			Process(block, [this, prvKeyAutomateDelegate_receiver, pendingBlocks, automateData](RESPONSE_PARAMETERS) mutable {
				auto processData = GetProcessResponseData(RESPONSE_ARGUMENTS);
				if (!processData.error) {
					automateData.frontier = processData.hash;
					automateData.hash = processData.hash;

					// Fire it back to the user
					prvKeyAutomateDelegate_receiver->delegate.ExecuteIfBound(automateData);

					// If there are any more pending, then redo this process
					if (pendingBlocks.Num() > 0) {
						FAccountFrontierResponseData accountFrontierData;

						accountFrontierData.account = automateData.account;
						accountFrontierData.balance = automateData.balance;
						accountFrontierData.hash = automateData.frontier;
						accountFrontierData.representative = automateData.representative;

						AutomateWorkGenerateLoop(accountFrontierData, prvKeyAutomateDelegate_receiver, pendingBlocks);
					}
				}
				else {
					fireAutomateDelegateError(prvKeyAutomateDelegate_receiver);
				}
			});
		}
		else {
			fireAutomateDelegateError(prvKeyAutomateDelegate_receiver);
		}
	});
}

void UNanoManager::GetFrontierAndFire(const TSharedPtr<FJsonObject>& message_json, FString const & account, PrvKeyAutomateDelegate const * prvKeyAutomateDelegate, bool isSend)
{
	// This is a send from a user we are tracking, so balance needs checking in case it has been subtracted
	auto amount = message_json->GetStringField("amount");
	auto hash = message_json->GetStringField("hash");

	// Get the account info and send that back along with the block that has been sent
	AccountFrontier(TCHAR_TO_UTF8(*account), [this, account, amount, hash, prvKeyAutomateDelegate, isSend](RESPONSE_PARAMETERS) {

		auto frontierData = GetAccountFrontierResponseData(RESPONSE_ARGUMENTS);
		if (!frontierData.error) {

			// Form the output data
			FAutomateResponseData automateData;
			automateData.isSend = isSend;
			automateData.amount = amount;
			automateData.balance = frontierData.balance;
			automateData.account = account;
			automateData.frontier = frontierData.hash;
			automateData.hash = hash;

			// Fire it back to the user
			prvKeyAutomateDelegate->delegate.ExecuteIfBound(automateData);
		} else {
			fireAutomateDelegateError(prvKeyAutomateDelegate);
		}
	});
}

void UNanoManager::OnReceiveMessage(const FString& data) {
	UE_LOG(LogTemp, Warning, TEXT("Received: %s"), *data);

	TSharedRef< TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(data);
	TSharedPtr<FJsonObject> response;
	if (FJsonSerializer::Deserialize(JsonReader, response)) {
		// Need to determine if it's:
		// 1 - Send from someone else to us (we need to check pending, and fire off a loop to get these blocks (highest amount first), if above a certain amount)
		// 2 - Send from us to someone else (our balance goes down, check account_info)
		// 3 - Receive for us (no need to check pending, but need to check balance. But this could be an old block, so need to check account_info balance)

		// We could be monitoring multiple accounts which may be interacting with each other so need to check all
		auto message_json = response->GetObjectField("message");
		auto block_json = message_json->GetObjectField("block");
		if (block_json->GetStringField("subtype") == "send") {
			auto prvKeyAutomateDelegate_receiver = keyDelegateMap.find (TCHAR_TO_UTF8(*block_json->GetStringField("link_as_account")));
			if (prvKeyAutomateDelegate_receiver != keyDelegateMap.end ()) {
				// This is a send to us from someone else
				auto account = block_json->GetStringField("link_as_account");
				if (prvKeyAutomateDelegate_receiver->second.prv_key.IsEmpty ()) {
					// We are just watching this so return now
					GetFrontierAndFire (message_json, account, &prvKeyAutomateDelegate_receiver->second, false);
				} else {
					AutomatePocketPendingUtility(account, &prvKeyAutomateDelegate_receiver->second);
				}
			}

			auto prvKeyAutomateDelegate_sender = keyDelegateMap.find (TCHAR_TO_UTF8(*block_json->GetStringField("account")));
			if (prvKeyAutomateDelegate_sender != keyDelegateMap.end ()) {
				// This is a send from us to someone else
				auto account = message_json->GetStringField("account");
				GetFrontierAndFire(message_json, account, &prvKeyAutomateDelegate_sender->second, true);

				auto hash = message_json->GetStringField("hash");
				auto hash_str = std::string (TCHAR_TO_UTF8 (*hash));
				// TODO: Should be under critical section (same with timer)
				auto it = send_block_listener.find (hash_str);
				if (it != send_block_listener.cend ())
				{
					// Call delegate now that the send has been confirmed
					it->second.delegate.ExecuteIfBound (it->second.data);
					send_block_listener.erase (it);
					GetWorld()->GetTimerManager().ClearTimer(it->second.timerHandle);
				}
			}
		}
		else if (response->GetStringField("subtype") == "receive") {
			auto prvKeyAutomateDelegate = keyDelegateMap.find (TCHAR_TO_UTF8(*response->GetStringField("account")));
			if (prvKeyAutomateDelegate != keyDelegateMap.end ()) {
				// This is a receive to us
				auto account = message_json->GetStringField("account");
				GetFrontierAndFire(message_json, account, &prvKeyAutomateDelegate->second, false);
			}
		}
	}
}

void UNanoManager::BlockConfirmed(FBlockConfirmedResponseReceivedDelegate delegate, FString hash) {
	BlockConfirmed(hash, [this, delegate](RESPONSE_PARAMETERS) {
		delegate.ExecuteIfBound(GetBlockConfirmedResponseData(RESPONSE_ARGUMENTS));
	});
}

void UNanoManager::BlockConfirmed(FString hash, TFunction<void(RESPONSE_PARAMETERS)> const& delegate) {
	MakeRequest(GetBlockConfirmedJsonObject(hash), delegate);
}

// This will only call the delegate after the send has been confirmed by the network. Requires a websocket connection
void UNanoManager::SendWaitConfirmation (FProcessResponseReceivedDelegate delegate, FString const& private_key, FString const& account, FString const& amount) {

	// Register a block hash listener which will fire the delegate and remove it
	Send (private_key, account, amount, [this, private_key, account, amount, delegate](FProcessResponseData process_response_data) {
		if (!process_response_data.error)
		{
			// Register a block listener which will call the user delegate when a confirmation response from the network is received
			nano::raw_key prv_key;
			prv_key.data = nano::uint256_union(TCHAR_TO_UTF8(*private_key));
			auto pub_key = nano::pub_key(prv_key.data);
			pub_key.to_string ();

			// Check we are listening for websocket events for this account
			check (keyDelegateMap.find (pub_key.to_account()) != keyDelegateMap.cend ());
			// Also check that we aren't specifically listening for this send block already
			assert (send_block_listener.find (pub_key.to_account()) == send_block_listener.cend ());

			auto block_hash_str = std::string (TCHAR_TO_UTF8 (*process_response_data.hash));

			// Keep a mapping of automatic listening delegates
			auto send_delegate = &(send_block_listener.emplace(std::piecewise_construct, std::forward_as_tuple(block_hash_str), std::forward_as_tuple(delegate, process_response_data)).first->second);

			// Set it up to check for pending blocks every few seconds in case the websocket connection has missed any
			GetWorld()->GetTimerManager().SetTimer(send_delegate->timerHandle, [this, block_hash_str, send_delegate]() {

				// Get block_info, if confirmed call delegate, remove timer
				BlockConfirmed(send_delegate->data.hash, [this, send_delegate](RESPONSE_PARAMETERS) {
					auto block_confirmed_data = GetBlockConfirmedResponseData(RESPONSE_ARGUMENTS);
					if (block_confirmed_data.confirmed)
					{
						if (send_block_listener.count (std::string (TCHAR_TO_UTF8 (*send_delegate->data.hash))) > 0)
						{
							send_delegate->delegate.ExecuteIfBound (send_delegate->data);
							send_block_listener.erase (std::string (TCHAR_TO_UTF8 (*send_delegate->data.hash)));
							GetWorld()->GetTimerManager().ClearTimer(send_delegate->timerHandle);
						}
					}
				});
			}, 5.0f, true, 5.0f);
		}
		else
		{
			delegate.ExecuteIfBound (process_response_data);
		}		
	});
}

void UNanoManager::Send(FString const& private_key, FString const& account, FString const& amount, TFunction<void(FProcessResponseData)> const& delegate) {

	// Need to construct the block myself
	nano::raw_key prv_key;
	prv_key.data = nano::uint256_union(TCHAR_TO_UTF8(*private_key));

	auto pub_key = nano::pub_key(prv_key.data);

	nano::account acc;
	acc.decode_account (TCHAR_TO_UTF8(*account));

	FSendArgs sendArgs;
	sendArgs.private_key = private_key;
	sendArgs.account = acc.to_string ().c_str ();
	sendArgs.amount = amount;
	sendArgs.delegate = delegate;

	// Get the frontier
	AccountFrontier(pub_key.to_account().c_str(), [this, sendArgs](RESPONSE_PARAMETERS) mutable {

		auto accountFrontierResponseData = GetAccountFrontierResponseData(RESPONSE_ARGUMENTS);
		if (!accountFrontierResponseData.error) {
			sendArgs.balance = accountFrontierResponseData.balance;
			sendArgs.frontier = accountFrontierResponseData.hash;
			sendArgs.representative = accountFrontierResponseData.representative;

			// Generate work
			WorkGenerate(accountFrontierResponseData.hash, [this, sendArgs](RESPONSE_PARAMETERS) {

				auto workGenerateResponseData = GetWorkGenerateResponseData(RESPONSE_ARGUMENTS);
				if (workGenerateResponseData.hash != "0") {

					auto prv_key = nano::uint256_union(TCHAR_TO_UTF8(*sendArgs.private_key));
					auto this_account_public_key = (nano::pub_key (prv_key));
					nano::account acc(TCHAR_TO_UTF8(*sendArgs.account));

					nano::amount bal;
					bal.decode_dec(TCHAR_TO_UTF8(*sendArgs.balance));

					nano::amount amo;
					amo.decode_dec(TCHAR_TO_UTF8(*sendArgs.amount));

					FBlock block;
					block.account = this_account_public_key.to_account().c_str();

					auto str = (bal.number() - amo.number()).ToString();
					str.RemoveFromStart(TEXT("0x"));
					block.balance = FString (BaseConverter::HexToDecimalConverter ().Convert(std::string (TCHAR_TO_UTF8 (*str.ToUpper()))).c_str ());

					block.link = acc.to_string().c_str();
					block.previous = sendArgs.frontier;
					block.private_key = sendArgs.private_key;
					block.representative = sendArgs.representative;
					block.work = workGenerateResponseData.work;

					// Process the process
					Process(block, [d = sendArgs.delegate](RESPONSE_PARAMETERS){
						d (GetProcessResponseData(RESPONSE_ARGUMENTS));
					});
				}
			});
		} else {
			FProcessResponseData processData;
			processData.error = true;
			sendArgs.delegate (processData);
		}
	});
}

// The will call the delegate when a send has been published, but not necessarily confirmed by the network yet, for ultimate security use SendWaitConfirmation.
void UNanoManager::Send(FProcessResponseReceivedDelegate delegate, FString const& private_key, FString const& account, FString const& amount) {
	Send(private_key, account, amount, [this, delegate](const FProcessResponseData& data) {
		delegate.ExecuteIfBound(data);
	});
}

FRequestNanoResponseData UNanoManager::GetRequestNanoData(RESPONSE_PARAMETERS) {
	FRequestNanoResponseData data;
	RETURN_ERROR_IF_INVALID_RESPONSE(data)

	data.account = reqRespJson.request->GetStringField("account");
	data.amount = reqRespJson.response->GetStringField("amount");
	data.src_hash = reqRespJson.response->GetStringField("send_hash");
	data.frontier = reqRespJson.response->GetStringField("frontier");
	return data;
}

FBlockConfirmedResponseData UNanoManager::GetBlockConfirmedResponseData(RESPONSE_PARAMETERS) {
	FBlockConfirmedResponseData data;
	RETURN_ERROR_IF_INVALID_RESPONSE(data)

	data.confirmed = reqRespJson.response->GetBoolField("confirmed");
	return data;
}

FAccountFrontierResponseData UNanoManager::GetAccountFrontierResponseData(RESPONSE_PARAMETERS) const {
	FAccountFrontierResponseData accountFrontierResponseData;

	if (!RequestResponseIsValid (RESPONSE_ARGUMENTS)) {
		accountFrontierResponseData.error = true;
		return accountFrontierResponseData;
	}

	auto reqRespJson = GetResponseJson(RESPONSE_ARGUMENTS);
	if (reqRespJson.response->HasField("error")) {

		if (reqRespJson.request->GetStringField("error") == "1") {
			accountFrontierResponseData.error = true;
		} else {
			// Account could not be found, fill in with default values
			nano::public_key public_key;
			public_key.decode_account (TCHAR_TO_UTF8(*reqRespJson.request->GetStringField("account")));
			accountFrontierResponseData.account = reqRespJson.request->GetStringField("account");
			accountFrontierResponseData.hash = public_key.to_string().c_str();

			accountFrontierResponseData.balance = "0";
			accountFrontierResponseData.representative = defaultRepresentative;
		}
	} else {
		accountFrontierResponseData.account = reqRespJson.request->GetStringField("account");
		accountFrontierResponseData.hash = reqRespJson.response->GetStringField("frontier");
		accountFrontierResponseData.balance = reqRespJson.response->GetStringField("balance");
		accountFrontierResponseData.representative = reqRespJson.response->GetStringField("representative");
	}

	return accountFrontierResponseData;
}

FGetBalanceResponseData UNanoManager::GetBalanceResponseData(RESPONSE_PARAMETERS) {
	FGetBalanceResponseData data;
	RETURN_ERROR_IF_INVALID_RESPONSE(data)

	data.account = reqRespJson.request->GetStringField("account");
	data.balance = reqRespJson.response->GetStringField("balance");
	data.pending = reqRespJson.response->GetStringField("pending");
	return data;
}

FPendingResponseData UNanoManager::GetPendingResponseData(RESPONSE_PARAMETERS) {
	FPendingResponseData pendingResponseData;
	RETURN_ERROR_IF_INVALID_RESPONSE(pendingResponseData)

	pendingResponseData.account = reqRespJson.request->GetStringField("account");

	for (auto currJsonValue = reqRespJson.response->GetObjectField("blocks")->Values.CreateConstIterator(); currJsonValue; ++currJsonValue) {
		// Get the key name
		FPendingBlock pendingBlock;
		pendingBlock.hash = (*currJsonValue).Key;

		// Get the value as a FJsonValue object
		TSharedPtr< FJsonValue > Value = (*currJsonValue).Value;
		TSharedPtr<FJsonObject> JsonObjectIn = Value->AsObject();

		pendingBlock.amount = JsonObjectIn->GetStringField("amount");
		pendingBlock.source = JsonObjectIn->GetStringField("source");

		pendingResponseData.blocks.Add(pendingBlock);
	}
	return pendingResponseData;
}

FWorkGenerateResponseData UNanoManager::GetWorkGenerateResponseData(RESPONSE_PARAMETERS) {
	FWorkGenerateResponseData data;
	RETURN_ERROR_IF_INVALID_RESPONSE(data);

	data.hash = reqRespJson.request->GetStringField("hash");
	data.work = reqRespJson.response->GetStringField("work");
	return data;
}

FProcessResponseData UNanoManager::GetProcessResponseData(RESPONSE_PARAMETERS) {
	FProcessResponseData data;
	RETURN_ERROR_IF_INVALID_RESPONSE(data);

	data.hash = reqRespJson.response->GetStringField("hash");
	return data;
}

TSharedRef<IHttpRequest> UNanoManager::CreateHttpRequest(TSharedPtr<FJsonObject> JsonObject) {
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("POST");
	HttpRequest->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	HttpRequest->SetHeader("Content-Type", "application/json");

	FString OutputString;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

	HttpRequest->SetURL(rpcUrl);
	HttpRequest->SetContentAsString(OutputString);
	return HttpRequest;
}

void UNanoManager::MakeRequest(TSharedPtr<FJsonObject> JsonObject, TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> delegate) {
	auto HttpRequest = CreateHttpRequest(JsonObject);
	HttpRequest->OnProcessRequestComplete().BindLambda(delegate);
	HttpRequest->ProcessRequest();
}

auto UNanoManager::GetResponseJson(RESPONSE_PARAMETERS) -> ReqRespJson {
	// Make sure we are getting json
	ReqRespJson reqRespJson;

	if (!wasSuccessful || !response.IsValid()) {
		return reqRespJson;
	}

	if (request->GetContentType() == "application/json" && response->GetContentType() == "application/json") {
		// Response
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(response->GetContentAsString());

		if (FJsonSerializer::Deserialize(Reader, reqRespJson.response)) {
			// Request
			auto length = request->GetContentLength();
			auto content = request->GetContent();
			auto originalRequest = BytesToStringFixed(content.GetData(), length);

			TSharedRef<TJsonReader<>> requestReader = TJsonReaderFactory<>::Create(originalRequest);

			// Might not work
			FJsonSerializer::Deserialize(requestReader, reqRespJson.request);
		}
	}
	return reqRespJson;
}

bool UNanoManager::RequestResponseIsValid(RESPONSE_PARAMETERS) {
	if (!wasSuccessful || !response.IsValid()) {
		return false;
	}
	if (EHttpResponseCodes::IsOk(response->GetResponseCode())) {
		auto reqRespJson = GetResponseJson(request, response, wasSuccessful);
		return (reqRespJson.request && reqRespJson.response);
	} else {
		UE_LOG(LogTemp, Warning, TEXT("Http Response returned error code: %d"), response->GetResponseCode());
		return false;
	}
}

#ifdef _WIN32
#include <shlobj.h>
#elif __APPLE__
#include <Foundation/Foundation.h>	
#else
#include <pwd.h>
#endif

FString UNanoManager::getDefaultDataPath() const
{
	FString result;
#ifdef _WIN32
	WCHAR path[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
		std::wstring string(path);
		string += L"\\Nano_Games";
		result = string.c_str();
	} else {
		assert(false);
	}
#elif __APPLE__
	NSString* dir_string = [NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES) lastObject];
	char const* dir_chars = [dir_string UTF8String];
	std::string string(dir_chars);
	string += "/Nano_Games";
	result = string.c_str();
	[dir_string release] ;
#else
	auto entry(getpwuid(getuid()));
	assert(entry != nullptr);
	std::wstring string(entry->pw_dir);
	string += "/Nano_Games";
	result = string.c_str();
#endif
	return result;
}

void UNanoManager::SetDataSubdirectory(FString const& subdir) {
	auto defaultPath = getDefaultDataPath();
	dataPath = FPaths::Combine(defaultPath, subdir);
}

int32 UNanoManager::GetNumSeedFiles() const {
	return GetSeedFiles().Num();
}

TArray<FString> UNanoManager::GetSeedFiles() const {
	// Loop through all files in data path
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();

	TArray<FString> files;
	platformFile.IterateDirectory(*dataPath, [&files](const TCHAR* fileOrDir, bool isDirectory) {
		if (!isDirectory) {
			files.Add(fileOrDir);
			return true;
		}

		return false;
	});
	return files;
}

void UNanoManager::SaveSeed(FString const& plainSeed, FString const& seedFileName, FString const& password) {
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.CreateDirectoryTree(*dataPath)) {
		FFileHelper::SaveStringToFile(UNanoBlueprintLibrary::Encrypt(plainSeed, password), *FPaths::Combine(dataPath, seedFileName));
	}
}

FString UNanoManager::GetPlainSeed(FString const& seedFileName, FString const& password) const {

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	auto seedFilePath = FPaths::Combine(dataPath, seedFileName);
	if (PlatformFile.FileExists(*seedFilePath)) {
		FString cipherSeed;
		FFileHelper::LoadFileToString(cipherSeed, *seedFilePath);
		return UNanoBlueprintLibrary::Decrypt(cipherSeed, password);
	}

	return "";
}
