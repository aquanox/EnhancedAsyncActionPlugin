// Fill out your copyright notice in the Description page of Project Settings.


#include "EnhancedAsyncActionContextImpl.h"
#include "EnhancedAsyncActionShared.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "Misc/DefinePrivateMemberPtr.h"


UE_DEFINE_PRIVATE_MEMBER_PTR(FPropertyBagPropertyDesc, GPropertyBagArrayRef_Desc, FPropertyBagArrayRef, ValueDesc);
UE_DEFINE_PRIVATE_MEMBER_PTR(FPropertyBagPropertyDesc, GPropertyBagSetRef_Desc, FPropertyBagSetRef, ValueDesc);

// typedef const void * (GetValueAddressFn)(const FPropertyBagPropertyDesc* Desc);
// UE_DEFINE_PRIVATE_MEMBER_PTR(GetValueAddressFn, GInstancedPropertyBag_GetValue, FInstancedPropertyBag, GetValueAddress);

// typedef void * (GetMutableValueAddressFn)(const FPropertyBagPropertyDesc* Desc);
// UE_DEFINE_PRIVATE_MEMBER_PTR(GetMutableValueAddressFn, GInstancedPropertyBag_GetMutableValue, FInstancedPropertyBag, GetMutableValueAddress);

class FFrieldlyInstancedPropertyBag : public FInstancedPropertyBag
{
	friend class FEnhancedAsyncActionContext_PropertyBagBase;
public:
	using FInstancedPropertyBag::GetValueAddress;
	using FInstancedPropertyBag::GetMutableValueAddress;

	const uint8* GetMemory() const { return Value.GetMemory(); }
	int32 GetStructSize() const { return Value.GetScriptStruct()->GetStructureSize(); }

	void DebugPurgeBag()
	{
		if (UObject* ScriptStruct = const_cast<UScriptStruct*>(Value.GetScriptStruct()))
		{
			const FString NewName = *(FString("TRASHED_") + ScriptStruct->GetName());
			ScriptStruct->Rename(*NewName);
			ScriptStruct->MarkAsGarbage();
		}
	}
	
	const FPropertyBagPropertyDesc* ValueDescriptorOf(const FPropertyBagArrayRef& InRef)
	{
		return & ( InRef.*GPropertyBagArrayRef_Desc );
	}
	const FPropertyBagPropertyDesc* ValueDescriptorOf(const FPropertyBagSetRef& InRef)
	{
		return & ( InRef.*GPropertyBagSetRef_Desc );
	}
};
static_assert(sizeof(FFrieldlyInstancedPropertyBag) == sizeof(FInstancedPropertyBag), "Must be no changes");

// ==============================================

#if !defined(EAA_DEBUG)
#define VALIDATE_RESULT(expr) expr
#else
#define VALIDATE_RESULT(expr) ::ValidateResultImpl( (expr), TEXT(#expr) )

FORCEINLINE void ValidateResultImpl(EPropertyBagResult Result, const TCHAR* Expression)
{
	ensureAlwaysMsgf(Result == EPropertyBagResult::Success, TEXT("Result failed on %s"), Expression);
}

template<typename T>
FORCEINLINE TValueOrError< T, EPropertyBagResult> ValidateResultImpl(TValueOrError< T, EPropertyBagResult>&& Result, const TCHAR* Expression)
{
	ensureAlwaysMsgf(!Result.HasError(), TEXT("Result failed on %s"), Expression);
	return Result;
}

template<typename T>
FORCEINLINE TValueOrError<T, EPropertyBagResult> ValidateResultImpl(TValueOrError<const T, EPropertyBagResult>&& Result, const TCHAR* Expression)
{
	ensureAlwaysMsgf(!Result.HasError(), TEXT("Result failed on %s"), Expression);
	return MakeValue( const_cast<T&>(Result.GetValue()) );
}

#endif

