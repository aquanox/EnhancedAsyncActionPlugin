// Copyright 2025, Aquanox.

#pragma once

#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "Templates/SharedPointer.h"

#define UE_API ENHANCEDASYNCACTION_API

class FEnhancedAsyncContextManager;
struct FEnhancedAsyncActionContext;
struct FInstancedPropertyBag;

/**
 * Handling mode of context resolve errors
 */
enum class EResolveErrorMode
{
	AllowNull,
	Fallback,
	Assert,

	Default = Fallback
};

/**
 * Unique identifier to context for async and latent actions
 */
struct UE_API FAsyncContextId
{
	friend FEnhancedAsyncContextManager;
private:
	// const void* Object = nullptr;
	uint32 Value = INDEX_NONE;
public:
	static const FAsyncContextId Invalid;

	FAsyncContextId() = default;
	explicit FAsyncContextId(uint32 Value) : Value(Value) {}

	bool operator<(const FAsyncContextId& Other) const { return Value < Other.Value; }
	bool operator==(const FAsyncContextId& Other) const { return Value == Other.Value; }
	bool operator!=(const FAsyncContextId& Other) const { return Value != Other.Value; }

	operator bool() const { return Value != INDEX_NONE; }

	friend uint32 GetTypeHash(const FAsyncContextId& Id)
	{
		return Id.Value;
	}

	/**
	 * Make identifier matching given parameters
	 */
	template<typename T>
	static FAsyncContextId Make(T const&) = delete;
};

/**
 * Base information needed expose context for blueprints
 */
struct UE_API FAsyncContextHandleBase
{
private:
	friend FEnhancedAsyncContextManager;
public:
	explicit FAsyncContextHandleBase(const FAsyncContextId& Id);

	FAsyncContextHandleBase(const FAsyncContextId& Id, TWeakObjectPtr<const UObject> Owner, TSharedRef<FEnhancedAsyncActionContext> Data);

	/** Get identifier for this context handle */
	const FAsyncContextId& GetId() const;
	/** Is handle valid (has valid owning object and data) */
	bool IsValid() const;

protected:
	FAsyncContextId ContextId;
	TWeakObjectPtr<const UObject> Owner;
	TWeakPtr<FEnhancedAsyncActionContext> Data;
};

#undef UE_API
