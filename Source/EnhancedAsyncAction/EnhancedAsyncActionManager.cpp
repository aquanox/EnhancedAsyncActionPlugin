#include "EnhancedAsyncActionManager.h"
#include "EnhancedAsyncActionContext.h"
#include "EnhancedAsyncActionContextImpl.h"
#include "Misc/CoreDelegates.h"

static FEnhancedAsyncActionManager Manager;

FEnhancedAsyncActionManager::FEnhancedAsyncActionManager()
	: DummyContext(MakeShared<FEnhancedAsyncActionContextStub>())
{
	FCoreDelegates::OnEnginePreExit.AddRaw(this, &FEnhancedAsyncActionManager::OnEngineShutdown);
}

FEnhancedAsyncActionManager::~FEnhancedAsyncActionManager()
{
	FCoreDelegates::OnEnginePreExit.RemoveAll(this);
}

FEnhancedAsyncActionManager& FEnhancedAsyncActionManager::Get()
{
	return Manager;
}

FEnhancedAsyncActionContextHandle FEnhancedAsyncActionManager::SetContext(const UObject* Action, TSharedRef<FEnhancedAsyncActionContext> Context)
{
	FTransactionallySafeScopeLock Lock(&MapCriticalSection);
	
	if (ActionContexts.Num() == 0)
	{
		EnableListener();
	}

	ensure(!ActionContexts.Contains(Action));
	ActionContexts.Add(Action, MoveTemp(Context));

	return FEnhancedAsyncActionContextHandle { const_cast<UObject*>(Action), Context };
}

void FEnhancedAsyncActionManager::OnObjectDeleted(const UObject* Object)
{
	FTransactionallySafeScopeLock Lock(&MapCriticalSection);
	if (ActionContexts.Remove(Object) > 0)
	{
		if (ActionContexts.Num() == 0)
		{
			DisableListener();
		}
	}
}

void FEnhancedAsyncActionManager::OnEngineShutdown()
{
	OnShutdown();
	FCoreDelegates::OnEnginePreExit.RemoveAll(this);
}

void FEnhancedAsyncActionManager::OnShutdown()
{
	FTransactionallySafeScopeLock Lock(&MapCriticalSection);
	DisableListener();
	ActionContexts.Empty();
}

FEnhancedAsyncActionContextHandle FEnhancedAsyncActionManager::FindContextHandle(const UObject* Action)
{
	FTransactionallySafeScopeLock Lock(&MapCriticalSection);

	if (auto* ContextObject = ActionContexts.Find(Action))
	{
		return FEnhancedAsyncActionContextHandle { const_cast<UObject*>(Action), ContextObject->ToSharedRef() };
	}
	return FEnhancedAsyncActionContextHandle();
}

TSharedPtr<FEnhancedAsyncActionContext> FEnhancedAsyncActionManager::FindContext(const UObject* Action)
{
	FTransactionallySafeScopeLock Lock(&MapCriticalSection);

	if (auto* ContextObject = ActionContexts.Find(Action))
	{
		return *ContextObject;
	}
	return nullptr;
}

TSharedRef<FEnhancedAsyncActionContext> FEnhancedAsyncActionManager::FindContextSafe(const UObject* Action)
{
	FTransactionallySafeScopeLock Lock(&MapCriticalSection);

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

void FEnhancedAsyncActionManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	FTransactionallySafeScopeLock Lock(&MapCriticalSection);
	for (auto It = ActionContexts.CreateIterator(); It; ++It)
	{
		if (It->Value->bAddReferencedObjectsAllowed && It->Value->IsValid())
		{
			It->Value->AddReferencedObjects(Collector);
		}
	}
}

void FEnhancedAsyncActionManager::FObjectListener::NotifyUObjectDeleted(const UObjectBase* Object, int32 Index)
{
	FEnhancedAsyncActionManager::Get().OnObjectDeleted(static_cast<const UObject*>(Object));
}

void FEnhancedAsyncActionManager::FObjectListener::OnUObjectArrayShutdown()
{
	FEnhancedAsyncActionManager::Get().OnShutdown();
}

void FEnhancedAsyncActionManager::EnableListener()
{
	GUObjectArray.AddUObjectDeleteListener(&ObjectListener);
}

void FEnhancedAsyncActionManager::DisableListener()
{
	GUObjectArray.RemoveUObjectDeleteListener(&ObjectListener);
}
