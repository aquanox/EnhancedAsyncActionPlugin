// Copyright 2025, Aquanox.

#pragma once

#include "EdGraph/EdGraphNode.h"
#include "UObject/Interface.h"
#include "K2Node_AsyncContextInterface.generated.h"

#define UE_API ENHANCEDASYNCACTIONEDITOR_API

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

UINTERFACE()
class UK2Node_AsyncContextInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for adding context capture features to node.
 */
class UE_API IK2Node_AsyncContextInterface
{
	GENERATED_BODY()
public:
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
};

#undef UE_API
