// Copyright 2020 Wesley Shillingford. All rights reserved.
#include "NanoManager.h"

#include "Engine.h"
#include "Engine/World.h"
#include "Http.h"
#include "HttpModule.h"
#include "Json.h"
#include "JsonObjectConverter.h"
#include "NanoBlueprintLibrary.h"

#include <ed25519-donna/ed25519.h>
#include <nano/blocks.h>
#include <nano/numbers.h>

#include <baseconverter/base_converter.hpp>

#if PLATFORM_WINDOWS
#include <shlobj.h>
#elif PLATFORM_MAC
#include <Foundation/Foundation.h>
#elif PLATFORM_LINUX
#include <pwd.h>
#endif

// clang-format off
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
// clang-format on

namespace {
void fireAutomateDelegateError(FAutomateResponseReceivedDelegate delegate) {
	FAutomateResponseData data;
	data.error = true;
	delegate.ExecuteIfBound(data);
}
}	 // namespace

template <class T, class T1>
void UNanoManager::RegisterBlockListener(std::string const& account, T const& responseData,
	std::unordered_map<std::string, BlockListenerDelegate<T, T1>>& blockListener, T1 delegate) {
	// Check we are listening for websocket events for this account
	{ check(keyDelegateMap.find(account) != keyDelegateMap.cend() || watchers.Find(account.c_str())); }
	// Also check that we aren't specifically listening for this send block already
	check(blockListener.find(account) == blockListener.cend());

	auto blockHashStdStr = std::string(TCHAR_TO_UTF8(*responseData.hash));

	// clang-format off
	// Keep a mapping of automatic listening delegates
	auto listenDelegate = &(blockListener.emplace(std::piecewise_construct, std::forward_as_tuple(blockHashStdStr), std::forward_as_tuple(responseData, delegate)).first->second);
	// clang-format on

	// Set it up to check if the block is confirmed every few seconds in case the websocket connection has missed any
	GetWorld()->GetTimerManager().SetTimer(
		listenDelegate->timerHandle,
		[this, &blockListener, hash = listenDelegate->data.hash]() {
			auto it = blockListener.find(std::string(TCHAR_TO_UTF8(*hash)));
			if (it != blockListener.cend()) {
				// Get block_info, if confirmed call delegate, remove timer
				BlockConfirmed(it->second.data.hash, [this, &blockListener, hash = it->second.data.hash](
																							 FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
					auto blockConfirmedData = GetBlockConfirmedResponseData(request, response, wasSuccessful);
					if (blockConfirmedData.confirmed) {
						auto it = blockListener.find(std::string(TCHAR_TO_UTF8(*hash)));
						if (it != blockListener.cend()) {
							auto delegate = it->second.delegate;
							GetWorld()->GetTimerManager().ClearTimer(it->second.timerHandle);
							delegate.ExecuteIfBound(it->second.data);
							blockListener.erase(it);
						}
					}
				});
			}
		},
		5.0f, true, 5.0f);
}

void UNanoManager::SetupFilteredConfirmationMessageWebsocketListener(UNanoWebsocket* websocket) {
	if (!websocket->onFilteredResponse.Contains(this, "OnConfirmationReceiveMessage")) {
		// Make sure to only call this once for the entirety of the program...
		websocket->onFilteredResponse.AddDynamic(this, &UNanoManager::OnConfirmationReceiveMessage);
	}
}

void UNanoManager::GetWalletBalance(
	FString address, TFunction<void(FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful)> const& delegate) {
	FGetBalanceRequestData getBalanceRequestData;
	getBalanceRequestData.account = address;

	TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(getBalanceRequestData);
	MakeRequest(JsonObject, delegate);
}

void UNanoManager::GetWalletBalance(FGetBalanceResponseReceivedDelegate delegate, FString address) {
	GetWalletBalance(address, [this, delegate](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
		delegate.ExecuteIfBound(GetBalanceResponseData(request, response, wasSuccessful));
	});
}

TSharedPtr<FJsonObject> UNanoManager::GetWorkGenerateJsonObject(FString hash) {
	FWorkGenerateRequestData workGenerateRequestData;
	workGenerateRequestData.hash = hash;
	return FJsonObjectConverter::UStructToJsonObject(workGenerateRequestData);
}

void UNanoManager::WorkGenerate(FWorkGenerateResponseReceivedDelegate delegate, FString hash) {
	WorkGenerate(hash, [delegate](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
		delegate.ExecuteIfBound(GetWorkGenerateResponseData(request, response, wasSuccessful));
	});
}

void UNanoManager::WorkGenerate(
	FString hash, TFunction<void(FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful)> const& d) {
	MakeRequest(GetWorkGenerateJsonObject(hash), d);
}

TSharedPtr<FJsonObject> UNanoManager::GetPendingJsonObject(FString account, FString threshold, int32 maxCount) {
	FPendingRequestData pendingRequestData;
	pendingRequestData.account = account;
	pendingRequestData.count = FString::FromInt(maxCount);
	pendingRequestData.threshold = threshold;
	return FJsonObjectConverter::UStructToJsonObject(pendingRequestData);
}

void UNanoManager::Pending(FPendingResponseReceivedDelegate delegate, FString account, FString threshold, int32 maxCount) {
	Pending(account, threshold, maxCount, [delegate](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
		delegate.ExecuteIfBound(GetPendingResponseData(request, response, wasSuccessful));
	});
}

