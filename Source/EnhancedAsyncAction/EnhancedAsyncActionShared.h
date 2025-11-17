// Copyright 2025, Aquanox.

#pragma once

#include "EnhancedAsyncActionContext.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObject/UnrealType.h"

class FMulticastDelegateProperty;
struct FPropertyTypeInfo;

#define UE_API ENHANCEDASYNCACTION_API

namespace EAA::Switches
{
	// gather known types and call setup to optimize property bag reconstruction
	constexpr bool bWithSetupContext = true;
	// enables debug tooltips
	constexpr bool bDebugTooltips = false;
	// uses variadic set/get function instead of multiple single get/set
	constexpr bool bVariadicGetSet = true;
}

namespace EAA::Internals
{
	// Flag to mark node to use async context feature
	//    specifies name for async context delegate parameter
	static const FName MD_HasAsyncContext = TEXT("HasAsyncContext");

	// container of async context (if present - use member, if missing - use external)
	// values:
	//    valid property name of type InstancedPropertyBag
	//	  default = external InstancedPropertyBag
	static const FName MD_AsyncContextContainer = TEXT("AsyncContextContainer");

	// Expose async context pin on node
	static const FName MD_ExposedAsyncContext = TEXT("ExposedAsyncContext");

	static const FName PIN_Handle = TEXT("Handle");
	static const FName PIN_Index = TEXT("Index");
	static const FName PIN_Value = TEXT("Value");
	static const FName PIN_Action = TEXT("Action");
	static const FName PIN_Object = TEXT("Object");

	bool IsValidContainerProperty(const UObject* Object, const FName& Property);

	template <typename T>
	T* GetMemberChecked(const UObject* Object, const FName& Property, const UScriptStruct* ExpectedType = nullptr)
	{
		const FStructProperty* Prop = CastField<FStructProperty>(Object->GetClass()->FindPropertyByName(Property));
		check(Prop && (!ExpectedType || ExpectedType == Prop->Struct));
		return const_cast<T*>(Prop->ContainerPtrToValuePtr<T>(Object));
	}

	/**
	 * Constant determining maximum amount of capture pins.
	 *
	 * Technically unlimited
	 */
	constexpr int32 MaxCapturePins = 16;

	/**
	 * Get property name within capture container for specific index.
	 *
	 * Note - bound properties still use index and bound property name shown only for UI
	 */
	UE_API FName IndexToName(int32 Index);

	UE_API int32 NameToIndex(const FName& Name);

	UE_API FName IndexToPinName(int32 Index, bool bIsInput);
	UE_API int32 PinNameToIndex(const FName& Name, bool bIsInput);

	UE_API int32 FindPinIndex(const FName& Name);
	UE_API FName MirrorPinName(const FName& Name);

	UE_API bool HasAccessorForType(const FPropertyTypeInfo& TypeInfo);

	enum class EAccessorRole { GETTER, SETTER };
	enum class EContainerType { SINGLE, ARRAY, MAP, SET };

	/**
	 * Select suitable accessor for specified property combination
	 */
	UE_API bool SelectAccessorForType(const FPropertyTypeInfo& TypeInfo, EAccessorRole Role, FName& OutFunction);

	EPropertyBagContainerType GetContainerTypeFromProperty(const FProperty* InSourceProperty);
	EPropertyBagPropertyType GetValueTypeFromProperty(const FProperty* InSourceProperty);
	UObject* GetValueTypeObjectFromProperty(const FProperty* InSourceProperty);

	/**
	 *
	 * @tparam TProperty
	 * @tparam TValueType
	 */
	template<typename TProperty, typename TValueType>
	struct TWeakMemberRef
	{
		TWeakMemberRef(UObject* InOwner, const FName& InMember) : Owner(InOwner), MemberProperty(InMember)
		{
			check(!InMember.IsNone());
		}

		const bool IsValid() const { return Owner.IsValid(); }

		UObject* GetOwner()
		{
			UObject* const WeakOwner = Owner.Get();
			if (WeakOwner != CachedOwner)
			{
				Initialize(WeakOwner);
			}
			return WeakOwner;
		}

		TPair<TProperty*, TValueType*> GetPropertyAndValue()
		{
			UObject* const WeakOwner = Owner.Get();
			if (WeakOwner != CachedOwner)
			{
				Initialize(WeakOwner);
			}
			return TPair(CachedProperty, CachedValueType);
		}

	private:
		void Initialize(UObject* In)
		{
			CachedOwner = In;
			CachedProperty = In ? CastField<TProperty>(In->GetClass()->FindPropertyByName(MemberProperty)) : nullptr;
			CachedValueType = CachedProperty ? CachedProperty->template ContainerPtrToValuePtr<TValueType>(In) : nullptr;
		}
	private:
		TWeakObjectPtr<UObject> Owner;
		FName MemberProperty;

		const UObject* CachedOwner = nullptr;
		TProperty* CachedProperty = nullptr;
		TValueType* CachedValueType = nullptr;
	};
}

UE_API DECLARE_LOG_CATEGORY_EXTERN(LogEnhancedAction, All, All);

#undef UE_API
