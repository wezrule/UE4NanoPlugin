// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"
	
#include <Runtime/Core/Public/Misc/AutomationTest.h>

#include "NanoBlueprintLibrary.h"

#include <string>

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNanoBlueprintLibraryTest, "NanoBlueprintLibrary", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FNanoBlueprintLibraryTest::RunTest(const FString& Parameters)
{
	// Validate nano/raw functions
	TestTrue (TEXT ("100.123 nano is valid"), UNanoBlueprintLibrary::ValidateNano ("100.123"));
	TestFalse (TEXT ("Invalid characters nano"), UNanoBlueprintLibrary::ValidateNano ("100,123"));
	TestTrue (TEXT ("Exact nano nano number"), UNanoBlueprintLibrary::ValidateNano ("340282366.920938463463374607431768211455"));
	TestTrue (TEXT ("Exact nano nano number"), UNanoBlueprintLibrary::ValidateNano ("0.1231231"));
	TestTrue (TEXT ("Exact nano nano number"), UNanoBlueprintLibrary::ValidateNano (".1223"));

	TestTrue (TEXT ("100 raw is valid"), UNanoBlueprintLibrary::ValidateRaw ("100"));
	TestTrue (TEXT ("Exactly max raw supply"), UNanoBlueprintLibrary::ValidateRaw ("340282366920938463463374607431768211455"));
	TestFalse (TEXT ("Invalid characters raw, decimal not allowed"), UNanoBlueprintLibrary::ValidateRaw ("100.123"));
	TestFalse (TEXT ("Invalid characters raw"), UNanoBlueprintLibrary::ValidateRaw ("100@123"));
	TestTrue (TEXT ("Leading zeroes are ok"), UNanoBlueprintLibrary::ValidateRaw ("0001"));
	TestFalse (TEXT ("A lot above max"), UNanoBlueprintLibrary::ValidateRaw ("1111111111111111111111111111111111111111111111111111111111"));
	TestFalse (TEXT ("1 above max raw supply"), UNanoBlueprintLibrary::ValidateRaw ("340282366920938463463374607431768211456"));

	// Raw to nano
	auto raw = FString (TEXT ("10000000000000000000000000000000"));
	auto nano = UNanoBlueprintLibrary::RawToNano (raw);
	TestEqual (TEXT ("10^31raw to Nano"), nano, TEXT ("10.0"));

	raw = TEXT ("1000000000000000000000000000000");
	nano = UNanoBlueprintLibrary::RawToNano (raw);
	TestEqual (TEXT ("10^30raw to Nano"), nano, TEXT ("1.0"));

	raw = TEXT ("100000000000000000000000000000");
	nano = UNanoBlueprintLibrary::RawToNano (raw);
	TestEqual (TEXT ("10^29raw to Nano"), nano, TEXT ("0.1"));

	raw = TEXT ("10000000000000000000000000000");
	nano = UNanoBlueprintLibrary::RawToNano (raw);
	TestEqual (TEXT ("10^28raw to Nano"), nano, TEXT ("0.01"));

	raw = TEXT ("1");
	nano = UNanoBlueprintLibrary::RawToNano (raw);
	TestEqual (TEXT ("1 raw to Nano"), nano, TEXT ("0.000000000000000000000000000001"));

	// Nano to raw
	nano = TEXT ("10.0");
	raw = UNanoBlueprintLibrary::NanoToRaw (nano);
	TestEqual (TEXT ("10.0Nano to raw"), raw, TEXT ("10000000000000000000000000000000"));

	nano = TEXT ("10");
	raw = UNanoBlueprintLibrary::NanoToRaw (nano);
	TestEqual (TEXT ("10Nano to raw"), raw, TEXT ("10000000000000000000000000000000"));

	nano = TEXT ("1.0");
	raw  = UNanoBlueprintLibrary::NanoToRaw (nano);
	TestEqual (TEXT ("1.0Nano to raw"), raw, TEXT ("1000000000000000000000000000000"));

	nano = TEXT ("0.1");
	raw  = UNanoBlueprintLibrary::NanoToRaw (nano);
	TestEqual (TEXT ("0.1Nano to raw"), raw, TEXT ("100000000000000000000000000000"));
	nano = TEXT (".1");
	raw  = UNanoBlueprintLibrary::NanoToRaw (nano);
	TestEqual (TEXT (".1Nano to raw"), raw, TEXT ("100000000000000000000000000000"));
	nano = TEXT ("00000.1");
	raw  = UNanoBlueprintLibrary::NanoToRaw (nano);
	TestEqual (TEXT (".1Nano to raw"), raw, TEXT ("100000000000000000000000000000"));

	nano = TEXT ("0.01");
	raw  = UNanoBlueprintLibrary::NanoToRaw (nano);
	TestEqual (TEXT ("0.01 Nano to raw"), raw, TEXT ("10000000000000000000000000000"));

	nano = TEXT ("0.000000000000000000000000000001");
	raw  = UNanoBlueprintLibrary::NanoToRaw (nano);
	TestEqual (TEXT ("0.000000000000000000000000000001 Nano to raw"), raw, TEXT ("1"));

	// nano to raw (1 nano is 10^6 Nano)
	auto Mnano = TEXT ("1");
	raw = UNanoBlueprintLibrary::ConvertnanoToRaw (Mnano);
	TestEqual (TEXT ("1 MNano to raw"), raw, TEXT ("1000000000000000000000000"));

	// Add
	auto op = UNanoBlueprintLibrary::Add (UNanoBlueprintLibrary::NanoToRaw ("1"), UNanoBlueprintLibrary::NanoToRaw ("2"));
	TestEqual (TEXT ("1Nano + 2Nano (as raw)"), op, UNanoBlueprintLibrary::NanoToRaw ("3"));

	// Subtract
	op = UNanoBlueprintLibrary::Subtract (UNanoBlueprintLibrary::NanoToRaw ("2"), UNanoBlueprintLibrary::NanoToRaw ("1"));
	TestEqual (TEXT ("2Nano - 1Nano (as raw)"), op, UNanoBlueprintLibrary::NanoToRaw ("1"));

	// Returns true if raw is greater than baseRaw
	TestEqual (TEXT ("raw Greater1"), true, UNanoBlueprintLibrary::Greater ("3000", "2000"));
	TestEqual (TEXT ("raw Greater2"), false, UNanoBlueprintLibrary::Greater ("2000", "2000"));
	TestEqual (TEXT ("raw Greater3"), false, UNanoBlueprintLibrary::Greater ("1999", "2000"));

	TestEqual (TEXT ("raw GreaterOrEqual1"), true, UNanoBlueprintLibrary::GreaterOrEqual ("3000", "2000"));
	TestEqual (TEXT ("raw GreaterOrEqual2"), true, UNanoBlueprintLibrary::GreaterOrEqual ("2000", "2000"));
	TestEqual (TEXT ("raw GreaterOrEqual3"), false, UNanoBlueprintLibrary::GreaterOrEqual ("1999", "2000"));

	TestEqual (TEXT ("Create seed length"), 64, UNanoBlueprintLibrary::CreateSeed().Len ());
	TestNotEqual (TEXT ("Create seed check"), FString(std::string (64, '0').c_str ()), UNanoBlueprintLibrary::CreateSeed());

	auto seed = TEXT ("1234567891234567891234567891234567891234567891234567891234567891");
	auto privateKey = TEXT ("64E4A5F0098E9330224975EB51D227BDDAD1E999E9AA2910B4724D78024CACF4");
	TestEqual (TEXT ("PrivateKeyFromSeed"), privateKey, UNanoBlueprintLibrary::PrivateKeyFromSeed(seed, 0));
	TestEqual (TEXT ("PublicKeyFromSeed"), TEXT ("1B228F3ACFE9508A331987746845DA25044D48D290489D015B1A39724A8BEFE7"), UNanoBlueprintLibrary::PublicKeyFromSeed(seed, 0));
	TestEqual (TEXT ("AccountFromSeed"), TEXT ("nano_18s4jwxeztcijasjm3unf34xnba6bo6f764amn1op8jsgb7aquz9ke8njujm"), UNanoBlueprintLibrary::AccountFromSeed(seed, 0));
	TestEqual (TEXT ("AccountFromPublicKey"), TEXT ("nano_18s4jwxeztcijasjm3unf34xnba6bo6f764amn1op8jsgb7aquz9ke8njujm"), UNanoBlueprintLibrary::AccountFromPublicKey("1B228F3ACFE9508A331987746845DA25044D48D290489D015B1A39724A8BEFE7"));
	TestEqual (TEXT ("PublicKeyFromAccount"), TEXT ("1B228F3ACFE9508A331987746845DA25044D48D290489D015B1A39724A8BEFE7"), UNanoBlueprintLibrary::PublicKeyFromAccount("nano_18s4jwxeztcijasjm3unf34xnba6bo6f764amn1op8jsgb7aquz9ke8njujm"));
	TestEqual (TEXT ("PublicKeyFromPrivateKey"), TEXT ("1B228F3ACFE9508A331987746845DA25044D48D290489D015B1A39724A8BEFE7"), UNanoBlueprintLibrary::PublicKeyFromPrivateKey(privateKey));
	TestEqual (TEXT ("AccountFromPrivateKey"), TEXT ("nano_18s4jwxeztcijasjm3unf34xnba6bo6f764amn1op8jsgb7aquz9ke8njujm"), UNanoBlueprintLibrary::AccountFromPrivateKey(privateKey));

	TestEqual (TEXT ("SHA256"), TEXT ("4c79de09ef123e2834dbe06d420158e455ac658aeb181da3b3ca5aabaf02dea3"), UNanoBlueprintLibrary::SHA256 (seed));

	auto password = "password123";
	auto cypherText = UNanoBlueprintLibrary::Encrypt (seed, password);

	auto plain_text = UNanoBlueprintLibrary::Decrypt (cypherText, password);
	TestEqual (TEXT ("Decrypt"), plain_text, seed);
	return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS
