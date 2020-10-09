#include "NanoBlueprintLibrary.h"
#include <memory>
#include <duthomhas/csprng.hpp>

#include <Runtime/Core/Public/Misc/AES.h>

#include <cassert>

#include <sha256/sha256.hpp>

#ifdef _WIN32
#pragma warning (disable : 4804 ) /* '/': unsafe use of type 'bool' in operation warnings */
#endif
#include <blake2/blake2.h>
#include <ed25519-donna/ed25519.h>
#include <nano/numbers.h>

namespace
{
nano::public_key PrivateKeyToPublicKeyData (const FString& privateKey);
nano::private_key SeedAccountPrvData (const FString& seed_f, int32 index);
nano::public_key SeedAccountPubData (const FString& seed, int32 index);
FAES::FAESKey GenKey (const FString& password);
}

FString UNanoBlueprintLibrary::NanoToRaw(const FString& nano) {

	// This can have up to 30 decimal places. Remove leading zeroes
	auto start_index = 0;
	auto str = std::string (TCHAR_TO_UTF8(*nano));
	for (auto i = str.begin (); i < str.end (); ++i, ++start_index)
	{
		if (*i == '0')
		{
			++start_index;
		}
		else
		{
			break;
		}
	}

	// Remove leading zeroes
	auto trimmed_left_zeroes = std::string (str.begin () + start_index, str.end ());

	// Remove decimal point (if exists) and add necessary trailing 0s to form exact raw number
	auto i = trimmed_left_zeroes.begin ();
	for (; i < trimmed_left_zeroes.end (); ++i)
	{
		if (*i == '.')
		{
			i = trimmed_left_zeroes.erase (i);
			break;
		}
	}

	auto num_zeroes_to_add = 30 - std::distance (i, trimmed_left_zeroes.end ());
	return (trimmed_left_zeroes + std::string (num_zeroes_to_add, '0')).c_str ();
}

using namespace std::literals;

