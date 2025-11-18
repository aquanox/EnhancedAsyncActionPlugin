// Copyright 2025, Aquanox.

#include "EnhancedAsyncContextPrivate.h"
#include "EnhancedAsyncContextShared.h"
#include "StructUtils/PropertyBag.h"
#include "K2Node_CallFunction.h"

FString EAA::Internals::ToDebugString(const FProperty* Pin)
{
	FStringBuilderBase Buffer;
	Buffer.Append(TEXT("Property["));
	Buffer.Append(TEXT("Name=")).Append(Pin->GetNameCPP());
	Buffer.Append(TEXT("]"));
	return Buffer.ToString();
}

FString EAA::Internals::ToDebugString(const UEdGraphPin* Pin)
{
	FStringBuilderBase Buffer;
	Buffer.Append(TEXT("Pin["));
	Buffer.Append(TEXT("Name=")).Append(Pin->PinName.ToString());
	Buffer.Append(TEXT(" Category=")).Append(Pin->PinType.PinCategory.ToString());
	Buffer.Append(TEXT(" SubCategory=")).Append(Pin->PinType.PinSubCategory.ToString());
	Buffer.Append(TEXT(" Object=")).Append(GetNameSafe(Pin->PinType.PinSubCategoryObject.Get()));
	Buffer.Append(TEXT("]"));
	return Buffer.ToString();
}

FString EAA::Internals::ToDebugString(const FEdGraphPinType& Type)
{
	FStringBuilderBase Buffer;
	Buffer.Append(TEXT("PinType["));
	Buffer.Append(TEXT(" Category=")).Append(Type.PinCategory.ToString());
	Buffer.Append(TEXT(" SubCategory=")).Append(Type.PinSubCategory.ToString());
	Buffer.Append(TEXT(" Object=")).Append(GetNameSafe(Type.PinSubCategoryObject.Get()));
	Buffer.Append(TEXT("]"));
	return Buffer.ToString();
}

FString EAA::Internals::ToDebugString(const UK2Node* Node)
{
	FStringBuilderBase Buffer;
	Buffer.Append(Node->GetPathName());
	return Buffer.ToString();
}

const FString* EAA::Internals::FindMetadataHierarchical(const UClass* InClass, const FName& Name)
{
	for (const UStruct* TestStruct = InClass; TestStruct; TestStruct = TestStruct->GetSuperStruct())
	{
		if (auto Ptr = TestStruct->FindMetaData(Name))
		{
			return Ptr;
		}
	}
	return nullptr;
}

