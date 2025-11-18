// Copyright 2025, Aquanox.

#include "EnhancedAsyncContextSettings.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnhancedAsyncContextSettings)

const UEnhancedAsyncContextSettings* UEnhancedAsyncContextSettings::Get()
{
	return GetDefault<UEnhancedAsyncContextSettings>();
}

const FExternalAsyncActionSpec* UEnhancedAsyncContextSettings::FindActionSpecForClass(UClass* Class) const
{
	for (const FExternalAsyncActionSpec& Spec : ExternalAsyncActions)
	{
		UClass* SpecClass = Spec.ActionClass.Get();
		if (SpecClass && (SpecClass == Class || Class->IsChildOf(SpecClass)))
		{
			return &Spec;
		}
	}
	return nullptr;
}