FString UNanoBlueprintLibrary::RawToNano(const FString& raw) {

	// Insert a decimal 30 decimal places from the right
	auto str = std::string (TCHAR_TO_UTF8(*raw));

	if (str.size () <= 30)
	{
		str = "0."s + std::string (30 - str.size (), '0');
	}
	else
	{
		auto decimal_index = str.size () - 30;
		str.insert (str.begin () + decimal_index, '.');
	}

	auto index = 0;
	auto is_decimal = false;
	for (auto i = str.rbegin (); i < str.rend (); ++i, ++index)
	{
		if (*i != '0')
		{
			if (*i == '.')
			{
				--index;
			}
			break;		
		}
	}

	return std::string (str.begin (), str.begin () + str.size () - index).c_str ();
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::Add(FString raw1, FString raw2) {
	nano::amount amount1;
	amount1.decode_dec (TCHAR_TO_UTF8(*raw1));

	nano::amount amount2;
	amount2.decode_dec (TCHAR_TO_UTF8(*raw2));

	nano::amount total = (amount1.number () + amount2.number ());
	return total.to_string_dec ().c_str ();
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::Subtract(FString raw1, FString raw2) {

	nano::amount amount1;
	amount1.decode_dec (TCHAR_TO_UTF8(*raw1));

	nano::amount amount2;
	amount2.decode_dec (TCHAR_TO_UTF8(*raw2));

	nano::amount total = (amount1.number () - amount2.number ());
	return total.to_string_dec ().c_str ();
}

UFUNCTION(BlueprintCallable, Category="Nano")
bool UNanoBlueprintLibrary::Greater(FString raw, FString baseRaw) {
	nano::amount amount1;
	amount1.decode_dec (TCHAR_TO_UTF8(*raw));

	nano::amount amount2;
	amount2.decode_dec (TCHAR_TO_UTF8(*baseRaw));

	return amount1 > amount2;
}

UFUNCTION(BlueprintCallable, Category="Nano")
bool UNanoBlueprintLibrary::GreaterOrEqual(FString raw, FString baseRaw) {
	nano::amount amount1;
	amount1.decode_dec (TCHAR_TO_UTF8(*raw));

	nano::amount amount2;
	amount2.decode_dec (TCHAR_TO_UTF8(*baseRaw));

	return amount1 == amount2 || amount1 > amount2;
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::ConvertnanoToRaw(FString nano_f)
{
	nano::amount amount1;
	amount1.decode_dec (TCHAR_TO_UTF8(*nano_f));

	nano::amount multiplier (nano::xrb_ratio);

	nano::amount new_amount = (amount1.number () * multiplier.number ());
	return new_amount.to_string_dec ().c_str ();
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::CreateSeed() {
	duthomhas::csprng rng;
	nano::uint256_union seed;
	rng (seed.bytes);
	return FString (seed.to_string ().c_str ());
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::PrivateKeyFromSeed(const FString& seed, int32 index) {
	return SeedAccountPrvData (seed, index).to_string ().c_str();
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::PublicKeyFromPrivateKey(const FString& privateKey) {
	return PrivateKeyToPublicKeyData (privateKey).to_string ().c_str ();
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::AccountFromPrivateKey(const FString& privateKey) {
	return PrivateKeyToPublicKeyData (privateKey).to_account ().c_str ();
}

// As hex
UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::PublicKeyFromSeed(const FString& seed, int32 index) {
	return SeedAccountPubData(seed, index).to_string ().c_str ();
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::AccountFromSeed(const FString& seed, int32 index) {
	return SeedAccountPubData(seed, index).to_account ().c_str ();
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::PublicKeyFromAccount(const FString& account_f) {
	std::string account_str (TCHAR_TO_UTF8(*account_f));
	nano::account account;
	account.decode_account (account_str);
	return account.to_string ().c_str ();
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::AccountFromPublicKey(const FString& publicKey) {
	nano::account account (TCHAR_TO_UTF8(*publicKey));
	return account.to_account ().c_str ();
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::SHA256(const FString& string) {
	return sha256(TCHAR_TO_UTF8(*string)).c_str();
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::Encrypt(const FString& plainSeed, const FString& password) {
	check (plainSeed.Len () == 64);

	constexpr auto size = 32;
	uint8 blob[size];
	FString::ToHexBlob(plainSeed, blob, size);

	FAES::EncryptData(blob, size, GenKey (password));

	return FString::FromHexBlob (blob, size); //now generate hex string of encrypted data
}

UFUNCTION(BlueprintCallable, Category="Nano")
FString UNanoBlueprintLibrary::Decrypt(const FString& cipherSeed, const FString& password) {
	constexpr auto size = 32;
	uint8 blob[size];
	check (cipherSeed.Len () == 64);

	FAES::DecryptData(blob, size, GenKey (password));
	auto plain_text = FString::FromHexBlob (blob,size);
	if (plain_text != "")
	{
		check (plain_text.Len () == 64);
	}

	return plain_text;
}

namespace
{
nano::public_key PrivateKeyToPublicKeyData (const FString& privateKey_f) {
	nano::private_key private_key (TCHAR_TO_UTF8(*privateKey_f));
	nano::public_key public_key;
	ed25519_publickey (private_key.bytes.data (), public_key.bytes.data ());
	return public_key;
}

nano::private_key SeedAccountPrvData (const FString& seed_f, int32 index) {
	std::string seed_str (TCHAR_TO_UTF8(*seed_f));
	nano::uint256_union seed (seed_str);
	nano::private_key private_key;
	nano::deterministic_key (seed, index, private_key);
	return private_key;
}

nano::public_key SeedAccountPubData (const FString& seed, int32 index) {
	auto private_key = SeedAccountPrvData (seed, index);
	nano::public_key public_key;
	ed25519_publickey (private_key.bytes.data (), public_key.bytes.data ());
	return public_key;
}

FAES::FAESKey GenKey (const FString& password) {
	uint8 blob[32];
	FString::ToBlob(sha256(TCHAR_TO_UTF8(*password)).c_str (), blob, 32);

	FAES::FAESKey key;
	std::copy (std::begin (blob), std::end (blob), std::begin (key.Key));
	return key;
}
}
