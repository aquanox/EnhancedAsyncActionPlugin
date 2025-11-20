// Copyright 2025, Aquanox.

#pragma once

#include "UObject/Object.h"
#include "Templates/TypeHash.h"
#include "EnhancedAsyncContextHandle.h"
#include "EnhancedAsyncActionHandle.generated.h"

#define UE_API ENHANCEDASYNCACTION_API

// Wrapper over actual context data for blueprints
USTRUCT(BlueprintType, meta=(DisableSplitPin))
struct UE_API FEnhancedAsyncActionContextHandle : public FAsyncContextHandleBase
{
	GENERATED_BODY()

public:
	FEnhancedAsyncActionContextHandle();
	FEnhancedAsyncActionContextHandle(FAsyncContextId ContextId, TWeakObjectPtr<const UObject> Owner, TSharedRef<FEnhancedAsyncActionContext> Data);

	/** Shortcut to resolve context from manager */
	TSharedPtr<FEnhancedAsyncActionContext> GetContext() const;
	/** Shortcut to resolve context from manager */
	TSharedRef<FEnhancedAsyncActionContext> GetContextSafe() const;
};

template <>
struct TStructOpsTypeTraits<FEnhancedAsyncActionContextHandle>
	: public TStructOpsTypeTraitsBase2<FEnhancedAsyncActionContextHandle>
{
	enum
	{
		// WithCopy = true,
	};
};

template <>
inline FAsyncContextId FAsyncContextId::Make(const UObject* const& InObject)
{
#if WITH_CONTEXT_TYPE_TAG
	return FAsyncContextId( ::PointerHash(InObject, INDEX_NONE), FAsyncContextId::EContextType::CT_AsyncAction );
#else
	return FAsyncContextId(::PointerHash(InObject, INDEX_NONE));
#endif
}

#undef UE_API
