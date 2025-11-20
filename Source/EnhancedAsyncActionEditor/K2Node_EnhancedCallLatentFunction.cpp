// Copyright 2025, Aquanox.

#include "K2Node_EnhancedCallLatentFunction.h"
#include "EnhancedAsyncContextShared.h"
#include "EnhancedAsyncContextPrivate.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "EnhancedAsyncContextLibrary.h"
#include "EnhancedLatentActionHandle.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_EnhancedAsyncTaskBase.h"
#include "K2Node_MakeArray.h"
#include "K2Node_Self.h"
#include "K2Node_TemporaryVariable.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KismetCompiler.h"

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

	for (TObjectIterator<UFunction> It; It; ++It)
	{
		UFunction* const Function = *It;
		if (!EAA::Internals::IsValidLatentCallable(Function) || !EAA::Switches::bEnableLatents)
			continue;

		UBlueprintNodeSpawner::FCustomizeNodeDelegate CustomizeNodeDelegate;
		UBlueprintNodeSpawner::FUiSpecOverrideDelegate DynamicUiSignatureGetter;

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

FText UK2Node_EnhancedCallLatentFunction::GetTooltipText() const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("FunctionTooltip"), Super::GetTooltipText());
	Args.Add(TEXT("CaptureString"), LOCTEXT("CaptureString", "Capture. This node supports capture context."));

	if (EAA::Switches::bDebugTooltips)
	{
		FString Config = BuildContextConfigString();
		Config.ReplaceInline(TEXT(";"), TEXT("\n"));
		Args.Add(TEXT("Extra"), FText::FromString(TEXT("\n\n") + Config));
	}
	else
	{
		Args.Add(TEXT("Extra"), INVTEXT(""));
	}
	return FText::Format(LOCTEXT("AsyncTaskTooltip", "{FunctionTooltip}\n\n{CaptureString}{Extra}"), Args);
}

void UK2Node_EnhancedCallLatentFunction::SetupFromFunction(const UFunction* Function)
{
	check(Function);
	SetFromFunction(Function);
}

void UK2Node_EnhancedCallLatentFunction::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	UEdGraphPin *OriginalInContextPin;
	if (GetContextPin(this, OriginalInContextPin))
	{
		OriginalInContextPin->bHidden = true;
	}

	const int32 NumCaptures = Captures.Num();
	Captures.Empty();

	for (int32 Index = 0; Index < NumCaptures; ++Index)
	{
		AddCaptureInternal();
	}
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

bool UK2Node_EnhancedCallLatentFunction::GetContextPin(UK2Node_CallFunction* Node, UEdGraphPin*& InPin)
{
	InPin = nullptr;

	auto* MetaValue = Node->GetTargetFunction()->FindMetaData(EAA::Internals::MD_HasLatentContext);
	if (!MetaValue) return false;

	auto IsValidPin = [Name = FName(*MetaValue->TrimStartAndEnd())](UEdGraphPin* Pin) -> bool
	{
		return Pin && Pin->Direction == EGPD_Input && Pin->PinName == Name
			&& Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct
			&& Pin->PinType.PinSubCategoryObject == FEnhancedLatentActionContextHandle::StaticStruct();
	};
	InPin = Node->FindPinByPredicate(IsValidPin);
	return InPin != nullptr;
}

bool UK2Node_EnhancedCallLatentFunction::IsEventMode(UK2Node_CallFunction* Node)
{
	auto* MetaValue = Node->GetTargetFunction()->FindMetaData(EAA::Internals::MD_RepeatableLatent);
	return MetaValue != nullptr;
}

bool UK2Node_EnhancedCallLatentFunction::ValidateCaptures(const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	bool bIsErrorFree = true;

	ForEachCapturePin(EGPD_Input, [&](int32 Index, UEdGraphPin* InputPin)
	{
		if (InputPin->LinkedTo.Num() && !EAA::Internals::IsCapturableType(InputPin->PinType))
		{
			const FString FormattedMessage = FText::Format(
				LOCTEXT("EnhancedLatentTaskBadInputType", "EnhancedCallLatentFunction: {0} is unsupported type for capture @@"),
				FText::FromName(InputPin->PinType.PinCategory)
			).ToString();
			CompilerContext.MessageLog.Error(*FormattedMessage, this);
			bIsErrorFree = false;
			return false;
		}
		return true;
	});

	return bIsErrorFree;
}

