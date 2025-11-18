// Copyright 2025, Aquanox.

#pragma once

#include "K2Node_AddPinInterface.h"
#include "K2Node_AsyncAction.h"
#include "K2Node_AsyncContextInterface.h"
#include "K2Node_EnhancedAsyncTaskBase.generated.h"

class FKismetCompilerContext;

/**
 * Base type for async nodes that support capture context
 *
 * @see UK2Node_BaseAsyncTask
 */
UCLASS(MinimalAPI)
class UK2Node_EnhancedAsyncTaskBase : public UK2Node_BaseAsyncTask, public IK2Node_AsyncContextInterface
{
	GENERATED_BODY()
public:
	UK2Node_EnhancedAsyncTaskBase();

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;

	virtual FText GetTooltipText() const override;

	void ImportConfigFromClass(UClass* InClass);
	void ImportConfigFromSpec(UClass* InClass, const struct FExternalAsyncActionSpec& InSpec);

	virtual const TArray<FEnhancedAsyncTaskCapture>& GetCapturesArray() const override;
	virtual TArray<FEnhancedAsyncTaskCapture>& GetMutableCapturesArray() override;
	virtual int32 GetNumCaptures() const override;

	virtual void AllocateDefaultPins() override;
	virtual bool CanSplitPin(const UEdGraphPin* Pin) const override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PinTypeChanged(UEdGraphPin* Pin) override;
	virtual void PostReconstructNode() override;

	bool HasContextExposed() const;
	bool IsContextPin(const UEdGraphPin* Pin) const;

	FText ToggleContextPinStateLabel() const;
	void ToggleContextPinState();
	void SyncContextPinState();

	struct FInputPinInfo
	{
		int32 CaptureIndex = INDEX_NONE;
		UEdGraphPin* InputPin = nullptr;
	};

	struct FOutputPinInfo
	{
		int32 CaptureIndex = INDEX_NONE;
		UEdGraphPin* OutputPin = nullptr;
		UK2Node_TemporaryVariable* TempVar = nullptr;

		const FName& GetName() const { return OutputPin->PinName; }
		const FName& GetCategory() const { return OutputPin->PinType.PinCategory; }
	};

	bool ValidateDelegates(
	    const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	bool ValidateCaptures(
	    const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	bool HandleSetContextData(
		const TArray<FInputPinInfo>& CaptureInputs, UEdGraphPin* InContextHandlePin, UEdGraphPin*& InOutLastThenPin,
		const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	bool HandleSetContextDataVariadic(
		const TArray<FInputPinInfo>& CaptureInputs, UEdGraphPin* InContextHandlePin, UEdGraphPin*& InOutLastThenPin,
		const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	bool HandleInvokeActivate(
		UEdGraphPin* InProxyObjectPin, UEdGraphPin*& InOutLastThenPin,
		const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	bool HandleActionDelegates(
		class UK2Node_CallFunction* CallCreateProxyObjectNode, UEdGraphPin*& InOutLastThenPin,
		const UEdGraphSchema_K2* Schema, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext);

	bool HandleGetContextData(
		const TArray<FOutputPinInfo>& CaptureOutputs, UEdGraphPin* ContextHandlePin, UEdGraphPin*& InOutLastThenPin,
		const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	bool HandleGetContextDataVariadic(
		const TArray<FOutputPinInfo>& CaptureOutputs, UEdGraphPin* ContextHandlePin, UEdGraphPin*& InOutLastThenPin,
		const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	void OrphanCapturePins();

	FString BuildContextConfigString() const;

	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

protected:
	// async context property in delegates
	UPROPERTY()
	FName AsyncContextParameterName;
	// marks context pin visible in graph
	UPROPERTY()
	bool bExposeContextParameter = false;
	// name of context container property, none for external
	UPROPERTY()
	FName AsyncContextContainerProperty;
	// recorded data on capture pairs
	UPROPERTY()
	TArray<FEnhancedAsyncTaskCapture> Captures;
};
