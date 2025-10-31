#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "UObject/UObjectArray.h"
#include "Misc/TransactionallySafeCriticalSection.h"
#include "UObject/ObjectKey.h"

struct FEnhancedAsyncActionContext;
struct FEnhancedAsyncActionContextHandle;

/**
 * Manager class for async action context data.
 *
 * For each instance of proxy class it can store one instance of context data
 */
class FEnhancedAsyncActionManager : public FGCObject
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
	
	/**
	 * Set context for the given action
	 * @param Action owning action object
	 * @param Context context object
	 * @return registered handle for public use
	 */
	FEnhancedAsyncActionContextHandle SetContext(const UObject* Action, TSharedRef<FEnhancedAsyncActionContext> Context);
	
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
	virtual FString GetReferencerName() const override { return TEXT("FEnhancedAsyncActionManager"); }
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	void EnableListener();
	void DisableListener();

	void OnObjectDeleted(const UObject* Action);
	void OnEngineShutdown();
	void OnShutdown();

	struct FObjectListener : public FUObjectArray::FUObjectDeleteListener
	{
		virtual void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index) override;
		virtual void OnUObjectArrayShutdown();
		virtual SIZE_T GetAllocatedSize() const override { return 0; }
	};

	friend struct FObjectListener;

	FObjectListener ObjectListener;

	TSharedRef<FEnhancedAsyncActionContext> DummyContext;

	FTransactionallySafeCriticalSection MapCriticalSection;

	TMap<TObjectKey<const UObject>, TSharedPtr<FEnhancedAsyncActionContext>> ActionContexts;

	int32 InternalIndexCounter = 0;
};