namespace EAA::Internals
{
	static inline void BytesToHexImpl(TConstArrayView<uint8> Bytes, FStringBuilderBase& OutHex)
	{
		const auto NibbleToHex = [](uint8 Value)  { return TCHAR(Value + (Value > 9 ? 'A' - 10 : '0')); };
		const uint8* Data = Bytes.GetData();

		int32 RowCounter = 16;
		for (const uint8* DataEnd = Data + Bytes.Num(); Data != DataEnd; ++Data)
		{
			OutHex.AppendChar(NibbleToHex(*Data >> 4));
			OutHex.AppendChar(NibbleToHex(*Data & 15));
			OutHex.AppendChar(TEXT(' '));
			if (!--RowCounter)
			{
				OutHex.AppendChar(TEXT('\n'));
				RowCounter = 16;
			}
		}
	}
}

// ==============================================

inline class FFrieldlyInstancedPropertyBag* FEnhancedAsyncActionContext_PropertyBagBase::GetValueRef() const
{
	return static_cast<class FFrieldlyInstancedPropertyBag*>(ValueRef);
}

void FEnhancedAsyncActionContext_PropertyBagBase::AddReferencedObjects(FReferenceCollector& Collector)
{
	GetValueRef()->AddStructReferencedObjects(Collector);
}

void FEnhancedAsyncActionContext_PropertyBagBase::DebugDump(FStringBuilderBase& Builder) const
{
	FConstStructView StructView = GetValueRef()->GetValue();

	{
		FStringBuilderBase HexData;
		EAA::Internals::BytesToHexImpl(TConstArrayView<uint8>(StructView.GetMemory(), StructView.GetScriptStruct()->GetStructureSize()), HexData);
		Builder.Appendf(TEXT("DATA base=%p:\n"), StructView.GetMemory()).Append(HexData).Append(TEXT("\n"));
	}
	
	for (TPropertyValueIterator<FProperty> It(StructView.GetScriptStruct(), StructView.GetMemory(), EPropertyValueIteratorFlags::NoRecursion); It; ++It)
	{
		const FProperty* Property = It->Key;
		if (It->Value)
		{
			Builder.Append("Property ").Append(Property->GetName());
			Builder.Append(" [").Append(Property->GetCPPType(nullptr, CPPF_None)).Append("]");
			Builder.Appendf(TEXT(" Offset=%d"), Property->GetOffset_ForInternal());
			Builder.Appendf(TEXT(" Value=%p"), It->Value);
			FString ExportResult;
			Property->ExportText_Direct(ExportResult, It->Value, It->Value, nullptr, PPF_Copy|PPF_DebugDump|PPF_Delimited);
			Builder.Append(" Value=[").Append(ExportResult).Append("]");
			Builder.Append(TEXT("\n"));
		}
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetupFromStringDefinition(const FString& InDefinition)
{
	UE_LOG(LogEnhancedAction, Log, TEXT("SetupFromStringData %s"), *InDefinition);

	if (InDefinition.IsEmpty())
		return;

	TArray<FString> Splits;
	InDefinition.ParseIntoArray(Splits, TEXT(";"));

	for (int32 PropIndex = 0, Max = Splits.Num(); PropIndex < Max; ++PropIndex)
	{
		const FName PropertyName = EAA::Internals::IndexToName(PropIndex);

		FPropertyTypeInfo TypeInfo;
		ensure(FPropertyTypeInfo::ParseTypeInfo(Splits[PropIndex], TypeInfo));
		if (TypeInfo.IsWildcard() || !TypeInfo.IsValid())
			continue;
		
		switch (TypeInfo.ContainerType)
		{
		case EPropertyBagContainerType::None:
			GetValueRef()->AddProperty(PropertyName, TypeInfo.ValueType, TypeInfo.ValueTypeObject);
			break;
		case EPropertyBagContainerType::Array:
		case EPropertyBagContainerType::Set:
			GetValueRef()->AddContainerProperty(PropertyName, TypeInfo.ContainerType, TypeInfo.ValueType, TypeInfo.ValueTypeObject);
			break;
		default:
			checkNoEntry();
			break;
		}
	}

	bPropertyBagStructureLocked = true;
}

bool FEnhancedAsyncActionContext_PropertyBagBase::CanAddNewProperty(const FName& Name, EPropertyBagPropertyType Type) const
{
	return !bPropertyBagStructureLocked || !GetValueRef()->FindPropertyDescByName(Name);
}

void FEnhancedAsyncActionContextStub::HandleStubCall()
{
#if (defined(EAA_DEBUG) && EAA_DEBUG == 1) //||  1
	ensureAlwaysMsgf(false, TEXT("Must not be called"));
#endif
}

FEnhancedAsyncActionContext_PropertyBagBase::~FEnhancedAsyncActionContext_PropertyBagBase()
{
	if (GetValueRef())
	{
		// GetValueRef()->DebugPurgeBag();
	}
}

bool FEnhancedAsyncActionContext_PropertyBagBase::IsValid() const
{
	return GetValueRef() != nullptr;
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueBool(int32 Index, const bool& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Bool))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::Bool);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueBool(Name, InValue));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueByte(int32 Index, const uint8& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Byte))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::Byte);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueByte(Name, InValue));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueInt32(int32 Index, const int32& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Int32))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::Int32);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueInt32(Name, InValue));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueInt64(int32 Index, const int64& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Int64))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::Int64);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueInt64(Name, InValue));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueFloat(int32 Index, const float& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Float))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::Float);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueFloat(Name, InValue));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueDouble(int32 Index, const double& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Double))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::Double);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueDouble(Name, InValue));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueString(int32 Index, const FString& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::String))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::String);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueString(Name, InValue));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueName(int32 Index, const FName& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Name))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::Name);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueName(Name, InValue));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueText(int32 Index, const FText& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Text))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::Text);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueText(Name, InValue));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueEnum(int32 Index, UEnum* ExpectedType, uint8 InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Enum))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::Enum, ExpectedType);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueEnum(Name, InValue, ExpectedType));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueStruct(int32 Index, UScriptStruct* ExpectedType, const uint8* InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Struct))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::Struct, ExpectedType);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueStruct(Name, FConstStructView(ExpectedType, InValue)));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueObject(int32 Index, UClass* ExpectedClass, UObject* const& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Object))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::Object, ExpectedClass);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueObject(Name, InValue));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueSoftObject(int32 Index, UClass* ExpectedClass, TSoftObjectPtr<UObject> const& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::SoftObject))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::SoftObject, ExpectedClass);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueSoftPath(Name, InValue.ToSoftObjectPath()));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueClass(int32 Index, UClass* ExpectedMetaClass, UClass* const& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Class))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::Class, ExpectedMetaClass);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueClass(Name, InValue));
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueSoftClass(int32 Index, UClass* ExpectedMetaClass, TSoftClassPtr<UObject> const& InValue)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::SoftClass))
	{
		GetValueRef()->AddProperty(Name, EPropertyBagPropertyType::SoftClass, ExpectedMetaClass);
	}
	VALIDATE_RESULT(GetValueRef()->SetValueSoftPath(Name, InValue.ToSoftObjectPath()));
}

