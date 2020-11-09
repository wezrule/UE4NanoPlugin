// Copyright 2020 Wesley Shillingford. All rights reserved.
#include "NanoBlueprintLibrary.h"

#include "Engine/Texture2D.h"
#include "HAL/FileManagerGeneric.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/AES.h"
#include "Modules/ModuleManager.h"

#include <baseconverter/base_converter.hpp>
#include <duthomhas/csprng.hpp>
#include <cctype>
#include <memory>
#include <qrcode/QrCode.hpp>
#include <sha256/sha256.hpp>

#ifdef _WIN32
#pragma warning(disable : 4804) /* '/': unsafe use of type 'bool' in operation warnings */
#endif
#include <blake2/blake2.h>
#include <ed25519-donna/ed25519.h>
#include <nano/numbers.h>

using namespace std::literals;

namespace {
nano::public_key PrivateKeyToPublicKeyData(const FString& privateKey);
nano::private_key SeedAccountPrvData(const FString& seed_f, int32 index);
nano::public_key SeedAccountPubData(const FString& seed, int32 index);
}	 // namespace

bool UNanoBlueprintLibrary::ValidateRaw(FString raw) {
	// Rudimentary check... it could still be above the largest raw value at 39 digits but shouldn't be a problem
	if (raw.Len() > 39 || raw.IsEmpty()) {
		return false;
	}

	// Check that it contains only 0-9 digits
	auto str = std::string(TCHAR_TO_UTF8(*raw));
	auto valid = str.end() == std::find_if(str.begin(), str.end(), [](unsigned char c) { return !isdigit(c); });

	if (valid) {
		nano::uint256_t num(BaseConverter::DecimalToHexConverter().Convert((TCHAR_TO_UTF8(*raw))).c_str());
		valid =
			(num <= nano::uint256_t(BaseConverter::DecimalToHexConverter().Convert("340282366920938463463374607431768211455").c_str()));
	}
	return valid;
}

// Could probably use regex for some of this
bool UNanoBlueprintLibrary::ValidateNano(FString nano) {
	if (nano.Len() > 40 || nano.IsEmpty()) {
		return false;
	}

	// Check that it contains only 0-9 digits and at most 1 decimal
	auto error = false;
	auto num_decimal_points = 0;
	auto decimal_point_index = -1;
	for (auto i = 0; i < nano.Len(); ++i) {
		auto c = nano[i];
		if (!std::isdigit(c)) {
			if (c == '.' || c == ',') {
				decimal_point_index = i;
				++num_decimal_points;
			} else {
				error = true;
				break;
			}
		}
	}

	if (error || num_decimal_points > 1) {
		return false;
	}

	if (decimal_point_index == -1) {
		// There is no decimal and it contains only digits
		auto as_int = FCString::Atoi(*nano);
		return as_int <= 340'282'366;	 // This is the maximum amount of Nano there is
	} else {
		// Split the string
		FString integer_part;
		FString fraction_part;
		if (!nano.Split(".", &integer_part, &fraction_part) || integer_part.Len() > 9) {
			if (!nano.Split(",", &integer_part, &fraction_part) || integer_part.Len() > 9) {
				return false;
			}
		}

		auto as_int = FCString::Atoi(*integer_part);
		error = as_int > 340'282'366;	 // This is the maximum amount of Nano there is
		if (!error) {
			nano::uint128_t max(BaseConverter::DecimalToHexConverter().Convert("920938463463374607431768211456").c_str());
			nano::uint128_t num(BaseConverter::DecimalToHexConverter().Convert((TCHAR_TO_UTF8(*fraction_part))).c_str());
			error = num > max;
		}
	}

	return !error;
}

FString UNanoBlueprintLibrary::NanoToRaw(FString nano) {
	check(ValidateNano(nano));

	// Remove decimal point (if exists) and add necessary trailing 0s to form exact raw number
	auto str = std::string(TCHAR_TO_UTF8(*nano));
	auto it = str.begin();
	for (; it < str.end(); ++it) {
		if (*it == '.' || *it == ',') {
			it = str.erase(it);
			break;
		}
	}
	auto num_zeroes_to_add = 30 - std::distance(it, str.end());
	auto raw = str + std::string(num_zeroes_to_add, '0');

	// Remove leading zeroes
	auto start_index = 0;
	for (auto i = raw.begin(); i < raw.end(); ++i) {
		if (*i == '0') {
			++start_index;
		} else {
			break;
		}
	}

	// Remove leading zeroes
	return std::string(raw.begin() + start_index, raw.end()).c_str();
}

