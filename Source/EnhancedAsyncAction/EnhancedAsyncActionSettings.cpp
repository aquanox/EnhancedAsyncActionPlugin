// Copyright 2025, Aquanox.

#include "EnhancedAsyncActionSettings.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnhancedAsyncActionSettings)

const UEnhancedAsyncActionSettings* UEnhancedAsyncActionSettings::Get()
{
	return GetDefault<UEnhancedAsyncActionSettings>();
}

const FExternalAsyncActionSpec* UEnhancedAsyncActionSettings::FindActionSpecForClass(UClass* Class) const
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

const FExternalLatentFunctionSpec* UEnhancedAsyncActionSettings::FindLatentSpecForFunction(const UFunction* Function) const
{
	const UClass* FunctionOwner = Function->GetOuterUClass();
	for (const FExternalLatentFunctionSpec& Spec : ExternalLatentFunctions)
	{
		if (Spec.FactoryClass.Get() == FunctionOwner && (Spec.FunctionName.IsNone() || Spec.FunctionName == Function->GetName()))
		{
			return &Spec;
		}
	}
	return nullptr;
}