// ===============================================
// 
// ===============================================

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueBool(int32 Index, bool& OutValue)
{
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueBool(EAA::Internals::IndexToName(Index)));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueByte(int32 Index, uint8& OutValue)
{
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueByte(EAA::Internals::IndexToName(Index)));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueInt32(int32 Index, int32& OutValue)
{
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueInt32(EAA::Internals::IndexToName(Index)));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueInt64(int32 Index, int64& OutValue)
{
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueInt64(EAA::Internals::IndexToName(Index)));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueFloat(int32 Index, float& OutValue)
{
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueFloat(EAA::Internals::IndexToName(Index)));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueDouble(int32 Index, double& OutValue)
{
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueDouble(EAA::Internals::IndexToName(Index)));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueName(int32 Index, FName& OutValue)
{
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueName(EAA::Internals::IndexToName(Index)));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueString(int32 Index, FString& OutValue)
{
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueString(EAA::Internals::IndexToName(Index)));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueText(int32 Index, FText& OutValue)
{
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueText(EAA::Internals::IndexToName(Index)));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueEnum(int32 Index, UEnum* ExpectedType, uint8& OutValue)
{
	OutValue = 0;
	
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueEnum(EAA::Internals::IndexToName(Index), ExpectedType));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueStruct(int32 Index, UScriptStruct* ExpectedType, const uint8*& OutValue)
{
	OutValue = nullptr;
	
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueStruct(EAA::Internals::IndexToName(Index), ExpectedType));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue().GetMemory();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueObject(int32 Index, UClass* ExpectedClass, UObject*& OutValue)
{
	OutValue = nullptr;
	
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueObject(EAA::Internals::IndexToName(Index), ExpectedClass));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueClass(int32 Index, UClass* ExpectedMetaClass, UClass*& OutValue)
{
	OutValue = nullptr;
	
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueClass(EAA::Internals::IndexToName(Index)));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueSoftObject(int32 Index, UClass* ExpectedClass, TSoftObjectPtr<UObject>& OutValue)
{
	OutValue = nullptr;
	
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueSoftPath(EAA::Internals::IndexToName(Index)));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueSoftClass(int32 Index, UClass* ExpectedMetaClass, TSoftClassPtr<UObject>& OutValue)
{
	OutValue = nullptr;
	
	auto Value = VALIDATE_RESULT(GetValueRef()->GetValueSoftPath(EAA::Internals::IndexToName(Index)));
	if (Value.HasValue())
	{
		OutValue = Value.GetValue();
	}
}

