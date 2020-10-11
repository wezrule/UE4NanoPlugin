#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Runtime/Online/HTTP/Public/Interfaces/IHttpRequest.h"
#include "NanoBlueprintLibrary.generated.h"

UCLASS()
class NANO_API UNanoBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString RawToNano(const FString& raw);

	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString NanoToRaw(const FString& nano);

	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString Add(FString raw1, FString raw2);

	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString Subtract(FString raw1, FString raw2);

	// 1 Nano = 10 ^ 6 nano
	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString ConvertnanoToRaw(FString nano);

	// Returns true if raw is greater than baseRaw
	UFUNCTION(BlueprintCallable, Category="Nano")
	static bool Greater(FString raw, FString baseRaw);

	UFUNCTION(BlueprintCallable, Category="Nano")
	static bool GreaterOrEqual(FString raw, FString baseRaw);

	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString CreateSeed();

	// Get the private key from a seed and index
	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString PrivateKeyFromSeed(const FString& seed, int32 index);

	// Get the public key in hex from a seed and index
	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString PublicKeyFromSeed(const FString& seed, int32 index);

	// Get the public key as an account from a seed and index
	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString AccountFromSeed(const FString& seed, int32 index);

	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString AccountFromPublicKey(const FString& publicKey);

	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString PublicKeyFromAccount(const FString& account);

	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString PublicKeyFromPrivateKey(const FString& privateKey);

	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString AccountFromPrivateKey(const FString& privateKey);

	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString SHA256(const FString& string);

	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString Encrypt(FString prvKey, const FString& password);

	UFUNCTION(BlueprintCallable, Category="Nano")
	static FString Decrypt(FString cipherPrvKey, const FString& password);
};
