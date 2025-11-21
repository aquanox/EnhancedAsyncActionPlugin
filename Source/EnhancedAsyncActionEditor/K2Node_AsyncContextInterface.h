// Copyright 2025, Aquanox.

#pragma once

#include "EdGraph/EdGraphNode.h"
#include "UObject/Interface.h"
#include "UObject/WeakObjectPtr.h"
#include "K2Node.h"
#include "K2Node_AddPinInterface.h"
#include "K2Node_AsyncContextInterface.generated.h"

#define UE_API ENHANCEDASYNCACTIONEDITOR_API

/**
 * Represents pair of capture pins at specific index.
 */
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
	FEnhancedAsyncTaskCapture(int32 InIndex, UEdGraphPin* Input, UEdGraphPin* Output);

	inline const FName& NameOf(EEdGraphPinDirection Dir) const
	{
		return Dir == EGPD_Input ? InputPinName : OutputPinName;
	}

	inline FName& NameOf(EEdGraphPinDirection Dir)
	{
		return Dir == EGPD_Input ? InputPinName : OutputPinName;
	}
};

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UK2Node_AsyncContextInterface : public UK2Node_AddPinInterface
{
	GENERATED_BODY()
};

/**
 * Interface for adding context capture features to node.
 *
 * This is nessessary due to different hierarchies used by nodes:
 * - UK2Node_EnhancedAsyncAction <- UK2Node_EnhancedAsyncTaskBase <- UK2Node_BaseAsyncTask <- UK2Node
 * - UK2Node_EnhancedCallLatentFunction <- UK2Node_CallFunction <- UK2Node
 *
 * Slight interface abuse to avoid duplicating most of shared code
 */
class UE_API IK2Node_AsyncContextInterface : public IK2Node_AddPinInterface
{
	GENERATED_BODY()
public:
	/**
	 * Get own node object
	 */
	UK2Node* GetNodeObject() const;

	/**
	 * Get pin name for the capture pin
	 * @param PinIndex capture pin index
	 * @param Dir pin direction
	 */
	FName GetCapturePinName(int32 PinIndex, EEdGraphPinDirection Dir) const;

	/**
	 * Get maximum number supported capture pins
	 */
	virtual int32 GetMaxCapturePins() const;

	/**
	 * Get current number of capture pin pairs
	 */
	virtual int32 GetNumCaptures() const = 0;

	/**
	 * Get capture data container
	 */
	virtual const TArray<FEnhancedAsyncTaskCapture>& GetCapturesArray() const = 0;

	/**
	 * Get capture data container for modification
	 */
	virtual TArray<FEnhancedAsyncTaskCapture>& GetMutableCapturesArray() = 0;

	virtual bool CanAddPin() const override;
	virtual void AddInputPin() override;
	virtual bool CanRemovePin(const UEdGraphPin* Pin) const override;
	virtual void RemoveInputPin(UEdGraphPin* Pin) override;

	void AddCaptureInternal();
	void RemoveCaptureInternal(int32 ArrayIndex);

	bool TestConnectPins(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const;

	int32 IndexOfCapturePin(const UEdGraphPin* Pin) const;

	bool IsCapturePin(const UEdGraphPin* Pin) const;

	UEdGraphPin* FindCapturePin(int32 PinIndex, EEdGraphPinDirection Dir) const;
	UEdGraphPin* FindCapturePinChecked(int32 PinIndex, EEdGraphPinDirection Dir) const;

	void ForEachCapturePin(EEdGraphPinDirection Dir, TFunction<bool(int32, UEdGraphPin*)> const& Func) const;
	void ForEachCapturePinPair(TFunction<bool(int32, UEdGraphPin*, UEdGraphPin*)> const& Func) const;

	void SyncPinIndexesAndNames();
	void SyncCapturePinTypes();

	void SynchronizeCapturePinType(UEdGraphPin* Pin);
	void SynchronizeCapturePinType(UEdGraphPin* InputPin, UEdGraphPin* OutputPin);

	bool AnyCapturePinHasLinks() const;

	void GetStandardPins(EEdGraphPinDirection Dir, TArray<UEdGraphPin*> &OutPins) const;

	FString BuildContextConfigString() const;
};

/**
 * Helper for working with menu actions
 */
struct FK2Node_AsyncContextMenuActions
{
	static void SetupActions(UK2Node* Node, UToolMenu* Menu, UGraphNodeContextMenuContext* Context);

	static void AddCapturePin(TWeakObjectPtr<UK2Node> Node);
	static void RemoveCapturePin(TWeakObjectPtr<UK2Node> Node, const UEdGraphPin* Pin);

	static void BreakAllCapturePins(TWeakObjectPtr<UK2Node> Node);
	static void RemoveAllCapturePins(TWeakObjectPtr<UK2Node> Node);
	static void RemoveUnusedCapturePins(TWeakObjectPtr<UK2Node> Node);
};

#undef UE_API

