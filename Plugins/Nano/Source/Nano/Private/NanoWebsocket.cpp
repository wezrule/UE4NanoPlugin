#include "NanoWebsocket.h"
#include "Json.h"
#include "JsonObjectConverter.h"
#include <nano/blocks.h>
#include <nano/numbers.h>
#include "NanoBlueprintLibrary.h"
#include "NanoTypes.h"

#include "Runtime/Engine/Public/TimerManager.h"
#include "Runtime/Engine/Classes/Engine/World.h"

#include "WebSocketsModule.h" // Module definition
#include "Modules/ModuleManager.h"

namespace
{
template<typename T>
FString MakeOutputString (T const & ustruct)
{
	TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(ustruct);
	FString OutputString;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
	return OutputString;
}
}

void UNanoWebsocket::BeginDestroy()
{
	Super::BeginDestroy();

	if (Websocket)
	{
		Websocket->Close();
	}
}

UFUNCTION(BlueprintCallable, Category="UNanoWebsocket")
void UNanoWebsocket::Connect (const FString &wsURL, FWebsocketConnectedDelegate delegate)
{
	if(!FModuleManager::Get().IsModuleLoaded("WebSockets"))
	{
		FModuleManager::Get().LoadModule("WebSockets");
	}

	Websocket = FWebSocketsModule::Get().CreateWebSocket(wsURL, TEXT("ws"));

	// Need to call all these before connecting (I think)
	Websocket->OnConnected().AddLambda([delegate, this]() -> void {
		{
			FScopeLock lk(&mutex);
			for (auto const & account : registeredAccounts) {
				FRegisterAccountRequestData registerAccount;
				registerAccount.account = account;
				Websocket->Send(MakeOutputString (registerAccount));
			}
		}

		 // This will run once connected.
		FWebsocketConnectResponseData data;
		data.error = false;
		delegate.ExecuteIfBound(data);
	});

	Websocket->OnConnectionError().AddLambda([delegate](const FString& errorMessage) -> void {
		// This will run if the connection failed. Check Error to see what happened.
		FWebsocketConnectResponseData data;
		data.error = true;
		data.errorMessage = errorMessage;
		delegate.ExecuteIfBound(data);
	});

	Websocket->OnClosed().AddLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean) -> void {
		// This code will run when the connection to the server has been terminated.
		// Because of an error or a call to Socket->Close().
		if (!bWasClean)
		{
			// Try to reconnect 
			Websocket->Connect();
		}
	});

	Websocket->OnMessage().AddLambda([this](const FString& MessageString) -> void {
		onResponse.Broadcast (MessageString);
	});

	Websocket->Connect();

	// Set up a timer to try and reconnects the websocket if the connection is lost.
	GetWorld()->GetTimerManager().SetTimer(timerHandle, [this]() {
		if (!Websocket->IsConnected ())
		{
			Websocket->Connect ();
		}
	}, 	5.0f, true, 5.f);
}

/*
void UNanoWebsocket::WatchAccount(const FString& account)
{
	// Create JSON
	FWatchAccountRequestData watchAccount;
	watchAccount.account = account;
	Websocket->SendText(MakeOutputString (watchAccount);
}*/

void UNanoWebsocket::RegisterAccount(const FString& account)
{
	{
		FScopeLock lk(&mutex);
		registeredAccounts.Emplace (account);
	}
	if (!Websocket->IsConnected())
	{
		// Don't send if we're not connected.
		return;
	}

	// Create JSON
	FRegisterAccountRequestData registerAccount;
	registerAccount.account = account;
	Websocket->Send(MakeOutputString (registerAccount));
}

void UNanoWebsocket::UnregisterAccount(const FString& account)
{
	{
		FScopeLock lk(&mutex);
		registeredAccounts.Remove (account);
	}
	if (!Websocket->IsConnected())
	{
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
