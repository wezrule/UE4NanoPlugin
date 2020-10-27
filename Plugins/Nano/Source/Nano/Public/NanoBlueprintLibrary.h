#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Runtime/Online/HTTP/Public/Interfaces/IHttpRequest.h"

#include "NanoBlueprintLibrary.generated.h"

UCLASS()
class NANO_API UNanoBlueprintLibrary : public UBlueprintFunctionLibrary {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString RawToNano(const FString& raw);

	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString NanoToRaw(const FString& nano);

	/** True if the raw entered is valid and can be processed by other functions. Always outputs decimal unit as . */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static bool ValidateRaw(const FString& raw);

	/** True if the nano entered is valid and can be processed by other functions. Accepts . or , as decimal */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static bool ValidateNano(const FString& nano);

	/** Need to ensure there is no overflow! */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString Add(FString raw1, FString raw2);

	/** Precondition that raw1 is >= raw2 */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString Subtract(FString raw1, FString raw2);

	/** 1 Nano = 10 ^ 6 nano */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString ConvertnanoToRaw(FString nano);

	/** Returns true if raw is greater than baseRaw */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static bool Greater(FString raw, FString baseRaw);

	/** Returns true if raw is greater than or equal to baseRaw */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static bool GreaterOrEqual(FString raw, FString baseRaw);

	/** Create a cryptographically secure random number as a seed */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString CreateSeed();

	/** Get the private key from a seed and index */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString PrivateKeyFromSeed(const FString& seed, int32 index);

	/** Get the public key in hex from a seed and index */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString PublicKeyFromSeed(const FString& seed, int32 index);

	/** Get the public key as an account from a seed and index */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString AccountFromSeed(const FString& seed, int32 index);

	/** Get the account from a hex public key */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString AccountFromPublicKey(const FString& publicKey);

	/** Get the public key as hex from a nano_ account */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString PublicKeyFromAccount(const FString& account);

	/** Get the public key as hex from a hex private key */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString PublicKeyFromPrivateKey(const FString& privateKey);

	/** Get the account from a hex private key */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString AccountFromPrivateKey(const FString& privateKey);

	/** Performs a sha256 hash on the passed in string */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString SHA256(const FString& string);

	/** Encrypt the private key with the password */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString Encrypt(FString prvKey, const FString& password);

	/** Decrypt the cypher text into plain text with the password */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static FString Decrypt(FString cipherPrvKey, const FString& password);

	/** Generate a QR Code of Width (Size) x Height (Size) */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static UTexture2D* GenerateQRCodeTexture(const int32& Size, const FString& account, int32 Margin = 10);
	
	/** Generate a QR code of Width (Size) x Height (Size) specifying an amount to pay as well */
	UFUNCTION(BlueprintCallable, Category = "Nano")
	static UTexture2D* GenerateQRCodeTextureWithAmount(
		const int32& Size, const FString& account, const FString& amount, int32 Margin = 10);
};
