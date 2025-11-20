// Copyright 2025, Aquanox.

#include "EnhancedAsyncActionHandle.h"
#include "EnhancedAsyncContextManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnhancedAsyncActionHandle)

FEnhancedAsyncActionContextHandle::FEnhancedAsyncActionContextHandle()
	: FAsyncContextHandleBase(FAsyncContextId(FAsyncContextId::InvalidValue))
{
}

FEnhancedAsyncActionContextHandle::FEnhancedAsyncActionContextHandle(FAsyncContextId ContextId, TWeakObjectPtr<const UObject> Owner, TSharedRef<FEnhancedAsyncActionContext> Data)
	: FAsyncContextHandleBase(ContextId, Owner, Data)
{
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedAsyncActionContextHandle::GetContext() const
{
	return FEnhancedAsyncContextManager::Get().FindContext(*this, EResolveErrorMode::AllowNull);
}

TSharedRef<FEnhancedAsyncActionContext> FEnhancedAsyncActionContextHandle::GetContextSafe() const
{
	return FEnhancedAsyncContextManager::Get().FindContext(*this, EResolveErrorMode::Fallback).ToSharedRef();
}
