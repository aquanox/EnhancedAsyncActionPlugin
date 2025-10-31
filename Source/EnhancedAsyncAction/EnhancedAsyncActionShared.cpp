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

#define CASE_NOT_IMPLEMENTED(Type) case EPropertyBagPropertyType::Type: ensureAlways(false); break
// #define NOT_IMPLEMENTED_YET() break
	
#define FUNC_SELECT(Set, Get) \
	OutFunction = Role == EAccessorRole::SETTER \
		? GET_MEMBER_NAME_CHECKED(UEnhancedAsyncActionContextLibrary, Set) \
		: GET_MEMBER_NAME_CHECKED(UEnhancedAsyncActionContextLibrary, Get);

#define CASE_SELECT(Type, Set, Get) case EPropertyBagPropertyType::Type: FUNC_SELECT(Set, Get); break

	if (TypeInfo.ContainerType == EPropertyBagContainerType::None)
	{
		switch(TypeInfo.ValueType)
		{
			CASE_SELECT(Bool, Handle_SetValue_Bool, Handle_GetValue_Bool);
			CASE_SELECT(Byte, Handle_SetValue_Byte, Handle_GetValue_Byte);
			CASE_SELECT(Int32, Handle_SetValue_Int32, Handle_GetValue_Int32);
			CASE_SELECT(Int64, Handle_SetValue_Int64, Handle_GetValue_Int64);
			CASE_SELECT(Float, Handle_SetValue_Float, Handle_GetValue_Float);
			CASE_SELECT(Double, Handle_SetValue_Double, Handle_GetValue_Double);
			CASE_SELECT(Name, Handle_SetValue_Name, Handle_GetValue_Name);
			CASE_SELECT(String, Handle_SetValue_String, Handle_GetValue_String);
			CASE_SELECT(Text, Handle_SetValue_Text, Handle_GetValue_Text);
			CASE_SELECT(Enum, Handle_SetValue_Enum, Handle_GetValue_Enum);
			CASE_SELECT(Struct, Handle_SetValue_Struct, Handle_GetValue_Struct);
			CASE_SELECT(Object, Handle_SetValue_Object, Handle_GetValue_Object);
			CASE_SELECT(Class, Handle_SetValue_Class, Handle_GetValue_Class);
			CASE_SELECT(SoftObject, Handle_SetValue_Struct, Handle_GetValue_Struct);
			CASE_SELECT(SoftClass, Handle_SetValue_Struct, Handle_GetValue_Struct);
			CASE_SELECT(UInt32, Handle_SetValue_Int32, Handle_GetValue_Int32);
			CASE_SELECT(UInt64, Handle_SetValue_Int64, Handle_GetValue_Int64);
		default:
			checkNoEntry();
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
	else
	{
		checkNoEntry();
	}
	
#undef NOT_IMPLEMENTED_YET
#undef CASE_SELECT
#undef FUNC_SELECT
	
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

FPropertyBagContainerTypes EAA::Internals::GetContainerTypesFromProperty(const FProperty* InSourceProperty)
{
	FPropertyBagContainerTypes ContainerTypes;

	while (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(InSourceProperty))
	{
		if (ContainerTypes.Add(EPropertyBagContainerType::Array))
		{
			InSourceProperty = CastField<FArrayProperty>(ArrayProperty->Inner);
		}
		else // we reached the nested containers limit
		{
			ContainerTypes.Reset();
			break;
		}
	}

	return ContainerTypes;
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
		checkNoEntry();
		return GetValueTypeFromProperty(ArrayProperty->Inner);	
	}

	// Handle set property
	if (const FSetProperty* SetProperty = CastField<FSetProperty>(InSourceProperty))
	{
		checkNoEntry();
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
		return Property->PropertyClass;
	}
	if (const auto* Property = CastField<FClassProperty>(InSourceProperty))
	{
		return Property->MetaClass;
	}
	if (const auto* Property = CastField<FSoftObjectProperty>(InSourceProperty))
	{
		return Property->PropertyClass;
	}
	if (const auto* Property = CastField<FSoftClassProperty>(InSourceProperty))
	{
		return Property->MetaClass;
	}
	
	return nullptr;
}