FString UNanoBlueprintLibrary::RawToNano(FString raw) {
	check(ValidateRaw(raw));

	// Insert a decimal 30 decimal places from the right
	auto str = std::string(TCHAR_TO_UTF8(*raw));

	if (str.size() <= 30) {
		str = "0."s + std::string(30 - str.size(), '0') + str;
	} else {
		auto decimal_index = str.size() - 30;
		str.insert(str.begin() + decimal_index, '.');
	}

	auto index = 0;
	for (auto i = str.rbegin(); i < str.rend(); ++i, ++index) {
		if (*i != '0') {
			if (*i == '.') {
				--index;
			}
			break;
		}
	}

	return std::string(str.begin(), str.begin() + str.size() - index).c_str();
}

UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::Add(FString raw1, FString raw2) {
	check(ValidateRaw(raw1) && ValidateRaw(raw2));

	nano::amount amount1;
	amount1.decode_dec(TCHAR_TO_UTF8(*raw1));

	nano::amount amount2;
	amount2.decode_dec(TCHAR_TO_UTF8(*raw2));

	nano::amount total = (amount1.number() + amount2.number());
	return total.to_string_dec().c_str();
}

UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::Subtract(FString raw1, FString raw2) {
	check(GreaterOrEqual(raw1, raw2));
	nano::amount amount1;
	amount1.decode_dec(TCHAR_TO_UTF8(*raw1));

	nano::amount amount2;
	amount2.decode_dec(TCHAR_TO_UTF8(*raw2));

	check(amount2 < amount1);

	nano::amount total = (amount1.number() - amount2.number());
	return total.to_string_dec().c_str();
}

UFUNCTION(BlueprintCallable, Category = "Nano")
bool UNanoBlueprintLibrary::Greater(FString raw, FString baseRaw) {
	check(ValidateRaw(raw) && ValidateRaw(baseRaw));
	nano::amount amount1;
	amount1.decode_dec(TCHAR_TO_UTF8(*raw));

	nano::amount amount2;
	amount2.decode_dec(TCHAR_TO_UTF8(*baseRaw));

	return amount1 > amount2;
}

UFUNCTION(BlueprintCallable, Category = "Nano")
bool UNanoBlueprintLibrary::GreaterOrEqual(FString raw, FString baseRaw) {
	check(ValidateRaw(raw) && ValidateRaw(baseRaw));
	nano::amount amount1;
	amount1.decode_dec(TCHAR_TO_UTF8(*raw));

	nano::amount amount2;
	amount2.decode_dec(TCHAR_TO_UTF8(*baseRaw));

	return amount1 == amount2 || amount1 > amount2;
}

UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::ConvertnanoToRaw(FString nano_f) {
	nano::amount amount1;
	amount1.decode_dec(TCHAR_TO_UTF8(*nano_f));

	nano::amount multiplier(nano::xrb_ratio);

	nano::amount new_amount = (amount1.number() * multiplier.number());
	return new_amount.to_string_dec().c_str();
}

UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::CreateSeed() {
	duthomhas::csprng rng;
	nano::uint256_union seed;
	rng(seed.bytes);
	return FString(seed.to_string().c_str());
}

UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::PrivateKeyFromSeed(FString seed, int32 index) {
	return SeedAccountPrvData(seed, index).to_string().c_str();
}

UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::PublicKeyFromPrivateKey(FString privateKey) {
	return PrivateKeyToPublicKeyData(privateKey).to_string().c_str();
}

UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::AccountFromPrivateKey(FString privateKey) {
	return PrivateKeyToPublicKeyData(privateKey).to_account().c_str();
}

// As hex
UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::PublicKeyFromSeed(FString seed, int32 index) {
	return SeedAccountPubData(seed, index).to_string().c_str();
}

UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::AccountFromSeed(FString seed, int32 index) {
	return SeedAccountPubData(seed, index).to_account().c_str();
}

UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::PublicKeyFromAccount(FString account_f) {
	std::string account_str(TCHAR_TO_UTF8(*account_f));
	nano::account account;
	account.decode_account(account_str);
	return account.to_string().c_str();
}

UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::AccountFromPublicKey(FString publicKey) {
	nano::account account(TCHAR_TO_UTF8(*publicKey));
	return account.to_account().c_str();
}

UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::SHA256(const FString& string) {
	return sha256(TCHAR_TO_UTF8(*string)).c_str();
}

