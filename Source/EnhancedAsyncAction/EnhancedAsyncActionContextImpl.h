// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"
#include "EnhancedAsyncActionShared.h"
#include "EnhancedAsyncActionContext.h"

/**
 * This is a stub with no implementation
 */
class FEnhancedAsyncActionContextStub : public FEnhancedAsyncActionContext
{
	using Super = FEnhancedAsyncActionContext;
public:
	virtual FString GetDebugName() const { return TEXT("FEnhancedAsyncActionContextStub"); }
	virtual bool IsValid() const override { return true; }
	
#define CONTEXT_PROPERTY_ACCESSOR_MODE override { }
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Bool, bool)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Byte, uint8)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Int32, int32)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Int64, int64)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Float, float)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Double, double)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(String, FString)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Name, FName)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Text, FText)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Object, UObject*)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Class, UClass*)
	CONTEXT_DECLARE_ENUM_ACCESSOR(Enum)
	CONTEXT_DECLARE_STRUCT_ACCESSOR(Struct)
	CONTEXT_DECLARE_CONTAINER_ACCESSOR(Array)
	CONTEXT_DECLARE_CONTAINER_ACCESSOR(Set)
#undef CONTEXT_PROPERTY_ACCESSOR_MODE
	
};

/**
 * Context container backed by FInstancedPropertyBag within Action object.
 *
 * Allows fixed amount of captures
 */
class FEnhancedAsyncActionContext_PropertyBagBase : public FEnhancedAsyncActionContext
{
	using Super = FEnhancedAsyncActionContext;
public:
	virtual bool IsValid() const override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	virtual FString GetDebugName() const { return TEXT("FEnhancedAsyncActionContext_PropertyBagBase"); }
	virtual void DebugDump(FStringBuilderBase& Builder) const override;

	virtual void SetupFromStringDefinition(const FString& InDefinition) override;
	virtual bool CanAddNewProperty(const FName& Name, EPropertyBagPropertyType Type) const;

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
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Object, UObject*)
	CONTEXT_DECLARE_SIMPLE_ACCESSOR(Class, UClass*)
	CONTEXT_DECLARE_ENUM_ACCESSOR(Enum)
	CONTEXT_DECLARE_STRUCT_ACCESSOR(Struct)
	CONTEXT_DECLARE_CONTAINER_ACCESSOR(Array)
	CONTEXT_DECLARE_CONTAINER_ACCESSOR(Set)
#undef CONTEXT_PROPERTY_ACCESSOR_MODE
	
protected:
	inline class FFrieldlyInstancedPropertyBag* GetValueRef() const;
	
	FInstancedPropertyBag* ValueRef = nullptr;
	bool ValueConfigured			= false;
};

/**
 * Context container backed by FInstancedPropertyBag.
 *
 * Allows any amount of captures
 */
class FEnhancedAsyncActionContext_PropertyBagRef : public FEnhancedAsyncActionContext_PropertyBagBase
{
	using Super = FEnhancedAsyncActionContext_PropertyBagBase;
public:		
	FEnhancedAsyncActionContext_PropertyBagRef(const UObject* OwningObject, FName PropertyName) : OwnerRef(OwningObject)
	{
		ValueRef = EAA::Internals::GetMemberChecked<FInstancedPropertyBag>(OwningObject, PropertyName, FInstancedPropertyBag::StaticStruct());
	}

	virtual bool IsValid() const override { return OwnerRef.IsValid() && Super::IsValid(); }
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override { /* not needed, node is already reflected */ }
protected:
	TWeakObjectPtr<const UObject> OwnerRef;
};

/**
 * Context container backed by FInstancedPropertyBag.
 *
 * Allows any amount of captures
 */
class FEnhancedAsyncActionContext_PropertyBag : public FEnhancedAsyncActionContext_PropertyBagBase
{
	using Super = FEnhancedAsyncActionContext_PropertyBagBase;
public:		
	FEnhancedAsyncActionContext_PropertyBag()
	{
		ValueRef = &Value;
	}
protected:
	FInstancedPropertyBag Value;
};

#if 0

/**
 * Context container backed by struct
 *
 * Allows fixed amount of captures
 */
class FEnhancedAsyncActionContext_StructBase : public FEnhancedAsyncActionContext
{
	using Super = FEnhancedAsyncActionContext;
public:
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override {}

	virtual void SetupFromStringDefinition(const FString& InDefinition) override {}

	virtual bool IsValid() const override { return ValueRef.IsValid(); }

	virtual void SetValueString(int32 Index, const FString& InValue) override {}
	virtual void SetValueInt(int32 Index, const int32& InValue) override {}
	virtual void SetValueBool(int32 Index, const bool& InValue) override {}
	virtual void SetValueObject(int32 Index, UObject* InValue) override {}

	virtual void GetValueString(int32 Index, FString& OutValue) override {}
	virtual void GetValueInt(int32 Index, int32& OutValue) override {}
	virtual void GetValueBool(int32 Index, bool& OutValue) override {}
	virtual void GetValueObject(int32 Index, UObject*& OutValue) override {}
protected:
	FStructView ValueRef;
};

/**
 * Context container backed by struct within Action object.
 *
 * Allows fixed amount of captures
 */
class FEnhancedAsyncActionContext_StructRef : public FEnhancedAsyncActionContext_StructBase
{
	using Super = FEnhancedAsyncActionContext_StructBase;
public:
	FEnhancedAsyncActionContext_StructRef(const UObject* OwningObject, FName PropertyName, const UScriptStruct* ExpectedType)
		: OwnerRef(OwningObject)
	{
		ValueRef = FStructView(ExpectedType, EAA::Internals::GetMemberChecked<uint8>(OwningObject, PropertyName, ExpectedType));
	}

	virtual bool IsValid() const override { return OwnerRef.IsValid() && Super::IsValid(); }
protected:
	TWeakObjectPtr<const UObject> OwnerRef;
};

/**
 * Context container backed by FInstancedStruct.
 *
 * Allows fixed amount of captures
 */
class FEnhancedAsyncActionContext_Struct : public FEnhancedAsyncActionContext_StructBase
{
	using Super = FEnhancedAsyncActionContext_StructBase;
public:
	FEnhancedAsyncActionContext_Struct(UScriptStruct* ExpectedType)
	{
		Value.InitializeAs(ExpectedType);
		ValueRef = Value;
	}
private:
	FInstancedStruct Value;
};

#endif
