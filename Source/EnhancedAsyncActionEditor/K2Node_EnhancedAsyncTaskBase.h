// Copyright 2025, Aquanox.

#pragma once

#include "K2Node_AddPinInterface.h"
#include "K2Node_AsyncAction.h"
#include "K2Node_EnhancedAsyncTaskBase.generated.h"

class FKismetCompilerContext;

USTRUCT()
struct FEnhancedAsyncTaskCapture
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Index = 0;
	UPROPERTY()
	FName InputPinName;
	UPROPERTY()
	FName OutputPinName;

	FEnhancedAsyncTaskCapture() = default;
	FEnhancedAsyncTaskCapture(int32 InIndex, UEdGraphPin* Input, UEdGraphPin* Output)
	{
		Index = InIndex;
		InputPinName = Input->PinName;
		OutputPinName = Output->PinName;
	}

	inline const FName& NameOf(EEdGraphPinDirection Dir) const
	{
		return Dir == EGPD_Input ? InputPinName : OutputPinName;
	}

	inline FName& NameOf(EEdGraphPinDirection Dir)
	{
		return Dir == EGPD_Input ? InputPinName : OutputPinName;
	}
};

/**
 * Base type for async nodes that support capture context
 *
 * @see UK2Node_BaseAsyncTask
 */
UCLASS(MinimalAPI)
class UK2Node_EnhancedAsyncTaskBase : public UK2Node_BaseAsyncTask, public IK2Node_AddPinInterface
{
	GENERATED_BODY()
public:
	UK2Node_EnhancedAsyncTaskBase();

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;

	virtual bool HasExternalDependencies(TArray<UStruct*>* OptionalOutput) const override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;

	virtual void ImportCaptureConfigFromProxyClass();

	virtual void AllocateDefaultPins() override;

	int32 GetNumCaptures() const { return Captures.Num(); }
	bool HasAnyCapturePins() const { return Captures.Num() > 0; }
	bool AnyCapturePinHasLinks() const;
	bool HasContextExposed() const;

	void GetStandardPins(EEdGraphPinDirection Dir, TArray<UEdGraphPin*>& OutPins) const;

	FName GetCapturePinName(int32 PinIndex, EEdGraphPinDirection Dir) const;
	bool IsCapturePin(const UEdGraphPin* Pin) const;

	int32 IndexOfCapturePin(const UEdGraphPin* Pin) const;
	UEdGraphPin* FindCapturePin(int32 PinIndex, EEdGraphPinDirection Dir) const;
	UEdGraphPin* FindCapturePinChecked(int32 PinIndex, EEdGraphPinDirection Dir) const;

	void ForEachCapturePin(EEdGraphPinDirection Dir, TFunction<bool(int32, UEdGraphPin*)> const& Func) const;
	void ForEachCapturePinPair(TFunction<bool(int32, UEdGraphPin*, UEdGraphPin*)> const& Func) const;

	bool IsContextPin(const UEdGraphPin* Pin) const;

	virtual bool CanSplitPin(const UEdGraphPin* Pin) const override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;

	void SyncPinIndexesAndNames();

	int32 GetMaxCapturePins() const;
	virtual bool CanAddPin() const override;
	virtual void AddInputPin() override;
	virtual bool CanRemovePin(const UEdGraphPin* Pin) const override;
	virtual void RemoveInputPin(UEdGraphPin* Pin) override;
	void RemoveCaptureAtIndex(int32 Index);

	void UnlinkAllCapturePins();
	void RemoveUnusedCapturePins();
	void RemoveAllCapturePins();

	FText ToggleContextPinStateLabel() const;
	void ToggleContextPinState();
	void SyncContextPinState();

	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PinTypeChanged(UEdGraphPin* Pin) override;
	virtual void PostReconstructNode() override;

	void SyncCapturePinTypes();
	void SynchronizeCapturePinType(UEdGraphPin* Pin);
	void SynchronizeCapturePinType(UEdGraphPin* InputPin, UEdGraphPin* OutputPin);
	static FEdGraphPinType DeterminePinType(const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin);

	struct FInputPinInfo
	{
		int32 CaptureIndex = INDEX_NONE;
		UEdGraphPin* InputPin = nullptr;
	};

	struct FOutputPinInfo
	{
		int32 CaptureIndex;
		UEdGraphPin* OutputPin;
		UK2Node_TemporaryVariable* TempVar;

		const FName& GetName() const { return OutputPin->PinName; }
		const FName& GetCategory() const { return OutputPin->PinType.PinCategory; }

		bool operator==(const FName& Key) const { return OutputPin->PinName == Key; }
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
	friend struct FEAATestAccessor;

	// async context property in delegates
	UPROPERTY()
	FName AsyncContextParameterName;
	// marks context pin visible in graph
	UPROPERTY()
	bool bExposeContextParameter = false;
	// type of context container
	UPROPERTY()
	TObjectPtr<UScriptStruct> AsyncContextContainerType;
	// name of context container property, none for external
	UPROPERTY()
	FName AsyncContextContainerProperty;
	// recorded data on capture pairs
	UPROPERTY()
	TArray<FEnhancedAsyncTaskCapture> Captures;
};
