// Copyright 2020 Wesley Shillingford. All rights reserved.
#include "NanoWebsocket.h"

#include "Engine/World.h"
#include "Json.h"
#include "JsonObjectConverter.h"
#include "Modules/ModuleManager.h"
#include "NanoBlueprintLibrary.h"
#include "NanoTypes.h"
#include "TimerManager.h"
#include "WebSocketsModule.h"

#include <nano/blocks.h>
#include <nano/numbers.h>

namespace {
template <typename T>
FString MakeOutputString(T const& ustruct) {
	TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(ustruct);
	FString OutputString;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
	return OutputString;
}
}	 // namespace

void UNanoWebsocket::BeginDestroy() {
	Super::BeginDestroy();

	if (Websocket) {
		Websocket->Close();
	}
}

UFUNCTION(BlueprintCallable, Category = "UNanoWebsocket")
void UNanoWebsocket::Connect(const FString& wsURL, FWebsocketConnectedDelegate delegate) {
	if (!FModuleManager::Get().IsModuleLoaded("WebSockets")) {
		FModuleManager::Get().LoadModule("WebSockets");
	}

	Websocket = FWebSocketsModule::Get().CreateWebSocket(wsURL, TEXT("ws"));

	// Need to call all these before connecting (I think)

	// This is when a connection is successfully made.
	Websocket->OnConnected().AddLambda([delegate, this]() -> void {
		for (auto const& accountNumPair : registeredAccounts) {
			FRegisterAccountRequestData registerAccount;
			registerAccount.account = accountNumPair.Key;
			Websocket->Send(MakeOutputString(registerAccount));
		}

		if (isListeningAll) {
			ListenAll();
		}

		// This will run once connected.
		if (!isReconnection) {
			FWebsocketConnectResponseData data;
			data.error = false;
			delegate.ExecuteIfBound(data);
		}
	});

	Websocket->OnConnectionError().AddLambda([delegate, this](const FString& errorMessage) -> void {
		// This will run if the connection failed. Check Error to see what happened.
		FWebsocketConnectResponseData data;
		data.error = true;
		data.errorMessage = errorMessage;
		if (!isReconnection) {
			delegate.ExecuteIfBound(data);
		} else {
			onReconnect.Broadcast(data);
		}
	});

	Websocket->OnClosed().AddLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean) -> void {
		// This code will run when the connection to the server has been terminated.
		// Because of an error or a call to Socket->Close().
		if (!bWasClean) {
			// Try to reconnect, set this flag so we don't call the delegates anymore
			isReconnection = true;
			Websocket->Connect();
		}
	});

	Websocket->OnMessage().AddLambda([this](const FString& inData) -> void {
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(inData);
		TSharedPtr<FJsonObject> response;
		if (FJsonSerializer::Deserialize(JsonReader, response)) {
			// Only care about confirmation websocket events
			auto topic = response->GetStringField("topic");
			if (topic == "confirmation") {
				auto messageJson = response->GetObjectField("message");

				FWebsocketConfirmationResponseData data;
				data.account = messageJson->GetStringField("account");
				data.amount = messageJson->GetStringField("amount");
				data.hash = messageJson->GetStringField("hash");

				auto blockJson = messageJson->GetObjectField("block");

				data.block.account = blockJson->GetStringField("account");
				data.block.balance = blockJson->GetStringField("balance");
				data.block.link = blockJson->GetStringField("link");
				data.block.previous = blockJson->GetStringField("previous");
				data.block.representative = blockJson->GetStringField("representative");
				data.block.work = blockJson->GetStringField("work");

				auto subtypeStr = blockJson->GetStringField("subtype");
				// If there's no subtype, it means it's not a state block
				if (!subtypeStr.IsEmpty()) {
					FSubtype subtype;
					if (subtypeStr == "send") {
						subtype = FSubtype::send;
					} else if (subtypeStr == "receive") {
						subtype = FSubtype::receive;
					} else if (subtypeStr == "open") {
						subtype = FSubtype::open;
					} else if (subtypeStr == "change") {
						subtype = FSubtype::change;
					} else {
						check(subtypeStr == "epoch");
						subtype = FSubtype::epoch;
					}

					data.block.subtype = subtype;

					auto isFiltered = response->GetBoolField("is_filtered");
					if (isFiltered) {
						onFilteredResponse.Broadcast(data, this);
					} else {
						onResponse.Broadcast(data, this);
					}
				} else {
					UE_LOG(LogTemp, Warning,
						TEXT("Receiving legacy confirmation callbacks, node is likely not synced yet so some operations will not work."));
				}
			}
		}
	});

	Websocket->Connect();

	// Set up a timer to try and reconnects the websocket if the connection is lost.
	GetWorld()->GetTimerManager().SetTimer(
		timerHandle,
		[this]() {
			if (!Websocket->IsConnected()) {
				Websocket->Connect();
			}
		},
		5.0f, true, 5.f);
}

void UNanoWebsocket::RegisterAccount(const FString& account) {
	auto val = registeredAccounts.Find(account);
	if (val != nullptr) {
		// Just increment the number of listeners
		++*val;
	} else {
		registeredAccounts.Emplace(account, 1);
		if (!Websocket->IsConnected()) {
			// Don't send if we're not connected.
			return;
		}

		// Create JSON
		FRegisterAccountRequestData registerAccount;
		registerAccount.account = account;
		Websocket->Send(MakeOutputString(registerAccount));
	}
}

void UNanoWebsocket::UnregisterAccount(const FString& account) {
	auto val = registeredAccounts.Find(account);
	if (val) {
		if (*val > 1) {
			--*val;
		} else {
			registeredAccounts.Remove(account);

			if (!Websocket->IsConnected()) {
				// Don't send if we're not connected.
				return;
			}

			// Create JSON
			FUnRegisterAccountRequestData unregisterAccount;
			unregisterAccount.account = account;

			TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(unregisterAccount);
			FString OutputString;
			TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
			FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

			Websocket->Send(OutputString);
		}
	}
}

void UNanoWebsocket::ListenAll() {
	Websocket->Send("{\"action\":\"listen_all\"}");
	isListeningAll = true;
}

void UNanoWebsocket::UnlistenAll() {
	Websocket->Send("{\"action\":\"unlisten_all\"}");
	isListeningAll = false;
}
