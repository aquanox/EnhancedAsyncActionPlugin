// Copyright 2025, Aquanox.

#pragma once

#include "EdGraphSchema_K2.h"
#include "EnhancedAsyncContextTypes.h"

class UK2Node_EnhancedAsyncTaskBase;
class UK2Node_CallFunction;
class UEdGraphSchema_K2;

namespace EAA::Internals
{
	static const FName PIN_Handle = TEXT("Handle");
	static const FName PIN_Index = TEXT("Index");
	static const FName PIN_Value = TEXT("Value");
	static const FName PIN_Action = TEXT("Action");
	static const FName PIN_Object = TEXT("Object");

	FString ToDebugString(const FProperty* Pin);
	FString ToDebugString(const UEdGraphPin* Pin);
	FString ToDebugString(const FEdGraphPinType& Type);
	FString ToDebugString(const UK2Node* Node);

	/**
	 * Search for first metadata encounter in hierarchy
	 *
	 * @param InClass Class to search
	 * @param Name Metadata key to search
	 * @return found metadata value
	 */
	const FString* FindMetadataHierarchical(const UClass* InClass, const FName& Name);

	const FEdGraphPinType& GetWildcardType();
	/**
	 * Does pin have wildcard type set
	 */
	bool IsWildcardType(const UEdGraphPin* Pin);
	bool IsWildcardType(const FEdGraphPinType& Type);

	/**
	 * Does pin have a capturable type set.
	 */
	bool IsCapturableType(const UEdGraphPin* Pin);
	bool IsCapturableType(const FEdGraphPinType& Type);

	/**
	 * Determine common pin type for capture pair
	 *
	 * @param InputPin
	 * @param OutputPin
	 */
	FEdGraphPinType DeterminePinType(const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin);

	/**
	 * Identify property type for the pin
	 * @param Pin
	 * @return if identification was successful
	 */
	FPropertyTypeInfo IdentifyPropertyTypeForPin(const UEdGraphPin* Pin);
	FPropertyTypeInfo IdentifyPropertyTypeForPin(const FEdGraphPinType& Type);

	/**
	 * Select matching library accessor
	 * @param Pin
	 * @param AccessType
	 * @param OutFunction
	 * @return
	 */
	bool SelectAccessorForType(UEdGraphPin* Pin, EAccessorRole AccessType, FName& OutFunction);
	bool SelectAccessorForType(const FEdGraphPinType& PinType, EAccessorRole AccessType, FName& OutFunction);
}