void UNanoManager::Pending(FString account, FString threshold, int32 maxCount,
	TFunction<void(FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful)> const& d) {
	MakeRequest(GetPendingJsonObject(account, threshold, maxCount), d);
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
	AccountFrontier(account, [this, delegate](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
		delegate.ExecuteIfBound(GetAccountFrontierResponseData(request, response, wasSuccessful));
	});
}

void UNanoManager::AccountFrontier(
	FString account, TFunction<void(FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful)> const& delegate) {
	MakeRequest(GetAccountFrontierJsonObject(account), delegate);
}

void UNanoManager::Process(FProcessResponseReceivedDelegate delegate, FBlock block) {
	Process(block, [delegate](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
		delegate.ExecuteIfBound(GetProcessResponseData(request, response, wasSuccessful));
	});
}

void UNanoManager::Process(
	FBlock block, TFunction<void(FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful)> const& delegate) {
	nano::account account;
	account.decode_account(TCHAR_TO_UTF8(*block.account));

	nano::account representative;
	representative.decode_account(TCHAR_TO_UTF8(*block.representative));

	nano::amount balance;
	balance.decode_dec(TCHAR_TO_UTF8(*block.balance));

	nano::account link(TCHAR_TO_UTF8(*block.link));

	nano::raw_key rawKey;
	rawKey.data = nano::uint256_union(TCHAR_TO_UTF8(*block.privateKey));

	nano::block_hash previous(TCHAR_TO_UTF8(*block.previous));

	auto publicKey = nano::pub_key(rawKey.data);

	nano::state_block stateBlock(account, previous, representative, balance, link, rawKey, publicKey);

	FProcessRequestData processRequestData;
	FBlockRequestData blockProcessRequestData;
	blockProcessRequestData.account = block.account;
	blockProcessRequestData.balance = block.balance;
	blockProcessRequestData.previous = block.previous;
	blockProcessRequestData.representative = block.representative;
	blockProcessRequestData.link = link.to_string().c_str();
	blockProcessRequestData.signature = stateBlock.block_signature().to_string().c_str();
	blockProcessRequestData.work = block.work;

	processRequestData.block = blockProcessRequestData;

	TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(processRequestData);
	MakeRequest(JsonObject, delegate);
}

// This will only call the delegate after the process has been confirmed by the network. Requires a websocket connection
void UNanoManager::ProcessSendWaitConfirmation(FProcessResponseReceivedDelegate delegate, FBlock block) {
	// Register a block hash listener which will fire the delegate and remove it
	Process(block, [this, block, delegate](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
		auto processResponseData = GetProcessResponseData(request, response, wasSuccessful);
		if (!processResponseData.error) {
			RegisterBlockListener<FProcessResponseData, FProcessResponseReceivedDelegate>(
				TCHAR_TO_UTF8(*block.account), processResponseData, sendBlockListener, delegate);
		} else {
			delegate.ExecuteIfBound(processResponseData);
		}
	});
}

// Used only be used for development if the server supports it (unless you want a faucet).
void UNanoManager::RequestNano(FReceivedNanoDelegate delegate, FString account) {
	// Ask server for Nano
	FRequestNanoData requestNanoData;
	requestNanoData.account = account;

	TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(requestNanoData);
	MakeRequest(JsonObject, [this, delegate](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
		delegate.ExecuteIfBound(GetRequestNanoData(request, response, wasSuccessful));
	});
}

int32 UNanoManager::Watch(const FWatchAccountReceivedDelegate& delegate, FString const& account, UNanoWebsocket* websocket) {
	websocket->RegisterAccount(account);

	// Keep a mapping of automatic listening delegates
	auto val = watchers.Find(account);
	if (val) {
		val->Add(watcherId, delegate);
	} else {
		watchers.Emplace(account, TMap<int32, FWatchAccountReceivedDelegate>{{watcherId, delegate}});
	}

	return watcherId++;
}

void UNanoManager::Unwatch(const FString& account, const int32& id, UNanoWebsocket* websocket) {
	// Check this id exists before unwatching
	auto map = watchers.Find(account);
	if (map) {
		auto delegate = map->Find(id);
		if (delegate) {
			map->Remove(id);
			if (map->Num() == 0) {
				watchers.Remove(account);
			}
			websocket->UnregisterAccount(account);
		}
	}
}

void UNanoManager::AutomaticallyPocketRegister(
	FAutomateResponseReceivedDelegate delegate, UNanoWebsocket* websocket, FString const& privateKey, FString minimum) {
	check(!privateKey.IsEmpty());
	nano::raw_key prvKey;
	prvKey.data = nano::uint256_union(TCHAR_TO_UTF8(*privateKey));

	auto pubKey = nano::pub_key(prvKey.data);

	// Check you haven't already added it
	check(keyDelegateMap.find(pubKey.to_account()) == keyDelegateMap.cend());

	websocket->RegisterAccount(pubKey.to_account().c_str());

	// Keep a mapping of automatic listening delegates
	// clang-format off
	auto prvKeyAutomateDelegate = &(keyDelegateMap.emplace(std::piecewise_construct, std::forward_as_tuple(pubKey.to_account()),
															std::forward_as_tuple(privateKey, delegate, minimum)).first->second);
	// clang-format on

	// Set it up to check for pending blocks every few seconds in case the websocket connection has missed any
	GetWorld()->GetTimerManager().SetTimer(
		prvKeyAutomateDelegate->timerHandle,
		[this, pubKey, minimum]() {
			auto it = keyDelegateMap.find(pubKey.to_account());
			if (it != keyDelegateMap.end()) {
				AutomatePocketPendingUtility(pubKey.to_account().c_str(), minimum);
			}
		},
		5.0f, true, 1.f);
}

