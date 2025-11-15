// Copyright 2025, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_EnhancedAsyncTaskBase.h"
#include "K2Node_EnhancedAsyncAction.generated.h"

#define UE_API ENHANCEDASYNCACTIONEDITOR_API

/**
 * 
 */
UCLASS()
class UE_API UK2Node_EnhancedAsyncAction : public UK2Node_EnhancedAsyncTaskBase
{
	GENERATED_BODY()
public:
	UK2Node_EnhancedAsyncAction();

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

#undef UE_API