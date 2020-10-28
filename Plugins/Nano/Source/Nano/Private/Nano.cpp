// Copyright 2020 Wesley Shillingford. All rights reserved.
#include "Nano.h"

#define LOCTEXT_NAMESPACE "FNanoModule"

void FNanoModule::StartupModule() {
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FNanoModule::ShutdownModule() {
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNanoModule, Nano)