void UNanoManager::AutomaticallyPocketUnregister(const FString& account, UNanoWebsocket* websocket) {
	// Remove timer (has to be the exact one, doesn't work if it's been copied.
	auto it = keyDelegateMap.find(TCHAR_TO_UTF8(*account));
	if (it != keyDelegateMap.cend()) {
		if (!it->second.prvKey.IsEmpty()) {
			GetWorld()->GetTimerManager().ClearTimer(it->second.timerHandle);
		}
		keyDelegateMap.erase(it);
		websocket->UnregisterAccount(account);
	}
}

void UNanoManager::AutomatePocketPendingUtility(const FString& account, const FString& minimum) {
	AccountFrontier(account, [this, account, minimum](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
		auto frontierData = GetAccountFrontierResponseData(request, response, wasSuccessful);
		if (!frontierData.error) {
			const auto numPending = 5;
			Pending(
				account, minimum, numPending, [this, frontierData](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
					auto pendingData = GetPendingResponseData(request, response, wasSuccessful);
					if (!pendingData.error) {
						if (pendingData.blocks.Num() > 0) {
							AutomateWorkGenerateLoop(frontierData, pendingData.blocks);
						}
					} else {
						auto it = keyDelegateMap.find(TCHAR_TO_UTF8(*frontierData.account));
						if (it != keyDelegateMap.end()) {
							fireAutomateDelegateError(it->second.delegate);
						}
					}
				});
		} else {
			auto it = keyDelegateMap.find(TCHAR_TO_UTF8(*account));
			if (it != keyDelegateMap.end()) {
				fireAutomateDelegateError(it->second.delegate);
			}
		}
	});
}

void UNanoManager::AutomateWorkGenerateLoop(FAccountFrontierResponseData frontierData, TArray<FPendingBlock> pendingBlocks) {
	WorkGenerate(frontierData.hash,
		[this, frontierData, pendingBlocks](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) mutable {
			auto workData = GetWorkGenerateResponseData(request, response, wasSuccessful);

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

				auto newBalance = balance.number() + amount.number();

				FBlock block;
				block.account = account.to_account().c_str();
				auto str = newBalance.ToString();
				str.RemoveFromStart(TEXT("0x"));
				block.balance = FString(BaseConverter::HexToDecimalConverter().Convert(std::string(TCHAR_TO_UTF8(*str.ToUpper()))).c_str());
				block.link = pendingBlock.hash;	 // source hash

				// Need to check if this is the open block
				if (account == nano::account(TCHAR_TO_UTF8(*frontierData.hash))) {
					block.previous = "0";
				} else {
					block.previous = frontierData.hash;
				}

				auto it = keyDelegateMap.find(TCHAR_TO_UTF8(*frontierData.account));
				if (it != keyDelegateMap.end()) {
					block.privateKey = it->second.prvKey;
					block.representative = TCHAR_TO_UTF8(*frontierData.representative);
					block.work = workData.work;

					// Form the output data
					FAutomateResponseData automateData;
					automateData.type = FConfType::receive;

					str = amount.number().ToString();
					str.RemoveFromStart(TEXT("0x"));
					automateData.amount =
						FString(BaseConverter::HexToDecimalConverter().Convert(std::string(TCHAR_TO_UTF8(*str.ToUpper()))).c_str());
					automateData.balance = block.balance;
					automateData.account = block.account;
					automateData.representative = block.representative;

					Process(block,
						[this, pendingBlocks, automateData](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) mutable {
							auto processData = GetProcessResponseData(request, response, wasSuccessful);
							if (!processData.error) {
								automateData.frontier = processData.hash;
								automateData.hash = processData.hash;

								auto it = keyDelegateMap.find(TCHAR_TO_UTF8(*automateData.account));
								if (it != keyDelegateMap.end()) {
									// Fire it back to the user
									FAutomateResponseReceivedDelegate delegate = it->second.delegate;
									RegisterBlockListener<FAutomateResponseData, FAutomateResponseReceivedDelegate>(
										TCHAR_TO_UTF8(*automateData.account), automateData, receiveBlockListener, delegate);

									// If there are any more pending, then redo this process
									if (pendingBlocks.Num() > 0) {
										FAccountFrontierResponseData accountFrontierData;

										accountFrontierData.account = automateData.account;
										accountFrontierData.balance = automateData.balance;
										accountFrontierData.hash = automateData.frontier;
										accountFrontierData.representative = automateData.representative;

										AutomateWorkGenerateLoop(accountFrontierData, pendingBlocks);
									}
								}
							} else {
								auto it = keyDelegateMap.find(TCHAR_TO_UTF8(*automateData.account));
								if (it != keyDelegateMap.end()) {
									fireAutomateDelegateError(it->second.delegate);
								}
							}
						});
				}
			} else {
				auto it = keyDelegateMap.find(TCHAR_TO_UTF8(*frontierData.account));
				if (it != keyDelegateMap.end()) {
					fireAutomateDelegateError(it->second.delegate);
				}
			}
		});
}

