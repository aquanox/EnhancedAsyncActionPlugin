// Copyright 2025, Aquanox.

#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "UObject/ObjectPtr.h"

#define UE_API ENHANCEDASYNCACTION_API

class FMulticastDelegateProperty;
enum class EPropertyBagContainerType : uint8;
enum class EPropertyBagPropertyType : uint8;

/**
 * A helper type containing information about runtime property data without ties to editor.
 */
struct UE_API FPropertyTypeInfo
{
	static const FPropertyTypeInfo Wildcard;
	static const FPropertyTypeInfo Invalid;

	// container
	EPropertyBagContainerType ContainerType;

	EPropertyBagPropertyType ValueType; // for singles
	TObjectPtr<const UObject> ValueTypeObject;

	EPropertyBagPropertyType KeyType; // for maps
	TObjectPtr<const UObject> KeyTypeObject;

	FPropertyTypeInfo();
	// normal
	explicit FPropertyTypeInfo(EPropertyBagPropertyType Type);
	// from reflection
	explicit FPropertyTypeInfo(const FProperty* ExistingProperty);
	// simple
	FPropertyTypeInfo(EPropertyBagPropertyType Type, TObjectPtr<const UObject> Object);

	//
	enum EInternalPreset { PRESET_Wildcard, PRESET_Invalid };
	explicit FPropertyTypeInfo(EInternalPreset Preset);

	bool operator==(const FPropertyTypeInfo&) const = default;
	bool operator!=(const FPropertyTypeInfo&) const = default;

	bool IsWildcard() const;
	bool IsValid() const;
	bool IsSupported() const;

	/**
	 *
	 */
	static FString EncodeTypeInfo(const FPropertyTypeInfo& TypeInfo);
	/**
	 *
	 */
	static bool ParseTypeInfo(FString Data, FPropertyTypeInfo& TypeInfo);
};

namespace EAA::Internals
{
	UE_API bool HasAccessorForType(const FPropertyTypeInfo& TypeInfo);

	enum class EAccessorRole { GETTER, SETTER };
	enum class EContainerType { SINGLE, ARRAY, MAP, SET };

	/**
	 * Select suitable accessor for specified property combination
	 */
	UE_API bool SelectAccessorForType(const FPropertyTypeInfo& TypeInfo, EAccessorRole Role, FName& OutFunction);

	UE_API EPropertyBagContainerType GetContainerTypeFromProperty(const FProperty* InSourceProperty);
	UE_API EPropertyBagPropertyType GetValueTypeFromProperty(const FProperty* InSourceProperty);
	UE_API UObject* GetValueTypeObjectFromProperty(const FProperty* InSourceProperty);
}

#undef UE_API
