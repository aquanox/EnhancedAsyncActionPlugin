// Copyright 2025, Aquanox.

#include "EnhancedAsyncContextTypes.h"
#include "StructUtils/PropertyBag.h"
#include "StructUtils/StructUtilsTypes.h"
#include "UObject/TextProperty.h"
#include "EnhancedAsyncContextLibrary.h"

const FPropertyTypeInfo FPropertyTypeInfo::Invalid(FPropertyTypeInfo::EInternalPreset::PRESET_Invalid);
const FPropertyTypeInfo FPropertyTypeInfo::Wildcard(FPropertyTypeInfo::EInternalPreset::PRESET_Wildcard);

static const FString EnumPrefix = TEXT("EPropertyBagPropertyType::");

bool FPropertyTypeInfo::IsWildcard() const
{
	return *this == Wildcard;
}

bool FPropertyTypeInfo::IsValid() const
{
	return ContainerType != EPropertyBagContainerType::Count
		&& ValueType != EPropertyBagPropertyType::Count
		&& KeyType != EPropertyBagPropertyType::Count;
}

bool FPropertyTypeInfo::IsSupported() const
{
	return IsValid() && EAA::Internals::HasAccessorForType(*this);
}

bool FPropertyTypeInfo::IsCompatibleWith(const FPropertyTypeInfo& Other) const
{
	// Containers must match
	if (ContainerType != Other.ContainerType)
		return false;

	// Values must match.
	if (ValueType != Other.ValueType)
		return false;

	// Struct and enum must have same value type class
	if (ValueType == EPropertyBagPropertyType::Enum || ValueType == EPropertyBagPropertyType::Struct)
	{
		return ValueTypeObject == Other.ValueTypeObject;
	}

	// Objects should be castable.
	if (ValueType == EPropertyBagPropertyType::Object)
	{
		const UClass* ObjectClass = Cast<const UClass>(ValueTypeObject);
		const UClass* OtherObjectClass = Cast<const UClass>(Other.ValueTypeObject);

		return ObjectClass && OtherObjectClass && ObjectClass->IsChildOf(OtherObjectClass);
	}

	return true;
}

FString FPropertyTypeInfo::EncodeTypeInfo(const FPropertyTypeInfo& TypeInfo)
{
	FStringBuilderBase Builder;

	switch (TypeInfo.ContainerType)
	{
	default:
		checkNoEntry();
		break;
	case EPropertyBagContainerType::None:
		Builder.Append(TEXT("N:"));
		break;
	case EPropertyBagContainerType::Array:
		Builder.Append(TEXT("A:"));
		break;
	case EPropertyBagContainerType::Set:
		Builder.Append(TEXT("S:"));
		break;
	// TODO: maps supported in 5.8
	// case EPropertyBagContainerType::Map:
		// Builder.Append(TEXT("M:"));
		// break;
	}

	FString Str = StaticEnum<EPropertyBagPropertyType>()->GetNameByValue((int64)TypeInfo.ValueType).ToString();
	check(Str.StartsWith(EnumPrefix, ESearchCase::CaseSensitive));
	Builder.Append(Str.RightChop(EnumPrefix.Len()));

	if (::IsValid(TypeInfo.ValueTypeObject))
	{
		Builder.Append(TEXT("="));
		Builder.Append(FSoftObjectPath(TypeInfo.ValueTypeObject).ToString());
	}

	return Builder.ToString();
}

bool FPropertyTypeInfo::ParseTypeInfo(FString Data, FPropertyTypeInfo& TypeInfo)
{
	if (Data.Len() < 4 || Data[1] != TEXT(':'))
	{
		ensureAlwaysMsgf(false, TEXT("Bad type format"));
		return false;
	}

	TCHAR ContainerTypeKey = Data[0];
	Data.MidInline(2);

	if (ContainerTypeKey == TEXT('N') || ContainerTypeKey == TEXT('A') || ContainerTypeKey == TEXT('S'))
	{
		switch (ContainerTypeKey)
		{
		case 'N': TypeInfo.ContainerType = EPropertyBagContainerType::None; break;
		case 'A': TypeInfo.ContainerType = EPropertyBagContainerType::Array; break;
		case 'S': TypeInfo.ContainerType = EPropertyBagContainerType::Set; break;
		default: checkNoEntry(); break;
		}

		FString Type, Object;
		if (Data.Split(TEXT("="), &Type, &Object))
		{
			int64 Value = StaticEnum<EPropertyBagPropertyType>()->GetValueByNameString( EnumPrefix + Type );
			check (Value != INDEX_NONE);
			TypeInfo.ValueType = static_cast<EPropertyBagPropertyType>(Value);
			TypeInfo.ValueTypeObject = FSoftObjectPath(Object).ResolveObject();
		}
		else
		{
			int64 Value = StaticEnum<EPropertyBagPropertyType>()->GetValueByNameString( EnumPrefix + Data );
			check (Value != INDEX_NONE);
			TypeInfo.ValueType = static_cast<EPropertyBagPropertyType>(Value);
		}
	}
	else if (ContainerTypeKey == TEXT('M'))
	{
		// todo
		return false;
	}

	return TypeInfo.IsValid();
}

