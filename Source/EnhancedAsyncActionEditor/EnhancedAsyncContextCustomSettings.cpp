// Copyright 2025, Aquanox.

#include "EnhancedAsyncContextCustomSettings.h"

#include "EdGraphSchema_K2.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "StructUtils/PropertyBag.h"
#include "Widgets/SToolTip.h"

void FExternalAsyncActionSpecCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	Super::CustomizeChildren(PropertyHandle, ChildBuilder, CustomizationUtils);

	ClassHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FExternalAsyncActionSpec, ActionClass));
	ContextHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FExternalAsyncActionSpec, ContextPropertyName));
	ContainerHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FExternalAsyncActionSpec, ContainerPropertyName));

	ClassHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &ThisClass::OnClassChanged));

	ChildBuilder.AddProperty(ClassHandle.ToSharedRef());

	ChildBuilder.AddProperty(ContextHandle.ToSharedRef())
		.CustomWidget()
		.NameContent()
		[
			ContextHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MaxDesiredWidth(500.0f)
		.MinDesiredWidth(100.0f)
		[
			PropertyCustomizationHelpers::MakePropertyComboBox(ContextHandle, FOnGetPropertyComboBoxStrings::CreateSP(this, &ThisClass::OnPopulateContextCombo))
		];


	ChildBuilder.AddProperty(ContainerHandle.ToSharedRef())
		.CustomWidget()
		.NameContent()
		[
			ContainerHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MaxDesiredWidth(500.0f)
		.MinDesiredWidth(100.0f)
		[
			PropertyCustomizationHelpers::MakePropertyComboBox(ContainerHandle, FOnGetPropertyComboBoxStrings::CreateSP(this, &ThisClass::OnPopulateContainerCombo))
		];
}

void FExternalAsyncActionSpecCustomization::OnClassChanged()
{
	ContextHandle->SetValueFromFormattedString(TEXT(""));
	ContainerHandle->SetValueFromFormattedString(TEXT(""));
}

void FExternalAsyncActionSpecCustomization::OnPopulateContextCombo(TArray<TSharedPtr<FString>>& OutComboBoxStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems) const
{
	UObject* ClassValue = nullptr;
	if ( ClassHandle->GetValue(ClassValue) == FPropertyAccess::Success && ClassValue && Cast<UClass>(ClassValue)->IsChildOf(UBlueprintAsyncActionBase::StaticClass()))
	{
		TArray<FName /* prop */> Uniques;
		TMultiMap<FName /* prop */, FString /* delegate */ > Choices;
		for (TFieldIterator<FMulticastDelegateProperty> PropertyIt(Cast<UClass>(ClassValue)); PropertyIt; ++PropertyIt)
		{
			for (TFieldIterator<FObjectProperty> DelegatePropertyIt( PropertyIt->SignatureFunction ); DelegatePropertyIt; ++DelegatePropertyIt)
			{
				Uniques.AddUnique( DelegatePropertyIt->GetFName() );
				Choices.Add(DelegatePropertyIt->GetFName(), PropertyIt->GetName() );
			}
		}

		for (const FName& Name : Uniques)
		{
			OutComboBoxStrings.Add(MakeShared<FString>(Name.ToString()));

			FString TooltipText = Name.ToString();
			TooltipText.Append(TEXT("["));
			TArray<FString> Tmp;  Choices.MultiFind(Name, Tmp);  TooltipText.Append( FString::Join(Tmp, TEXT(", ")) );
			TooltipText.Append(TEXT("]"));

			OutToolTips.Add(SNew(SToolTip).Text(FText::FromString(TooltipText)));
			OutRestrictedItems.Add(false);
		}
	}
}

void FExternalAsyncActionSpecCustomization::OnPopulateContainerCombo(TArray<TSharedPtr<FString>>& OutComboBoxStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems) const
{
	OutComboBoxStrings.Add(MakeShared<FString>(TEXT("None")));
	OutToolTips.Add(nullptr);
	OutRestrictedItems.Add(false);

	UObject* ClassValue = nullptr;
	if ( ClassHandle->GetValue(ClassValue) == FPropertyAccess::Success && ClassValue && Cast<UClass>(ClassValue)->IsChildOf(UBlueprintAsyncActionBase::StaticClass()))
	{
		for (TFieldIterator<FStructProperty> MemberIt( Cast<UClass>(ClassValue) ); MemberIt; ++MemberIt)
		{
			if (MemberIt->Struct->IsChildOf(FInstancedPropertyBag::StaticStruct()))
			{
				OutComboBoxStrings.Add( MakeShared<FString>( MemberIt->GetName() ) );
				OutToolTips.Add(nullptr);
				OutRestrictedItems.Add(false);
			}
		}
	}
}

bool FExternalAsyncActionSpecCustomization::IsValidClass() const
{
	UObject* ClassValue = nullptr;
	return ClassHandle->GetValue(ClassValue) == FPropertyAccess::Success && ClassValue != nullptr;
}
