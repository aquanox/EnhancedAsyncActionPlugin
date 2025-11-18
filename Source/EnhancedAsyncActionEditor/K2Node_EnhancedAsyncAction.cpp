// Copyright 2025, Aquanox.

#include "K2Node_EnhancedAsyncAction.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "EnhancedAsyncContextSettings.h"
#include "EnhancedAsyncContextShared.h"
#include "EnhancedAsyncContextPrivate.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_EnhancedAsyncAction)

UK2Node_EnhancedAsyncAction::UK2Node_EnhancedAsyncAction()
{
	ProxyActivateFunctionName = GET_FUNCTION_NAME_CHECKED(UBlueprintAsyncActionBase, Activate);
}

void UK2Node_EnhancedAsyncAction::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	struct GetMenuActions_Utils
	{
		static void SetNodeFunc(UEdGraphNode* NewNode, bool bIsTemplateNode, TWeakObjectPtr<UFunction> FunctionPtr)
		{
			UK2Node_EnhancedAsyncAction* AsyncTaskNode = CastChecked<UK2Node_EnhancedAsyncAction>(NewNode);
			if (FunctionPtr.IsValid())
			{
				UFunction* Func = FunctionPtr.Get();
				FObjectProperty* ReturnProp = CastFieldChecked<FObjectProperty>(Func->GetReturnProperty());

				AsyncTaskNode->ProxyFactoryFunctionName = Func->GetFName();
				AsyncTaskNode->ProxyFactoryClass        = Func->GetOuterUClass();
				AsyncTaskNode->ProxyClass               = ReturnProp->PropertyClass;

				AsyncTaskNode->ImportConfigFromClass(AsyncTaskNode->ProxyClass);
			}
		}

		static void SetNodeExternal(UEdGraphNode* NewNode, bool bIsTemplateNode, TWeakObjectPtr<UFunction> FunctionPtr, FExternalAsyncActionSpec Spec)
		{
			UK2Node_EnhancedAsyncAction* AsyncTaskNode = CastChecked<UK2Node_EnhancedAsyncAction>(NewNode);
			if (FunctionPtr.IsValid())
			{
				UFunction* Func = FunctionPtr.Get();
				FObjectProperty* ReturnProp = CastFieldChecked<FObjectProperty>(Func->GetReturnProperty());

				AsyncTaskNode->ProxyFactoryFunctionName = Func->GetFName();
				AsyncTaskNode->ProxyFactoryClass        = Func->GetOuterUClass();
				AsyncTaskNode->ProxyClass               = ReturnProp->PropertyClass;

				AsyncTaskNode->ImportConfigFromSpec(AsyncTaskNode->ProxyClass, Spec);
			}
		}

		static void UiSpecCustomizer(FBlueprintActionContext const& Context, IBlueprintNodeBinder::FBindingSet const& Bindings, FBlueprintActionUiSpec* UiSpecOut, bool bSuffix, bool bSpec)
		{
			if (bSuffix)
			{
				if (bSpec)
					UiSpecOut->MenuName = FText::Format(INVTEXT("{0} (Capture from Spec)"), UiSpecOut->MenuName);
				else
					UiSpecOut->MenuName = FText::Format(INVTEXT("{0} (Capture)"), UiSpecOut->MenuName);
			}
		}
	};

	UClass* NodeClass = GetClass();
	ActionRegistrar.RegisterClassFactoryActions<UBlueprintAsyncActionBase>(FBlueprintActionDatabaseRegistrar::FMakeFuncSpawnerDelegate::CreateLambda([NodeClass](const UFunction* FactoryFunc) -> UBlueprintNodeSpawner*
	{
		UClass* FactoryClass = FactoryFunc ? FactoryFunc->GetOwnerClass() : nullptr;
		if (!FactoryClass || !FactoryClass->IsChildOf(UBlueprintAsyncActionBase::StaticClass()) )
		{
			return nullptr;
		}

		TWeakObjectPtr<UFunction> FunctionPtr = MakeWeakObjectPtr(const_cast<UFunction*>(FactoryFunc));
		UBlueprintNodeSpawner::FCustomizeNodeDelegate CustomizeNodeDelegate;
		UBlueprintNodeSpawner::FUiSpecOverrideDelegate DynamicUiSignatureGetter;

		if (FactoryClass->HasMetaData(EAA::Internals::MD_HasAsyncContext))
		{
			bool bSuffix = !FactoryClass->HasMetaData(TEXT("HasDedicatedAsyncNode"));
			CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(&GetMenuActions_Utils::SetNodeFunc, FunctionPtr);
			DynamicUiSignatureGetter = UBlueprintNodeSpawner::FUiSpecOverrideDelegate::CreateStatic(&GetMenuActions_Utils::UiSpecCustomizer, bSuffix, false);
		}
		else if (auto* Spec = UEnhancedAsyncContextSettings::Get()->FindActionSpecForClass(FactoryClass))
		{
			bool bSuffix = true;
			CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(&GetMenuActions_Utils::SetNodeExternal, FunctionPtr, *Spec);
			DynamicUiSignatureGetter = UBlueprintNodeSpawner::FUiSpecOverrideDelegate::CreateStatic(&GetMenuActions_Utils::UiSpecCustomizer, bSuffix, true);
		}

		if (CustomizeNodeDelegate.IsBound())
		{
			UBlueprintNodeSpawner* NodeSpawner = UBlueprintFunctionNodeSpawner::Create(FactoryFunc);
			check(NodeSpawner != nullptr);
			NodeSpawner->NodeClass = NodeClass;
			NodeSpawner->CustomizeNodeDelegate = MoveTemp(CustomizeNodeDelegate);
			NodeSpawner->DynamicUiSignatureGetter = MoveTemp(DynamicUiSignatureGetter);
			return NodeSpawner;
		}
		return nullptr;
	}) );
}

void UK2Node_EnhancedAsyncAction::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);
}