FPropertyTypeInfo::FPropertyTypeInfo()
	: ContainerType(EPropertyBagContainerType::None)
	, ValueType(EPropertyBagPropertyType::None)
	, KeyType(EPropertyBagPropertyType::None)
{
}

FPropertyTypeInfo::FPropertyTypeInfo(EPropertyBagPropertyType Type)
	: ContainerType(EPropertyBagContainerType::None)
	, ValueType(Type)
	, KeyType(EPropertyBagPropertyType::None)
{
}

FPropertyTypeInfo::FPropertyTypeInfo(const FProperty* ExistingProperty)
	: KeyType(EPropertyBagPropertyType::None)
{
	ContainerType = EAA::Internals::GetContainerTypeFromProperty(ExistingProperty);
	if (auto AsMap = CastField<FMapProperty>(ExistingProperty))
	{
		ValueType = EAA::Internals::GetValueTypeFromProperty(AsMap->ValueProp);
		ValueTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(AsMap->ValueProp);
		KeyType = EAA::Internals::GetValueTypeFromProperty(AsMap->KeyProp);
		KeyTypeObject =  EAA::Internals::GetValueTypeObjectFromProperty(AsMap->KeyProp);
	}
	else
	{
		ValueType = EAA::Internals::GetValueTypeFromProperty(ExistingProperty);
		ValueTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(ExistingProperty);
		KeyType = EPropertyBagPropertyType::None;
	}
}

FPropertyTypeInfo::FPropertyTypeInfo(const FPropertyBagPropertyDesc* ExistingProperty)
	: ContainerType(ExistingProperty->ContainerTypes.GetFirstContainerType())
	, ValueType(ExistingProperty->ValueType), ValueTypeObject(ExistingProperty->ValueTypeObject)
	, KeyType(EPropertyBagPropertyType::None)
{
}

FPropertyTypeInfo::FPropertyTypeInfo(EPropertyBagPropertyType Type, TObjectPtr<const UObject> Object)
	: ContainerType(EPropertyBagContainerType::None)
	, ValueType(Type), ValueTypeObject(Object)
	, KeyType(EPropertyBagPropertyType::None)
{
}

FPropertyTypeInfo::FPropertyTypeInfo(EInternalPreset Preset)
{
	switch (Preset)
	{
	case PRESET_Wildcard:
		ContainerType = EPropertyBagContainerType::None;
		KeyType = ValueType = EPropertyBagPropertyType::None;
		KeyTypeObject = ValueTypeObject = nullptr;
		break;
	case PRESET_Invalid:
		ContainerType = EPropertyBagContainerType::Count;
		KeyType = ValueType = EPropertyBagPropertyType::Count;
		KeyTypeObject = ValueTypeObject = nullptr;
		break;
	default:
		break;
	}
}


bool EAA::Internals::HasAccessorForType(const FPropertyTypeInfo& TypeInfo)
{
	FName TmpUnused;
	return EAA::Internals::SelectAccessorForType(TypeInfo, EAccessorRole::SETTER, TmpUnused) && !TmpUnused.IsNone();
}

