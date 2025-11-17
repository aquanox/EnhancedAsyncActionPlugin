// Copyright 2025, Aquanox.

#include "EAADemoLatent.h"

#include "DelayAction.h"
#include "Engine/LatentActionManager.h"
#include "Engine/Engine.h"

void UEAADemoLatent::SuperDelay(const UObject* WorldContextObject, float Duration, struct FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FDelayAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == NULL)
		{
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FDelayAction(Duration, LatentInfo));
		}
	}
}
