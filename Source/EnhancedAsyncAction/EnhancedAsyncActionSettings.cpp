// Copyright 2025, Aquanox.

#include "EnhancedAsyncActionSettings.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnhancedAsyncActionSettings)

bool UEnhancedAsyncActionSettings::IsClassAllowed(UClass* InClass) const
{
	if (AllowedNodes.Num() == 0)
		return true;

	for (auto& Class : AllowedNodes)
	{
		UClass* SomeBase = Class.LoadSynchronous();
		if (SomeBase && InClass->IsChildOf(SomeBase))
			return true;
	}
	return false;
}