bool EAA::Internals::SelectAccessorForType(const FPropertyTypeInfo& TypeInfo, EAccessorRole Role, FName& OutFunction)
{
	OutFunction = NAME_None;

#define FUNC_SELECT(Set, Get) \
	OutFunction = Role == EAccessorRole::SETTER \
		? GET_MEMBER_NAME_CHECKED(UEnhancedAsyncContextLibrary, Set) \
		: GET_MEMBER_NAME_CHECKED(UEnhancedAsyncContextLibrary, Get);

#define FUNC_SELECT_GENERIC()  \
	OutFunction = Role == EAccessorRole::SETTER  \
		? GET_MEMBER_NAME_CHECKED(UEnhancedAsyncContextLibrary, Handle_SetValue_Generic)   \
		: GET_MEMBER_NAME_CHECKED(UEnhancedAsyncContextLibrary, Handle_GetValue_Generic);

	if (TypeInfo.ContainerType == EPropertyBagContainerType::None)
	{
		switch(TypeInfo.ValueType)
		{
		case EPropertyBagPropertyType::Bool:
			FUNC_SELECT(Handle_SetValue_Bool, Handle_GetValue_Bool);
			break;
		case EPropertyBagPropertyType::Byte:
			FUNC_SELECT(Handle_SetValue_Byte, Handle_GetValue_Byte);
			break;
		case EPropertyBagPropertyType::Int32:
			FUNC_SELECT(Handle_SetValue_Int32, Handle_GetValue_Int32);
			break;
		case EPropertyBagPropertyType::Int64:
			FUNC_SELECT(Handle_SetValue_Int64, Handle_GetValue_Int64);
			break;
		case EPropertyBagPropertyType::Float:
			FUNC_SELECT(Handle_SetValue_Float, Handle_GetValue_Float);
			break;
		case EPropertyBagPropertyType::Double:
			FUNC_SELECT(Handle_SetValue_Double, Handle_GetValue_Double);
			break;
		case EPropertyBagPropertyType::Name:
			FUNC_SELECT(Handle_SetValue_Name, Handle_GetValue_Name);
			break;
		case EPropertyBagPropertyType::String:
			FUNC_SELECT(Handle_SetValue_String, Handle_GetValue_String);
			break;
		case EPropertyBagPropertyType::Text:
			FUNC_SELECT(Handle_SetValue_Text, Handle_GetValue_Text);
			break;
		case EPropertyBagPropertyType::Enum:
			FUNC_SELECT(Handle_SetValue_Enum, Handle_GetValue_Enum);
			break;
		case EPropertyBagPropertyType::Struct:
			FUNC_SELECT(Handle_SetValue_Struct, Handle_GetValue_Struct);
			break;
		case EPropertyBagPropertyType::Object:
			FUNC_SELECT(Handle_SetValue_Object, Handle_GetValue_Object);
			break;
		case EPropertyBagPropertyType::Class:
			FUNC_SELECT(Handle_SetValue_Class, Handle_GetValue_Class);
			break;
		case EPropertyBagPropertyType::SoftObject:
			FUNC_SELECT(Handle_SetValue_SoftObject, Handle_GetValue_SoftObject);
			break;
		case EPropertyBagPropertyType::SoftClass:
			FUNC_SELECT(Handle_SetValue_SoftClass, Handle_GetValue_SoftClass);
			break;
		default:
			break;
		}
	}
	else if (TypeInfo.ContainerType == EPropertyBagContainerType::Array)
	{
		FName TmpUnused;
		if (SelectAccessorForType(FPropertyTypeInfo(TypeInfo.ValueType), Role, TmpUnused))
		{
			FUNC_SELECT(Handle_SetValue_Array, Handle_GetValue_Array);
		}
	}
	else if (TypeInfo.ContainerType == EPropertyBagContainerType::Set)
	{
		FName TmpUnused;
		if (SelectAccessorForType(FPropertyTypeInfo(TypeInfo.ValueType), Role, TmpUnused))
		{
			FUNC_SELECT(Handle_SetValue_Set, Handle_GetValue_Set);
		}
	}

#undef NOT_IMPLEMENTED_YET
#undef CASE_SELECT
#undef FUNC_SELECT
#undef FUNC_SELECT_GENERIC

	return !OutFunction.IsNone();
}

EPropertyBagContainerType EAA::Internals::GetContainerTypeFromProperty(const FProperty* InSourceProperty)
{
	if (CastField<FArrayProperty>(InSourceProperty))
	{
		return EPropertyBagContainerType::Array;
	}
	else if (CastField<FSetProperty>(InSourceProperty))
	{
		return EPropertyBagContainerType::Set;
	}
	else if (CastField<FMapProperty>(InSourceProperty))
	{ // todo: maps support in 5.8
		return EPropertyBagContainerType::Count;
	}

	return EPropertyBagContainerType::None;
}

