// Copyright 2025, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_AddPinInterface.h"
#include "K2Node_AsyncContextInterface.h"
#include "K2Node_CallFunction.h"
#include "K2Node_EnhancedCallLatentFunction.generated.h"

#define UE_API ENHANCEDASYNCACTIONEDITOR_API

/**
 * WIP K2Node_CallFunction for Latents with Async Context
 */
UCLASS(MinimalAPI)
class UK2Node_EnhancedCallLatentFunction : public UK2Node_CallFunction, public IK2Node_AddPinInterface, public IK2Node_AsyncContextInterface
{
	GENERATED_BODY()
public:

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

	void SetupFromFunction(const UFunction* Function);
	void SetupFromSpec(const struct FExternalLatentFunctionSpec& Spec);

	virtual void AllocateDefaultPins() override;

	virtual int32 GetNumCaptures() const override;
	virtual const TArray<FEnhancedAsyncTaskCapture>& GetCapturesArray() const override;
	virtual TArray<FEnhancedAsyncTaskCapture>& GetMutableCapturesArray() override;

	virtual bool CanAddPin() const override;
	virtual void AddInputPin() override;
	virtual bool CanRemovePin(const UEdGraphPin* Pin) const override;
	virtual void RemoveInputPin(UEdGraphPin* Pin) override;

	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

private:

	UPROPERTY()
	TArray<FEnhancedAsyncTaskCapture> Captures;
};

#undef UE_API
