// Copyright 2025, Aquanox.

#include "EnhancedAsyncContextManager.h"

#include "EnhancedAsyncContext.h"
#include "EnhancedAsyncContextImpl.h"
#include "EnhancedAsyncActionHandle.h"
#include "EnhancedLatentActionHandle.h"
#include "Misc/CoreDelegates.h"
#include "Misc/ScopeLock.h"

FEnhancedAsyncContextManager::FEnhancedAsyncContextManager()
	: DummyContext(MakeShared<FEnhancedAsyncActionContextStub>())
{
}

FEnhancedAsyncContextManager::~FEnhancedAsyncContextManager()
{
	ensure(ActionContexts.Num() == 0);
	ensure(!ObjectCollector.IsValid());
	//ObjectListener.DisableListener();
}

FEnhancedAsyncContextManager& FEnhancedAsyncContextManager::Get()
{
	static FEnhancedAsyncContextManager Manager;
	return Manager;
}

TValueOrError<FEnhancedAsyncActionContextHandle, FString> FEnhancedAsyncContextManager::CreateContext(const UObject* Action, FName InnerProperty)
{
	FScopeLock Lock(&MapCriticalSection);

	if (!IsValid(Action))
	{
		return MakeError(TEXT("Invalid action object"));
	}

	const FAsyncContextId Id = FAsyncContextId::Make(Action);

	if (auto ExistingHandle = FindContextInternal(Id))
	{
		return MakeError(TEXT("Object already has bound context"));
	}

	TSharedPtr<FEnhancedAsyncActionContext> Context;

	FName InDataProperty = InnerProperty;
	if (EAA::Internals::IsValidContainerProperty(Action, InDataProperty))
	{
		Context = MakeShared<FEnhancedAsyncActionContext_PropertyBagRef>(Action, InDataProperty);
	}
	else
	{
		if (!InDataProperty.IsNone())
		{
			UE_LOG(LogEnhancedAction, Log, TEXT("Missing expected member property %s:%s"), *Action->GetClass()->GetName(), *InDataProperty.ToString());
			InDataProperty = NAME_None;
		}

		Context = MakeShared<FEnhancedAsyncActionContext_PropertyBag>(Action);
	}

	if (!Context.IsValid())
	{
		return MakeError(TEXT("Failed to select context backend implementation"));
	}

	SetContextInternal(Id, Context.ToSharedRef(), Action);

	const FEnhancedAsyncActionContextHandle Result(Id, Action, Context.ToSharedRef());

	return MakeValue(Result);
}

TValueOrError<FEnhancedLatentActionContextHandle, FString> FEnhancedAsyncContextManager::CreateContext(const FLatentCallResult& CallResult)
{
	FScopeLock Lock(&MapCriticalSection);

	if (!CallResult.IsValid())
	{
		return MakeError(TEXT("Invalid latent info"));
	}

	const FAsyncContextId Id = FAsyncContextId::Make(CallResult);

	if (TSharedPtr<FEnhancedAsyncActionContext> ExistingHandle = FindContextInternal(Id))
	{
		return MakeError(TEXT("Invalid action identifier"));
	}

	TSharedPtr<FEnhancedAsyncActionContext> Context;
	if (CallResult.Container)
	{
		Context = MakeShared<FEnhancedAsyncActionContext_PropertyBagRef>(CallResult.OwningObject.Get(), CallResult.Container, true);
	}
	else
	{
		Context = MakeShared<FEnhancedAsyncActionContext_PropertyBag>(CallResult.OwningObject.Get());
	}

	if (!Context.IsValid())
	{
		return MakeError(TEXT("Failed to select context backend implementation"));
	}

	SetContextInternal(Id, Context.ToSharedRef(), CallResult.OwningObject.Get());

	const FEnhancedLatentActionContextHandle Result(Id, CallResult.OwningObject, Context.ToSharedRef());

	return MakeValue(Result);
}

void FEnhancedAsyncContextManager::DestroyContext(const struct FLatentCallResult& CallResult)
{
	FScopeLock Lock(&MapCriticalSection);

	const FAsyncContextId Id = FAsyncContextId::Make(CallResult);
	if (ActionContexts.Remove(Id) > 0)
	{
		if (const UObject* Owner = CallResult.OwningObject.GetEvenIfUnreachable())
		{
			TrackedObjects.RemoveSingle(Owner, Id);
		}
	}
	ObjectListener.UpdateState();
}

