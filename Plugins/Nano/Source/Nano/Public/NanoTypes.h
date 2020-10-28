// Copyright 2020 Wesley Shillingford. All rights reserved.
#pragma once

#include "NanoTypes.generated.h"

UENUM(BlueprintType)
enum class FConfType : uint8 { send_to, send_from, receive };

// IMPORTANT, all *RequestData objects must match the json keys on the server, which is why they underscores instead of lower camel
// case like the rest of the codebase.

USTRUCT(BlueprintType)
struct NANO_API FGetBalanceRequestData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "GetBalance")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "GetBalance")
	bool include_only_confirmed{true};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "GetBalance")
	FString action{"account_balance"};
};

USTRUCT(BlueprintType)
struct NANO_API FGetBalanceResponseData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "GetBalance")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "GetBalance")
	FString balance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "GetBalance")
	FString pending;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "GetBalance")
	bool error{false};
};

USTRUCT(BlueprintType)
struct NANO_API FWorkGenerateRequestData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WorkGenerate")
	FString hash;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WorkGenerate")
	FString action{"work_generate"};
};

USTRUCT(BlueprintType)
struct NANO_API FWorkGenerateResponseData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WorkGenerate")
	FString hash;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WorkGenerate")
	FString work;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WorkGenerate")
	bool error{false};
};

USTRUCT(BlueprintType)
struct NANO_API FAccountFrontierRequestData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AccountFrontier")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AccountFrontier")
	FString representative{"true"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AccountFrontier")
	FString pending{"true"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AccountFrontier")
	bool include_confirmed{true};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AccountFrontier")
	FString action{"account_info"};
};

USTRUCT(BlueprintType)
struct NANO_API FAccountFrontierResponseData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AccountFrontier")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AccountFrontier")
	FString hash;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AccountFrontier")
	FString balance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AccountFrontier")
	FString representative;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AccountFrontier")
	bool error{false};
};

USTRUCT(BlueprintType)
struct NANO_API FPendingRequestData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pending")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pending")
	FString action{"pending"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pending")
	FString sort{"true"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pending")
	FString source{"true"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pending")
	FString include_only_confirmed{"true"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pending")
	FString count{"10"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pending")
	FString threshold{"0"};
};

USTRUCT(BlueprintType)
struct NANO_API FPendingBlock {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PendingBlock")
	FString hash;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PendingBlock")
	FString amount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PendingBlock")
	FString source;
};

USTRUCT(BlueprintType)
struct NANO_API FPendingResponseData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pending")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pending")
	TArray<FPendingBlock> blocks;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pending")
	bool error{false};
};

USTRUCT(BlueprintType)
struct NANO_API FBlockConfirmedResponseData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BlockConfirmed")
	bool confirmed{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BlockConfirmed")
	bool error{false};
};

// This is needed for Blueprint by user
USTRUCT(BlueprintType)
struct NANO_API FBlock {
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
	FString privateKey;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Block")
	FString work;
};

// This is needed for JSON
USTRUCT(BlueprintType)
struct NANO_API FBlockRequestData {
	GENERATED_USTRUCT_BODY()

	// Assuming only dealing with state blocks
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Block")
	FString type{"state"};

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
	FString link_as_account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Block")
	FString signature;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Block")
	FString work;
};

USTRUCT(BlueprintType)
struct NANO_API FProcessRequestData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Process")
	FString action{"process"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Process")
	FBlockRequestData block;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Process")
	FString json_block{"true"};
};

USTRUCT(BlueprintType)
struct NANO_API FBlockConfirmedRequestData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BlockConfirmed")
	FString action{"block_info"};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BlockConfirmed")
	bool json_block{true};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BlockConfirmed")
	FString hash;
};

USTRUCT(BlueprintType)
struct NANO_API FProcessResponseData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Process")
	FString hash;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Process")
	bool error{false};
};

USTRUCT(BlueprintType)
struct NANO_API FRequestNanoData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RequestNano")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RequestNano")
	FString action;

	FRequestNanoData() {
		action = "request_nano";
	}
};

USTRUCT(BlueprintType)
struct NANO_API FRequestNanoResponseData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ReceiveNano")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ReceiveNano")
	FString amount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ReceiveNano")
	FString srcHash;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ReceiveNano")
	FString frontier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ReceiveNano")
	bool error{false};
};

USTRUCT(BlueprintType)
struct NANO_API FAutomateResponseData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Automate")
	FConfType type;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Automate")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Automate")
	FString amount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Automate")
	FString balance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Automate")
	FString hash;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Automate")
	FString frontier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Automate")
	FString representative;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Automate")
	bool error{false};
};

USTRUCT(BlueprintType)
struct NANO_API FMakeBlockResponseData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MakeBlock")
	FBlock block;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MakeBlock")
	bool error{false};
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FGetBalanceResponseReceivedDelegate, FGetBalanceResponseData, data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FWorkGenerateResponseReceivedDelegate, FWorkGenerateResponseData, data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FProcessResponseReceivedDelegate, FProcessResponseData, data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FReceivedNanoDelegate, FRequestNanoResponseData, data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAccountFrontierResponseReceivedDelegate, FAccountFrontierResponseData, data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FPendingResponseReceivedDelegate, FPendingResponseData, data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FBlockConfirmedResponseReceivedDelegate, FBlockConfirmedResponseData, data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAutomateResponseReceivedDelegate, FAutomateResponseData, data);

DECLARE_DYNAMIC_DELEGATE_OneParam(FMakeBlockDelegate, FMakeBlockResponseData, data);

DECLARE_DYNAMIC_DELEGATE_OneParam(FWatchAccountReceivedDelegate, FAutomateResponseData, data);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FListenPaymentDelegate, const FString&, hash, const FString&, pendingAmount);

DECLARE_DYNAMIC_DELEGATE_OneParam(FListenPayoutDelegate, bool, expired);

inline FString BytesToStringFixed(const uint8* In, int32 Count) {
	FString Broken = BytesToString(In, Count);
	FString Fixed;

	for (int i = 0; i < Broken.Len(); i++) {
		const TCHAR c = Broken[i] - 1;
		Fixed.AppendChar(c);
	}

	return Fixed;
}

// WEBSOCKETS
USTRUCT(BlueprintType)
struct NANO_API FRegisterAccountRequestData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RegisterAccount")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RegisterAccount")
	FString action = "register_account";
};

USTRUCT(BlueprintType)
struct NANO_API FUnRegisterAccountRequestData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UnRegisterAccount")
	FString account;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UnRegisterAccount")
	FString action = "unregister_account";
};
