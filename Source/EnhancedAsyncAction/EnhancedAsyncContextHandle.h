// Copyright 2025, Aquanox.

#pragma once

#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "Templates/SharedPointer.h"
#include "EnhancedAsyncContextHandle.generated.h"

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
	Assert
};

/**
 * Unique identifier to context for async and latent actions
 */
struct UE_API FAsyncContextId
{
	friend FEnhancedAsyncContextManager;

	enum EContextType : uint32
	{
		CT_Unknown = 0,
		CT_AsyncAction = 1,
		CT_LatentAction = 2,

		CT_SHIFT = 2,
		CT_MASK = 1 << CT_SHIFT
	};
private:
	uint32 Value;
	// EContextType Type = EContextType::CT_Unknown;
public:
	static const FAsyncContextId Invalid;

	FAsyncContextId() : Value(INDEX_NONE)
	{
	}
	FAsyncContextId(uint32 InValue, EContextType InType) : Value(InValue)
	{
		// Value = (InValue << CT_SHIFT) | InType;
	}

	bool operator<(const FAsyncContextId& Other) const { return Value < Other.Value; }
	bool operator==(const FAsyncContextId& Other) const { return Value == Other.Value; }
	bool operator!=(const FAsyncContextId& Other) const { return !operator==(Other); }

	explicit operator bool() const { return Value != INDEX_NONE; }
	explicit operator uint32() const { return Value; }

	bool IsA(EContextType InType) const { return (Value & CT_MASK) == InType; }

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
USTRUCT(BlueprintType)
struct UE_API FAsyncContextHandleBase
{
	GENERATED_BODY()
private:
	friend FEnhancedAsyncContextManager;
public:
	FAsyncContextHandleBase() = default; // for bp use
	explicit FAsyncContextHandleBase(const FAsyncContextId& Id);
	explicit FAsyncContextHandleBase(const FAsyncContextHandleBase& Other);
	FAsyncContextHandleBase(const FAsyncContextId& Id, TWeakObjectPtr<const UObject> Owner, TSharedRef<FEnhancedAsyncActionContext> Data);

	/** Get identifier for this context handle */
	const FAsyncContextId& GetId() const;
	/** Is handle valid (has valid owning object and data) */
	bool IsValid() const;

	FString GetDebugString() const
	{
		return FString::Printf(TEXT("Id=%x Owner=%s"), (uint32)GetId(), *GetNameSafe(Owner.GetEvenIfUnreachable()));
	}
protected:
	FAsyncContextId ContextId;
	TWeakObjectPtr<const UObject> Owner;
	TWeakPtr<FEnhancedAsyncActionContext> Data;
};

template<>
struct TStructOpsTypeTraits<FAsyncContextHandleBase>
	: public TStructOpsTypeTraitsBase2<FAsyncContextHandleBase>
{
	enum
	{
		// WithCopy = true,
	};
};

#undef UE_API
