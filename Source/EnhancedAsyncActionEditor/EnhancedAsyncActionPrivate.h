// Copyright 2025, Aquanox.

#pragma once

#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "EnhancedAsyncActionShared.h"

class UK2Node_EnhancedAsyncTaskBase;
class UK2Node_CallFunction;
class UEdGraphSchema_K2;

namespace EAA::Internals
{
	FString ToDebugString(const FProperty* Pin);
	FString ToDebugString(const UEdGraphPin* Pin);
	FString ToDebugString(const FEdGraphPinType& Type);
	FString ToDebugString(const UK2Node_EnhancedAsyncTaskBase* Node);

	/**
	 * Test if class suitable for the enhancement
	 */
	bool IsValidProxyClass(UClass* InClass);

	/**
	 * Serch for first metadata encounter first on proxy type (return value of proxy factory function)
	 * in second round do search on proxy factory type.
	 *
	 * Case: in case factory functions returns base pointer type metadata won't be found
	 *
	 * @param FactoryClass
	 * @param ProxyClass
	 * @param Name
	 * @return
	 */
	//const FString* FindMetadataHierarchical(const UClass* FactoryClass, const UClass* ProxyClass, const FName& Name);

	/**
	 * Search for first metadata encounter in hierarchy
	 *
	 * @param InClass Class to search
	 * @param Name Metadata key to search
	 * @return found metadata value
	 */
	const FString* FindMetadataHierarchical(const UClass* InClass, const FName& Name);


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
	 * Identify property type for the pin
	 * @param Pin
	 * @return if identification was successful
	 */
	FPropertyTypeInfo IdentifyPropertyTypeForPin(const UEdGraphPin* Pin);
	FPropertyTypeInfo IdentifyPropertyTypeForPin(const FEdGraphPinType& Type);

	/**
	 * Detect pin type for in/out pair.
	 *
	 * @param InputPin
	 * @param OutputPin

	 * @return shared type or wildcard
	 */
	FEdGraphPinType DeterminePinType(UEdGraphPin* InputPin, UEdGraphPin* OutputPin);

	/**
	 * Select matching library accessor
	 * @param Pin
	 * @param AccessType
	 * @param OutFunction
	 * @return
	 */
	bool SelectAccessorForType(UEdGraphPin* Pin, EAccessorRole AccessType, FName& OutFunction);
	bool SelectAccessorForType(const FEdGraphPinType& PinType, EAccessorRole AccessType, FName& OutFunction);

	void BuildDefaultCapturePins(UK2Node_EnhancedAsyncTaskBase* Node);

	FString BuildContextConfigString(const UK2Node_EnhancedAsyncTaskBase* Node, int32 NumCaptures);
}
