// Copyright 2025, Aquanox.

#pragma once

#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObject/UnrealType.h"

#define UE_API ENHANCEDASYNCACTION_API

namespace EAA::Switches
{
	// gather known types and call setup to optimize property bag reconstruction
	constexpr bool bWithSetupContext = true;
	// enables debug tooltips
	constexpr bool bDebugTooltips = true;
	// uses variadic set/get function instead of multiple single get/set
	constexpr bool bVariadicGetSet = true;
	// latents use only variadic args nodes for now
	constexpr bool bEnableLatents = true;
	// optimize generated graph by skipping literals and using pin values directly
	constexpr bool bOptimizeSkipLiterals = true;
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

	// Expose context pin on async action node
	static const FName MD_ExposedAsyncContext = TEXT("ExposedAsyncContext");

	// Marker to use enhanced latent node.
	// Example: HasLatentContext=HandleParameter
	static const FName MD_HasLatentContext = TEXT("HasLatentContext");

	// Switch defining which node generation path should be used
	// Example: LatentTrigger=(Then or Event)
	static const FName MD_LatentTrigger = TEXT("LatentTrigger");

	UE_API bool IsValidContainerProperty(const UObject* Object, const FName& Property);

	UE_API bool IsValidLatentCallable(const UFunction* Object);

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
}

UE_API DECLARE_LOG_CATEGORY_EXTERN(LogEnhancedAction, All, All);

#undef UE_API
