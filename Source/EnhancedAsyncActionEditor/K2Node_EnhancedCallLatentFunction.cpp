// Copyright 2025, Aquanox.

#include "K2Node_EnhancedCallLatentFunction.h"
#include "EnhancedAsyncContextShared.h"
#include "EnhancedAsyncContextPrivate.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "EnhancedLatentActionHandle.h"

#define LOCTEXT_NAMESPACE "UK2Node_EnhancedCallLatentFunction"

void UK2Node_EnhancedCallLatentFunction::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* NodeClass = GetClass();
	if (!ActionRegistrar.IsOpenForRegistration(NodeClass))
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

		static void UiSpecCustomizer(FBlueprintActionContext const& Context, IBlueprintNodeBinder::FBindingSet const& Bindings, FBlueprintActionUiSpec* UiSpecOut)
		{
			UiSpecOut->MenuName = FText::Format(INVTEXT("{0} (Capture)"), UiSpecOut->MenuName);
		}
	};

	const UStruct* RequiredReturnParamType = FLatentCallResult::StaticStruct();

	for (TObjectIterator<UFunction> It; It; ++It)
	{
		UFunction* const Function = *It;
		if (!Function->HasAllFunctionFlags(FUNC_BlueprintCallable))
			continue;
		if (!Function->HasMetaData(FBlueprintMetadata::MD_Latent))
			continue;

		auto ReturnProp = CastField<FStructProperty>(Function->GetReturnProperty());
		if (!ReturnProp || ReturnProp->Struct != RequiredReturnParamType)
			continue;

		UBlueprintNodeSpawner::FCustomizeNodeDelegate CustomizeNodeDelegate;
		UBlueprintNodeSpawner::FUiSpecOverrideDelegate DynamicUiSignatureGetter;

		if (Function->HasMetaData(EAA::Internals::MD_HasAsyncContext) || Function->HasMetaData(EAA::Internals::MD_HasLatentContext))
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
	static const FName NodeSection = FName("LatentContextNode");
	static const FText NodeSectionDesc = LOCTEXT("LatentContextNode", "Async Context Node");

	Super::GetNodeContextMenuActions(Menu, Context);

	ThisClass* const MutableThis = const_cast<ThisClass*>(this);

	FK2Node_AsyncContextMenuActions::SetupActions(MutableThis, Menu, Context);

}

void UK2Node_EnhancedCallLatentFunction::SetupFromFunction(const UFunction* Function)
{
}

void UK2Node_EnhancedCallLatentFunction::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	auto ReturnPin = GetReturnValuePin();
	check(ReturnPin && ReturnPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct);
	ReturnPin->PinFriendlyName = INVTEXT("Latent Result");
	ReturnPin->bHidden = true;
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

bool UK2Node_EnhancedCallLatentFunction::CanSplitPin(const UEdGraphPin* Pin) const
{
	return Super::CanSplitPin(Pin) && !IsCapturePin(Pin);
}

bool UK2Node_EnhancedCallLatentFunction::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	if (!TestConnectPins(MyPin, OtherPin, OutReason))
	{
		return true;
	}
	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_EnhancedCallLatentFunction::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Modify();

	Super::PinConnectionListChanged(Pin);

	if (IsCapturePin(Pin) && !Pin->IsPendingKill())
	{
		SynchronizeCapturePinType(Pin);
	}
}

void UK2Node_EnhancedCallLatentFunction::PinTypeChanged(UEdGraphPin* Pin)
{
	if (IsCapturePin(Pin) && !Pin->IsPendingKill())
	{
		SynchronizeCapturePinType(Pin);
	}
	Super::PinTypeChanged(Pin);
}

void UK2Node_EnhancedCallLatentFunction::PostReconstructNode()
{
	Super::PostReconstructNode();

	if (!IsTemplate())
	{
		UEdGraph* OuterGraph = GetGraph();
		if (OuterGraph && OuterGraph->Schema)
		{
			SyncCapturePinTypes();
		}
	}
}

void UK2Node_EnhancedCallLatentFunction::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
}

#undef LOCTEXT_NAMESPACE
