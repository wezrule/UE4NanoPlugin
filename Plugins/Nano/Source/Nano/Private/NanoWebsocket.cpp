#include "NanoWebsocket.h"
#include "Json.h"
#include "JsonObjectConverter.h"
#include <nano/blocks.h>
#include <nano/numbers.h>
#include "NanoBlueprintLibrary.h"
#include "NanoTypes.h"

void UNanoWebsocket::BeginDestroy()
{
	Super::BeginDestroy();

	if (Websocket)
	{
		Websocket->Close();
	}
}

template<typename T>
FString MakeOutputString (T const & ustruct)
{
	TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(ustruct);
	FString OutputString;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
	return OutputString;
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
	// Create JSON
	FRegisterAccountRequestData registerAccount;
	registerAccount.account = account;
	Websocket->SendText(MakeOutputString (registerAccount));
}

void UNanoWebsocket::UnregisterAccount(const FString& account)
{
	// Create JSON
	FUnRegisterAccountRequestData unregisterAccount;
	unregisterAccount.account = account;

	TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(unregisterAccount);
	FString OutputString;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

	Websocket->SendText(OutputString);
}
