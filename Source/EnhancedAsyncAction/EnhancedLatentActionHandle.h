// Copyright 2025, Aquanox.

#pragma once

#include "UObject/Object.h"
#include "UObject/Package.h"
#include "LatentActions.h"
#include "Templates/TypeHash.h"
#include "EnhancedAsyncContextHandle.h"
#include "EnhancedAsyncContextShared.h"
#include "EnhancedLatentActionHandle.generated.h"

#define UE_API ENHANCEDASYNCACTION_API

class FEnhancedLatentActionDelegate;
struct FEnhancedLatentActionContextHandle;

/**
 * Internal structure to pass information about latent action and build unique context identifier
 */
struct FLatentCallInfo
{
	using FDelegate = TDelegate<void(const FEnhancedLatentActionContextHandle&)>;

	// Owning object
	TWeakObjectPtr<const UObject> OwningObject;
	// Stable latent function identifier
	int32 UUID = 0;
	// Unique random identifier to track each call
	int32 CallID = 0;
	// optional action handle
	FDelegate Delegate;

	inline bool IsValid() const
	{
		return OwningObject.IsValid() && UUID != 0 && CallID != 0;
	}

	FString GetDebugString() const
	{
		return FString::Printf(TEXT("Owner=%s UUID=%d CallID=%x Delegate=%d"), *GetNameSafe(OwningObject.Get()), UUID, CallID, (int32)Delegate.IsBound());
	}
};

/**
 * Latent call context handle that is passed around
 */
USTRUCT(BlueprintType)
struct UE_API FEnhancedLatentActionContextHandle : public FAsyncContextHandleBase
{
	GENERATED_BODY()

public:
	FEnhancedLatentActionContextHandle();
	FEnhancedLatentActionContextHandle(FLatentCallInfo CallInfo);
	FEnhancedLatentActionContextHandle(FAsyncContextId ContextId, FLatentCallInfo CallInfo, TSharedRef<FEnhancedAsyncActionContext> Data);

	/**
	 * Is this handle considered valid?
	 *
	 * Tests for valid call identifier and alive owning object.
	 */
	bool IsValid() const;

	FString GetDebugString() const
	{
		return FString::Printf(TEXT("Id=%x %s"), (uint32)GetId(), *CallInfo.GetDebugString());
	}

	/** Shortcut to resolve context from manager */
	TSharedPtr<FEnhancedAsyncActionContext> GetContext() const;
	/** Shortcut to resolve context from manager */
	TSharedRef<FEnhancedAsyncActionContext> GetContextSafe() const;

	/*
	 * Free capture context. Usually to be called from wrappers
	 */
	void ReleaseContext() const;
	/*
	 * Free capture context and invalidate handle used by this action.
	 */
	void ReleaseContextAndInvalidate();

protected:
	friend class FEnhancedAsyncContextManager;

	template <typename>
	friend class TEnhancedLatentAction;
	template <typename>
	friend class TEnhancedRepeatableLatentAction;

	// Latent function call unique identifier that can be obtained from outside
	FLatentCallInfo CallInfo;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FEnhancedLatentActionDelegate, const FEnhancedLatentActionContextHandle&, Handle);

template <>
struct TStructOpsTypeTraits<FEnhancedLatentActionContextHandle>
	: public TStructOpsTypeTraitsBase2<FEnhancedLatentActionContextHandle>
{
	enum
	{
		// WithCopy = true,
	};
};

/**
 * A decorator to handle unique latent actions, when each call checks "Does the manager have another task with ID".
 *
 * Decorator saves current context and restores it when triggered.
 *
 * @tparam TLatentBase Base class of latent action
 */
template <typename TLatentBase = FPendingLatentAction>
class TEnhancedLatentAction : public TLatentBase
{
	using Super = TLatentBase;
	using ThisClass = TEnhancedLatentAction;

public:
	template <typename... TArgs>
	TEnhancedLatentAction(const FEnhancedLatentActionContextHandle& Handle, TArgs&&... Args)
		: Super(Forward<TArgs>(Args)...), SavedContextHandle(Handle), ContextHandleRef(const_cast<FEnhancedLatentActionContextHandle&>(Handle))
	{
		ContextHandleRef = SavedContextHandle;
	}

