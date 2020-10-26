#include "NanoWebsocket.h"

#include "Json.h"
#include "JsonObjectConverter.h"
#include "Modules/ModuleManager.h"
#include "NanoBlueprintLibrary.h"
#include "NanoTypes.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "Runtime/Engine/Public/TimerManager.h"
#include "WebSocketsModule.h"	 // Module definition

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
	Websocket->OnConnected().AddLambda([delegate, this]() -> void {
		for (auto const& account_num_pair : registeredAccounts) {
			FRegisterAccountRequestData registerAccount;
			registerAccount.account = account_num_pair.Key;
			Websocket->Send(MakeOutputString(registerAccount));
		}

		// This will run once connected.
		FWebsocketConnectResponseData data;
		data.error = false;
		delegate.ExecuteIfBound(data);
	});

	Websocket->OnConnectionError().AddLambda([delegate, this](const FString& errorMessage) -> void {
		// This will run if the connection failed. Check Error to see what happened.
		if (!isReconnection) {
			FWebsocketConnectResponseData data;
			data.error = true;
			data.errorMessage = errorMessage;
			delegate.ExecuteIfBound(data);
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

	Websocket->OnMessage().AddLambda([this](const FString& in_data) -> void {
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(in_data);
		TSharedPtr<FJsonObject> response;
		if (FJsonSerializer::Deserialize(JsonReader, response)) {
			// Only care about confirmation websocket events
			auto topic = response->GetStringField("topic");
			if (topic == "confirmation") {
				auto message_json = response->GetObjectField("message");

				FWebsocketConfirmationResponseData data;
				data.account = message_json->GetStringField("account");
				data.amount = message_json->GetStringField("amount");
				data.hash = message_json->GetStringField("hash");

				auto block_json = message_json->GetObjectField("block");

				data.block.account = block_json->GetStringField("account");
				data.block.balance = block_json->GetStringField("balance");
				data.block.link = block_json->GetStringField("link");
				data.block.previous = block_json->GetStringField("previous");
				data.block.representative = block_json->GetStringField("representative");
				data.block.work = block_json->GetStringField("work");

				auto subtype_str = block_json->GetStringField("subtype");
				// If there's no subtype, it means it's not a state block
				if (!subtype_str.IsEmpty()) {
					FSubtype subtype;
					if (subtype_str == "send") {
						subtype = FSubtype::send;
					} else if (subtype_str == "receive") {
						subtype = FSubtype::receive;
					} else if (subtype_str == "open") {
						subtype = FSubtype::open;
					} else if (subtype_str == "change") {
						subtype = FSubtype::change;
					} else {
						check(subtype_str == "epoch");
						subtype = FSubtype::epoch;
					}

					data.block.subtype = subtype;

					onResponse.Broadcast(data);
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

// Do not mix this websocket object
void UNanoWebsocket::ListenAllConfirmations() {
	Websocket->Send("listen_all");
}
