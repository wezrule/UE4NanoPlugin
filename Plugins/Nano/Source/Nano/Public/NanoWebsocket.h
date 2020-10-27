#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "IWebSocket.h"	 // Socket definition
#include "Runtime/Online/HTTP/Public/Http.h"

#include "NanoWebsocket.generated.h"

USTRUCT(BlueprintType)
struct NANO_API FWebsocketConnectResponseData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WebsocketConnect")
	bool error{true};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WebsocketConnect")
	FString errorMessage;
};

UENUM(BlueprintType)
enum class FSubtype : uint8 { send, receive, change, epoch, open };

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
class NANO_API UNanoWebsocket : public UObject {
	GENERATED_BODY()

public:
	/** Register an account to listen to websocket confirmation events for **/
	UFUNCTION(BlueprintCallable, Category = "UNanoWebsocket")
	void RegisterAccount(const FString& account);

	/** Unregister an account**/
	UFUNCTION(BlueprintCallable, Category = "UNanoWebsocket")
	void UnregisterAccount(const FString& account);

	/**
	* This will attempt to connect to the websocket. On first successful connection,
	* will keep trying to reconnect
	*/
	UFUNCTION(BlueprintCallable, Category = "UNanoWebsocket")
	void Connect(const FString& url, FWebsocketConnectedDelegate delegate);

	/** This hooks onto all available websocket events */
	UPROPERTY(BlueprintAssignable, Category = WebSocket)
	FWebsocketMessageResponseDelegate onResponse;

protected:
	void BeginDestroy() override;

private:
	TSharedPtr<IWebSocket> Websocket;
	FTimerHandle timerHandle;

	bool isReconnection{false};

	// TODO: Should use nano::account
	TMap<FString, int> registeredAccounts;	// account and number of times it was registered
};
