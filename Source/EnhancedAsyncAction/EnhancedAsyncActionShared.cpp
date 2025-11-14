#include "EnhancedAsyncActionShared.h"

#include "StructUtils/PropertyBag.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "EnhancedAsyncActionContextLibrary.h"

DEFINE_LOG_CATEGORY(LogEnhancedAction);

// Allocate more for debug purposes
static TArray<FName, TInlineAllocator<std::max(EAA::Internals::MaxCapturePins, 64)>> CapturePinNames;

FName EAA::Internals::IndexToName(int32 Index)
{
	if (!CapturePinNames.Num())
	{
		CapturePinNames.SetNum(std::max(EAA::Internals::MaxCapturePins, 64));
		for (int32 I = 0; I < CapturePinNames.Num(); I++)
		{
			CapturePinNames[I] = *FString::Printf(TEXT("ctx%02d"), I);
		}
	}
	return CapturePinNames.IsValidIndex(Index) ? CapturePinNames[Index] : NAME_Name;
}

bool EAA::Internals::IsValidContainerProperty(const UObject* Object, const FName& Property)
{
	if (Property.IsNone() || !IsValid(Object))
		return false;

	FStructProperty* StructProperty = CastField<FStructProperty>(Object->GetClass()->FindPropertyByName(Property));
	if (!StructProperty)
		return false;
	
	return StructProperty->Struct && StructProperty->Struct->IsChildOf(FInstancedPropertyBag::StaticStruct());
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
		? GET_MEMBER_NAME_CHECKED(UEnhancedAsyncActionContextLibrary, Set) \
		: GET_MEMBER_NAME_CHECKED(UEnhancedAsyncActionContextLibrary, Get);

#define FUNC_SELECT_GENERIC()  \
	OutFunction = Role == EAccessorRole::SETTER  \
		? GET_MEMBER_NAME_CHECKED(UEnhancedAsyncActionContextLibrary, Handle_SetValue_Generic)   \
		: GET_MEMBER_NAME_CHECKED(UEnhancedAsyncActionContextLibrary, Handle_GetValue_Generic);
	
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
	
	return nullptr;
}
