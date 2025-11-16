// Copyright 2025, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "UObject/UObjectArray.h"
#include "HAL/CriticalSection.h"
#include "Templates/ValueOrError.h"

struct FEnhancedAsyncActionContext;
struct FEnhancedAsyncActionContextHandle;

/**
 * Manager class for async action context data.
 *
 * For each instance of proxy class it can store one instance of context data
 */
class ENHANCEDASYNCACTION_API FEnhancedAsyncActionManager
{
public:
	/**
	 * Returns context manager instance singleton
	 */
	static FEnhancedAsyncActionManager& Get();

	/**
	 *
	 */
	FEnhancedAsyncActionManager();

	/**
	 *
	 */
	virtual ~FEnhancedAsyncActionManager();

	struct FCreateContextParams
	{
		const UObject* Action = nullptr;
		FName InnerProperty;
		bool bTest = false;
		// bool bAllowReuse; ?
		// bool bRecreate; ?
	};

	/**
	 * Set context for the given action
	 * @param Action owning action object
	 * @param Params context creation parameters
	 *
	 * @return registered handle for public use
	 */
	TValueOrError<FEnhancedAsyncActionContextHandle, FString> CreateContext(const FCreateContextParams& Params);

	/**
	 * Find public handle for given action object
	 * @param Action owning action object
	 * @return registered handle for public use
	 */
	FEnhancedAsyncActionContextHandle FindContextHandle(const UObject* Action);

	/**
	 * Find context object for given action object
	 * @param Action owning action object
	 * @return bound context object
	 */
	TSharedPtr<FEnhancedAsyncActionContext> FindContext(const UObject* Action);

	/**
	 * Find context object for given action object
	 * @param Action owning action object
	 * @return bound context object
	 */
	TSharedRef<FEnhancedAsyncActionContext> FindContextSafe(const UObject* Action);

	/**
	 * Find context object for given action object
	 * @param Handle  requestor handle
	 * @return bound context object
	 */
	TSharedPtr<FEnhancedAsyncActionContext> FindContext(const FEnhancedAsyncActionContextHandle& Handle);

	/**
	 * Find context object for given action object
	 * @param Handle  requestor handle
	 * @return bound context object, never null
	 */
	TSharedRef<FEnhancedAsyncActionContext> FindContextSafe(const FEnhancedAsyncActionContextHandle& Handle);

protected:
	/**
	 * Set context for the given action
	 *
	 * @param Action owning action object
	 * @param Context context object
	 * @return registered handle for public use
	 */
	FEnhancedAsyncActionContextHandle SetContext(const UObject* Action, TSharedRef<FEnhancedAsyncActionContext> Context);

protected:
	void AddReferencedObjects(FReferenceCollector& Collector);

	void EnableListener();
	void DisableListener();

	void OnObjectDeleted(const UObject* Action);
	void OnShutdown();

	struct FGCCollector : public FGCObject
	{
		explicit FGCCollector(FEnhancedAsyncActionManager* Owner) : Owner(Owner) { }
		virtual FString GetReferencerName() const override { return TEXT("FEnhancedAsyncActionManager"); }
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	private:
		FEnhancedAsyncActionManager* const Owner;
	};
	friend struct FGCCollector;

	TSharedPtr<FGCCollector>  ObjectCollector;

	struct FObjectListener : public FUObjectArray::FUObjectDeleteListener
	{
		explicit FObjectListener(FEnhancedAsyncActionManager* Owner) : Owner(Owner) { }
		virtual void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index) override;
		virtual void OnUObjectArrayShutdown();
		virtual SIZE_T GetAllocatedSize() const override { return 0; }
	private:
		FEnhancedAsyncActionManager* const Owner;
	};

	friend struct FObjectListener;

	FObjectListener ObjectListener { this };

	TSharedRef<FEnhancedAsyncActionContext> DummyContext;

	FCriticalSection MapCriticalSection;

	TMap<const UObject*, TSharedPtr<FEnhancedAsyncActionContext>> ActionContexts;

	int32 InternalIndexCounter = 0;
};
