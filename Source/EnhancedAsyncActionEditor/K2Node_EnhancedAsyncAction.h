// Copyright 2025, Aquanox.

#pragma once

#include "K2Node_EnhancedAsyncTaskBase.h"
#include "K2Node_EnhancedAsyncAction.generated.h"

#define UE_API ENHANCEDASYNCACTIONEDITOR_API

class FBlueprintActionDatabaseRegistrar;

/**
 * @see UK2Node_AsyncAction
 */
UCLASS(MinimalAPI)
class UK2Node_EnhancedAsyncAction : public UK2Node_EnhancedAsyncTaskBase
{
	GENERATED_BODY()
public:
	UK2Node_EnhancedAsyncAction();

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
};

#undef UE_API