const FEdGraphPinType& EAA::Internals::GetWildcardType()
{
	static const FEdGraphPinType WildcardPinType = FEdGraphPinType(UEdGraphSchema_K2::PC_Wildcard, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
	return WildcardPinType;
}

bool EAA::Internals::IsWildcardType(const UEdGraphPin* Pin)
{
	return Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard;
}

bool EAA::Internals::IsWildcardType(const FEdGraphPinType& Type)
{
	return Type.PinCategory == UEdGraphSchema_K2::PC_Wildcard;
}

bool EAA::Internals::IsCapturableType(const UEdGraphPin* Pin)
{
	return Pin && IsCapturableType(Pin->PinType);
}

bool EAA::Internals::IsCapturableType(const FEdGraphPinType& Type)
{
	if (Type.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		return false;

	FPropertyTypeInfo TypeInfo = IdentifyPropertyTypeForPin(Type);
	if (TypeInfo.IsValid())
	{
		FName TmpUnused;
		return EAA::Internals::SelectAccessorForType(TypeInfo, EAccessorRole::SETTER, TmpUnused) && !TmpUnused.IsNone();
	}
	return false;
}

FPropertyTypeInfo EAA::Internals::IdentifyPropertyTypeForPin(const UEdGraphPin* Pin)
{
	return IdentifyPropertyTypeForPin(Pin->PinType);
}

FPropertyTypeInfo EAA::Internals::IdentifyPropertyTypeForPin(const FEdGraphPinType& Type)
{
	FPropertyTypeInfo TypeInfo;

	const FName& PinCategory = Type.PinCategory;

	if (PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		return FPropertyTypeInfo::Wildcard;
	if (PinCategory == UEdGraphSchema_K2::PC_Exec
		|| PinCategory == UEdGraphSchema_K2::PC_Delegate
		|| PinCategory == UEdGraphSchema_K2::PC_MCDelegate
		|| PinCategory == UEdGraphSchema_K2::PC_Interface)
		return FPropertyTypeInfo::Invalid;

	const EPinContainerType ContainerType = Type.ContainerType;
	if (ContainerType == EPinContainerType::None)
	{
		TypeInfo.ContainerType = EPropertyBagContainerType::None;
	}
	else if (ContainerType == EPinContainerType::Array)
	{
		TypeInfo.ContainerType = EPropertyBagContainerType::Array;
	}
	else if (ContainerType == EPinContainerType::Set)
	{
		TypeInfo.ContainerType = EPropertyBagContainerType::Set;
	}
	else if (ContainerType == EPinContainerType::Map)
	{
		ensureAlways(false);
		return FPropertyTypeInfo::Wildcard;
	}
	else
	{
		return FPropertyTypeInfo::Invalid;
	}

	auto GuessType = [](const FEdGraphPinType& InPinType, EPropertyBagPropertyType& OutType, TObjectPtr<const UObject>& OutTypeObject)
	{
		const FName& PinCategory = InPinType.PinCategory;
		if (PinCategory == UEdGraphSchema_K2::PC_Boolean)
		{
			OutType = EPropertyBagPropertyType::Bool;
			OutTypeObject = nullptr;
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Byte)
		{
			if (UEnum* Enum = Cast<UEnum>(InPinType.PinSubCategoryObject))
			{
				OutType = EPropertyBagPropertyType::Enum;
				OutTypeObject = Enum;
			}
			else
			{
				OutType = EPropertyBagPropertyType::Byte;
				OutTypeObject = nullptr;
			}
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Int)
		{
			OutType = EPropertyBagPropertyType::Int32;
			OutTypeObject = nullptr;
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Int64)
		{
			OutType = EPropertyBagPropertyType::Int64;
			OutTypeObject = nullptr;
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Float)
		{
			OutType = EPropertyBagPropertyType::Float;
			OutTypeObject = nullptr;
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Double)
		{
			OutType = EPropertyBagPropertyType::Double;
			OutTypeObject = nullptr;
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Real)
		{
			if (InPinType.PinSubCategory == UEdGraphSchema_K2::PC_Float)
			{
				OutType = EPropertyBagPropertyType::Float;
				OutTypeObject = nullptr;
			}
			if (InPinType.PinSubCategory == UEdGraphSchema_K2::PC_Double)
			{
				OutType = EPropertyBagPropertyType::Double;
				OutTypeObject = nullptr;
			}
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Name)
		{
			OutType = EPropertyBagPropertyType::Name;
			OutTypeObject = nullptr;
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_String)
		{
			OutType = EPropertyBagPropertyType::String;
			OutTypeObject = nullptr;
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Text)
		{
			OutType = EPropertyBagPropertyType::Text;
			OutTypeObject = nullptr;
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Enum)
		{
			OutType = EPropertyBagPropertyType::Enum;
			OutTypeObject = InPinType.PinSubCategoryObject.Get();
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Struct)
		{
			OutType = EPropertyBagPropertyType::Struct;
			OutTypeObject = InPinType.PinSubCategoryObject.Get();
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Object)
		{
			OutType = EPropertyBagPropertyType::Object;
			OutTypeObject = InPinType.PinSubCategoryObject.Get();
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_SoftObject)
		{
			OutType = EPropertyBagPropertyType::SoftObject;
			OutTypeObject = InPinType.PinSubCategoryObject.Get();
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Class)
		{
			OutType = EPropertyBagPropertyType::Class;
			OutTypeObject = InPinType.PinSubCategoryObject.Get();
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_SoftClass)
		{
			OutType = EPropertyBagPropertyType::SoftClass;
			OutTypeObject = InPinType.PinSubCategoryObject.Get();
		}
		else
		{
			ensureMsgf(false, TEXT("Unhandled pin category %s"), *InPinType.PinCategory.ToString());
		}
	};

	if (ContainerType != EPinContainerType::Map)
	{
		GuessType(Type, TypeInfo.ValueType, TypeInfo.ValueTypeObject);
	}
	else
	{
		// todo: maps
		ensureAlways(false);
	}

	return TypeInfo;
}

bool EAA::Internals::SelectAccessorForType(UEdGraphPin* Pin, EAccessorRole AccessType, FName& OutFunction)
{
	FPropertyTypeInfo TypeInfo = IdentifyPropertyTypeForPin(Pin);
	return SelectAccessorForType(TypeInfo, AccessType, OutFunction);
}

bool EAA::Internals::SelectAccessorForType(const FEdGraphPinType& PinType, EAccessorRole AccessType, FName& OutFunction)
{
	FPropertyTypeInfo TypeInfo = IdentifyPropertyTypeForPin(PinType);
	return SelectAccessorForType(TypeInfo, AccessType, OutFunction);
}
