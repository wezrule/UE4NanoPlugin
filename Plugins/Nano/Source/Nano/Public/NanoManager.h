#pragma once  

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "NanoTypes.h"
#include "NanoWebsocket.h"

#include "Runtime/Core/Public/GenericPlatform/GenericPlatformMisc.h"

#include <functional>
#include <unordered_map>
#include <string>
#include "NanoManager.generated.h"

#define RESPONSE_PARAMETERS FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful

class PrvKeyAutomateDelegate {
public:
	PrvKeyAutomateDelegate() = default;
	PrvKeyAutomateDelegate(const FString& prv_key, const FAutomateResponseReceivedDelegate& delegate)
		: prv_key(prv_key)
		, delegate(delegate) { }

	PrvKeyAutomateDelegate(const PrvKeyAutomateDelegate&) = delete;
	PrvKeyAutomateDelegate& operator= (const PrvKeyAutomateDelegate&) = delete;

	FString prv_key; // If null that means we are just watching it
	FAutomateResponseReceivedDelegate delegate;
	FTimerHandle timerHandle;
};

class SendDelegate {
public:
	SendDelegate() = default;
	SendDelegate(const FProcessResponseReceivedDelegate& delegate_, const FProcessResponseData& data)
		: delegate(delegate_),
	data (data) { }

	SendDelegate(const SendDelegate&) = delete;
	SendDelegate& operator= (const SendDelegate&) = delete;

	FProcessResponseReceivedDelegate delegate;
	FProcessResponseData data;
	FTimerHandle timerHandle;
};

struct ListeningPayment {
	FListenPaymentDelegate delegate;
	FString account;
	FString amount;
};

UCLASS(BlueprintType, Blueprintable)
class NANO_API UNanoManager : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void GetWalletBalance(FGetBalanceResponseReceivedDelegate delegate, FString nanoAddress);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void WorkGenerate(FWorkGenerateResponseReceivedDelegate delegate, FString hash);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void Process(FProcessResponseReceivedDelegate delegate, FBlock block);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void ProcessWaitConfirmation (FProcessResponseReceivedDelegate delegate, FBlock block);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void RequestNano(FReceivedNanoDelegate delegate, FString nanoAddress);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void AccountFrontier(FAccountFrontierResponseReceivedDelegate delegate, FString account);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void Pending(FPendingResponseReceivedDelegate delegate, FString account);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void BlockConfirmed(FBlockConfirmedResponseReceivedDelegate delegate, FString hash);

	// Utility functions
	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void Send(FProcessResponseReceivedDelegate delegate, FString const& private_key, FString const& account, FString const& amount);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void SendWaitConfirmation (FProcessResponseReceivedDelegate delegate, FString const& private_key, FString const& account, FString const& amount);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void Automate(FAutomateResponseReceivedDelegate delegate, FString const& private_key, UNanoWebsocket* websocket);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void AutomateUnregister(const FString& account, UNanoWebsocket* websocket);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void Watch(FAutomateResponseReceivedDelegate delegate, FString const& account, UNanoWebsocket* websocket);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void Unwatch(FString const& account, UNanoWebsocket* websocket);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void SetDataSubdirectory(FString const& subdir);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	FString GetPlainSeed(FString const& seedFileName, FString const& password) const;

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void SaveSeed(FString const& plainSeed, FString const& seedFileName, FString const& password);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	int32 GetNumSeedFiles() const;

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	TArray<FString> GetSeedFiles() const;

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void SetupConfirmationMessageWebsocketListener(UNanoWebsocket* websocket);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void ListenForPaymentWaitConfirmation (FListenPaymentDelegate delegate, FString const& account, FString const& amount);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NanoManager")
	FString rpcUrl;

	UPROPERTY(EditAnywhere, Category = "NanoManager")
	FString defaultRepresentative { "nano_1iuz18n4g4wfp9gf7p1s8qkygxw7wx9qfjq6a9aq68uyrdnningdcjontgar" };

private:
	FCriticalSection keyDelegateMutex;
	std::unordered_map<std::string, PrvKeyAutomateDelegate> keyDelegateMap; // Can't use TMap due to reference invalidation after emplace.

	FCriticalSection sendBlockListenerMutex;
	std::unordered_map<std::string, SendDelegate> sendBlockListener;

	FCriticalSection listeningPaymentMutex;
	ListeningPayment listeningPayment;

	struct ReqRespJson {
		TSharedPtr<FJsonObject> request;
		TSharedPtr<FJsonObject> response;
	};

	static ReqRespJson GetResponseJson(RESPONSE_PARAMETERS);
	static bool RequestResponseIsValid(RESPONSE_PARAMETERS);

	void MakeRequest(TSharedPtr<FJsonObject> JsonObject, TFunction<void(RESPONSE_PARAMETERS)> delegate);

	TSharedRef<IHttpRequest> CreateHttpRequest(TSharedPtr<FJsonObject> JsonObject);

	TSharedPtr<FJsonObject> GetAccountFrontierJsonObject(FString const& account);
	FAccountFrontierResponseData GetAccountFrontierResponseData(RESPONSE_PARAMETERS) const;

	void AccountFrontier(FString account, TFunction<void(RESPONSE_PARAMETERS)> const& d);

	TSharedPtr<FJsonObject> GetWorkGenerateJsonObject(FString hash);
	static FWorkGenerateResponseData GetWorkGenerateResponseData(RESPONSE_PARAMETERS);

	void WorkGenerate(FString hash, TFunction<void(RESPONSE_PARAMETERS)> const& d);

	void Send(FString const& private_key, FString const& account, FString const& amount, TFunction<void(FProcessResponseData)> const& delegate);

	void RegisterBlockListener (std::string const & account, FProcessResponseData const & process_response_data, FProcessResponseReceivedDelegate delegate);

	TSharedPtr<FJsonObject> GetPendingJsonObject(FString account);
	static FPendingResponseData GetPendingResponseData(RESPONSE_PARAMETERS);
	static FGetBalanceResponseData GetBalanceResponseData(RESPONSE_PARAMETERS);
	static FProcessResponseData GetProcessResponseData(RESPONSE_PARAMETERS);
	static FRequestNanoResponseData GetRequestNanoData(RESPONSE_PARAMETERS);

	void BlockConfirmed(FString hash, TFunction<void(RESPONSE_PARAMETERS)> const& d);
	TSharedPtr<FJsonObject> GetBlockConfirmedJsonObject(FString const& hash);
	static FBlockConfirmedResponseData GetBlockConfirmedResponseData(RESPONSE_PARAMETERS);

	void Pending(FString account, TFunction<void(RESPONSE_PARAMETERS)> const& d);

	void Process(FBlock block, TFunction<void(RESPONSE_PARAMETERS)> const& delegate);

	void AutomateWorkGenerateLoop(FAccountFrontierResponseData frontierData, TArray<FPendingBlock> pendingBlocks);
	void AutomatePocketPendingUtility(const FString& account);

	void GetFrontierAndFire(const FString& amount, const FString& hash, FString const & account, FConfType type);

	UFUNCTION()
	void OnConfirmationReceiveMessage(const FWebsocketConfirmationResponseData & data);

	FString getDefaultDataPath() const;
	FString dataPath{ getDefaultDataPath() };
};
