// Copyright 2025, Aquanox.

#include "EnhancedAsyncContextHandle.h"
#include "EnhancedAsyncContextManager.h"

const FAsyncContextId FAsyncContextId::Invalid(INDEX_NONE);

FAsyncContextHandleBase::FAsyncContextHandleBase(const FAsyncContextId& Id)
	: ContextId(Id)
{
}

FAsyncContextHandleBase::FAsyncContextHandleBase(const FAsyncContextId& Id, TWeakObjectPtr<const UObject> Owner, TSharedRef<FEnhancedAsyncActionContext> Data)
	: ContextId(Id), Owner(Owner), Data(Data)
{
}

const FAsyncContextId& FAsyncContextHandleBase::GetId() const
{
	return ContextId;
}

bool FAsyncContextHandleBase::IsValid() const
{
	return ContextId && Owner.IsValid() && Data.IsValid();
}
