// Copyright 2025, Aquanox.

#include "EnhancedAsyncActionManager.h"
#include "EnhancedAsyncActionContext.h"
#include "EnhancedAsyncActionContextImpl.h"
#include "Misc/CoreDelegates.h"
#include "Misc/ScopeLock.h"

FEnhancedAsyncActionManager::FEnhancedAsyncActionManager()
	: DummyContext(MakeShared<FEnhancedAsyncActionContextStub>())
{
}

FEnhancedAsyncActionManager::~FEnhancedAsyncActionManager()
{
	check(ActionContexts.Num() == 0);
	check(!ObjectCollector.IsValid());
}

FEnhancedAsyncActionManager& FEnhancedAsyncActionManager::Get()
{
	static FEnhancedAsyncActionManager Manager;
	return Manager;
}

TValueOrError<FEnhancedAsyncActionContextHandle, FString> FEnhancedAsyncActionManager::CreateContext(const FCreateContextParams& Params)
{
	FScopeLock Lock(&MapCriticalSection);

	const UObject* Action = Params.Action;
	if (!IsValid(Action))
	{
		return MakeError(TEXT("Invalid action object"));
	}

	auto* ExistingHandle = ActionContexts.Find(Action);
	if (ExistingHandle)
	{
		return MakeError(TEXT("Object already has bound context"));
	}

	TSharedPtr<FEnhancedAsyncActionContext> Context;

	FName InDataProperty = Params.InnerProperty;
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

	FEnhancedAsyncActionContextHandle Handle = SetContext(Action, Context.ToSharedRef());
	Handle.DataProperty = InDataProperty;
	return MakeValue(Handle);
}

FEnhancedAsyncActionContextHandle FEnhancedAsyncActionManager::SetContext(const UObject* Action, TSharedRef<FEnhancedAsyncActionContext> Context)
{
	FScopeLock Lock(&MapCriticalSection);

	if (ActionContexts.Num() == 0)
	{
		EnableListener();
	}

	ensure(!ActionContexts.Contains(Action));
	ActionContexts.Add(Action, MoveTemp(Context));

	return FEnhancedAsyncActionContextHandle { const_cast<UObject*>(Action), Context };
}

FEnhancedAsyncActionContextHandle FEnhancedAsyncActionManager::FindContextHandle(const UObject* Action)
{
	FScopeLock Lock(&MapCriticalSection);

	if (auto* ContextObject = ActionContexts.Find(Action))
	{
		return FEnhancedAsyncActionContextHandle { const_cast<UObject*>(Action), ContextObject->ToSharedRef() };
	}
	return FEnhancedAsyncActionContextHandle();
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedAsyncActionManager::FindContext(const UObject* Action)
{
	FScopeLock Lock(&MapCriticalSection);

	if (auto* ContextObject = ActionContexts.Find(Action))
	{
		return *ContextObject;
	}
	return nullptr;
}

TSharedRef<FEnhancedAsyncActionContext> FEnhancedAsyncActionManager::FindContextSafe(const UObject* Action)
{
	FScopeLock Lock(&MapCriticalSection);

	if (auto* ContextObject = ActionContexts.Find(Action))
	{
		return ContextObject->ToSharedRef();
	}
	return DummyContext;
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedAsyncActionManager::FindContext(const FEnhancedAsyncActionContextHandle& Handle)
{
	if (!Handle.IsValid())
	{ // handle itself is not valid
		return nullptr;
	}
	auto ActualContext = Handle.Data.Pin();

	if (!ActualContext || !ActualContext->IsValid())
	{ // implementation is bad (like inner uobject member ref)
		return nullptr;
	}
	return ActualContext;
}

TSharedRef<FEnhancedAsyncActionContext> FEnhancedAsyncActionManager::FindContextSafe(const FEnhancedAsyncActionContextHandle& Handle)
{
	auto ActualContext = FindContext(Handle);
	return ActualContext ? ActualContext.ToSharedRef() : DummyContext;
}

void FEnhancedAsyncActionManager::FGCCollector::AddReferencedObjects(FReferenceCollector& Collector)
{
	Owner->AddReferencedObjects(Collector);
}

void FEnhancedAsyncActionManager::AddReferencedObjects(FReferenceCollector& Collector)
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

void FEnhancedAsyncActionManager::FObjectListener::NotifyUObjectDeleted(const UObjectBase* Object, int32 Index)
{
	Owner->OnObjectDeleted(static_cast<const UObject*>(Object));
}

void FEnhancedAsyncActionManager::OnObjectDeleted(const UObject* Object)
{
	FScopeLock Lock(&MapCriticalSection);
	if (ActionContexts.Remove(Object) > 0)
	{
		if (ActionContexts.Num() == 0)
		{
			DisableListener();
		}
	}
}

void FEnhancedAsyncActionManager::FObjectListener::OnUObjectArrayShutdown()
{
	Owner->OnShutdown();
}

void FEnhancedAsyncActionManager::OnShutdown()
{
	FScopeLock Lock(&MapCriticalSection);
	DisableListener();
	ObjectCollector.Reset();
	ActionContexts.Empty();
}

void FEnhancedAsyncActionManager::EnableListener()
{
	GUObjectArray.AddUObjectDeleteListener(&ObjectListener);

	if (!ObjectCollector.IsValid())
	{
		ObjectCollector = MakeShared<FGCCollector>(this);
	}
}

void FEnhancedAsyncActionManager::DisableListener()
{
	GUObjectArray.RemoveUObjectDeleteListener(&ObjectListener);
}
