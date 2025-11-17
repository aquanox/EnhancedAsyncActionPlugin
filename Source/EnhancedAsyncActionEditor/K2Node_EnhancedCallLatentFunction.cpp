// Copyright 2025, Aquanox.

#include "K2Node_EnhancedCallLatentFunction.h"
#include "EnhancedAsyncActionShared.h"
#include "EnhancedAsyncActionPrivate.h"
#include "EnhancedAsyncActionSettings.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"

void UK2Node_EnhancedCallLatentFunction::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* NodeClass = GetClass();
	if (!ActionRegistrar.IsOpenForRegistration(NodeClass) || !EAA::Switches::bWithLatent)
		return;

	struct GetMenuActions_Utils
	{
		static void SetNodeFunc(UEdGraphNode* NewNode, bool bIsTemplateNode, TWeakObjectPtr<UFunction> FunctionPtr)
		{
			if (FunctionPtr.IsValid())
			{
				CastChecked<ThisClass>(NewNode)->SetupFromFunction(FunctionPtr.Get());
			}
		}

		static void SetNodeExternal(UEdGraphNode* NewNode, bool bIsTemplateNode, TWeakObjectPtr<UFunction> FunctionPtr, FExternalLatentFunctionSpec Spec)
		{
			if (FunctionPtr.IsValid())
			{
				CastChecked<ThisClass>(NewNode)->SetupFromSpec(Spec);
			}
		}

		static void UiSpecCustomizer(FBlueprintActionContext const& Context, IBlueprintNodeBinder::FBindingSet const& Bindings, FBlueprintActionUiSpec* UiSpecOut)
		{
			UiSpecOut->MenuName = FText::Format(INVTEXT("{0} (Capture)"), UiSpecOut->MenuName);
		}
	};

	for (TObjectIterator<UFunction> It; It; ++It)
	{
		UFunction* const Function = *It;
		if (!Function->HasAllFunctionFlags(FUNC_BlueprintCallable))
			continue;
		if (!Function->HasMetaData(FBlueprintMetadata::MD_Latent))
			continue;

		UBlueprintNodeSpawner::FCustomizeNodeDelegate CustomizeNodeDelegate;
		UBlueprintNodeSpawner::FUiSpecOverrideDelegate DynamicUiSignatureGetter;

		if (Function->HasMetaData(EAA::Internals::MD_HasAsyncContext) || Function->HasMetaData(EAA::Internals::MD_HasLatentContext))
		{
			CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(&GetMenuActions_Utils::SetNodeFunc, MakeWeakObjectPtr(Function));
			DynamicUiSignatureGetter = UBlueprintNodeSpawner::FUiSpecOverrideDelegate::CreateStatic(&GetMenuActions_Utils::UiSpecCustomizer);
		}
		else if (auto* Spec = UEnhancedAsyncActionSettings::Get()->FindLatentSpecForFunction(Function))
		{
			CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(&GetMenuActions_Utils::SetNodeFunc, MakeWeakObjectPtr(Function));
			DynamicUiSignatureGetter = UBlueprintNodeSpawner::FUiSpecOverrideDelegate::CreateStatic(&GetMenuActions_Utils::UiSpecCustomizer);
		}

		if (CustomizeNodeDelegate.IsBound())
		{
			// hide from default
			Function->SetMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly, TEXT("true"));

			UBlueprintNodeSpawner* NodeSpawner = UBlueprintFunctionNodeSpawner::Create(Function);
			check(NodeSpawner != nullptr);
			NodeSpawner->NodeClass = NodeClass;
			NodeSpawner->CustomizeNodeDelegate = MoveTemp(CustomizeNodeDelegate);
			NodeSpawner->DynamicUiSignatureGetter = MoveTemp(DynamicUiSignatureGetter);
			ActionRegistrar.AddBlueprintAction(Function, NodeSpawner);
		}
	}
}

void UK2Node_EnhancedCallLatentFunction::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);
}

void UK2Node_EnhancedCallLatentFunction::SetupFromFunction(const UFunction* Function)
{
}

void UK2Node_EnhancedCallLatentFunction::SetupFromSpec(const FExternalLatentFunctionSpec& Spec)
{
}

void UK2Node_EnhancedCallLatentFunction::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
}

int32 UK2Node_EnhancedCallLatentFunction::GetNumCaptures() const
{
	return Captures.Num();
}

const TArray<FEnhancedAsyncTaskCapture>& UK2Node_EnhancedCallLatentFunction::GetCapturesArray() const
{
	return Captures;
}

TArray<FEnhancedAsyncTaskCapture>& UK2Node_EnhancedCallLatentFunction::GetMutableCapturesArray()
{
	return Captures;
}

bool UK2Node_EnhancedCallLatentFunction::CanAddPin() const
{
	return IK2Node_AddPinInterface::CanAddPin();
}

void UK2Node_EnhancedCallLatentFunction::AddInputPin()
{
}

bool UK2Node_EnhancedCallLatentFunction::CanRemovePin(const UEdGraphPin* Pin) const
{
	return IK2Node_AddPinInterface::CanRemovePin(Pin);
}

void UK2Node_EnhancedCallLatentFunction::RemoveInputPin(UEdGraphPin* Pin)
{
	IK2Node_AddPinInterface::RemoveInputPin(Pin);
}

void UK2Node_EnhancedCallLatentFunction::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
}
