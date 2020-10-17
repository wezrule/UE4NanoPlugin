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

UENUM(BlueprintType)
 enum class FSubtype : uint8 {
	send,
	receive,
	change,
	epoch,
	open
};

// This is needed for Blueprint by user
USTRUCT(BlueprintType)
struct NANO_API FWebsocketBlock {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Block")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Block")
	FString previous;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Block")
	FString representative;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Block")
	FString balance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Block")
	FString link;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Block")
	FSubtype subtype;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Block")
	FString work;
};

USTRUCT(BlueprintType)
struct NANO_API FWebsocketConfirmationResponseData {
	
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WebsocketConfirmationResponse")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WebsocketConfirmationResponse")
	FString amount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WebsocketConfirmationResponse")
	FString hash;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WebsocketConfirmationResponse")
	FWebsocketBlock block;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FWebsocketConnectedDelegate, FWebsocketConnectResponseData, data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWebsocketMessageResponseDelegate, const FWebsocketConfirmationResponseData&, data);

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

	UFUNCTION(BlueprintCallable, Category="UNanoWebsocket")
	void ListenAllConfirmations ();

protected:
	void BeginDestroy () override;

private:
	TSharedPtr<IWebSocket> Websocket;
	FTimerHandle timerHandle;

	FCriticalSection mutex;

	// TODO: Should use nano::account
	TSet<FString> registeredAccounts;
};