FAutomateResponseData UNanoManager::GetWebsocketResponseData(const FString& amount, const FString& hash, FString const& account,
	FConfType type, FAccountFrontierResponseData const& frontierData) {
	FAutomateResponseData automateData;
	automateData.type = type;
	automateData.amount = amount;
	automateData.balance = frontierData.balance;
	automateData.account = account;
	automateData.frontier = frontierData.hash;
	automateData.hash = hash;
	return automateData;
}

void UNanoManager::GetFrontierAndFireWatchers(const FString& amount, const FString& hash, FString const& account, FConfType type) {
	// Get the account info and send that back along with the block that has been sent
	AccountFrontier(TCHAR_TO_UTF8(*account),
		[this, account, amount, hash, type](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
			auto frontierData = GetAccountFrontierResponseData(request, response, wasSuccessful);

			auto idDelegateMap = watchers.Find(account);
			if (idDelegateMap) {
				// Copy delegates in case someone unwatches during this call
				TArray<FWatchAccountReceivedDelegate> delegates;
				for (auto delegate : *idDelegateMap) {
					delegates.Add(delegate.Value);
				}
				if (!frontierData.error) {
					// Form the output data
					auto automateData = GetWebsocketResponseData(amount, hash, account, type, frontierData);

					// Fire it back to the user
					for (auto delegate : delegates) {
						delegate.ExecuteIfBound(automateData);
					}
				} else {
					FAutomateResponseData data;
					data.error = true;
					for (auto delegate : delegates) {
						delegate.ExecuteIfBound(data);
					}
				}
			}
		});
}

void UNanoManager::GetFrontierAndFire(const FString& amount, const FString& hash, FString const& account, FConfType type) {
	// Get the account info and send that back along with the block that has been sent
	AccountFrontier(TCHAR_TO_UTF8(*account),
		[this, account, amount, hash, type](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
			auto frontierData = GetAccountFrontierResponseData(request, response, wasSuccessful);

			auto it = keyDelegateMap.find(TCHAR_TO_UTF8(*account));
			if (it != keyDelegateMap.end()) {
				if (!frontierData.error) {
					// Form the output data
					auto automateData = GetWebsocketResponseData(amount, hash, account, type, frontierData);

					// Fire it back to the user
					it->second.delegate.ExecuteIfBound(automateData);
				} else {
					fireAutomateDelegateError(it->second.delegate);
				}
			}
		});
}

void UNanoManager::OnConfirmationReceiveMessage(const FWebsocketConfirmationResponseData& data, UNanoWebsocket* websocket) {
	// Need to determine if it's:
	// 1 - Send to an account we are watching (we need to check pending, and fire off a loop to get these blocks (highest amount
	// first), if above a certain amount)
	// 2 - Send from an account we are watching (our balance goes down, check account_info)
	// 3 - Receive (pocket) an account we are watching (no need to check pending, but need to check balance. But this could be an old
	// block, so need to check account_info balance)

	// We could be monitoring multiple accounts which may be interacting with each other so need to check all
	if (data.block.subtype == FSubtype::send) {
		// Check if this is a send to an account we are watching
		auto linkAsAccount =
			FString(nano::account(TCHAR_TO_UTF8(*data.block.link)).to_account().c_str());	 // Convert link as hash to account

		auto it = keyDelegateMap.find(TCHAR_TO_UTF8(*linkAsAccount));
		if (it != keyDelegateMap.end()) {
			if (UNanoBlueprintLibrary::GreaterOrEqual(data.amount, it->second.minimum)) {
				// Pocket the block, also check if there are more pending
				AutomatePocketPendingUtility(linkAsAccount, it->second.minimum);
			}
		}

		auto accountWatchers = watchers.Find(linkAsAccount);
		if (accountWatchers) {
			// We are just watching this so return the block (after getting account frontier information)
			GetFrontierAndFireWatchers(data.amount, data.hash, linkAsAccount, FConfType::send_to);
		}

		// Check if this is a send from an account we are watching
		auto account = data.block.account;
		if (keyDelegateMap.count(TCHAR_TO_UTF8(*account)) > 0) {
			// This is a send from us to someone else
			GetFrontierAndFire(data.amount, data.hash, account, FConfType::send_from);
		}

		accountWatchers = watchers.Find(account);
		if (accountWatchers) {
			// We are just watching this so return the block (after getting account frontier information)
			GetFrontierAndFireWatchers(data.amount, data.hash, account, FConfType::send_from);
		}

		// If we are listening for confirmations for this send block
		auto hash_std_str = std::string(TCHAR_TO_UTF8(*data.hash));
		auto it1 = sendBlockListener.find(hash_std_str);
		if (it1 != sendBlockListener.cend()) {
			// Call delegate now that the send has been confirmed
			GetWorld()->GetTimerManager().ClearTimer(it1->second.timerHandle);
			it1->second.delegate.ExecuteIfBound(it1->second.data);
			sendBlockListener.erase(it1);
		}

		// Are we listening for a payment? Only one of these will be active at once
		if (listeningPayment.delegate.IsBound()) {
			if (listeningPayment.account == linkAsAccount && listeningPayment.amount == data.amount) {
				Unwatch(listeningPayment.account, listeningPayment.watchId, websocket);
				GetWorld()->GetTimerManager().ClearTimer(listeningPayment.timerHandle);
				listeningPayment.delegate.ExecuteIfBound(data.hash, listeningPayment.amount);
				listeningPayment.delegate.Unbind();
			}
		}

		if (listeningPayout.delegate.IsBound()) {
			if (listeningPayout.account == account && data.amount == "0") {
				Unwatch(listeningPayment.account, listeningPayout.watchId, websocket);
				GetWorld()->GetTimerManager().ClearTimer(listeningPayout.timerHandle);
				listeningPayout.delegate.ExecuteIfBound(false);
				listeningPayout.delegate.Unbind();
			}
		}

	} else if (data.block.subtype == FSubtype::receive || data.block.subtype == FSubtype::open) {
		auto account = data.account;
		if (keyDelegateMap.count(TCHAR_TO_UTF8(*account)) > 0) {
			// Received this block from websocket so don't need to have the receive block listener timer listening for it anymore.
			auto it = receiveBlockListener.find(std::string(TCHAR_TO_UTF8(*data.hash)));
			if (it != receiveBlockListener.cend()) {
				GetWorld()->GetTimerManager().ClearTimer(it->second.timerHandle);
				receiveBlockListener.erase(it);
			}

			GetFrontierAndFire(data.amount, data.hash, account, FConfType::receive);
		}

		auto accountWatchers = watchers.Find(account);
		if (accountWatchers) {
			// We are just watching this so return the block (after getting account frontier information)
			GetFrontierAndFireWatchers(data.amount, data.hash, account, FConfType::receive);
		}
	}
}

