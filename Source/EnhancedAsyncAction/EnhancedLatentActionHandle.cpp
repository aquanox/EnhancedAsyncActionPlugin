// Copyright 2025, Aquanox.

#include "EnhancedLatentActionHandle.h"
#include "EnhancedAsyncContextManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnhancedLatentActionHandle)

FEnhancedLatentActionContextHandle::FEnhancedLatentActionContextHandle()
	: FAsyncContextHandleBase(FAsyncContextId(FAsyncContextId::InvalidValue))
{
}

FEnhancedLatentActionContextHandle::FEnhancedLatentActionContextHandle(FLatentCallInfo InCallInfo)
	: FAsyncContextHandleBase(FAsyncContextId(FAsyncContextId::InvalidValue)), CallInfo(InCallInfo)
{
}

FEnhancedLatentActionContextHandle::FEnhancedLatentActionContextHandle(FAsyncContextId ContextId, FLatentCallInfo InCallInfo, TSharedRef<FEnhancedAsyncActionContext> Data)
	: FAsyncContextHandleBase(ContextId, InCallInfo.OwningObject, Data), CallInfo(InCallInfo)
{
}

bool FEnhancedLatentActionContextHandle::IsValid() const
{
	return Super::IsValid() && CallInfo.UUID != 0 && CallInfo.CallID != 0;
}

void FEnhancedLatentActionContextHandle::ReleaseContext() const
{
	FEnhancedAsyncContextManager::Get().DestroyContext(GetId());
}

void FEnhancedLatentActionContextHandle::ReleaseContextAndInvalidate()
{
	ReleaseContext();
	*this = FEnhancedLatentActionContextHandle();
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedLatentActionContextHandle::GetContext() const
{
	return FEnhancedAsyncContextManager::Get().FindContext(*this, EResolveErrorMode::AllowNull);
}

TSharedRef<FEnhancedAsyncActionContext> FEnhancedLatentActionContextHandle::GetContextSafe() const
{
	return FEnhancedAsyncContextManager::Get().FindContext(*this, EResolveErrorMode::Fallback).ToSharedRef();
}
