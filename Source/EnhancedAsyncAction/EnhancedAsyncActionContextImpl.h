// Copyright 2025, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"
#include "EnhancedAsyncActionShared.h"
#include "EnhancedAsyncActionContext.h"

#define UE_API ENHANCEDASYNCACTION_API

/**
 * This is a stub with no implementation
 */
class UE_API FEnhancedAsyncActionContextStub : public FEnhancedAsyncActionContext
{
	using Super = FEnhancedAsyncActionContext;
public:
	virtual FString GetDebugName() const { return TEXT("FEnhancedAsyncActionContextStub"); }
	virtual bool IsValid() const override { return true; }

	static void HandleStubCall();
	
#define CONTEXT_PROPERTY_ACCESSOR_MODE override { HandleStubCall(); }
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Bool, bool)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Byte, uint8)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Int32, int32)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Int64, int64)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Float, float)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Double, double)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(String, FString)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Name, FName)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Text, FText)
	CONTEXT_DECLARE_ENUM_ACCESSOR(Enum)
	CONTEXT_DECLARE_STRUCT_ACCESSOR(Struct)
	CONTEXT_DECLARE_OBJECT_ACCESSOR(Object)
	CONTEXT_DECLARE_CLASS_ACCESSOR(Class)
	CONTEXT_DECLARE_CONTAINER_ACCESSOR(Array)
	CONTEXT_DECLARE_CONTAINER_ACCESSOR(Set)
#undef CONTEXT_PROPERTY_ACCESSOR_MODE

	virtual bool SetValueByName(FName Name, const FProperty* Property, const void* Value, FString& Message) override { HandleStubCall();return true; }
	virtual bool GetValueByName(FName Name, const FProperty* Property, void* OutValue, FString& Message) override { HandleStubCall();return true; }
};

/**
 * Context container backed by FInstancedPropertyBag within Action object.
 *
 * Allows fixed amount of captures
 */
class UE_API FEnhancedAsyncActionContext_PropertyBagBase : public FEnhancedAsyncActionContext
{
	using Super = FEnhancedAsyncActionContext;
public:
	virtual ~FEnhancedAsyncActionContext_PropertyBagBase();
	virtual bool IsValid() const override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	virtual FString GetDebugName() const { return TEXT("FEnhancedAsyncActionContext_PropertyBagBase"); }
	virtual void DebugDump(FStringBuilderBase& Builder) const override;

	virtual void SetupFromProperties(TConstArrayView<TPair<FName, const FProperty*>> Properties) override;
	virtual void SetupFromStringDefinition(const FString& InDefinition) override;
	bool CanAddNewProperty(const FName& Name, EPropertyBagPropertyType Type) const;

#define CONTEXT_PROPERTY_ACCESSOR_MODE override
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Bool, bool)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Byte, uint8)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Int32, int32)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Int64, int64)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Float, float)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Double, double)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(String, FString)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Name, FName)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Text, FText)
	CONTEXT_DECLARE_ENUM_ACCESSOR(Enum)
	CONTEXT_DECLARE_STRUCT_ACCESSOR(Struct)
	CONTEXT_DECLARE_OBJECT_ACCESSOR(Object)
	CONTEXT_DECLARE_CLASS_ACCESSOR(Class)
	CONTEXT_DECLARE_CONTAINER_ACCESSOR(Array)
	CONTEXT_DECLARE_CONTAINER_ACCESSOR(Set)
#undef CONTEXT_PROPERTY_ACCESSOR_MODE

	virtual bool SetValueByName(FName Name, const FProperty* Property, const void* Value, FString& Message) override;
	virtual bool GetValueByName(FName Name, const FProperty* Property, void* OutValue, FString& Message) override;
protected:
	inline class FFrieldlyInstancedPropertyBag* GetValueRef() const;

	FInstancedPropertyBag* ValueRef = nullptr;
	bool bPropertyBagStructureLocked			= false;
};

/**
 * Context container backed by FInstancedPropertyBag.
 *
 * Allows any amount of captures
 */
class UE_API FEnhancedAsyncActionContext_PropertyBagRef : public FEnhancedAsyncActionContext_PropertyBagBase
{
	using Super = FEnhancedAsyncActionContext_PropertyBagBase;
public:		
	FEnhancedAsyncActionContext_PropertyBagRef(const UObject* OwningObject, FName PropertyName);

	virtual FString GetDebugName() const override { return TEXT("FEnhancedAsyncActionContext_PropertyBagRef"); }
	virtual bool IsValid() const override;
protected:
	TWeakObjectPtr<const UObject> OwnerRef;
};

/**
 * Context container backed by FInstancedPropertyBag.
 *
 * Allows any amount of captures
 */
class UE_API FEnhancedAsyncActionContext_PropertyBag : public FEnhancedAsyncActionContext_PropertyBagBase
{
	using Super = FEnhancedAsyncActionContext_PropertyBagBase;
public:		
	FEnhancedAsyncActionContext_PropertyBag(const UObject* OwningObject);

	virtual FString GetDebugName() const override { return TEXT("FEnhancedAsyncActionContext_PropertyBag"); }
	virtual bool IsValid() const override;
protected:
	TWeakObjectPtr<const UObject> OwnerRef;
	FInstancedPropertyBag Value;
};

#undef UE_API