void UNanoManager::BlockConfirmed(FBlockConfirmedResponseReceivedDelegate delegate, FString hash) {
	BlockConfirmed(hash, [this, delegate](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
		delegate.ExecuteIfBound(GetBlockConfirmedResponseData(request, response, wasSuccessful));
	});
}

void UNanoManager::BlockConfirmed(
	FString hash, TFunction<void(FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful)> const& delegate) {
	MakeRequest(GetBlockConfirmedJsonObject(hash), delegate);
}

void UNanoManager::SendWaitConfirmationBlock(FProcessResponseReceivedDelegate delegate, FBlock block) {
	Process(block, [this, delegate, account = nano::account(TCHAR_TO_UTF8(*block.link)).to_account()](
									 FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
		auto processResponseData = GetProcessResponseData(request, response, wasSuccessful);
		if (!processResponseData.error) {
			RegisterBlockListener<FProcessResponseData, FProcessResponseReceivedDelegate>(
				account, processResponseData, sendBlockListener, delegate);
		} else {
			delegate.ExecuteIfBound(processResponseData);
		}
	});
}

// This will only call the delegate after the send has been confirmed by the network. Requires a websocket connection
void UNanoManager::SendWaitConfirmation(
	FProcessResponseReceivedDelegate delegate, FString const& privateKey, FString const& account, FString const& amount) {
	// Register a block hash listener which will fire the delegate and remove it
	Send(privateKey, account, amount, [this, privateKey, account, amount, delegate](FProcessResponseData processResponseData) {
		if (!processResponseData.error) {
			// Register a block listener which will call the user delegate when a confirmation response from the network is received
			nano::raw_key prvKey;
			prvKey.data = nano::uint256_union(TCHAR_TO_UTF8(*privateKey));
			auto pubKey = nano::pub_key(prvKey.data);
			RegisterBlockListener<FProcessResponseData, FProcessResponseReceivedDelegate>(
				pubKey.to_account(), processResponseData, sendBlockListener, delegate);
		} else {
			delegate.ExecuteIfBound(processResponseData);
		}
	});
}

void UNanoManager::Send(
	FString const& privateKey, FString const& account, FString const& amount, TFunction<void(FProcessResponseData)> const& delegate) {
	MakeSendBlock(privateKey, amount, account, [delegate, this](FMakeBlockResponseData data) {
		if (!data.error) {
			// Process the process
			Process(data.block, [delegate](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
				delegate(GetProcessResponseData(request, response, wasSuccessful));
			});
		} else {
			FProcessResponseData processData;
			processData.error = true;
			delegate(processData);
		}
	});
}

// The will call the delegate when a send has been published, but not necessarily confirmed by the network yet, for ultimate
// security use SendWaitConfirmation.
void UNanoManager::Send(
	FProcessResponseReceivedDelegate delegate, FString const& privateKey, FString const& account, FString const& amount) {
	Send(privateKey, account, amount, [this, delegate](const FProcessResponseData& data) { delegate.ExecuteIfBound(data); });
}

void UNanoManager::MakeSendBlock(
	FMakeBlockDelegate delegate, FString const& prvKey, FString const& amount, FString const& destinationAccount) {
	MakeSendBlock(
		prvKey, amount, destinationAccount, [this, delegate](const FMakeBlockResponseData& data) { delegate.ExecuteIfBound(data); });
}

