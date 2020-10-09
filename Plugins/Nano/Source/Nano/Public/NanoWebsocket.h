#pragma once  

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Online/HTTP/Public/Http.h"

#include "IWebSocket.h"       // Socket definition

#include "NanoWebsocket.generated.h"

USTRUCT(BlueprintType)
struct NANO_API FWebsocketConnectResponseData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WebsocketConnect")
	bool error {true};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WebsocketConnect")
	FString errorMessage;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FWebsocketConnectedDelegate, FWebsocketConnectResponseData, data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWebsocketMessageResponseDelegate, const FString&, data);

UCLASS(BlueprintType, Blueprintable)
class NANO_API UNanoWebsocket : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="UNanoWebsocket")
	void RegisterAccount (const FString &account);

	UFUNCTION(BlueprintCallable, Category="UNanoWebsocket")
	void UnregisterAccount (const FString &account);

	UFUNCTION(BlueprintCallable, Category="UNanoWebsocket")
	void Connect (const FString &url, FWebsocketConnectedDelegate delegate);

	UPROPERTY(BlueprintAssignable, Category = WebSocket)
	FWebsocketMessageResponseDelegate onResponse;

protected:
	void BeginDestroy () override;

private:
	TSharedPtr<IWebSocket> Websocket;
};