// ======================================
// ============ CONTAINERS ==============
// ======================================

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueArray(int32 Index, EPropertyBagPropertyType Type, const UObject* TypeObject, const void* Value)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Struct))
	{
		GetValueRef()->AddContainerProperty(Name, EPropertyBagContainerType::Array, Type, TypeObject);
	}
	TValueOrError<const FPropertyBagArrayRef, EPropertyBagResult> ArrayData = GetValueRef()->GetArrayRef(Name);
	if (ensureAlways(ArrayData.IsValid()))
	{
		const FPropertyBagPropertyDesc* Desc = GetValueRef()->FindPropertyDescByName(Name);

		void* Storage = GetValueRef()->GetMutableValueAddress(Desc);

		Desc->CachedProperty->CopyCompleteValueFromScriptVM(Storage, Value);
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueArray(int32 Index, EPropertyBagPropertyType Type, const UObject* TypeObject, void* Value)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	TValueOrError<const FPropertyBagArrayRef, EPropertyBagResult> ArrayData = GetValueRef()->GetArrayRef(Name);
	if (ensureAlways(ArrayData.IsValid()))
	{
		const FPropertyBagPropertyDesc* Desc = GetValueRef()->FindPropertyDescByName(Name);

		const void* Storage = GetValueRef()->GetValueAddress(Desc);

		Desc->CachedProperty->CopyCompleteValueToScriptVM(Value, Storage);
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::SetValueSet(int32 Index, EPropertyBagPropertyType Type, const UObject* TypeObject, const void* Value)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	if (CanAddNewProperty(Name, EPropertyBagPropertyType::Struct))
	{
		GetValueRef()->AddContainerProperty(Name, EPropertyBagContainerType::Set, Type, TypeObject);
	}
	TValueOrError<const FPropertyBagSetRef, EPropertyBagResult> SetData = GetValueRef()->GetSetRef(Name);
	if (ensureAlways(SetData.IsValid()))
	{
		const FPropertyBagPropertyDesc* Desc = GetValueRef()->FindPropertyDescByName(Name);

		void* Storage = GetValueRef()->GetMutableValueAddress(Desc);

		Desc->CachedProperty->CopyCompleteValueFromScriptVM(Storage, Value);
	}
}

void FEnhancedAsyncActionContext_PropertyBagBase::GetValueSet(int32 Index, EPropertyBagPropertyType Type, const UObject* TypeObject, void* Value)
{
	const FName Name = EAA::Internals::IndexToName(Index);
	TValueOrError<const FPropertyBagSetRef, EPropertyBagResult> SetData = GetValueRef()->GetSetRef(Name);
	if (ensureAlways(SetData.IsValid()))
	{
		const FPropertyBagPropertyDesc* Desc = GetValueRef()->FindPropertyDescByName(Name);

		const void* Storage = GetValueRef()->GetValueAddress(Desc);

		Desc->CachedProperty->CopyCompleteValueToScriptVM(Value, Storage);
	}
}


// ======================================
// ============ GENERICS ==============
// ======================================


#define RETURN_GENERIC_FAIL(msg) { if (Message) *Message = TEXT(msg); return false; }

static bool IsContainerProperty(const FProperty* Property)
{
	return CastField<FArrayProperty>(Property)
		|| CastField<FSetProperty>(Property)
		|| CastField<FMapProperty>(Property);
}

bool FEnhancedAsyncActionContext_PropertyBagBase::SetValue(int32 Index, const FProperty* Property, const void* Value, FString* Message)
{
	if (!Property || !Value || IsContainerProperty(Property))
		RETURN_GENERIC_FAIL("Bad property");
	
	const FName Name = EAA::Internals::IndexToName(Index);
	
	const FPropertyBagPropertyDesc* ContextProperty = GetValueRef()->FindPropertyDescByName(Name);
	if (!ContextProperty && !bPropertyBagStructureLocked)
	{ // structure is not locked - free to add new property
		GetValueRef()->AddProperty(Name, Property);
		ContextProperty = GetValueRef()->FindPropertyDescByName(Name);
	}
	if (!ContextProperty || !ContextProperty->CachedProperty)
		RETURN_GENERIC_FAIL("Unknown context property");
	if (!ContextProperty->CompatibleType(FPropertyBagPropertyDesc(Name, Property)))
		RETURN_GENERIC_FAIL("Incompatible type");
	void* ContextValueAddress = GetValueRef()->GetMutableValueAddress(ContextProperty);
	if (!ContextValueAddress)
		RETURN_GENERIC_FAIL("Missing context property data");

	Property->CopyCompleteValueFromScriptVM(ContextValueAddress, Value);
	return true;
}

bool FEnhancedAsyncActionContext_PropertyBagBase::GetValue(int32 Index, const FProperty* Property, void* OutValue, FString* Message)
{
	if (!Property || !OutValue || IsContainerProperty(Property))
	{
		RETURN_GENERIC_FAIL("Bad property");
	}
	
	const FName Name = EAA::Internals::IndexToName(Index);
	
	const FPropertyBagPropertyDesc* ContextProperty = GetValueRef()->FindPropertyDescByName(Name);
	if (!ContextProperty || !ContextProperty->CachedProperty)
		RETURN_GENERIC_FAIL("Unknown context property");
	if (!ContextProperty->CompatibleType(FPropertyBagPropertyDesc(Name, Property)))
		RETURN_GENERIC_FAIL("Incompatible type");
	const void* ContextValueAddress = GetValueRef()->GetValueAddress(ContextProperty);
	if (!ContextValueAddress)
		RETURN_GENERIC_FAIL("Missing context property data");

	Property->CopyCompleteValueToScriptVM(OutValue, ContextValueAddress);
	return true;
}

#undef RETURN_GENERIC_FAIL

// ======================================
// ============ OTHER ==============
// ======================================


FEnhancedAsyncActionContext_PropertyBagRef::FEnhancedAsyncActionContext_PropertyBagRef(const UObject* OwningObject, FName PropertyName): OwnerRef(OwningObject)
{
	OwnerRef = OwningObject;
	ValueRef = EAA::Internals::GetMemberChecked<FInstancedPropertyBag>(OwningObject, PropertyName, FInstancedPropertyBag::StaticStruct());
	bAddReferencedObjectsAllowed = false; // do not need as it is already a reflected UPROPERTY
}

bool FEnhancedAsyncActionContext_PropertyBagRef::IsValid() const
{
	return OwnerRef.IsValid() && GetValueRef() != nullptr;
}

FEnhancedAsyncActionContext_PropertyBag::FEnhancedAsyncActionContext_PropertyBag(const UObject* OwningObject)
{
	OwnerRef = OwningObject;
	ValueRef = &Value;
	bAddReferencedObjectsAllowed = true;
}

bool FEnhancedAsyncActionContext_PropertyBag::IsValid() const
{
	return OwnerRef.IsValid() && GetValueRef() != nullptr;
}

#undef VALIDATE_RESULT