void UNanoManager::MakeSendBlock(FString const& privateKey, FString const& amount, FString const& destinationAccount,
	TFunction<void(FMakeBlockResponseData)> const& delegate) {
	// Need to construct the block myself
	nano::raw_key prvKey;
	prvKey.data = nano::uint256_union(TCHAR_TO_UTF8(*privateKey));

	auto pubKey = nano::pub_key(prvKey.data);

	nano::account acc;
	acc.decode_account(TCHAR_TO_UTF8(*destinationAccount));

	FSendArgs sendArgs;
	sendArgs.privateKey = privateKey;
	sendArgs.account = acc.to_string().c_str();
	sendArgs.amount = amount;

	// Get the frontier
	AccountFrontier(pubKey.to_account().c_str(),
		[this, sendArgs, delegate](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) mutable {
			auto accountFrontierResponseData = GetAccountFrontierResponseData(request, response, wasSuccessful);
			if (!accountFrontierResponseData.error) {
				sendArgs.balance = accountFrontierResponseData.balance;
				sendArgs.frontier = accountFrontierResponseData.hash;
				sendArgs.representative = accountFrontierResponseData.representative;

				// Generate work
				WorkGenerate(accountFrontierResponseData.hash,
					[this, sendArgs, delegate](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
						auto workGenerateResponseData = GetWorkGenerateResponseData(request, response, wasSuccessful);
						if (workGenerateResponseData.hash != "0") {
							auto prvKey = nano::uint256_union(TCHAR_TO_UTF8(*sendArgs.privateKey));
							auto thisAccountPublicKey = (nano::pub_key(prvKey));
							nano::account acc(TCHAR_TO_UTF8(*sendArgs.account));

							nano::amount bal;
							bal.decode_dec(TCHAR_TO_UTF8(*sendArgs.balance));

							nano::amount amo;
							amo.decode_dec(TCHAR_TO_UTF8(*sendArgs.amount));

							FBlock block;
							block.account = thisAccountPublicKey.to_account().c_str();

							auto str = (bal.number() - amo.number()).ToString();
							str.RemoveFromStart(TEXT("0x"));
							block.balance =
								FString(BaseConverter::HexToDecimalConverter().Convert(std::string(TCHAR_TO_UTF8(*str.ToUpper()))).c_str());

							block.link = acc.to_string().c_str();
							block.previous = sendArgs.frontier;
							block.privateKey = sendArgs.privateKey;
							block.representative = sendArgs.representative;
							block.work = workGenerateResponseData.work;

							FMakeBlockResponseData makeBlockData;
							makeBlockData.block = block;
							delegate(makeBlockData);
						}
					});
			} else {
				FMakeBlockResponseData makeBlockData;
				makeBlockData.error = true;
				delegate(makeBlockData);
			}
		});
}

void UNanoManager::Receive(
	const FProcessResponseReceivedDelegate& delegate, FString const& privateKey, FString sourceHash, FString const& amount) {
	MakeReceiveBlock(privateKey, sourceHash, amount, [delegate, this](FMakeBlockResponseData data) {
		if (!data.error) {
			// Process the process
			Process(data.block, [delegate](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
				delegate.ExecuteIfBound(GetProcessResponseData(request, response, wasSuccessful));
			});
		} else {
			FProcessResponseData processData;
			processData.error = true;
			delegate.ExecuteIfBound(processData);
		}
	});
}

void UNanoManager::MakeReceiveBlock(
	FMakeBlockDelegate delegate, FString const& privateKey, FString sourceHash, FString const& amount) {
	MakeReceiveBlock(
		privateKey, sourceHash, amount, [this, delegate](const FMakeBlockResponseData& data) { delegate.ExecuteIfBound(data); });
}

void UNanoManager::MakeReceiveBlock(
	FString const& privateKey, FString sourceHash, FString const& amount, TFunction<void(FMakeBlockResponseData)> const& delegate) {
	// Need to construct the block myself
	nano::raw_key prvKey;
	prvKey.data = nano::uint256_union(TCHAR_TO_UTF8(*privateKey));

	auto pubKey = nano::pub_key(prvKey.data);

	// Get the frontier
	AccountFrontier(pubKey.to_account().c_str(), [this, privateKey, sourceHash, amount, delegate](
																								 FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) mutable {
		auto accountFrontierResponseData = GetAccountFrontierResponseData(request, response, wasSuccessful);
		if (!accountFrontierResponseData.error) {
			// Generate work
			WorkGenerate(accountFrontierResponseData.hash, [this, privateKey, sourceHash, amount, delegate, accountFrontierResponseData](
																											 FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
				auto workGenerateResponseData = GetWorkGenerateResponseData(request, response, wasSuccessful);
				if (workGenerateResponseData.hash != "0") {
					auto prvKey = nano::uint256_union(TCHAR_TO_UTF8(*privateKey));
					auto thisAccountPublicKey = (nano::pub_key(prvKey));

					nano::amount bal;
					bal.decode_dec(TCHAR_TO_UTF8(*accountFrontierResponseData.balance));

					nano::amount amo;
					amo.decode_dec(TCHAR_TO_UTF8(*amount));

					FBlock block;
					block.account = thisAccountPublicKey.to_account().c_str();

					auto str = (bal.number() + amo.number()).ToString();
					str.RemoveFromStart(TEXT("0x"));
					block.balance =
						FString(BaseConverter::HexToDecimalConverter().Convert(std::string(TCHAR_TO_UTF8(*str.ToUpper()))).c_str());

					block.link = sourceHash;

					// Set previous to 0 if this is an open block
					if (thisAccountPublicKey == nano::account(TCHAR_TO_UTF8(*accountFrontierResponseData.hash))) {
						block.previous = "0";
					} else {
						block.previous = accountFrontierResponseData.hash;
					}

					block.privateKey = privateKey;
					block.representative = accountFrontierResponseData.representative;
					block.work = workGenerateResponseData.work;

					FMakeBlockResponseData makeBlockData;
					makeBlockData.block = block;
					delegate(makeBlockData);
				}
			});
		} else {
			FMakeBlockResponseData makeBlockData;
			makeBlockData.error = true;
			delegate(makeBlockData);
		}
	});
}

