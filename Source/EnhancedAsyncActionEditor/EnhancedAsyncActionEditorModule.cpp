// Copyright 2025, Aquanox.

#include "EnhancedAsyncActionEditorModule.h"
#include "EnhancedAsyncContextCustomSettings.h"
#include "PropertyEditorModule.h"

IMPLEMENT_MODULE(FEnhancedAsyncActionEditorModule, EnhancedAsyncActionEditor)

void FEnhancedAsyncActionEditorModule::StartupModule()
{
	if (GIsEditor && !IsRunningCommandlet())
	{
		RegisterCustomSettings();
	}
}

void FEnhancedAsyncActionEditorModule::ShutdownModule()
{
	if (GIsEditor && !IsRunningCommandlet())
	{
		UnRegisterCustomSettings();
	}
}

void FEnhancedAsyncActionEditorModule::RegisterCustomSettings()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FExternalAsyncActionSpecCustomization::GetTypeName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FExternalAsyncActionSpecCustomization::MakeInstance)
	);
}

void FEnhancedAsyncActionEditorModule::UnRegisterCustomSettings()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomPropertyTypeLayout(
		FExternalAsyncActionSpecCustomization::GetTypeName()
	);
}
