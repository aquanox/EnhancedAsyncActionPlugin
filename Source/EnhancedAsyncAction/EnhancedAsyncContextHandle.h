// Copyright 2025, Aquanox.

#pragma once

#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "Templates/SharedPointer.h"
#include "EnhancedAsyncContextHandle.generated.h"

#define UE_API ENHANCEDASYNCACTION_API

// Experiment with packed identifier - reserve two lowest bits for handle type
#define WITH_CONTEXT_TYPE_TAG 0

struct FEnhancedAsyncActionContext;

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
	static constexpr uint32 InvalidValue = static_cast<uint32>(INDEX_NONE);
public:
#if WITH_CONTEXT_TYPE_TAG
	enum EContextType : uint32
	{
		CT_Unknown = 0,
		CT_AsyncAction = 1,
		CT_LatentAction = 2,

		CT_SHIFT = 2,
		CT_MASK = 1 << CT_SHIFT
	};
#endif

#if WITH_CONTEXT_TYPE_TAG
	explicit FAsyncContextId(uint32 InValue = InvalidValue, EContextType InType = EContextType::CT_Unknown) : Value(InValue)
	{
		Value = (InValue << CT_SHIFT) | (uint32)InType;
	}
#else
	explicit FAsyncContextId(uint32 InValue = InvalidValue) : Value(InValue) {}
#endif

	bool operator<(const FAsyncContextId& Other) const { return Value < Other.Value; }
	bool operator==(const FAsyncContextId& Other) const { return Value == Other.Value; }
	bool operator!=(const FAsyncContextId& Other) const { return !operator==(Other); }

	explicit operator bool() const { return Value != InvalidValue; }
	explicit operator uint32() const { return Value; }

	bool IsValid() const { return Value != InvalidValue; }

#if WITH_CONTEXT_TYPE_TAG
	bool IsA(EContextType InType) const { return (Value & CT_MASK) == InType; }
#endif

	friend uint32 GetTypeHash(const FAsyncContextId& Id)
	{
		return Id.Value;
	}

	/**
	 * Make identifier matching given parameters
	 */
	template<typename T>
	static FAsyncContextId Make(T const&) = delete;

private:
	friend class FEnhancedAsyncContextManager;

	// The unique context identifier value
	uint32 Value;

#if WITH_CONTEXT_TYPE_TAG
	// The handle type
	EContextType Type = EContextType::CT_Unknown;
#endif
};

/**
 * Base information needed expose context for blueprints
 */
USTRUCT(BlueprintType)
struct UE_API FAsyncContextHandleBase
{
	GENERATED_BODY()
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
	friend class FEnhancedAsyncContextManager;

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
