// Copyright 2025, Aquanox.

#include "EAADemoLatent.h"

#include "DelayAction.h"
#include "Engine/LatentActionManager.h"
#include "Engine/Engine.h"
#include "EnhancedAsyncContextShared.h"
#include "EnhancedLatentActionHandle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EAADemoLatent)

struct FResponseInfo
{
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	FResponseInfo(const FLatentActionInfo& LatentInfo)
		: ExecutionFunction(LatentInfo.ExecutionFunction)
		, OutputLink(LatentInfo.Linkage)
		, CallbackTarget(LatentInfo.CallbackTarget)
	{

	}
};

class FMyDelayAction : public FPendingLatentAction
{
public:
	float TimeRemaining;
	FLatentActionInfo Callback;

	FMyDelayAction(float Duration, const FLatentActionInfo& LatentInfo)
		: TimeRemaining(Duration), Callback(LatentInfo)
	{
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		TimeRemaining -= Response.ElapsedTime();
		Response.FinishAndTriggerIf(TimeRemaining <= 0.0f, Callback.ExecutionFunction, Callback.Linkage, Callback.CallbackTarget);
	}
};

void UEAADemoLatent::SuperDelay(
	const UObject* WorldContextObject, float Duration,
	const FEnhancedLatentActionContextHandle& LatentContext, FLatentActionInfo LatentInfo)
{
	UE_LOG(LogEnhancedAction, Log, TEXT("CALL SUPER DELAY %s"), *LatentContext.GetDebugString());
	using FActionType = TEnhancedLatentAction<FMyDelayAction>;
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (FActionType* Existing = LatentActionManager.FindExistingAction<FActionType>(LatentInfo.CallbackTarget, LatentInfo.UUID); !Existing)
		{
			FActionType* Action = new FActionType(LatentContext, Duration, LatentInfo);
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, Action);
			UE_LOG(LogEnhancedAction, Log, TEXT("RETURN NEW SUPER DELAY %s"), *LatentContext.GetDebugString());
			return;
		}
		else
		{
			UE_LOG(LogEnhancedAction, Log, TEXT("RETURN EXISTING SUPER DELAY %s"), *LatentContext.GetDebugString());
			LatentContext.ReleaseContext();
			return;
		}
	}
	UE_LOG(LogEnhancedAction, Log, TEXT("RETURN SUPER DELAY FAILED %s"), *LatentContext.GetDebugString());
	LatentContext.ReleaseContext();
	return;
}

void UEAADemoLatent::SuperDelayRepeatable(
	const UObject* WorldContextObject, float Duration,
	const FEnhancedLatentActionContextHandle& LatentContext,
	FLatentActionInfo LatentInfo)
{
	UE_LOG(LogEnhancedAction, Log, TEXT("CALL REPEATABLE SUPER DELAY %s"), *LatentContext.GetDebugString());

	using FActionType = TEnhancedRepeatableLatentAction<FMyDelayAction>;

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();

		FActionType* Action = new FActionType(LatentContext, Duration, LatentInfo);
		LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, Action);

		UE_LOG(LogEnhancedAction, Log, TEXT("RETURN REPEATABLE NEW SUPER DELAY %s"), *LatentContext.GetDebugString());
		return;
	}

	UE_LOG(LogEnhancedAction, Log, TEXT("RETURN REPEATABLE SUPER DELAY FAILED %s"), *LatentContext.GetDebugString());
	LatentContext.ReleaseContext();
	return;
}
