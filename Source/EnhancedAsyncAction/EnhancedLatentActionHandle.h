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
    enum class ETriggerMode
    {
	    /**
	     * The latent action will continue from "Then" pin.
	     *
	     * UpdateOperation -> Write context variable -> Execute
	     */
	    FromThenPin,
	    /**
	     * The latent action will continue by calling continuation delegate.
	     *
	     * UpdateOperation -> Call delegate -> Read context from delegate
	     */
	    FromEventPin,

    	/**
    	 * What mode to select when no metadata present
    	 */
    	Default = FromThenPin
	};

	using FDelegate = TDelegate<void(const FEnhancedLatentActionContextHandle&)>;

	// Active trigger mode.
	ETriggerMode TriggerMode;
	// Owning object
	TWeakObjectPtr<const UObject> OwningObject;
	// Stable latent function identifier
	int32 UUID = 0;
	// Unique random identifier to track each call
	int32 CallID = 0;
	// optional action trigger delegate
	// to avoid circular dependency it does not use dynamic delegate.
	FDelegate Delegate;

	inline bool IsValid() const
	{
		return OwningObject.IsValid() && UUID != 0 && CallID != 0;
	}

	FString GetDebugString() const
	{
		return FString::Printf(TEXT("Owner=%s UUID=%d CallID=%x Delegate=%d"), *GetNameSafe(OwningObject.Get()), UUID, CallID, (int32)Delegate.IsBound());
	}

    /**
     * Construct new call info object
     * @param Owner
     * @param UUID
     * @param CallUUID
     * @param Delegate
     * @return
     */
    template<typename TDelegate = class FEnhancedLatentActionDelegate>
	static FLatentCallInfo Make(const UObject* Owner, int32 UUID, int32 CallUUID, TDelegate Delegate = TDelegate())
	{
		FLatentCallInfo Info;
		Info.OwningObject = Owner;
		Info.UUID = UUID;
		Info.CallID = CallUUID;
		if (Delegate.IsBound())
		{
			Info.TriggerMode = ETriggerMode::FromEventPin;
			Info.Delegate.BindUFunction(Delegate.GetUObject(), Delegate.GetFunctionName());
		}
		else
		{
			Info.TriggerMode = ETriggerMode::FromThenPin;
		}
		return Info;
	}

	/**
	 * Returns signature object of trigger delegate.
	 */
	static UFunction* GetDelegateSignature()
	{
		UFunction* Function = FindObject<UFunction>(
			FindPackage(nullptr, TEXT("/Script/EnhancedAsyncAction")), TEXT("EnhancedLatentActionDelegate__DelegateSignature")
		);
		check(Function != nullptr);
		return Function;
	}
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

public:

	// Latent function call unique identifier that can be obtained from outside
	FLatentCallInfo CallInfo;
};

/**
 * Continuation trigger delegate that provides context to target
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FEnhancedLatentActionDelegate, const FEnhancedLatentActionContextHandle&, Handle);

template <>
struct TStructOpsTypeTraits<FEnhancedLatentActionContextHandle>
	: public TStructOpsTypeTraitsBase2<FEnhancedLatentActionContextHandle>
{
	enum
	{
		WithCopy = true,
	};
};

/**
 * A decorator to handle latent actions with context
 *
 * @tparam TLatentBase Base class of latent action
 * @tparam TriggerMode The continuation trigger mode used for this action
 */
template <typename TLatentBase, FLatentCallInfo::ETriggerMode TriggerMode>
class TEnhancedLatentActionBase : public TLatentBase
{
	using Super = TLatentBase;
public:

	template <typename... TArgs>
	TEnhancedLatentActionBase(const FEnhancedLatentActionContextHandle& Handle, TArgs&&... Args)
		: Super(Forward<TArgs>(Args)...)
		, SavedContextHandle(Handle)
		, ContextHandleRef(const_cast<FEnhancedLatentActionContextHandle&>(Handle))
	{
		check(SavedContextHandle.CallInfo.TriggerMode == TriggerMode);
		// here may be some modifications in future so handle so kept the assign
		if (TriggerMode == FLatentCallInfo::ETriggerMode::FromThenPin)
		{
			ContextHandleRef = SavedContextHandle;
		}
	}