EPropertyBagPropertyType EAA::Internals::GetValueTypeFromProperty(const FProperty* InSourceProperty)
{
	if (CastField<FBoolProperty>(InSourceProperty))
	{
		return EPropertyBagPropertyType::Bool;
	}
	if (const FByteProperty* ByteProperty = CastField<FByteProperty>(InSourceProperty))
	{
		return ByteProperty->IsEnum() ? EPropertyBagPropertyType::Enum : EPropertyBagPropertyType::Byte;
	}
	if (CastField<FIntProperty>(InSourceProperty))
	{
		return EPropertyBagPropertyType::Int32;
	}
	if (CastField<FUInt32Property>(InSourceProperty))
	{
		return EPropertyBagPropertyType::UInt32;
	}
	if (CastField<FInt64Property>(InSourceProperty))
	{
		return EPropertyBagPropertyType::Int64;
	}
	if (CastField<FUInt64Property>(InSourceProperty))
	{
		return EPropertyBagPropertyType::UInt64;
	}
	if (CastField<FFloatProperty>(InSourceProperty))
	{
		return EPropertyBagPropertyType::Float;
	}
	if (CastField<FDoubleProperty>(InSourceProperty))
	{
		return EPropertyBagPropertyType::Double;
	}
	if (CastField<FNameProperty>(InSourceProperty))
	{
		return EPropertyBagPropertyType::Name;
	}
	if (CastField<FStrProperty>(InSourceProperty))
	{
		return EPropertyBagPropertyType::String;
	}
	if (CastField<FTextProperty>(InSourceProperty))
	{
		return EPropertyBagPropertyType::Text;
	}
	if (CastField<FEnumProperty>(InSourceProperty))
	{
		return EPropertyBagPropertyType::Enum;
	}
	if (CastField<FStructProperty>(InSourceProperty))
	{
		return EPropertyBagPropertyType::Struct;
	}
	if (CastField<FObjectProperty>(InSourceProperty))
	{
		if (CastField<FClassProperty>(InSourceProperty))
		{
			return EPropertyBagPropertyType::Class;
		}

		return EPropertyBagPropertyType::Object;
	}
	if (CastField<FSoftObjectProperty>(InSourceProperty))
	{
		if (CastField<FSoftClassProperty>(InSourceProperty))
		{
			return EPropertyBagPropertyType::SoftClass;
		}

		return EPropertyBagPropertyType::SoftObject;
	}

	// Handle array property
	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(InSourceProperty))
	{
		return GetValueTypeFromProperty(ArrayProperty->Inner);
	}

	// Handle set property
	if (const FSetProperty* SetProperty = CastField<FSetProperty>(InSourceProperty))
	{
		return GetValueTypeFromProperty(SetProperty->ElementProp);
	}

	// Handle map property
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(InSourceProperty))
	{ // todo: maps support in 5.8
		return EPropertyBagPropertyType::Count;
	}

	return EPropertyBagPropertyType::None;
}

UObject* EAA::Internals::GetValueTypeObjectFromProperty(const FProperty* InSourceProperty)
{
	if (const auto* Property = CastField<FByteProperty>(InSourceProperty))
	{
		if (Property->IsEnum())
			return Property->Enum;
	}
	if (const auto* Property = CastField<FEnumProperty>(InSourceProperty))
	{
		return Property->GetEnum();
	}
	if (const auto* Property = CastField<FStructProperty>(InSourceProperty))
	{
		return Property->Struct;
	}
	if (const auto* Property = CastField<FObjectProperty>(InSourceProperty))
	{
		if (const auto* ClassProperty = CastField<FClassProperty>(InSourceProperty))
		{
			return ClassProperty->MetaClass;
		}
		return Property->PropertyClass;
	}
	if (const auto* SoftObjectProperty = CastField<FSoftObjectProperty>(InSourceProperty))
	{
		if (const auto* SoftClassProperty = CastField<FSoftClassProperty>(InSourceProperty))
		{
			return SoftClassProperty->MetaClass;
		}
		return SoftObjectProperty->PropertyClass;
	}

	// Handle array property
	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(InSourceProperty))
	{
		return GetValueTypeObjectFromProperty(ArrayProperty->Inner);
	}

	// Handle set property
	if (const FSetProperty* SetProperty = CastField<FSetProperty>(InSourceProperty))
	{
		return GetValueTypeObjectFromProperty(SetProperty->ElementProp);
	}

	// Handle map property
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(InSourceProperty))
	{ // todo: maps support in 5.8
		return nullptr;
	}

	return nullptr;
}

bool EAA::Internals::IsContainerProperty(const FProperty* Property)
{
	return CastField<FArrayProperty>(Property)
		|| CastField<FSetProperty>(Property)
		|| CastField<FMapProperty>(Property);
}

bool EAA::Internals::IsCompatibleWithProperty(EAccessorRole Role, const FPropertyBagPropertyDesc* Descriptor, const FProperty* Property)
{
	check(Descriptor && Property);

	// if (Descriptor->Name == Property->GetFName())
		// return true;

	// This is broken in 5.5
	// return Descriptor->CompatibleType(FPropertyBagPropertyDesc(NAME_None, Property));

	FPropertyTypeInfo DescInfo(Descriptor);
	FPropertyTypeInfo PropInfo(Property);
	if (Role == EAccessorRole::SETTER)
	{
		return DescInfo.IsCompatibleWith(PropInfo);
	}
	else
	{
		return PropInfo.IsCompatibleWith(DescInfo);
	}
}