FRequestNanoResponseData UNanoManager::GetRequestNanoData(FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
	FRequestNanoResponseData data;
	RETURN_ERROR_IF_INVALID_RESPONSE(data)

	data.account = reqRespJson.request->GetStringField("account");
	data.amount = reqRespJson.response->GetStringField("amount");
	data.srcHash = reqRespJson.response->GetStringField("send_hash");
	data.frontier = reqRespJson.response->GetStringField("frontier");
	return data;
}

FBlockConfirmedResponseData UNanoManager::GetBlockConfirmedResponseData(
	FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
	FBlockConfirmedResponseData data;
	RETURN_ERROR_IF_INVALID_RESPONSE(data)

	data.confirmed = reqRespJson.response->GetBoolField("confirmed");
	return data;
}

FAccountFrontierResponseData UNanoManager::GetAccountFrontierResponseData(
	FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) const {
	FAccountFrontierResponseData accountFrontierResponseData;

	if (!RequestResponseIsValid(request, response, wasSuccessful)) {
		accountFrontierResponseData.error = true;
		return accountFrontierResponseData;
	}

	auto reqRespJson = GetResponseJson(request, response, wasSuccessful);
	if (reqRespJson.response->HasField("error")) {
		if (reqRespJson.request->GetStringField("error") == "1") {
			accountFrontierResponseData.error = true;
		} else {
			// Account could not be found, fill in with default values
			nano::public_key publicKey;
			publicKey.decode_account(TCHAR_TO_UTF8(*reqRespJson.request->GetStringField("account")));
			accountFrontierResponseData.account = reqRespJson.request->GetStringField("account");
			accountFrontierResponseData.hash = publicKey.to_string().c_str();

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

FGetBalanceResponseData UNanoManager::GetBalanceResponseData(
	FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
	FGetBalanceResponseData data;
	RETURN_ERROR_IF_INVALID_RESPONSE(data)

	data.account = reqRespJson.request->GetStringField("account");
	data.balance = reqRespJson.response->GetStringField("balance");
	data.pending = reqRespJson.response->GetStringField("pending");
	return data;
}

FPendingResponseData UNanoManager::GetPendingResponseData(FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
	FPendingResponseData pendingResponseData;
	RETURN_ERROR_IF_INVALID_RESPONSE(pendingResponseData)

	pendingResponseData.account = reqRespJson.request->GetStringField("account");

	for (auto currJsonValue = reqRespJson.response->GetObjectField("blocks")->Values.CreateConstIterator(); currJsonValue;
			 ++currJsonValue) {
		// Get the key name
		FPendingBlock pendingBlock;
		pendingBlock.hash = (*currJsonValue).Key;

		// Get the value as a FJsonValue object
		TSharedPtr<FJsonValue> Value = (*currJsonValue).Value;
		TSharedPtr<FJsonObject> JsonObjectIn = Value->AsObject();

		pendingBlock.amount = JsonObjectIn->GetStringField("amount");
		pendingBlock.source = JsonObjectIn->GetStringField("source");

		pendingResponseData.blocks.Add(pendingBlock);
	}
	return pendingResponseData;
}

FWorkGenerateResponseData UNanoManager::GetWorkGenerateResponseData(
	FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
	FWorkGenerateResponseData data;
	RETURN_ERROR_IF_INVALID_RESPONSE(data);

	data.hash = reqRespJson.request->GetStringField("hash");
	data.work = reqRespJson.response->GetStringField("work");
	return data;
}

FProcessResponseData UNanoManager::GetProcessResponseData(FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
	FProcessResponseData data;
	RETURN_ERROR_IF_INVALID_RESPONSE(data);

	data.hash = reqRespJson.response->GetStringField("hash");
	return data;
}

void UNanoManager::ListenForPaymentWaitConfirmation(
	FListenPaymentDelegate delegate, FString const& account, FString const& amount, UNanoWebsocket* websocket) {
	// Clear timer if this payment exists already.
	if (listeningPayment.delegate.IsBound()) {
		GetWorld()->GetTimerManager().ClearTimer(listeningPayment.timerHandle);
		Unwatch(account, listeningPayment.watchId, websocket);
	}

	listeningPayment.account = account;
	listeningPayment.amount = amount;
	listeningPayment.delegate = delegate;
	listeningPayment.timerHandle = FTimerHandle();

	FWatchAccountReceivedDelegate emptyDelegate;
	listeningPayment.watchId = Watch(emptyDelegate, account, websocket);

	// Set it up to check for pending blocks every few seconds in case the websocket connection has missed any
	GetWorld()->GetTimerManager().SetTimer(
		listeningPayment.timerHandle,
		[this, account, websocket]() {
			if (listeningPayment.delegate.IsBound() && listeningPayment.account == account) {
				// Get a single pending block of at least the minimum amount, if there's there consider payment as going through!
				Pending(account, TCHAR_TO_UTF8(*listeningPayment.amount), 1,
					[this, account, websocket](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
						auto pendingData = GetPendingResponseData(request, response, wasSuccessful);
						if (!pendingData.error && pendingData.blocks.Num() > 0) {
							for (auto pendingBlock : pendingData.blocks) {
								nano::amount pendingAmount;
								pendingAmount.decode_dec(TCHAR_TO_UTF8(*pendingBlock.amount));
								nano::amount listenAmount;
								listenAmount.decode_dec(TCHAR_TO_UTF8(*listeningPayment.amount));

								// Payment successful! clear and call delegate
								if ((pendingAmount > listenAmount || pendingAmount == listenAmount) && listeningPayment.account == account &&
										listeningPayment.delegate.IsBound()) {
									auto delegate = listeningPayment.delegate;
									Unwatch(account, listeningPayment.watchId, websocket);
									GetWorld()->GetTimerManager().ClearTimer(listeningPayment.timerHandle);
									delegate.ExecuteIfBound(pendingBlock.hash, pendingBlock.amount);
									delegate.Unbind();
								}
							}
						}
					});
			};
		},
		5.0f, true, 1.f);
}

void UNanoManager::CancelPayment(FString const& account, UNanoWebsocket* websocket) {
	Unwatch(account, listeningPayment.watchId, websocket);
	GetWorld()->GetTimerManager().ClearTimer(listeningPayment.timerHandle);

	if (listeningPayment.delegate.IsBound()) {
		listeningPayment.delegate.Unbind();
	}
}

void UNanoManager::ListenPayoutWaitConfirmation(
	const FListenPayoutDelegate& delegate, FString const& account, UNanoWebsocket* websocket, float expiryTime) {
	// Clear timer if this payment exists already.
	if (listeningPayout.delegate.IsBound()) {
		GetWorld()->GetTimerManager().ClearTimer(listeningPayout.timerHandle);
		Unwatch(account, listeningPayout.watchId, websocket);
	}
	listeningPayout.account = account;
	listeningPayout.expiryTime = expiryTime;
	listeningPayout.timerStart = std::chrono::steady_clock::now();
	listeningPayout.delegate = delegate;
	listeningPayout.timerHandle = FTimerHandle();

	FWatchAccountReceivedDelegate emptyDelegate;
	listeningPayout.watchId = Watch(emptyDelegate, account, websocket);

	// Set it up to check for pending blocks every few seconds in case the websocket connection has missed any
	GetWorld()->GetTimerManager().SetTimer(
		listeningPayout.timerHandle,
		[this, account, websocket]() {
			if (listeningPayout.account == account) {
				// First check if timer has expired

				auto expired = std::chrono::steady_clock::now() - listeningPayout.timerStart >
											 std::chrono::seconds(static_cast<int>(listeningPayout.expiryTime));
				if (expired) {
					GetWorld()->GetTimerManager().ClearTimer(listeningPayout.timerHandle);
					Unwatch(account, listeningPayout.watchId, websocket);
					if (listeningPayout.delegate.IsBound()) {
						auto delegate = listeningPayout.delegate;
						delegate.ExecuteIfBound(true);
						delegate.Unbind();
					}
				} else {
					// Check if balance is 0, if so then call delegate
					GetWalletBalance(
						account, [this, account, websocket](FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
							auto balanceResponseData = GetBalanceResponseData(request, response, wasSuccessful);
							if (!balanceResponseData.error && balanceResponseData.balance == "0" && listeningPayout.account == account) {
								Unwatch(account, listeningPayout.watchId, websocket);
								if (listeningPayout.delegate.IsBound()) {
									GetWorld()->GetTimerManager().ClearTimer(listeningPayout.timerHandle);
									auto delegate = listeningPayout.delegate;
									delegate.ExecuteIfBound(false);
									delegate.Unbind();
								}
							}
						});
				}
			}
		},
		5.0f, true, 1.f);
}

void UNanoManager::CancelPayout(FString const& account, UNanoWebsocket* websocket) {
	Unwatch(account, listeningPayout.watchId, websocket);
	GetWorld()->GetTimerManager().ClearTimer(listeningPayout.timerHandle);

	if (listeningPayout.delegate.IsBound()) {
		listeningPayout.delegate.Unbind();
	}
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

void UNanoManager::MakeRequest(
	TSharedPtr<FJsonObject> JsonObject, TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> delegate) {
	auto HttpRequest = CreateHttpRequest(JsonObject);
	HttpRequest->OnProcessRequestComplete().BindLambda(delegate);
	HttpRequest->ProcessRequest();
}

auto UNanoManager::GetResponseJson(FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) -> ReqRespJson {
	// Make sure we are getting json
	ReqRespJson reqRespJson;

	if (!wasSuccessful || !response.IsValid()) {
		return reqRespJson;
	}

	if (request->GetContentType() == "application/json" && response->GetContentType().StartsWith("application/json")) {
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

bool UNanoManager::RequestResponseIsValid(FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful) {
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

FString UNanoManager::getDefaultDataPath() const {
	FString result;
#if PLATFORM_WINDOWS
	WCHAR path[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
		std::wstring string(path);
		string += L"\\NanoGames";
		result = string.c_str();
	} else {
		check(false);
	}
#elif PLATFORM_MAC
	NSString* dir_string = [NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES) lastObject];
	char const* dir_chars = [dir_string UTF8String];
	std::string string(dir_chars);
	string += "/NanoGames";
	result = string.c_str();
	[dir_string release];
#elif PLATFORM_LINUX
	auto entry(getpwuid(getuid()));
	check(entry != nullptr);
	std::string string(entry->pw_dir);
	string += "/NanoGames";
	result = string.c_str();
#else
	result = FPaths::ProjectSavedDir() / "NanoGames";
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
