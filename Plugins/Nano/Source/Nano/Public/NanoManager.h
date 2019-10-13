#pragma once  

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "NanoTypes.h"
#include "NanoWebsocket.h"

#include "Runtime/Core/Public/GenericPlatform/GenericPlatformMisc.h"

#include <functional>
#include <unordered_map>
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

	FString prv_key; // If null that means we are just watching it?
	FAutomateResponseReceivedDelegate delegate;
	FTimerHandle timerHandle;
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
	void RequestNano(FReceivedNanoDelegate delegate, FString nanoAddress);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void AccountFrontier(FAccountFrontierResponseReceivedDelegate delegate, FString account);

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void Pending(FPendingResponseReceivedDelegate delegate, FString account);

	// Utility functions
	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void Send(FProcessResponseReceivedDelegate delegate, FString const& private_key, FString const& account, FString const& amount);

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
	TArray<FString> GetSeedFiles1();

	UFUNCTION(BlueprintCallable, Category = "NanoManager")
	void SetupReceiveMessageWebsocketListener(UNanoWebsocket* websocket);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NanoManager")
	FString rpcAddress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NanoManager")
	int rpcPort;

	UPROPERTY(EditAnywhere, Category = "NanoManager")
	FString defaultRepresentative { "nano_1thingspmippfngcrtk1ofd3uwftffnu4qu9xkauo9zkiuep6iknzci3jxa6" };

private:
	std::unordered_map<std::string, PrvKeyAutomateDelegate> keyDelegateMap;

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

	TSharedPtr<FJsonObject> GetPendingJsonObject(FString account);
	static FPendingResponseData GetPendingResponseData(RESPONSE_PARAMETERS);
	static FGetBalanceResponseData GetBalanceResponseData(RESPONSE_PARAMETERS);
	static FProcessResponseData GetProcessResponseData(RESPONSE_PARAMETERS);
	static FRequestNanoResponseData GetRequestNanoData(RESPONSE_PARAMETERS);

	void Pending(FString account, TFunction<void(RESPONSE_PARAMETERS)> const& d);

	void Process(FBlock block, TFunction<void(RESPONSE_PARAMETERS)> const& delegate);

	void AutomateWorkGenerateLoop(FAccountFrontierResponseData frontierData, const PrvKeyAutomateDelegate* prvKeyAutomateDelegate_receiver, TArray<FPendingBlock> pendingBlocks);
	void AutomatePocketPendingUtility(const FString& account, const PrvKeyAutomateDelegate* prvKeyAutomateDelegate_receiver);

	void GetFrontierAndFire(const TSharedPtr<FJsonObject>& message_json, FString const & account, PrvKeyAutomateDelegate const * prvKeyAutomateDelegate, bool isSend);

	UFUNCTION()
	void OnReceiveMessage(const FString& data);

	FString getDefaultDataPath() const;
	FString dataPath{ getDefaultDataPath() };
};
