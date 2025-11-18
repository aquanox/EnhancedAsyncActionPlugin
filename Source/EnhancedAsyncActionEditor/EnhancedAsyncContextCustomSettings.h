// Copyright 2025, Aquanox.

#pragma once

#include "EnhancedAsyncContextSettings.h"
#include "IPropertyTypeCustomization.h"

class SToolTip;

class FPropertyTypeCustomizationShared : public IPropertyTypeCustomization
{
	using Super = IPropertyTypeCustomization;
public:
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		RootHandle = PropertyHandle;
		PropertyUtils = CustomizationUtils.GetPropertyUtilities();
	}

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}

protected:
	TSharedPtr<IPropertyHandle> RootHandle;
	TSharedPtr<IPropertyUtilities> PropertyUtils;
};

class FExternalAsyncActionSpecCustomization : public FPropertyTypeCustomizationShared
{
	using Super = FPropertyTypeCustomizationShared;
	using ThisClass = FExternalAsyncActionSpecCustomization;
public:
	static FName GetTypeName()
	{
		return FExternalAsyncActionSpec::StaticStruct()->GetFName();
	}
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShared<ThisClass>();
	}

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	void OnClassChanged();
	void OnPopulateContextCombo(TArray<TSharedPtr<FString>>& OutComboBoxStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems) const;
	void OnPopulateContainerCombo(TArray<TSharedPtr<FString>>& OutComboBoxStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems) const;
	bool IsValidClass() const;

private:
	TSharedPtr<IPropertyHandle> ClassHandle;
	TSharedPtr<IPropertyHandle> ContextHandle;
	TSharedPtr<IPropertyHandle> ContainerHandle;
};