	virtual ~TEnhancedLatentActionBase()
	{
		// Context will be released by DestroyContext call that is integrated to graph

		// In case latent action was aborted or owning object is gone
		// context will be freed by NotifyActionAborted or NotifyObjectDestroyed
	}

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

		if (TriggerMode == FLatentCallInfo::ETriggerMode::FromThenPin)
		{
			// Switch to context, when LatentActionManager finishes checking all actions for the current object
			// it will trigger execution pin
			ContextHandleRef = SavedContextHandle;
			return;
		}

		if (TriggerMode == FLatentCallInfo::ETriggerMode::FromEventPin)
		{
			const FLatentCallInfo::FDelegate& Callback = SavedContextHandle.CallInfo.Delegate;
			check(Callback.IsBound());
			// There is no need to touch ContextHandleRef as it may be garbage if reference to function local variable was captured
			// Delegate is only what important that was acquired initially

			// Directly trigger the continuation.
			// The "Then" pin of original latent task not connected to anything.
			Callback.ExecuteIfBound(SavedContextHandle);
			return;
		}
	}

	virtual void NotifyObjectDestroyed()
	{
		UE_LOG(LogEnhancedAction, Verbose, TEXT("%p DESTROY %s"), this, *SavedContextHandle.GetDebugString());

		ReleaseContextAndInvalidate();
		Super::NotifyObjectDestroyed();
	}

	virtual void NotifyActionAborted()
	{
		UE_LOG(LogEnhancedAction, Verbose, TEXT("%p ABORT %s"), this, *SavedContextHandle.GetDebugString());

		ReleaseContextAndInvalidate();
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

	// Helper to reset context
	void ReleaseContextAndInvalidate()
	{
		if (SavedContextHandle.IsValid())
		{
			// will trigger DestroyContext by id and force invalidate the handle stored in action
			// repeated calls wont pass valid check
			const_cast<FEnhancedLatentActionContextHandle&>(SavedContextHandle).ReleaseContextAndInvalidate();
		}
	}

private:
	FORCEINLINE static int32 CountResponses(struct FLatentResponse& Response)
	{
		struct FriendlyResponse : public FLatentResponse
		{
			friend TEnhancedLatentActionBase;
		};
		return static_cast<FriendlyResponse&>(Response).LinksToExecute.Num();
	}

protected:
	// The capture context handle
	const FEnhancedLatentActionContextHandle SavedContextHandle;
	// The reference to context handle in graph for updates
	FEnhancedLatentActionContextHandle& ContextHandleRef;
};

/**
 * A decorator to handle unique latent actions, when each call checks "Does the manager have another task with ID".
 *
 * Decorator saves current context and restores it when triggered.
 *
 * @tparam TLatentBase Base class of latent action
 */
template <typename TLatentBase>
class TEnhancedLatentAction : public TEnhancedLatentActionBase<TLatentBase, FLatentCallInfo::ETriggerMode::FromThenPin>
{
	using Super = TEnhancedLatentActionBase<TLatentBase, FLatentCallInfo::ETriggerMode::FromThenPin>;
public:
	template <typename... TArgs>
	TEnhancedLatentAction(const FEnhancedLatentActionContextHandle& Handle, TArgs&&... Args)
		: Super(Handle, Forward<TArgs>(Args)...)
	{
		checkf(!Handle.CallInfo.Delegate.IsBound(), TEXT("TEnhancedLatentAction requires metadata LatentTrigger=Then on function or does not specify it"));
	}
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
template <typename TLatentBase>
class TEnhancedRepeatableLatentAction : public TEnhancedLatentActionBase<TLatentBase, FLatentCallInfo::ETriggerMode::FromEventPin>
{
	using Super = TEnhancedLatentActionBase<TLatentBase, FLatentCallInfo::ETriggerMode::FromEventPin>;
public:
	template <typename... TArgs>
	TEnhancedRepeatableLatentAction(const FEnhancedLatentActionContextHandle& Handle, TArgs&&... Args)
		: Super(Handle, Forward<TArgs>(Args)...)
	{
		checkf(Handle.CallInfo.Delegate.IsBound(), TEXT("TEnhancedRepeatableLatentAction requires metadata LatentTrigger=Event on function"));
	}
};

#undef UE_API
