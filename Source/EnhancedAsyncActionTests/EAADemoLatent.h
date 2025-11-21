// Copyright 2025, Aquanox.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "EAADemoLatent.generated.h"

class FRepeatableLatentActionDelegate;
struct FEnhancedLatentActionContextHandle;
struct FLatentActionInfo;

DECLARE_DYNAMIC_DELEGATE_OneParam(FLatentDemoDelegate, int32, Value);

/**
 *
 */
UCLASS(MinimalAPI)
class UEAADemoLatent : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Latent", meta=(Latent, HasLatentContext="LatentContext", WorldContext="WorldContextObject", LatentInfo="LatentInfo", Duration="0.2", Keywords="sleep"))
	static void SuperDelay(
		const UObject* WorldContextObject, float Duration,
		const FEnhancedLatentActionContextHandle& LatentContext,
		FLatentActionInfo LatentInfo
	);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Latent", meta=(Latent, LatentTrigger=Event, HasLatentContext="LatentContext", WorldContext="WorldContextObject", LatentInfo="LatentInfo", Duration="0.2", Keywords="sleep"))
	static void SuperDelayRepeatable(
		const UObject* WorldContextObject, float Duration,
		const FEnhancedLatentActionContextHandle& LatentContext,
		FLatentActionInfo LatentInfo
	);

};

