// Copyright 2025, Aquanox.

#pragma once

#include "UObject/GCObject.h"
#include "UObject/UObjectArray.h"
#include "HAL/CriticalSection.h"
#include "Templates/ValueOrError.h"
#include "EnhancedAsyncContextHandle.h"

#define UE_API ENHANCEDASYNCACTION_API

struct FEnhancedAsyncActionContextHandle;
struct FEnhancedLatentActionContextHandle;

/**
 * Manager class for async action context data.
 *
 * For each instance of proxy class it can store one instance of context data
 */
class UE_API FEnhancedAsyncContextManager
{
public:
	/**
	 * Returns context manager instance singleton
	 */
	static FEnhancedAsyncContextManager& Get();

	/**
	 * Set context for the given action
	 *
	 * @return registered handle for public use
	 */
	TValueOrError<FEnhancedAsyncActionContextHandle, FString> CreateContext(const UObject* Action, FName InnerProperty);

	/**
	 * Set context for the given action
	 *
	 * @return registered handle for public use
	 */
	TValueOrError<FEnhancedLatentActionContextHandle, FString> CreateContext(const struct FLatentCallResult& CallResult);

	/**
	 * Destroy context used by latent action
	 */
	void DestroyContext(const struct FLatentCallResult& CallResult);

	/**
	 * Find context instance for given identifier
	 *
	 * @param ContextId context identifier
	 * @return bound context object
	 */
	TSharedPtr<FEnhancedAsyncActionContext> FindContext(FAsyncContextId ContextId, bool bAllowNull = true);

	/**
	 * Find public handle for given action object
	 *
	 * @param Action owning action object
	 * @return registered handle for public use
	 */
	FEnhancedAsyncActionContextHandle FindContextHandle(const UObject* Action);

	/**
	 * Find public handle for given latent action
	 *
	 * @param CallResult latent call identifier
	 * @return registered handle for public use
	 */
	FEnhancedLatentActionContextHandle FindContextHandle(const FLatentCallResult& CallResult);

	/**
	 * Resolve context instance from given action handle
	 *
	 * @param Handle  requestor handle
	 * @return bound context object
	 */
	TSharedPtr<FEnhancedAsyncActionContext> ResolveContextHandle(const FEnhancedAsyncActionContextHandle& Handle, EResolveErrorMode OnError = EResolveErrorMode::Default);

	/**
	 * Resolve context instance from given latent handle
	 *
	 * @param Handle  requestor handle
	 * @return bound context object
	 */
	TSharedPtr<FEnhancedAsyncActionContext> ResolveContextHandle(const FEnhancedLatentActionContextHandle& Handle, EResolveErrorMode OnError = EResolveErrorMode::Default);

private:

	FEnhancedAsyncContextManager();
	~FEnhancedAsyncContextManager();

	void SetContextInternal(FAsyncContextId ContextId, TSharedRef<FEnhancedAsyncActionContext> Context, const UObject* TrackedOwner);

	TSharedPtr<FEnhancedAsyncActionContext> FindContextInternal(const FAsyncContextId& ContextId);

	template<typename T>
	TSharedPtr<FEnhancedAsyncActionContext> ResolveContextHandleInternal(const T& Handle, EResolveErrorMode OnError);

private:
	void AddReferencedObjects(FReferenceCollector& Collector);

	void OnObjectDeleted(const UObject* Action);
	void OnShutdown();

	struct FGCCollector : public FGCObject
	{
		explicit FGCCollector(FEnhancedAsyncContextManager* Owner) : Owner(Owner) { }
		virtual FString GetReferencerName() const override { return TEXT("FEnhancedAsyncContextManager"); }
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	private:
		FEnhancedAsyncContextManager* const Owner;
	};
	friend struct FGCCollector;

	TSharedPtr<FGCCollector>  ObjectCollector;

	struct FObjectListener : public FUObjectArray::FUObjectDeleteListener
	{
		explicit FObjectListener(FEnhancedAsyncContextManager* Owner) : Owner(Owner) { }
		virtual void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index) override;
		virtual void OnUObjectArrayShutdown();
		virtual SIZE_T GetAllocatedSize() const override { return 0; }

		// enable or disable listener if there are active contexts.
		void UpdateState();

		void EnableListener();
		void DisableListener();
	private:
		FEnhancedAsyncContextManager* const Owner;
		bool bEnabled = false;
	};

	friend struct FObjectListener;

	FObjectListener ObjectListener { this };

	TSharedPtr<FEnhancedAsyncActionContext> DummyContext;

	FCriticalSection MapCriticalSection;

	// All known async contexts
	TMap<FAsyncContextId, TSharedPtr<FEnhancedAsyncActionContext>> ActionContexts;

	// Owning object to a context identifier (async action or latent)
	TMultiMap<const UObject*, FAsyncContextId> TrackedObjects;
};

#undef UE_API