void UK2Node_EnhancedCallLatentFunction::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	check(SourceGraph && Schema);

	bool bIsErrorFree = true;

	if (!EAA::Internals::IsValidLatentCallable(GetTargetFunction()))
	{
		const FString FormattedMessage = LOCTEXT("CallLatentFunction", "CallLatentFunction: Bad function for node @@").ToString();
		CompilerContext.MessageLog.Error(*FormattedMessage, this);
		return;
	}

	if (!ValidateCaptures(Schema, CompilerContext, SourceGraph))
	{
		return;
	}

	UEdGraphPin* LastContextPin = nullptr;
	if (!GetContextPin(this, LastContextPin))
	{
		const FString FormattedMessage = LOCTEXT("CallLatentFunction", "CallLatentFunction: Context pin not found in node @@").ToString();
		CompilerContext.MessageLog.Error(*FormattedMessage, this);
		return;
	}

	// Toggle repeatable mode
	const bool bEventMode = IsEventMode(this) && EAA::Switches::bEnableRepeatableLatents;
	// Toggles Context feature used on this node
	const bool bContextRequired = AnyCapturePinHasLinks();

	LastContextPin = nullptr;
	UEdGraphPin* LastThenPin = nullptr;

	// Make context storage variable. Required as Handle is a ref.

	UK2Node_TemporaryVariable* LocalContextVar = CompilerContext.SpawnInternalVariable(this,
	   UEdGraphSchema_K2::PC_Struct, NAME_None, FEnhancedLatentActionContextHandle::StaticStruct(),
	   EPinContainerType::None, FEdGraphTerminalType());

	// create custom event node for retriggerable mode

	UK2Node_CustomEvent* OnTriggerCE = nullptr;
	if (bEventMode)
	{
		OnTriggerCE = CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this, SourceGraph);
		OnTriggerCE->CustomFunctionName = *FString::Printf(TEXT("OnTrigger_%s"), *CompilerContext.GetGuid(this));
		OnTriggerCE->AllocateDefaultPins();
		{
			const UFunction* Signature = TEnhancedRepeatableLatentAction<FPendingLatentAction>::GetDelegateSignature();
			for (TFieldIterator<FProperty> PropIt(Signature); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
			{
				const FProperty* Param = *PropIt;
				if (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm))
				{
					FEdGraphPinType PinType;
					bIsErrorFree &= Schema->ConvertPropertyToPinType(Param, /*out*/ PinType);
					bIsErrorFree &= (nullptr != OnTriggerCE->CreateUserDefinedPin(Param->GetFName(), PinType, EGPD_Output));
				}
			}
		}
	}

	// call create latent context node

	{
		auto* CallCreateContext = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		if (bContextRequired)
			CallCreateContext->SetFromFunction(UEnhancedAsyncContextLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UEnhancedAsyncContextLibrary, CreateContextForLatent)));
		else
			CallCreateContext->SetFromFunction(UEnhancedAsyncContextLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UEnhancedAsyncContextLibrary, CreateEmptyHandleForLatent)));
		CallCreateContext->AllocateDefaultPins();

		// Set Owner pin
		auto* CallSelf = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(this, SourceGraph);
		CallSelf->AllocateDefaultPins();

		bIsErrorFree &= Schema->TryCreateConnection(CallSelf->FindPinChecked(UEdGraphSchema_K2::PN_Self, EGPD_Output), CallCreateContext->FindPinChecked(TEXT("Owner"), EGPD_Input));

		// Set fixed value of The unique identifier for this latent action node in graph
		const int32 UUID = CompilerContext.MessageLog.CalculateStableIdentifierForLatentActionManager(this);
		Schema->TrySetDefaultValue(*CallCreateContext->FindPinChecked(TEXT("UUID"), EGPD_Input), FString::Printf(TEXT("%d"), UUID));

		// Make Random and connect to CallUUID pin
		auto* CallRandom = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallRandom->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, RandomInteger)));
		check(CallRandom->IsNodePure());
		CallRandom->AllocateDefaultPins();

		Schema->TrySetDefaultValue(*CallRandom->FindPinChecked(TEXT("Max"), EGPD_Input), FString::Printf(TEXT("%d"), INT_MAX - 1));
		bIsErrorFree &= Schema->TryCreateConnection(CallRandom->GetReturnValuePin(), CallCreateContext->FindPinChecked(TEXT("CallUUID"), EGPD_Input));

		if (bEventMode)
		{
			// Connect Delegate pin
			UEdGraphPin* FunctionPin = CallCreateContext->FindPinChecked(TEXT("Delegate"));
			UEdGraphPin* EventDelegatePin = OnTriggerCE->FindPin(UK2Node_CustomEvent::DelegateOutputName);
			bIsErrorFree &= FunctionPin && EventDelegatePin && Schema->TryCreateConnection(FunctionPin, EventDelegatePin);
		}

		bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *CallCreateContext->GetExecPin()).CanSafeConnect();
		LastThenPin = CallCreateContext->GetThenPin();

		LastContextPin = CallCreateContext->GetReturnValuePin();
	}

	// assign context to lvar
	{
		auto* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
		AssignNode->AllocateDefaultPins();

		bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, AssignNode->GetExecPin());

		bIsErrorFree &= Schema->TryCreateConnection(LocalContextVar->GetVariablePin(), AssignNode->GetVariablePin());
		AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());

		bIsErrorFree &= Schema->TryCreateConnection(AssignNode->GetValuePin(), LastContextPin);
		AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());

		LastThenPin = AssignNode->GetThenPin();

		LastContextPin = LocalContextVar->GetVariablePin();
	}

	if (bContextRequired && !EAA::Switches::bVariadicGetSet)
	{
		bIsErrorFree &= UK2Node_EnhancedAsyncTaskBase::HandleSetupContext(LastContextPin, LastThenPin, BuildContextConfigString(), this, Schema, CompilerContext, SourceGraph);
	}

	if (bContextRequired)
	{ // create set context data variadic
		TArray<FInputPinInfo> CaptureInputs;
		ForEachCapturePin(EGPD_Input, [&](int32 Index, UEdGraphPin* InputPin)
		{
			const bool bInUse = !EAA::Internals::IsWildcardType(InputPin->PinType);
			if (bInUse)
			{
				FInputPinInfo& Info = CaptureInputs.AddDefaulted_GetRef();
				Info.CaptureIndex = Index;
				Info.InputPin = InputPin;
			}
			return true;
		});

		if (EAA::Switches::bVariadicGetSet)
		{
			bIsErrorFree &= UK2Node_EnhancedAsyncTaskBase::HandleSetContextDataVariadic(CaptureInputs, LastContextPin, LastThenPin, this, Schema, CompilerContext, SourceGraph);
		}
		else
		{
			bIsErrorFree &= UK2Node_EnhancedAsyncTaskBase::HandleSetContextData(CaptureInputs, LastContextPin, LastThenPin, this, Schema, CompilerContext, SourceGraph);
		}
	}

	{ // call real latent function
		auto* CallLatent = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallLatent->SetFromFunction(GetTargetFunction());
		CallLatent->AllocateDefaultPins();

		UEdGraphPin *InContextPin;
		GetContextPin(CallLatent, InContextPin);

		// move normal links to inner node
		for (UEdGraphPin* Pin : CallLatent->Pins)
		{
			if (!Schema->IsMetaPin(*Pin) && Pin != InContextPin)
			{
				bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate( *FindPinChecked(Pin->PinName), *Pin ).CanSafeConnect();
			}
		}

		// connect input context pin
		bIsErrorFree &= Schema->TryCreateConnection(LocalContextVar->GetVariablePin(), InContextPin);

		// connect execs
		if (LastThenPin) // simply add to chain after last then
			bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, CallLatent->GetExecPin());
		else // this will be the first node - take the outside exec
			bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *CallLatent->GetExecPin()).CanSafeConnect();

		// Select where READ will happen. In normal mode - from Async Then, In event mode - from CustomEvent Then
		if (bEventMode)
			LastThenPin = OnTriggerCE->GetThenPin();
		else
			LastThenPin = CallLatent->GetThenPin();
	}

	// Select where to read context handle from
	if (bContextRequired && bEventMode)
	{
		LastContextPin = OnTriggerCE->FindPinByPredicate([](UEdGraphPin* Pin) -> bool
		{
			return Pin && Pin->Direction == EGPD_Output
				&& Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct
				&& Pin->PinType.PinSubCategoryObject == FEnhancedLatentActionContextHandle::StaticStruct();
		});
		check(LastContextPin);
	}
	else if (bContextRequired)
	{
		LastContextPin = LocalContextVar->GetVariablePin();
	}

	if (bContextRequired)
	{ // create get context data variadic
		TArray<FOutputPinInfo> CaptureOutputs;
		ForEachCapturePin(EGPD_Output, [&](int32 Index, UEdGraphPin* CurrentPin)
		{
			const bool bInUse = CurrentPin->LinkedTo.Num() && !EAA::Internals::IsWildcardType(CurrentPin->PinType);
			if (bInUse)
			{
				FOutputPinInfo& Info = CaptureOutputs.AddDefaulted_GetRef();
				Info.CaptureIndex = Index;
				Info.OutputPin = CurrentPin;
			}
			return true;
		});

		if (EAA::Switches::bVariadicGetSet)
		{
			bIsErrorFree &= UK2Node_EnhancedAsyncTaskBase::HandleGetContextDataVariadic(CaptureOutputs, LastContextPin, LastThenPin, this, Schema, CompilerContext, SourceGraph);
		}
		else
		{
			bIsErrorFree &= UK2Node_EnhancedAsyncTaskBase::HandleGetContextData(CaptureOutputs, LastContextPin, LastThenPin, this, Schema, CompilerContext, SourceGraph);
		}
	}

	if (bContextRequired)
	{ // make DestroyContext call as all data already written
		auto* CallDestroyContext = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallDestroyContext->SetFromFunction(UEnhancedAsyncContextLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UEnhancedAsyncContextLibrary, DestroyContextForLatent)));
		CallDestroyContext->AllocateDefaultPins();

		// connect input context pin
		bIsErrorFree &= Schema->TryCreateConnection(LastContextPin, CallDestroyContext->FindPinChecked(TEXT("Handle")));

		// connect execs
		bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, CallDestroyContext->GetExecPin());
		LastThenPin = CallDestroyContext->GetThenPin();
	}

	{ // move outer Then to last Then
		bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *LastThenPin).CanSafeConnect();
	}

	if (!bIsErrorFree)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("InternalConnectionError", "EnhancedLatentFunction: Internal connection error. @@").ToString(), this);
	}

	BreakAllNodeLinks();
}

#undef LOCTEXT_NAMESPACE
