// Copyright 2025, Aquanox.

#pragma once

#include "UObject/Object.h"
#include "Engine/LatentActionManager.h"
#include "LatentActions.h"
#include "Templates/TypeHash.h"
#include "StructUtils/PropertyBag.h" // TBD: remove and force it to be included if used?
#include "EnhancedAsyncContextHandle.h"
#include "EnhancedLatentActionHandle.generated.h"

#define UE_API ENHANCEDASYNCACTION_API

template<typename TLatentBase>
class TEnhancedLatentActionWrapper : public FPendingLatentAction
{
	using Super = TLatentBase;
public:
	TEnhancedLatentActionWrapper(FPendingLatentAction* Inner) : Inner(Inner) {  }
	virtual ~TEnhancedLatentActionWrapper() = default;

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		Inner->UpdateOperation(Response);
	}

	virtual void NotifyObjectDestroyed() override
	{
		Context.Reset();
		Inner->NotifyObjectDestroyed();
	}

	virtual void NotifyActionAborted() override
	{
		Context.Reset();
		Inner->NotifyActionAborted();
	}

#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		FString Base = Inner->GetDescription();
		Base.Append(TEXT(" (with context)"));
		return Base;
	}
#endif

	FInstancedPropertyBag* GetContextContainer() const { return &Context; }
private:
	FPendingLatentAction* const Inner;
	mutable FInstancedPropertyBag Context;
};

template<typename TLatentBase = FPendingLatentAction>
class TEnhancedLatentActionExtension : public TLatentBase
{
	using Super = TLatentBase;
	using ThisClass = TEnhancedLatentActionExtension;
public:
	// use all parent type constructors
	using TLatentBase::TLatentBase;
	virtual ~TEnhancedLatentActionExtension() = default;

	virtual void NotifyObjectDestroyed()
	{
		Context.Reset();
		Super::NotifyObjectDestroyed();
	}

	virtual void NotifyActionAborted()
	{
		Context.Reset();
		Super::NotifyActionAborted();
	}

#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		FString Base = Super::GetDescription();
		Base.Append(TEXT(" (with context)"));
		return Base;
	}
#endif

	FInstancedPropertyBag* GetContextContainer() const { return &Context; }
private:
	mutable FInstancedPropertyBag Context;
};

/**
 * Internal structure to pass information about latent action to plugin
 */
USTRUCT(BlueprintType, meta=(DisableSplitPin))
struct UE_API FLatentCallResult
{
	GENERATED_BODY()
public:
	FLatentCallResult() = default;

	bool IsValid() const;

	template<typename TAction>
	static FLatentCallResult Make(const FLatentActionInfo& Info, TEnhancedLatentActionWrapper<TAction>* Action)
	{
		check(Action != nullptr);
		FLatentCallResult Handle;
		Handle.OwningObject = Info.CallbackTarget;
		Handle.UUID = Info.UUID;
		Handle.Action = Action;
		Handle.Container = Action->GetContextContainer();
		return Handle;
	}

	template<typename TAction>
	static FLatentCallResult Make(const FLatentActionInfo& Info, TEnhancedLatentActionExtension<TAction>* Action)
	{
		check(Action != nullptr);
		FLatentCallResult Handle;
		Handle.OwningObject = Info.CallbackTarget;
		Handle.UUID = Info.UUID;
		Handle.Action = Action;
		Handle.Container = Action->GetContextContainer();
		return Handle;
	}

	template<typename TAction>
	static FLatentCallResult Make(const FLatentActionInfo& Info, TAction* Action)
	{
		check(Action != nullptr);
		FLatentCallResult Handle;
		Handle.OwningObject = Info.CallbackTarget;
		Handle.UUID = Info.UUID;
		Handle.Action = Action;
		return Handle;
	}

	static FLatentCallResult MakeInvalid(const FLatentActionInfo& Info)
	{
		return FLatentCallResult();
	}

private:
	friend class FEnhancedAsyncContextManager;
	friend struct FAsyncContextId;

	TWeakObjectPtr<UObject> OwningObject;
	int32 UUID = 0;
	const void* Action = nullptr;
	FInstancedPropertyBag* Container = nullptr;
};

USTRUCT(BlueprintType, meta=(DisableSplitPin))
struct UE_API FEnhancedLatentActionContextHandle
#if CPP
	: public FAsyncContextHandleBase
#endif
{
	GENERATED_BODY()
private:
	friend class FEnhancedAsyncContextManager;
public:
	FEnhancedLatentActionContextHandle();
	FEnhancedLatentActionContextHandle(FAsyncContextId ContextId, TWeakObjectPtr<const UObject> Owner, TSharedRef<FEnhancedAsyncActionContext> Data);

	/** Shortcut to resolve context from manager */
	TSharedPtr<FEnhancedAsyncActionContext> GetContext() const;
	/** Shortcut to resolve context from manager */
	TSharedRef<FEnhancedAsyncActionContext> GetContextSafe() const;
};

template<>
struct TStructOpsTypeTraits<FEnhancedLatentActionContextHandle>
	: public TStructOpsTypeTraitsBase2<FEnhancedLatentActionContextHandle>
{
	enum
	{
		WithCopy = true,
	};
};

template<>
inline FAsyncContextId FAsyncContextId::Make(const FLatentCallResult& InResult)
{
	return FAsyncContextId( ::PointerHash(InResult.Action, InResult.UUID) );
}

#undef UE_API
