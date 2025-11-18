// Copyright 2025, Aquanox.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "EAADemoLatent.generated.h"

/**
 *
 */
UCLASS(MinimalAPI)
class UEAADemoLatent : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	/**
	 * Perform a latent action with a delay (specified in seconds).  Calling again while it is counting down will be ignored.
	 *
	 * @param WorldContextObject	World context.
	 * @param Duration 		length of delay (in seconds).
	 * @param LatentInfo 	The latent action.
	 */
	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Latent", meta=(Latent, LatentContext, HasAsyncContext, WorldContext="WorldContextObject", LatentInfo="LatentInfo", Duration="0.2", Keywords="sleep"))
	static FLatentCallResult SuperDelay(const UObject* WorldContextObject, float Duration, struct FLatentActionInfo LatentInfo );

	/**
	 * Perform a latent action with a delay (specified in seconds).  Calling again while it is counting down will be ignored.
	 *
	 * @param WorldContextObject	World context.
	 * @param Duration 		length of delay (in seconds).
	 * @param LatentInfo 	The latent action.
	 */
	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Latent", meta=(Latent, LatentContext, HasAsyncContext, WorldContext="WorldContextObject", LatentInfo="LatentInfo", Duration="0.2", Keywords="sleep"))
	static FLatentCallResult SuperDelayV2(const UObject* WorldContextObject, float Duration, struct FLatentActionInfo LatentInfo );

};
