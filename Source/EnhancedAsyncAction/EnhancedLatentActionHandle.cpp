// Copyright 2025, Aquanox.

#include "EnhancedLatentActionHandle.h"
#include "EnhancedAsyncContextManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnhancedLatentActionHandle)

bool FLatentCallResult::IsValid() const
{
	return OwningObject.IsValid() && UUID != 0 && Action != nullptr;
}

FEnhancedLatentActionContextHandle::FEnhancedLatentActionContextHandle()
	: FAsyncContextHandleBase(FAsyncContextId::Invalid)
{
}

FEnhancedLatentActionContextHandle::FEnhancedLatentActionContextHandle(FAsyncContextId ContextId, TWeakObjectPtr<const UObject> Owner, TSharedRef<FEnhancedAsyncActionContext> Data)
	: FAsyncContextHandleBase(ContextId, Owner, Data)
{
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedLatentActionContextHandle::GetContext() const
{
	return FEnhancedAsyncContextManager::Get().ResolveContextHandle(*this, EResolveErrorMode::AllowNull);
}

TSharedRef<FEnhancedAsyncActionContext> FEnhancedLatentActionContextHandle::GetContextSafe() const
{
	return FEnhancedAsyncContextManager::Get().ResolveContextHandle(*this, EResolveErrorMode::Fallback).ToSharedRef();
}
