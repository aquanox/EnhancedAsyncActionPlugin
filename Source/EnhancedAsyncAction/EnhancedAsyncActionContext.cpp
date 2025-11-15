// Copyright 2025, Aquanox.

#include "EnhancedAsyncActionContext.h"

#include "EnhancedAsyncActionManager.h"
#include "EnhancedAsyncActionShared.h"
#include "StructUtils/StructView.h"
#include "StructUtils/PropertyBag.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnhancedAsyncActionContext)

const FPropertyTypeInfo FPropertyTypeInfo::Invalid(EPropertyBagContainerType::Count, EPropertyBagPropertyType::Count, EPropertyBagPropertyType::Count);
const FPropertyTypeInfo FPropertyTypeInfo::Wildcard(EPropertyBagContainerType::None, EPropertyBagPropertyType::None, EPropertyBagPropertyType::None);

static const FString EnumPrefix = TEXT("EPropertyBagPropertyType::");

bool FPropertyTypeInfo::IsWildcard() const
{
	return ContainerType == EPropertyBagContainerType::None
		&& ValueType == EPropertyBagPropertyType::None
		&& KeyType == EPropertyBagPropertyType::None;
	
}

bool FPropertyTypeInfo::IsValid() const
{
	return ContainerType != EPropertyBagContainerType::Count
		&& ValueType != EPropertyBagPropertyType::Count
		&& KeyType != EPropertyBagPropertyType::Count;
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
	// case EPropertyBagContainerType::Count:
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

FPropertyTypeInfo::FPropertyTypeInfo(EPropertyBagPropertyType Type, TObjectPtr<const UObject> Object)
	: ContainerType(EPropertyBagContainerType::None)
	, ValueType(Type), ValueTypeObject(Object)
	, KeyType(EPropertyBagPropertyType::None)
{
}

FPropertyTypeInfo::FPropertyTypeInfo(EPropertyBagContainerType Container, EPropertyBagPropertyType Type, TObjectPtr<const UObject> Object)
	: ContainerType(Container)
	, ValueType(Type), ValueTypeObject(Object)
	, KeyType(EPropertyBagPropertyType::None)
{
}

FPropertyTypeInfo::FPropertyTypeInfo(EPropertyBagContainerType Container, EPropertyBagPropertyType KeyType, EPropertyBagPropertyType ValueType)
	: ContainerType(Container)
	, ValueType(ValueType)
	, KeyType(KeyType)
{
}

FEnhancedAsyncActionContextHandle::FEnhancedAsyncActionContextHandle(UObject* InOwner, TSharedRef<FEnhancedAsyncActionContext> Ctx)
	: Owner(InOwner), Data(Ctx)
{
}

FEnhancedAsyncActionContextHandle::FEnhancedAsyncActionContextHandle(UObject* InOwner, FName InDataProp, TSharedRef<FEnhancedAsyncActionContext> Ctx)
	: Owner(InOwner), DataProperty(InDataProp), Data(Ctx)
{
}

bool FEnhancedAsyncActionContextHandle::IsValid() const
{
	return Owner.IsValid() && Data.IsValid();
}

bool FEnhancedAsyncActionContextHandle::IsExternal() const
{
	return DataProperty.IsNone();
}