void FEnhancedAsyncContextManager::SetContextInternal(FAsyncContextId ContextId, TSharedRef<FEnhancedAsyncActionContext> Context, const UObject* TrackedOwner)
{
	FScopeLock Lock(&MapCriticalSection);

	if (!ObjectCollector.IsValid() && Context->CanAddReferencedObjects())
	{ // create collector lazily
		ObjectCollector = MakeShared<FGCCollector>(this);
	}

	bool bDuplicate = ActionContexts.Contains(ContextId);
	checkf(!bDuplicate, TEXT("CRITICAL: For some unknown reason there is a context with same identifier???"));
	if (!bDuplicate)
	{
		ActionContexts.Add(ContextId, Context);
		if (TrackedOwner)
		{
			TrackedObjects.AddUnique(TrackedOwner, ContextId);
		}
	}

	ObjectListener.UpdateState();
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedAsyncContextManager::FindContextInternal(const FAsyncContextId& ContextId)
{
	FScopeLock Lock(&MapCriticalSection);

	return ActionContexts.FindRef(ContextId);
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedAsyncContextManager::FindContext(FAsyncContextId ContextId, bool bAllowNull)
{
	if (TSharedPtr<FEnhancedAsyncActionContext> ContextObject = FindContextInternal(ContextId))
	{
		return ContextObject;
	}
	return bAllowNull ? TSharedPtr<FEnhancedAsyncActionContext>() : DummyContext;
}

FEnhancedAsyncActionContextHandle FEnhancedAsyncContextManager::FindContextHandle(const UObject* Action)
{
	const FAsyncContextId Id = FAsyncContextId::Make(Action);
	if (TSharedPtr<FEnhancedAsyncActionContext> ContextObject = FindContextInternal(Id))
	{
		return FEnhancedAsyncActionContextHandle( Id, Action, ContextObject.ToSharedRef() );
	}
	return FEnhancedAsyncActionContextHandle();
}

FEnhancedLatentActionContextHandle FEnhancedAsyncContextManager::FindContextHandle(const FLatentCallResult& CallResult)
{
	const FAsyncContextId Id = FAsyncContextId::Make(CallResult);
	if (TSharedPtr<FEnhancedAsyncActionContext> ContextObject = FindContextInternal(Id))
	{
		return FEnhancedLatentActionContextHandle( Id, CallResult.OwningObject, ContextObject.ToSharedRef()  );
	}
	return FEnhancedLatentActionContextHandle();
}

template<typename T>
TSharedPtr<FEnhancedAsyncActionContext> FEnhancedAsyncContextManager::ResolveContextHandleInternal(const T& Handle, EResolveErrorMode OnError)
{
	if (!Handle.IsValid())
	{ // handle itself is not valid
		switch (OnError)
		{
		case EResolveErrorMode::AllowNull:
			return TSharedPtr<FEnhancedAsyncActionContext>();
		case EResolveErrorMode::Fallback:
			return DummyContext;
		case EResolveErrorMode::Assert:
			ensureAlwaysMsgf(false, TEXT("Failed to locate bound context object"));
			return DummyContext;
		}
	}

	// TBD: choose how to lookup context
	// option 1: handle has weak ptr to data and use it (faster, blueprints carry weakptr over)
	// option 2: lock & search in map by Handle.ContextId (slower, needs lock)
	auto ActualContext = Handle.Data.Pin();

	if (!ActualContext || !ActualContext->IsValid())
	{ // implementation is bad (like inner uobject member ref)
		switch (OnError)
		{
		case EResolveErrorMode::AllowNull:
			return TSharedPtr<FEnhancedAsyncActionContext>();
		case EResolveErrorMode::Fallback:
			return DummyContext;
		case EResolveErrorMode::Assert:
			ensureAlwaysMsgf(false, TEXT("Failed to locate bound context object"));
			return DummyContext;
		}
	}
	return ActualContext;
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedAsyncContextManager::ResolveContextHandle(const FEnhancedAsyncActionContextHandle& Handle, EResolveErrorMode OnError)
{
	return ResolveContextHandleInternal(Handle, OnError);
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedAsyncContextManager::ResolveContextHandle(const FEnhancedLatentActionContextHandle& Handle, EResolveErrorMode OnError)
{
	return ResolveContextHandleInternal(Handle, OnError);
}

void FEnhancedAsyncContextManager::FGCCollector::AddReferencedObjects(FReferenceCollector& Collector)
{
	Owner->AddReferencedObjects(Collector);
}

void FEnhancedAsyncContextManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	FScopeLock Lock(&MapCriticalSection);
	for (auto It = ActionContexts.CreateIterator(); It; ++It)
	{
		if (It->Value->CanAddReferencedObjects() && It->Value->IsValid())
		{
			It->Value->AddReferencedObjects(Collector);
		}
	}
}

void FEnhancedAsyncContextManager::FObjectListener::NotifyUObjectDeleted(const UObjectBase* Object, int32 Index)
{
	Owner->OnObjectDeleted(static_cast<const UObject*>(Object));
}

void FEnhancedAsyncContextManager::OnObjectDeleted(const UObject* Object)
{
	FScopeLock Lock(&MapCriticalSection);

	const FAsyncContextId Id = FAsyncContextId::Make(Object);
	if (ActionContexts.Remove(Id) > 0)
	{ // this was an async action object. it will have just one entry as it can't have latents within
		TrackedObjects.Remove(Object);
	}
	else if (TrackedObjects.Contains(Object))
	{ // this will be latent action owner. once it is gone - all latents should be gone
		TArray<FAsyncContextId, TFixedAllocator<32>> BoundContextIds;
		TrackedObjects.MultiFind(Object, BoundContextIds);
		TrackedObjects.Remove(Object);

		for (const FAsyncContextId& ContextId : BoundContextIds)
		{
			ActionContexts.Remove(ContextId);
		}
	}

	ObjectListener.UpdateState();
}

void FEnhancedAsyncContextManager::FObjectListener::OnUObjectArrayShutdown()
{
	Owner->OnShutdown();
}

void FEnhancedAsyncContextManager::OnShutdown()
{
	FScopeLock Lock(&MapCriticalSection);
	ObjectListener.DisableListener();
	ObjectCollector.Reset();
	ActionContexts.Empty();
	TrackedObjects.Empty();
}

void FEnhancedAsyncContextManager::FObjectListener::UpdateState()
{
	const int32 NumContexts = Owner->ActionContexts.Num();
	if (bEnabled && NumContexts == 0)
	{
		DisableListener();
	}
	if (!bEnabled && NumContexts != 0)
	{
		EnableListener();
	}
}

void FEnhancedAsyncContextManager::FObjectListener::EnableListener()
{
	GUObjectArray.AddUObjectDeleteListener(this);
	bEnabled = true;
}

void FEnhancedAsyncContextManager::FObjectListener::DisableListener()
{
	GUObjectArray.RemoveUObjectDeleteListener(this);
	bEnabled = false;
}