	virtual ~TEnhancedLatentAction() = default;

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		UE_LOG(LogEnhancedAction, Verbose, TEXT("%p UPDATE %s"), this, *SavedContextHandle.GetDebugString());

		const int32 InitialCount = CountResponses(Response);

		// Call parent
		Super::UpdateOperation(Response);

		const int32 UpdatedResponse = CountResponses(Response);

		// action was triggered for execution, select context
		if (InitialCount != UpdatedResponse)
		{
			NotifyActionTriggered();
		}
	}

	virtual void NotifyActionTriggered()
	{
		UE_LOG(LogEnhancedAction, Verbose, TEXT("%p SELECT %s"), this, *SavedContextHandle.GetDebugString());

		// Switch to context
		ContextHandleRef = SavedContextHandle;
	}

	virtual void NotifyObjectDestroyed()
	{
		UE_LOG(LogEnhancedAction, Verbose, TEXT("%p DESTROY %s"), this, *SavedContextHandle.GetDebugString());

		SavedContextHandle.ReleaseContextAndInvalidate();
		Super::NotifyObjectDestroyed();
	}

	virtual void NotifyActionAborted()
	{
		UE_LOG(LogEnhancedAction, Verbose, TEXT("%p ABORT %s"), this, *SavedContextHandle.GetDebugString());

		SavedContextHandle.ReleaseContextAndInvalidate();
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

private:
	FORCEINLINE static int32 CountResponses(struct FLatentResponse& Response)
	{
		struct FriendlyResponse : public FLatentResponse
		{
			friend class TEnhancedLatentAction;
		};
		return static_cast<FriendlyResponse&>(Response).LinksToExecute.Num();
	}

protected:
	// The capture context handle
	FEnhancedLatentActionContextHandle SavedContextHandle;
	// The reference to context handle in graph for updates
	FEnhancedLatentActionContextHandle& ContextHandleRef;
};

/**
 * A decorator to handle `repeatable latent actions` that schedule new action every time function is invoked.
 *
 * If multiple repeatable actions triggered by normal means - only latest context will be picked after NotifyActionTriggered,
 * to avoid that a different node generation has to be used.
 *
 * @see UK2Node_LoadAsset
 * @see UK2Node_LoadAssetClass
 *
 * @tparam TLatentBase Latent action base class
 */
template <typename TLatentBase = FPendingLatentAction>
class TEnhancedRepeatableLatentAction : public TEnhancedLatentAction<TLatentBase>
{
	using Super = TEnhancedLatentAction<TLatentBase>;
	using ThisClass = TEnhancedRepeatableLatentAction;

public:
	template <typename... TArgs>
	TEnhancedRepeatableLatentAction(const FEnhancedLatentActionContextHandle& Handle, TArgs&&... Args)
		: Super(Handle, Forward<TArgs>(Args)...)
	{
		Callback = Handle.CallInfo.Delegate;
		ensureAlways(Callback.IsBound());
	}

	virtual ~TEnhancedRepeatableLatentAction() = default;

	virtual void NotifyActionTriggered() override
	{
		// There is no need to "Switch" to context as it is handled differently by node
		// Each trigger will invoke a delegate instead.
		// The "Then" pin of original latent task not connected to anything.
		Callback.ExecuteIfBound(this->SavedContextHandle);
	}

	static UFunction* GetDelegateSignature()
	{
		UFunction* Function = FindObject<UFunction>(
			FindPackage(nullptr, TEXT("/Script/EnhancedAsyncAction")), TEXT("EnhancedLatentActionDelegate__DelegateSignature")
		);
		check(Function != nullptr);
		return Function;
	}

protected:
	FLatentCallInfo::FDelegate Callback;
};

template <>
inline FAsyncContextId FAsyncContextId::Make(const FLatentCallInfo& InResult)
{
	const uint32 Hash = ::PointerHash(InResult.OwningObject.Get(), ::HashCombine(InResult.UUID, InResult.CallID));
#if WITH_CONTEXT_TYPE_TAG
	return FAsyncContextId(Hash, FAsyncContextId::EContextType::CT_LatentAction);
#else
	return FAsyncContextId(Hash);
#endif
}

#undef UE_API