// Encrypt/Decrypt both taken from https://kelheor.space/2018/11/12/how-to-encrypt-data-with-aes-256-in-ue4/
UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::Encrypt(FString plainSeed, FString password) {
	check(plainSeed.Len() == 64);
	auto key = SHA256(password);

	// Check inputs
	if (plainSeed.IsEmpty())
		return "";	// empty string? do nothing
	if (key.IsEmpty())
		return "";

	// To split correctly final result of decryption from trash symbols
	FString SplitSymbol = "EL@$@!";
	plainSeed.Append(SplitSymbol);

	// We need at least 32 symbols key
	key = FMD5::HashAnsiString(*key);
	TCHAR* KeyTChar = key.GetCharArray().GetData();
	ANSICHAR* KeyAnsi = (ANSICHAR*) TCHAR_TO_ANSI(KeyTChar);

	// Calculate blob size and create blob
	uint8* Blob;
	uint32 Size;

	Size = plainSeed.Len();
	Size = Size + (FAES::AESBlockSize - (Size % FAES::AESBlockSize));

	Blob = new uint8[Size];

	// Convert string to bytes and encrypt
	if (StringToBytes(plainSeed, Blob, Size)) {
		FAES::EncryptData(Blob, Size, KeyAnsi);
		plainSeed = FString::FromHexBlob(Blob, Size);

		delete Blob;
		return plainSeed;
	}

	delete Blob;
	return "";
}

UFUNCTION(BlueprintCallable, Category = "Nano")
FString UNanoBlueprintLibrary::Decrypt(FString cipherSeed, FString password) {
	// Check inputs
	if (cipherSeed.IsEmpty())
		return "";
	auto key = SHA256(password);
	if (key.IsEmpty())
		return "";

	// To split correctly final result of decryption from trash symbols
	FString SplitSymbol = "EL@$@!";

	// We need at least 32 symbols key
	key = FMD5::HashAnsiString(*key);
	TCHAR* KeyTChar = key.GetCharArray().GetData();
	ANSICHAR* KeyAnsi = (ANSICHAR*) TCHAR_TO_ANSI(KeyTChar);

	// Calculate blob size and create blob
	uint8* Blob;
	uint32 Size;

	Size = cipherSeed.Len();
	Size = Size + (FAES::AESBlockSize - (Size % FAES::AESBlockSize));

	Blob = new uint8[Size];

	// Convert string to bytes and decrypt
	if (FString::ToHexBlob(cipherSeed, Blob, Size)) {
		FAES::DecryptData(Blob, Size, KeyAnsi);
		cipherSeed = BytesToString(Blob, Size);

		// Split required data from trash
		FString LeftData;
		FString RightData;
		cipherSeed.Split(SplitSymbol, &LeftData, &RightData, ESearchCase::CaseSensitive, ESearchDir::FromStart);
		cipherSeed = LeftData;

		delete Blob;
		return cipherSeed;
	}

	delete Blob;
	return "";
}

