// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_AddPinInterface.h"
#include "K2Node_AsyncAction.h"
#include "K2Node_EnhancedAsyncTaskBase.generated.h"

class FKismetCompilerContext;

USTRUCT()
struct FEATCapturePinPair
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Index = 0;
	UPROPERTY()
	FName BoundPropertyName;
	UPROPERTY()
	FName InputPinName;
	UPROPERTY()
	FName OutputPinName;

	FEATCapturePinPair() = default;
	FEATCapturePinPair(int32 InIndex, UEdGraphPin* Input, UEdGraphPin* Output)
	{
		Index = InIndex;
		InputPinName = Input->PinName;
		OutputPinName = Output->PinName;
	}

	FEATCapturePinPair(int32 InIndex, FName BoundName, UEdGraphPin* Input, UEdGraphPin* Output)
	{
		Index = InIndex;
		BoundPropertyName = BoundName;
		InputPinName = Input->PinName;
		OutputPinName = Output->PinName;
	}

	bool operator==(const UEdGraphPin* Pin) const { return InputPinName == Pin->PinName || OutputPinName == Pin->PinName; }
	bool operator==(const FName& PinName) const { return InputPinName == PinName || OutputPinName == PinName; }

	inline const FName& NameOf(EEdGraphPinDirection Dir) const
	{
		return Dir == EGPD_Input ? InputPinName : OutputPinName;
	}
	
	inline FName& NameOf(EEdGraphPinDirection Dir)
	{
		return Dir == EGPD_Input ? InputPinName : OutputPinName;
	}

	inline UEdGraphPin* FindPin(EEdGraphPinDirection Dir, const UK2Node* Node) const
	{
		return Node->FindPin(NameOf(Dir));
	}

	inline UEdGraphPin* FindPinChecked(EEdGraphPinDirection Dir, const UK2Node* Node) const
	{
		return Node->FindPinChecked(NameOf(Dir));
	}
};

/**
 * Base type for async nodes that support capture context
 */
UCLASS()
class UK2Node_EnhancedAsyncTaskBase : public UK2Node_BaseAsyncTask, public IK2Node_AddPinInterface
{
	GENERATED_BODY()
public:
	UK2Node_EnhancedAsyncTaskBase();
	
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;

	virtual bool HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;

	virtual void ImportCaptureConfigFromProxyClass();

	virtual void AllocateDefaultPins() override;
	void AllocateDefaultCapturePinsForStruct(const UScriptStruct* Struct);

	bool HasAnyCapturePins() const { return NumCaptures > 0; }
	bool HasAnyLinkedCaptures() const;
	bool HasContextExposed() const;
	bool NeedSetupCaptureContext() const;
	
	TArray<UEdGraphPin*> GetStandardPins(EEdGraphPinDirection Dir) const;

	bool HasDefaultDynamicPins(const UScriptStruct*& Skeleton) const;
	bool IsDynamicContainerType() const;

	UEdGraphPin* FindDynamicPin(EEdGraphPinDirection Dir, int32 PinIndex) const;
	UEdGraphPin* FindMatchingPin(const UEdGraphPin* Pin, EEdGraphPinDirection Dir) const;
	UEdGraphPin* FindMatchingPin(const UEdGraphPin* Pin) const;
	bool IsDynamicPin(const UEdGraphPin* Pin) const;
	FName GetPinName(EEdGraphPinDirection Dir, int32 PinIndex) const;
	
	bool IsContextPin(const UEdGraphPin* Pin) const;

	virtual bool CanSplitPin(const UEdGraphPin* Pin) const override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;

	void SyncPinNames();
	void SyncPinTypes();

	static int32 GetMaxCapturePinsNum();
	virtual bool CanAddPin() const override;
	virtual void AddInputPin() override;
	virtual bool CanRemovePin(const UEdGraphPin* Pin) const override;
	virtual void RemoveInputPin(UEdGraphPin* Pin) override;

	void UnlinkAllDynamicPins();
	void RemoveAllDynamicPins();

	FText ToggleContextPinStateLabel() const;
	void ToggleContextPinState();
	void SyncContextPinState();

	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PinTypeChanged(UEdGraphPin* Pin) override;
	virtual void PostReconstructNode() override;
	void SynchronizeArgumentPinType(UEdGraphPin* Pin);

	struct FOutputPinInfo
	{
		enum ESource { DELEGATE, CAPTURE };

		int32 CaptureIndex = -1;
		ESource Source;
		UEdGraphPin* OutputPin;
		UK2Node_TemporaryVariable* TempVar;

		//UK2Node_AssignmentStatement* AssignNode = nullptr;

		const FName& GetName() const { return OutputPin->PinName; }
		const FName& GetCategory() const { return OutputPin->PinType.PinCategory; }

		FOutputPinInfo(ESource Src, UEdGraphPin* Pin, UK2Node_TemporaryVariable* Var) : Source(Src), OutputPin(Pin), TempVar(Var) {}
		FOutputPinInfo(ESource Src, UEdGraphPin* Pin, UK2Node_TemporaryVariable* Var, int32 Idx) : CaptureIndex(Idx), Source(Src), OutputPin(Pin), TempVar(Var) {}

		bool operator==(const FName& Key) const { return OutputPin->PinName == Key; }
	};

	bool ValidateDelegates(
	    const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	bool ValidateCaptures(
	    const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	bool HandleSetContextData(
		UEdGraphPin* InContextPin, UEdGraphPin*& InOutLastThenPin,
		const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	bool HandleInvokeActivate(
		UEdGraphPin* InProxyObjectPin, UEdGraphPin*& InOutLastThenPin,
		const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);

	bool HandleActionDelegates(
		class UK2Node_CallFunction* CallCreateProxyObjectNode, UEdGraphPin*& InOutLastThenPin,
		const UEdGraphSchema_K2* Schema, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext);

	void OrphanCapturePins();

	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

protected:
	friend struct FEAATestAccessor;
	
	// async context property in delegates
	UPROPERTY()
	FName AsyncContextParameterName;
	UPROPERTY()
	bool bExposeContextParameter = false;	
	// type of context container
	UPROPERTY()
	TObjectPtr<UScriptStruct> AsyncContextContainerType;
	// name of context container property, none for external
	UPROPERTY()
	FName AsyncContextContainerProperty;
	// number of captured pins
	UPROPERTY()
	int32 NumCaptures = 0;
	// recorded data on capture pairs
	UPROPERTY()
	TArray<FEATCapturePinPair> Captures;
};
