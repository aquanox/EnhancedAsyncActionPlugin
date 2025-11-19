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
	FCoreDelegates::OnExit.AddRaw(this, &FEnhancedAsyncContextManager::OnShutdown);
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

	if (auto ExistingHandle = ActionContexts.FindRef(Id))
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

TValueOrError<FEnhancedLatentActionContextHandle, FString> FEnhancedAsyncContextManager::CreateContext(const FLatentCallInfo& CallInfo)
{
	FScopeLock Lock(&MapCriticalSection);

	if (!CallInfo.IsValid())
	{
		return MakeError(TEXT("Invalid latent info"));
	}

	const FAsyncContextId Id = FAsyncContextId::Make(CallInfo);

	if (TSharedPtr<FEnhancedAsyncActionContext> ExistingHandle = ActionContexts.FindRef(Id))
	{
		return MakeError(TEXT("Invalid action identifier"));
	}

	auto Context = MakeShared<FEnhancedAsyncActionContext_PropertyBag>(CallInfo.OwningObject.Get());

	SetContextInternal(Id, Context, CallInfo.OwningObject.Get());

	const FEnhancedLatentActionContextHandle Result(Id, CallInfo, Context);

	return MakeValue(Result);
}

TValueOrError<int32, FString> FEnhancedAsyncContextManager::DestroyContext(const FAsyncContextId& ContextId)
{
	FScopeLock Lock(&MapCriticalSection);

	if (TSharedPtr<FEnhancedAsyncActionContext> ContextObject = ActionContexts.FindRef(ContextId))
	{
		int32 Removed = ActionContexts.Remove(ContextId);

		if (const UObject* Owner = ContextObject->GetOwningObject())
		{
			TrackedObjects.RemoveSingle(Owner, ContextId);
		}

		return MakeValue(Removed);
	}

	ObjectListener.UpdateState();
	return MakeValue(0);
}

TValueOrError<int32, FString> FEnhancedAsyncContextManager::DestroyContext(const FEnhancedLatentActionContextHandle& Handle)
{
	FScopeLock Lock(&MapCriticalSection);

	auto Value = DestroyContext(Handle.GetId());

	if (const UObject* Owner = Handle.Owner.GetEvenIfUnreachable())
	{
		TrackedObjects.RemoveSingle(Owner, Handle.GetId());
	}

	return Value;
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

FEnhancedAsyncActionContextHandle FEnhancedAsyncContextManager::FindContextHandle(const UObject* Action)
{
	FScopeLock Lock(&MapCriticalSection);

	const FAsyncContextId Id = FAsyncContextId::Make(Action);
	if (TSharedPtr<FEnhancedAsyncActionContext> ContextObject = ActionContexts.FindRef(Id))
	{
		return FEnhancedAsyncActionContextHandle( Id, Action, ContextObject.ToSharedRef() );
	}
	return FEnhancedAsyncActionContextHandle();
}

FEnhancedLatentActionContextHandle FEnhancedAsyncContextManager::FindContextHandle(const FLatentCallInfo& CallInfo)
{
	FScopeLock Lock(&MapCriticalSection);

	const FAsyncContextId Id = FAsyncContextId::Make(CallInfo);
	if (TSharedPtr<FEnhancedAsyncActionContext> ContextObject = ActionContexts.FindRef(Id))
	{
		return FEnhancedLatentActionContextHandle( Id, CallInfo, ContextObject.ToSharedRef()  );
	}
	return FEnhancedLatentActionContextHandle();
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedAsyncContextManager::HandleError(EResolveErrorMode OnError, const TCHAR* Message) const
{
	switch (OnError)
	{
	case EResolveErrorMode::AllowNull:
		return TSharedPtr<FEnhancedAsyncActionContext>();
	case EResolveErrorMode::Fallback:
		return DummyContext;
	case EResolveErrorMode::Assert:
		ensureAlwaysMsgf(false, TEXT("%s"), Message);
		return DummyContext;
	default:
		checkNoEntry();
		return DummyContext;
	}
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedAsyncContextManager::FindContext(const FAsyncContextHandleBase& Handle, EResolveErrorMode OnError)
{
	if (!Handle.IsValid())
	{
		return HandleError(OnError, TEXT("Failed to locate bound context object"));
	}

	auto ActualContext = Handle.Data.Pin();

	if (!ActualContext)
	{
		return HandleError(OnError, TEXT("Failed to locate bound context object"));
	}

	if (!ActualContext->IsValid())
	{
		return HandleError(OnError, TEXT("Bound context object is stale"));
	}
	return ActualContext;
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedAsyncContextManager::FindContext(const FAsyncContextId& ContextId, EResolveErrorMode OnError)
{
	if (!ContextId)
	{
		return HandleError(OnError, TEXT("Failed to locate bound context object"));
	}

	FScopeLock Lock(&MapCriticalSection);

	// auto ActualContext = Handle.Data.Pin();
	auto ActualContext = ActionContexts.FindRef(ContextId);

	if (!ActualContext)
	{
		return HandleError(OnError, TEXT("Failed to locate bound context object"));
	}

	if (!ActualContext->IsValid())
	{
		return HandleError(OnError, TEXT("Bound context object is stale"));
	}
	return ActualContext;
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

	FCoreDelegates::OnExit.RemoveAll(this);
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
