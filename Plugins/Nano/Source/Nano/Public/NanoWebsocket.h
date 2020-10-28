// Copyright 2020 Wesley Shillingford. All rights reserved.
#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "IWebSocket.h"
#include "Http.h"

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FWebsocketMessageResponseDelegate, const FWebsocketConfirmationResponseData&, data, UNanoWebsocket*, websocket);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWebsocketReconnectDelegate, const FWebsocketConnectResponseData&, data);

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
	 * will keep trying to reconnect, will only call delegate once!
	 */
	UFUNCTION(BlueprintCallable, Category = "UNanoWebsocket")
	void Connect(const FString& url, FWebsocketConnectedDelegate delegate);

	/** To save some initialization troubles Connect only calls the delegate once, if you
	 * require knowing if there is a reconnection, hook into this event
	 */
	UPROPERTY(BlueprintAssignable, Category = "UNanoWebsocket")
	FWebsocketReconnectDelegate onReconnect;

	/** This hooks onto all the websocket events for registered accounts */
	UPROPERTY(BlueprintAssignable, Category = "UNanoWebsocket")
	FWebsocketMessageResponseDelegate onFilteredResponse;

	/** This hooks onto all available websocket events */
	UPROPERTY(BlueprintAssignable, Category = "UNanoWebsocket")
	FWebsocketMessageResponseDelegate onResponse;

	UFUNCTION(BlueprintCallable, Category = "UNanoWebsocket")
	void ListenAll();

	UFUNCTION(BlueprintCallable, Category = "UNanoWebsocket")
	void UnlistenAll();

protected:
	void BeginDestroy() override;

private:
	TSharedPtr<IWebSocket> Websocket;
	FTimerHandle timerHandle;

	bool isReconnection{false};
	bool isListeningAll{false};

	// TODO: Should use nano::account
	TMap<FString, int> registeredAccounts;	// account and number of times it was registered
};
