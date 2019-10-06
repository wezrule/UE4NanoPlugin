#pragma once  

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "WebSocketBlueprintLibrary.h"
#include "WebSocketBase.h"
#include "NanoWebsocket.generated.h"

UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NANO_API UNanoWebsocket : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="UNanoWebsocket")
	void RegisterAccount (const FString &account);

	UFUNCTION(BlueprintCallable, Category="UNanoWebsocket")
	void UnregisterAccount (const FString &account);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UWebSocketBase *Websocket = nullptr;

protected:
	void BeginDestroy () override;
};
