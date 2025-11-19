// Copyright 2025, Aquanox.

#pragma once

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "UObject/SoftObjectPtr.h"
#include "EnhancedAsyncContextTypes.h"

#define UE_API ENHANCEDASYNCACTION_API

#define CONTEXT_DECLARE_SIMPLE_ACCESSOR(Name, Type) \
	virtual void SetValue ##Name(int32 Index, Type const& InValue) CONTEXT_PROPERTY_ACCESSOR_MODE; \
	virtual void GetValue ##Name(int32 Index, Type& OutValue) CONTEXT_PROPERTY_ACCESSOR_MODE;

#define CONTEXT_DECLARE_ENUM_ACCESSOR(Name) \
	virtual void SetValue ##Name(int32 Index, UEnum* ExpectedType, uint8 InValue) CONTEXT_PROPERTY_ACCESSOR_MODE; \
	virtual void GetValue ##Name(int32 Index, UEnum* ExpectedType, uint8& OutValue) CONTEXT_PROPERTY_ACCESSOR_MODE;

#define CONTEXT_DECLARE_OBJECT_ACCESSOR(Name) \
	virtual void SetValue ##Name(int32 Index, UClass* ExpectedClass, UObject* const& InValue) CONTEXT_PROPERTY_ACCESSOR_MODE; \
	virtual void GetValue ##Name(int32 Index, UClass* ExpectedClass, UObject*& OutValue) CONTEXT_PROPERTY_ACCESSOR_MODE; \
	virtual void SetValueSoft##Name(int32 Index, UClass* ExpectedClass, TSoftObjectPtr<UObject> const& InValue) CONTEXT_PROPERTY_ACCESSOR_MODE; \
	virtual void GetValueSoft##Name(int32 Index, UClass* ExpectedClass, TSoftObjectPtr<UObject>& OutValue) CONTEXT_PROPERTY_ACCESSOR_MODE;

#define CONTEXT_DECLARE_CLASS_ACCESSOR(Name) \
	virtual void SetValue ##Name(int32 Index, UClass* ExpectedMetaClass, UClass* const& InValue) CONTEXT_PROPERTY_ACCESSOR_MODE; \
	virtual void GetValue ##Name(int32 Index, UClass* ExpectedMetaClass, UClass*& OutValue) CONTEXT_PROPERTY_ACCESSOR_MODE; \
	virtual void SetValueSoft ##Name(int32 Index, UClass* ExpectedMetaClass, TSoftClassPtr<UObject> const& InValue) CONTEXT_PROPERTY_ACCESSOR_MODE; \
	virtual void GetValueSoft ##Name(int32 Index, UClass* ExpectedMetaClass, TSoftClassPtr<UObject>& OutValue) CONTEXT_PROPERTY_ACCESSOR_MODE; \

#define CONTEXT_DECLARE_STRUCT_ACCESSOR(Name) \
	virtual void SetValue ##Name(int32 Index, UScriptStruct* ExpectedType, const uint8* InValue) CONTEXT_PROPERTY_ACCESSOR_MODE; \
	virtual void GetValue ##Name(int32 Index, UScriptStruct* ExpectedType, const uint8*& OutValue) CONTEXT_PROPERTY_ACCESSOR_MODE;

#define CONTEXT_DECLARE_CONTAINER_ACCESSOR(Name) \
	virtual void SetValue ##Name(int32 Index, EPropertyBagPropertyType Type, const UObject* TypeObject, const void* PropertyAddress) CONTEXT_PROPERTY_ACCESSOR_MODE; \
	virtual void GetValue ##Name(int32 Index, EPropertyBagPropertyType Type, const UObject* TypeObject, void* PropertyAddress) CONTEXT_PROPERTY_ACCESSOR_MODE;

#define CONTEXT_DECLARE_SET_ACCESSOR(Name) \
	virtual void SetValue ##Name(int32 Index, EPropertyBagPropertyType Type, const UObject* TypeObject, const void* PropertyAddress) CONTEXT_PROPERTY_ACCESSOR_MODE; \
	virtual void GetValue ##Name(int32 Index, EPropertyBagPropertyType Type, const UObject* TypeObject, void* PropertyAddress) CONTEXT_PROPERTY_ACCESSOR_MODE;

/**
 * Async action context data container that is held by manager
 */
struct UE_API FEnhancedAsyncActionContext
{
	FEnhancedAsyncActionContext() = default;
	virtual ~FEnhancedAsyncActionContext() = default;

	virtual void SetupFromStringDefinition(const FString& InDefinition) {}
	virtual void SetupFromProperties(TConstArrayView<TPair<FName, const FProperty*>> Properties) {}

	virtual const UObject* GetOwningObject() const { return nullptr; }
	virtual bool IsValid() const = 0;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) {}

	virtual FString GetDebugName() const { return TEXT("FEnhancedAsyncActionContext"); }
	virtual void DebugDump(FStringBuilderBase& Builder) const {}

#define CONTEXT_PROPERTY_ACCESSOR_MODE =0
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

	bool SetValueByIndex(int32 Index, const FProperty* Property, const void* Value, FString& Message);
	virtual bool SetValueByName(FName Name, const FProperty* Property, const void* Value, FString& Message) =0;

	bool GetValueByIndex(int32 Index, const FProperty* Property, void* OutValue, FString& Message);
	virtual bool GetValueByName(FName Name, const FProperty* Property, void* OutValue, FString& Message) =0;

	bool CanAddReferencedObjects() const { return bAddReferencedObjectsAllowed; }
	bool CanSetupContext() const { return bSetupContextAllowed; }
protected:
	bool bAddReferencedObjectsAllowed = true;
	bool bSetupContextAllowed = true;
};

#undef UE_API