// A lot of this was taken from: https://github.com/hzm/QRCode
UTexture2D* UNanoBlueprintLibrary::GenerateQRCodeTexture(int32 Size, FString qrString, int32 Margin /* = 10 */) {
	UTexture2D* texture = nullptr;

	auto Width = (uint32) Size;
	auto Height = (uint32) Size;

	// Create the QR Code object
	auto qr = qrcodegen::QrCode::encodeText(TCHAR_TO_UTF8(*qrString), qrcodegen::QrCode::Ecc::MEDIUM);

	uint32 QRWidth, QRWidthAdjustedX, QRHeightAdjustedY, QRDataBytes;

	QRWidth = qr.getSize();
	uint32 ScaleX = (Width - 2 * Margin) / QRWidth;
	uint32 ScaleY = (Height - 2 * Margin) / QRWidth;
	QRWidthAdjustedX = QRWidth * ScaleX;
	QRHeightAdjustedY = QRWidth * ScaleY;
	QRDataBytes = QRWidthAdjustedX * QRHeightAdjustedY * 3;

	std::vector<uint8> RGBData(QRDataBytes, 0xff);
	uint8* QRCodeDestData;
	for (uint32 y = 0; y < QRWidth; y++) {
		QRCodeDestData = RGBData.data() + ScaleY * y * QRWidthAdjustedX * 3;
		for (uint32 x = 0; x < QRWidth; x++) {
			if (qr.getModule(x, y))	 // black
			{
				for (uint32 rectY = 0; rectY < ScaleY; rectY++) {
					for (uint32 rectX = 0; rectX < ScaleX; rectX++) {
						*(QRCodeDestData + rectY * QRWidthAdjustedX * 3 + rectX * 3) = 0;			 // Blue
						*(QRCodeDestData + rectY * QRWidthAdjustedX * 3 + rectX * 3 + 1) = 0;	 // Green
						*(QRCodeDestData + rectY * QRWidthAdjustedX * 3 + rectX * 3 + 2) = 0;	 // Red
					}
				}
			}
			QRCodeDestData += ScaleX * 3;
		}
	}

	auto scale_difference = Width - ScaleX;

	TArray<uint8> ImageBGRAData;

	for (uint32 i = 0; i < Width * Height * 4; i++) {
		ImageBGRAData.Add(0xFF);
	}
	for (uint32 y = Margin; y < QRHeightAdjustedY + Margin; y++) {
		for (uint32 x = Margin; x < QRWidthAdjustedX + Margin; x++) {
			for (int32 PixelByte = 0; PixelByte < 3; PixelByte++) {
				uint32 ImageIndex = (y * Width + x) * 4 + PixelByte;
				uint32 RGBDataIndex = ((y - Margin) * QRWidthAdjustedX + (x - Margin)) * 3 + PixelByte;
				ImageBGRAData[ImageIndex] = RGBData[RGBDataIndex];
			}
		}
	}

	// Make the excess width/height have alpha to remove it, else there is excess white on the bottom/right for scale discrepancies.
	for (uint32 y = QRHeightAdjustedY + Margin * 2; y < Height; y++) {
		for (uint32 x = 0; x < Width; x++) {
			for (int32 PixelByte = 0; PixelByte < 4; PixelByte++) {
				uint32 ImageIndex = (y * Width + x) * 4 + PixelByte;
				ImageBGRAData[ImageIndex] = 0x0;
			}
		}
	}

	for (uint32 y = 0; y < Height; y++) {
		for (uint32 x = QRWidthAdjustedX + Margin * 2; x < Width; x++) {
			for (int32 PixelByte = 0; PixelByte < 4; PixelByte++) {
				uint32 ImageIndex = (y * Width + x) * 4 + PixelByte;
				ImageBGRAData[ImageIndex] = 0x0;
			}
		}
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> TargetImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP);
	if (TargetImageWrapper.IsValid()) {
		texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
		if (texture != nullptr) {
			FTexture2DMipMap& Mip = texture->PlatformData->Mips[0];
			void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(TextureData, ImageBGRAData.GetData(), ImageBGRAData.Num());
			Mip.BulkData.Unlock();
			texture->UpdateResource();
		}
	}

	return texture;
}

UTexture2D* UNanoBlueprintLibrary::GenerateQRCodeTextureOnlyAccount(
	int32 Size, FString account, int32 Margin /* = 10 */) {
	FString qrString = "nano:" + account;
	return GenerateQRCodeTexture(Size, qrString, Margin);
}

UTexture2D* UNanoBlueprintLibrary::GenerateQRCodeTextureWithPrivateKey(int32 Size, FString privateKey, int32 Margin) {
	FString qrString = privateKey.ToUpper();
	return GenerateQRCodeTexture(Size, qrString, Margin);
}

UTexture2D* UNanoBlueprintLibrary::GenerateQRCodeTextureWithAmount(
	int32 Size, FString account, FString amount, int32 Margin /* = 10 */) {
	FString qrString = "nano:" + account;
	if (amount != "") {
		qrString += "?amount=" + amount;
	}
	return GenerateQRCodeTexture(Size, qrString, Margin);
}

namespace {
nano::public_key PrivateKeyToPublicKeyData(const FString& privateKey_f) {
	nano::private_key privateKey(TCHAR_TO_UTF8(*privateKey_f));
	nano::public_key publicKey;
	ed25519_publickey(privateKey.bytes.data(), publicKey.bytes.data());
	return publicKey;
}

nano::private_key SeedAccountPrvData(const FString& seed_f, int32 index) {
	std::string seed_str(TCHAR_TO_UTF8(*seed_f));
	nano::uint256_union seed(seed_str);
	nano::private_key privateKey;
	nano::deterministic_key(seed, index, privateKey);
	return privateKey;
}

nano::public_key SeedAccountPubData(const FString& seed, int32 index) {
	auto private_key = SeedAccountPrvData(seed, index);
	nano::public_key publicKey;
	ed25519_publickey(private_key.bytes.data(), publicKey.bytes.data());
	return publicKey;
}
}	 // namespace
