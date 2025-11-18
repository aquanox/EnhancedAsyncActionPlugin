// Copyright 2025, Aquanox.

#include "EAADemoLatent.h"

#include "DelayAction.h"
#include "Engine/LatentActionManager.h"
#include "Engine/Engine.h"
#include "EnhancedLatentActionHandle.h"

FLatentCallResult UEAADemoLatent::SuperDelay(const UObject* WorldContextObject, float Duration, struct FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FDelayAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == NULL)
		{
			FDelayAction* Action = new FDelayAction(Duration, LatentInfo);
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, Action);
			return FLatentCallResult::Make(LatentInfo, Action);
		}
	}
	return FLatentCallResult::MakeInvalid(LatentInfo);
}

FLatentCallResult UEAADemoLatent::SuperDelayV2(const UObject* WorldContextObject, float Duration, struct FLatentActionInfo LatentInfo)
{
	using FActionType = TEnhancedLatentActionExtension<FDelayAction>;
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FActionType>(LatentInfo.CallbackTarget, LatentInfo.UUID) == NULL)
		{
			FActionType* Action = new FActionType(Duration, LatentInfo);
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, Action);
			return FLatentCallResult::Make(LatentInfo, Action);
		}
	}
	return FLatentCallResult::MakeInvalid(LatentInfo);
}
