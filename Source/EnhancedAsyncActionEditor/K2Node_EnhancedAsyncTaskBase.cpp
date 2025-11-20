// Copyright 2025, Aquanox.

#include "K2Node_EnhancedAsyncTaskBase.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "EnhancedAsyncContextLibrary.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_TemporaryVariable.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "ScopedTransaction.h"
#include "EnhancedAsyncContextShared.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CustomEvent.h"
#include "EnhancedAsyncContextPrivate.h"
#include "EnhancedAsyncContextSettings.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_MakeArray.h"
#include "ToolMenu.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_EnhancedAsyncTaskBase)

#define LOCTEXT_NAMESPACE "UK2Node_EnhancedAsyncTaskBase"

UK2Node_EnhancedAsyncTaskBase::UK2Node_EnhancedAsyncTaskBase()
{
}

void UK2Node_EnhancedAsyncTaskBase::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
}

void UK2Node_EnhancedAsyncTaskBase::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	static const FName NodeSection = FName("AsyncContextNode");
	static const FText NodeSectionDesc = LOCTEXT("AsyncContextNode", "Async Context Node");

	Super::GetNodeContextMenuActions(Menu, Context);

	ThisClass* const MutableThis = const_cast<ThisClass*>(this);

	FK2Node_AsyncContextMenuActions::SetupActions(MutableThis, Menu, Context);

	if (!Context->Pin)
	{
		FToolMenuSection& Section = Menu->FindOrAddSection(NodeSection, NodeSectionDesc);
		Section.AddMenuEntry(
			"ContextToggle",
			MakeAttributeUObject(MutableThis, &ThisClass::ToggleContextPinStateLabel),
			LOCTEXT("ContextToggleTooltip", "Toggle context pin visibility"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateUObject(MutableThis, &ThisClass::ToggleContextPinState)
			)
		);
	}
}

FText UK2Node_EnhancedAsyncTaskBase::GetTooltipText() const
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

void UK2Node_EnhancedAsyncTaskBase::ImportConfigFromClass(UClass* InClass)
{
	const FString* ContextParamName = EAA::Internals::FindMetadataHierarchical(InClass, EAA::Internals::MD_HasAsyncContext);
	check(ContextParamName);
	AsyncContextParameterName = **ContextParamName;

	if (const FString* ContextPropName = EAA::Internals::FindMetadataHierarchical(InClass, EAA::Internals::MD_AsyncContextContainer))
	{
		// search for member property with explicitly specified with AsyncContextContainer or having same name as AsyncContext
		if (EAA::Internals::IsValidContainerProperty(InClass, **ContextPropName))
		{
			AsyncContextContainerProperty = **ContextPropName;
		}
	}
	else if (EAA::Internals::IsValidContainerProperty(InClass, AsyncContextParameterName))
	{
		AsyncContextContainerProperty = AsyncContextParameterName;
	}
	else
	{
		AsyncContextContainerProperty = NAME_None;
	}

	bExposeContextParameter = EAA::Internals::FindMetadataHierarchical(InClass, EAA::Internals::MD_ExposedAsyncContext) != nullptr;
}

void UK2Node_EnhancedAsyncTaskBase::ImportConfigFromSpec(UClass* InClass, const FExternalAsyncActionSpec& InSpec)
{
	AsyncContextParameterName = InSpec.ContextPropertyName;
	bExposeContextParameter = InSpec.bExposedContext;

	if (!InSpec.ContainerPropertyName.IsNone() && EAA::Internals::IsValidContainerProperty(InClass, InSpec.ContainerPropertyName))
	{
		AsyncContextContainerProperty = InSpec.ContainerPropertyName;
	}
	else if (EAA::Internals::IsValidContainerProperty(InClass, AsyncContextParameterName))
	{
		AsyncContextContainerProperty = AsyncContextParameterName;
	}
	else
	{
		AsyncContextContainerProperty = NAME_None;
	}
}

void UK2Node_EnhancedAsyncTaskBase::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Optionally hide the async context pin if requested
	SyncContextPinState();

	const int32 NumCaptures = Captures.Num();
	Captures.Empty();

	for (int32 Index = 0; Index < NumCaptures; ++Index)
	{
		AddCaptureInternal();
	}
}

const TArray<FEnhancedAsyncTaskCapture>& UK2Node_EnhancedAsyncTaskBase::GetCapturesArray() const
{
	return Captures;
}

TArray<FEnhancedAsyncTaskCapture>& UK2Node_EnhancedAsyncTaskBase::GetMutableCapturesArray()
{
	return Captures;
}

int32 UK2Node_EnhancedAsyncTaskBase::GetNumCaptures() const
{
	return Captures.Num();
}

bool UK2Node_EnhancedAsyncTaskBase::HasContextExposed() const
{
	return bExposeContextParameter;
}

bool UK2Node_EnhancedAsyncTaskBase::IsContextPin(const UEdGraphPin* Pin) const
{
	return Pin && Pin->PinName == AsyncContextParameterName
		&& Pin->Direction == EGPD_Output
		&& Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object;
}

bool UK2Node_EnhancedAsyncTaskBase::CanSplitPin(const UEdGraphPin* Pin) const
{
	return Super::CanSplitPin(Pin) && !IsCapturePin(Pin);
}

bool UK2Node_EnhancedAsyncTaskBase::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	if (!TestConnectPins(MyPin, OtherPin, OutReason))
	{
		return true;
	}
	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

FText UK2Node_EnhancedAsyncTaskBase::ToggleContextPinStateLabel() const
{
	return HasContextExposed()
		? LOCTEXT("ContextToggleOff", "Hide context pin")
		: LOCTEXT("ContextToggleOn", "Show context pin");
}

void UK2Node_EnhancedAsyncTaskBase::ToggleContextPinState()
{
	FScopedTransaction Transaction(LOCTEXT("SetContextPinStateTx", "ToggleContextPinState"));
	Modify();

	bExposeContextParameter = !bExposeContextParameter;
	SyncContextPinState();

	GetGraph()->NotifyNodeChanged(this);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

void UK2Node_EnhancedAsyncTaskBase::SyncContextPinState()
{
	const bool bHidden = !bExposeContextParameter;

	if (UEdGraphPin* ContextPin = FindPin(AsyncContextParameterName, EGPD_Output))
	{
		if (ContextPin->bHidden != bHidden)
		{
			ContextPin->Modify();
			ContextPin->bHidden = bHidden;

			if (bHidden && ContextPin->LinkedTo.Num())
			{
				ContextPin->BreakAllPinLinks();
			}

			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
		}
	}
}

void UK2Node_EnhancedAsyncTaskBase::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Modify();

	Super::PinConnectionListChanged(Pin);

	if (IsCapturePin(Pin) && !Pin->IsPendingKill())
	{
		SynchronizeCapturePinType(Pin);
	}
}

void UK2Node_EnhancedAsyncTaskBase::PinTypeChanged(UEdGraphPin* Pin)
{
	if (IsCapturePin(Pin) && !Pin->IsPendingKill())
	{
		SynchronizeCapturePinType(Pin);
	}
	Super::PinTypeChanged(Pin);
}

void UK2Node_EnhancedAsyncTaskBase::PostReconstructNode()
{
	Super::PostReconstructNode();

	if (!IsTemplate())
	{
		SyncContextPinState();

		UEdGraph* OuterGraph = GetGraph();
		if (OuterGraph && OuterGraph->Schema)
		{
			SyncCapturePinTypes();
		}
	}
}

bool UK2Node_EnhancedAsyncTaskBase::ValidateDelegates(
    const UEdGraphSchema_K2* Schema, class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	const FName& ContextPinName = AsyncContextParameterName;
	if (ContextPinName.IsNone())
	{
		const FString FormattedMessage = FText::Format(
			LOCTEXT("AsyncTaskBadContext", "EnhancedAsyncAction: {0} Context property is missing in @@"),
			FText::FromString(ProxyClass.GetName())
		).ToString();

		CompilerContext.MessageLog.Error(*FormattedMessage, this);
		return false;
	}

	auto IsValidDelegate = [&ContextPinName](const FMulticastDelegateProperty* Prop) -> bool
	{
		auto SigProp = CastField<FObjectProperty>(Prop->SignatureFunction->FindPropertyByName(ContextPinName));
		return SigProp != nullptr;
	};

	bool bIsErrorFree = true;
	for (TFieldIterator<FMulticastDelegateProperty> PropertyIt(ProxyClass); PropertyIt; ++PropertyIt)
	{
		const FMulticastDelegateProperty* Property = *PropertyIt;
		if (!IsValidDelegate(Property))
		{
			bIsErrorFree = false;

			const FString FormattedMessage = FText::Format(
				LOCTEXT("AsyncTaskBadDelegate", "EnhancedAsyncAction: Function signature of {0}::{1} does not have required context parameter {2} in @@"),
				FText::FromString(ProxyClass.GetName()),
				FText::FromString(Property->GetName()),
				FText::FromName(ContextPinName)
			).ToString();

			CompilerContext.MessageLog.Error(*FormattedMessage, this);

			break;
		}
	}
	return bIsErrorFree;
}

bool UK2Node_EnhancedAsyncTaskBase::ValidateCaptures(const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	bool bIsErrorFree = true;

	ForEachCapturePin(EGPD_Input, [&](int32 Index, UEdGraphPin* InputPin)
	{
		if (InputPin->LinkedTo.Num() && !EAA::Internals::IsCapturableType(InputPin->PinType))
		{
			const FString FormattedMessage = FText::Format(
				LOCTEXT("EnhancedAsyncTaskBadInputType", "EnhancedAsyncTaskBase: {0} is unsupported type for capture @@"),
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

bool UK2Node_EnhancedAsyncTaskBase::HandleSetupContext(
	UEdGraphPin* InContextHandlePin, UEdGraphPin*& InOutLastThenPin, FString Config,
	UK2Node* Self, const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	bool bIsErrorFree = true;

	// Create Make Literal String function
	UK2Node_CallFunction* const CallMakeLiteral = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Self, SourceGraph);
	CallMakeLiteral->FunctionReference.SetExternalMember(
		GET_MEMBER_NAME_CHECKED(UKismetSystemLibrary, MakeLiteralString),
		UKismetSystemLibrary::StaticClass()
	);
	CallMakeLiteral->AllocateDefaultPins();

	auto LiteralValuePin = CallMakeLiteral->FindPinChecked(TEXT("Value"), EGPD_Input);
	LiteralValuePin->DefaultValue = Config;

	// Create a call to context setup
	UK2Node_CallFunction* const CallSetupNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Self, SourceGraph);
	CallSetupNode->FunctionReference.SetExternalMember(
		GET_MEMBER_NAME_CHECKED(UEnhancedAsyncContextLibrary, SetupContextContainer),
		UEnhancedAsyncContextLibrary::StaticClass()
	);
	CallSetupNode->AllocateDefaultPins();

	UEdGraphPin* HandlePin = CallSetupNode->FindPinChecked(EAA::Internals::PIN_Handle);
	if (!Schema->CanCreateConnection(InContextHandlePin, HandlePin).CanSafeConnect())
	{
		bIsErrorFree &= Schema->CreateAutomaticConversionNodeAndConnections(InContextHandlePin, HandlePin);
	}
	else
	{
		bIsErrorFree &= Schema->TryCreateConnection(InContextHandlePin, HandlePin);
	}
	CallSetupNode->NotifyPinConnectionListChanged(HandlePin);

	// Set config value
	UEdGraphPin* ConfigPin = CallSetupNode->FindPinChecked(TEXT("Config"));
	// Schema->TrySetDefaultValue(*ConfigPin, CapturesConfigString);
	bIsErrorFree &= Schema->TryCreateConnection(CallMakeLiteral->GetReturnValuePin(), ConfigPin);

	// Connect [CreateContext] and [SetupContext]
	bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, CallSetupNode->GetExecPin());
	InOutLastThenPin = CallSetupNode->GetThenPin();

	return bIsErrorFree;
}

bool UK2Node_EnhancedAsyncTaskBase::HandleSetContextData(
	const TArray<FInputPinInfo>& CaptureInputs, UEdGraphPin* InContextHandlePin, UEdGraphPin*& InOutLastThenPin,
	UK2Node* Self, const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	bool bIsErrorFree = true;
	for (const FInputPinInfo& Info : CaptureInputs)
	{
		UEdGraphPin* const InputPin = Info.InputPin;

		UK2Node_CallFunction* CallNode = nullptr;

		FName SetValueFunc;
		if (EAA::Internals::SelectAccessorForType(InputPin, EAA::Internals::EAccessorRole::SETTER, SetValueFunc))
		{
			if (InputPin->PinType.IsArray())
			{
				CallNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(Self, SourceGraph);
			}
			else
			{
				CallNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Self, SourceGraph);
			}

			CallNode->FunctionReference.SetExternalMember(SetValueFunc, UEnhancedAsyncContextLibrary::StaticClass());
			CallNode->AllocateDefaultPins();
			ensure(CallNode->GetTargetFunction() != nullptr);
		}

		if (!CallNode)
		{
			const FString FormattedMessage = FText::Format(
				LOCTEXT("AsyncTaskNoSetter", "EnhancedAsyncTaskBase: no SETTER for {0} in async task @@"),
				FText::FromName(InputPin->PinType.PinCategory)
			).ToString();
			CompilerContext.MessageLog.Error(*FormattedMessage, Self);
			bIsErrorFree = false;
			continue;
		}

		// match function inputs, to pass data to function from CallFunction node

		// Set the handle parameter
		UEdGraphPin* HandlePin = CallNode->FindPinChecked(EAA::Internals::PIN_Handle);
		if (!Schema->CanCreateConnection(InContextHandlePin, HandlePin).CanSafeConnect())
		{
			bIsErrorFree &= Schema->CreateAutomaticConversionNodeAndConnections(InContextHandlePin, HandlePin);
		}
		else
		{
			bIsErrorFree &= Schema->TryCreateConnection(InContextHandlePin, HandlePin);
		}
		CallNode->NotifyPinConnectionListChanged(HandlePin);

		UEdGraphPin* IndexPin = CallNode->FindPinChecked(EAA::Internals::PIN_Index);
		Schema->TrySetDefaultValue(*IndexPin, FString::Printf(TEXT("%d"), Info.CaptureIndex));

		UEdGraphPin* ValuePin = CallNode->FindPinChecked(EAA::Internals::PIN_Value);

		if (!InputPin->LinkedTo.Num())
		{ // create a dummy local input
			ValuePin->PinType = InputPin->PinType;
			ValuePin->PinType.bIsReference = true;
			UK2Node_CallFunction::InnerHandleAutoCreateRef(Self, ValuePin, CompilerContext, SourceGraph, false);
		}
		else
		{ // take link from outside
			bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*InputPin, *ValuePin).CanSafeConnect();
		}
		CallNode->NotifyPinConnectionListChanged(ValuePin);

		// Connect [Last] and [Set]
		bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, CallNode->GetExecPin());
		InOutLastThenPin = CallNode->GetThenPin();
	}

	return bIsErrorFree;
}

bool UK2Node_EnhancedAsyncTaskBase::HandleSetContextDataVariadic(
	const TArray<FInputPinInfo>& CaptureInputs, UEdGraphPin* InContextHandlePin, UEdGraphPin*& InOutLastThenPin,
	UK2Node* Self, const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	bool bIsErrorFree = true;

	// Create a "Make Array" node to compile the list of arguments into an array
	UK2Node_MakeArray* MakeArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(Self, SourceGraph);
	MakeArrayNode->NumInputs = CaptureInputs.Num();
	MakeArrayNode->AllocateDefaultPins();

	// Create Call "Set Context Variadic"
	UK2Node_CallFunction* CallSetVariadic = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Self, SourceGraph);
	CallSetVariadic->FunctionReference.SetExternalMember(
		GET_MEMBER_NAME_CHECKED(UEnhancedAsyncContextLibrary, Handle_SetValue_Variadic),
		UEnhancedAsyncContextLibrary::StaticClass()
	);
	CallSetVariadic->AllocateDefaultPins();

	int32 ArrayInputIndex = 0;
	for (const FInputPinInfo& Info : CaptureInputs)
	{
		const FName VariadicPropertyName = EAA::Internals::IndexToName(Info.CaptureIndex);

		UEdGraphPin* const InputPin = Info.InputPin;
		check(InputPin->Direction == EGPD_Input);

		// Create the "Make Literal String" node
		UK2Node_CallFunction* MakeLiteralStringNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Self, SourceGraph);
		MakeLiteralStringNode->FunctionReference.SetExternalMember(
			GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeLiteralString),
			UKismetSystemLibrary::StaticClass()
		);
		MakeLiteralStringNode->AllocateDefaultPins();

		// Set the literal value to the capture parameter name
		UEdGraphPin* MakeLiteralStringValuePin = MakeLiteralStringNode->FindPinChecked(TEXT("Value"), EGPD_Input);
		Schema->TrySetDefaultValue(*MakeLiteralStringValuePin, VariadicPropertyName.ToString());

		// Find the input pin on the "Make Array" node by index and link it to the literal string
		UEdGraphPin* ArrayIn = MakeArrayNode->FindPinChecked(MakeArrayNode->GetPinName(ArrayInputIndex++));
		bIsErrorFree &= Schema->TryCreateConnection(MakeLiteralStringNode->GetReturnValuePin(), ArrayIn);

		FEdGraphPinType PinType = InputPin->PinType;
		PinType.bIsReference = true;
		PinType.bIsConst = true;

		// Create variadic pin on setter function
		UEdGraphPin* ValuePin = CallSetVariadic->CreatePin(EGPD_Input, PinType, VariadicPropertyName);

		if (!InputPin->LinkedTo.Num())
		{ // create a dummy local input
			UK2Node_CallFunction::InnerHandleAutoCreateRef(Self, ValuePin, CompilerContext, SourceGraph, false);
		}
		else
		{ // Move Capture pin to generated intermediate
			bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*InputPin, *ValuePin).CanSafeConnect();
		}

		CallSetVariadic->NotifyPinConnectionListChanged(ValuePin);
	}

	// Set the handle parameter
	UEdGraphPin* HandlePin = CallSetVariadic->FindPinChecked(EAA::Internals::PIN_Handle);
	if (!Schema->CanCreateConnection(InContextHandlePin, HandlePin).CanSafeConnect())
	{
		bIsErrorFree &= Schema->CreateAutomaticConversionNodeAndConnections(InContextHandlePin, HandlePin);
	}
	else
	{
		bIsErrorFree &= Schema->TryCreateConnection(InContextHandlePin, HandlePin);
	}
	CallSetVariadic->NotifyPinConnectionListChanged(HandlePin);

	// Set the names parameter and force the makearray pin type
	UEdGraphPin* ArrayOut = MakeArrayNode->GetOutputPin();
	bIsErrorFree &= Schema->TryCreateConnection(ArrayOut, CallSetVariadic->FindPinChecked(TEXT("Names")));
	MakeArrayNode->PinConnectionListChanged(ArrayOut);

	// Connect execs
	bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, CallSetVariadic->GetExecPin());
	InOutLastThenPin = CallSetVariadic->GetThenPin();

	return bIsErrorFree;
}

bool UK2Node_EnhancedAsyncTaskBase::HandleActionDelegates(
	UK2Node_CallFunction* CallCreateProxyObjectNode, UEdGraphPin*& InOutLastThenPin,
	const UEdGraphSchema_K2* Schema, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext)
{
	UEdGraphPin* const OutputAsyncTaskProxy = FindPin(FBaseAsyncTaskHelper::GetAsyncTaskProxyName());
	UEdGraphPin* const ProxyObjectPin = CallCreateProxyObjectNode->GetReturnValuePin();

	bool bIsErrorFree = true;

	TArray<FOutputPinInfo> VariableOutputs;
	TArray<FOutputPinInfo> CaptureOutputs;

	// GATHER OUTPUT PARAMETERS AND PAIR THEM WITH LOCAL VARIABLES
	{
		bool bPassedFactoryOutputs = false;

		TArray<UEdGraphPin*> StandardOutputPins;
		GetStandardPins(EGPD_Output, StandardOutputPins);
		for (UEdGraphPin* CurrentPin : StandardOutputPins)
		{
			const bool bIsProxyPin  = (OutputAsyncTaskProxy == CurrentPin);
			const bool bIsContextPin = IsContextPin(CurrentPin);

			if (!bIsProxyPin && FBaseAsyncTaskHelper::ValidDataPin(CurrentPin, EGPD_Output))
			{
				if (!bPassedFactoryOutputs)
				{
					UEdGraphPin* DestPin = CallCreateProxyObjectNode->FindPin(CurrentPin->PinName);
					bIsErrorFree &= DestPin && CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *DestPin).CanSafeConnect();
					continue;
				}
				// exclude context pin from making temporary var
				if (!bIsContextPin || (bIsContextPin && HasContextExposed()))
				{
					const FEdGraphPinType& PinType = CurrentPin->PinType;

					UK2Node_TemporaryVariable* TempVarOutput = CompilerContext.SpawnInternalVariable(this, PinType.PinCategory, PinType.PinSubCategory, PinType.PinSubCategoryObject.Get(), PinType.ContainerType, PinType.PinValueType);
					// move external pins to the temporary variable
					bIsErrorFree &= TempVarOutput->GetVariablePin() && CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *TempVarOutput->GetVariablePin()).CanSafeConnect();
					VariableOutputs.Add(FOutputPinInfo { INDEX_NONE, CurrentPin, TempVarOutput });
				}
			}
			else if (!bPassedFactoryOutputs && CurrentPin && CurrentPin->Direction == EGPD_Output)
			{
				// the first exec that isn't the node's then pin is the start of the asyc delegate pins
				// once we hit this point, we've iterated beyond all outputs for the factory function
				bPassedFactoryOutputs = (CurrentPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) && (CurrentPin->PinName != UEdGraphSchema_K2::PN_Then);
			}
		}
	}

	// GATHER CAPTURE PARAMETERS AND PAIR THEM WITH LOCAL VARIABLES
	{
		ForEachCapturePin(EGPD_Output, [&](int32 Index, UEdGraphPin* CurrentPin)
		{
			const bool bInUse = CurrentPin->LinkedTo.Num() && !EAA::Internals::IsWildcardType(CurrentPin->PinType);
			if (bInUse)
			{
				const FEdGraphPinType& PinType = CurrentPin->PinType;
				UK2Node_TemporaryVariable* TempVarOutput = CompilerContext.SpawnInternalVariable(this, PinType.PinCategory, PinType.PinSubCategory, PinType.PinSubCategoryObject.Get(), PinType.ContainerType, PinType.PinValueType);
				// move external pins to the temporary variable
				bIsErrorFree &= TempVarOutput->GetVariablePin() && CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *TempVarOutput->GetVariablePin()).CanSafeConnect();

				FOutputPinInfo& Info = CaptureOutputs.AddDefaulted_GetRef();
				Info.CaptureIndex = Index;
				Info.OutputPin = CurrentPin;
				Info.TempVar = TempVarOutput;
			}
			return true;
		});
	}

	for (TFieldIterator<FMulticastDelegateProperty> PropertyIt(ProxyClass); PropertyIt && bIsErrorFree; ++PropertyIt)
	{
		FMulticastDelegateProperty* const CurrentProperty = *PropertyIt;

		UEdGraphPin* OutLastActivatedThenPin = nullptr;

		UEdGraphPin* PinForCurrentDelegateProperty = FindPin(CurrentProperty->GetFName());
		if (!PinForCurrentDelegateProperty || (UEdGraphSchema_K2::PC_Exec != PinForCurrentDelegateProperty->PinType.PinCategory))
		{
			FText ErrorMessage = FText::Format(LOCTEXT("WrongDelegateProperty", "EnhancedAsyncTaskBase: Cannot find execution pin for delegate "), FText::FromString(CurrentProperty->GetName()));
			CompilerContext.MessageLog.Error(*ErrorMessage.ToString(), this);
			return false;
		}

		/// ==================
		///  [Node] - [Add Delegate]
		///             |
		///  [CustomEvent] - [FindCaptureContextForObject] - [ assign context vars ] - [ assign event vars ]
		/// ==================

		// Create custom event node matching the signature
		UK2Node_CustomEvent* CurrentCENode = CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this, SourceGraph);

		// Register node with async action
		{
			UK2Node_AddDelegate* AddDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(this, SourceGraph);
			AddDelegateNode->SetFromProperty(CurrentProperty, false, CurrentProperty->GetOwnerClass());
			AddDelegateNode->AllocateDefaultPins();
			bIsErrorFree &= Schema->TryCreateConnection(AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Self), ProxyObjectPin);
			bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
			InOutLastThenPin = AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);
			CurrentCENode->CustomFunctionName = *FString::Printf(TEXT("%s_%s"), *CurrentProperty->GetName(), *CompilerContext.GetGuid(this));
			CurrentCENode->AllocateDefaultPins();

			bIsErrorFree &= FBaseAsyncTaskHelper::CreateDelegateForNewFunction(AddDelegateNode->GetDelegatePin(), CurrentCENode->GetFunctionName(), this, SourceGraph, CompilerContext);
			bIsErrorFree &= FBaseAsyncTaskHelper::CopyEventSignature(CurrentCENode, AddDelegateNode->GetDelegateSignature(), Schema);
		}

		OutLastActivatedThenPin = CurrentCENode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

		UEdGraphPin* ContextHandlePin = nullptr;

		// Create context loader
		{
			UK2Node_CallFunction* const CallGetCtx = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			CallGetCtx->FunctionReference.SetExternalMember(
				GET_MEMBER_NAME_CHECKED(UEnhancedAsyncContextLibrary, FindCaptureContextForObject),
				UEnhancedAsyncContextLibrary::StaticClass());
			CallGetCtx->AllocateDefaultPins();

			UEdGraphPin* PinWithData = CurrentCENode->FindPin(AsyncContextParameterName);
			if (PinWithData == nullptr)
			{
				FText ErrorMessage = FText::Format(
					LOCTEXT("MissingContextPin", "EnhancedAsyncTaskBase: Node @@ was expecting a data output pin named {0} on @@ (each delegate must have the same signature)"),
					FText::FromName(AsyncContextParameterName));
				CompilerContext.MessageLog.Error(*ErrorMessage.ToString(), this, CurrentCENode);
				return false;
			}

			ContextHandlePin = CallGetCtx->FindPinChecked(EAA::Internals::PIN_Handle);

			bIsErrorFree &= Schema->TryCreateConnection(PinWithData, CallGetCtx->FindPinChecked(EAA::Internals::PIN_Action));

			bIsErrorFree &= Schema->TryCreateConnection(OutLastActivatedThenPin, CallGetCtx->GetExecPin());
			OutLastActivatedThenPin = CallGetCtx->GetThenPin();
		}

		// CREATE CONTEXT GETTERS
		if (EAA::Switches::bVariadicGetSet)
		{
			bIsErrorFree &= HandleGetContextDataVariadic(CaptureOutputs, ContextHandlePin, OutLastActivatedThenPin, this, Schema, CompilerContext, SourceGraph);
		}
		else
		{
			bIsErrorFree &= HandleGetContextData(CaptureOutputs, ContextHandlePin, OutLastActivatedThenPin, this, Schema, CompilerContext, SourceGraph);
		}
		ensureAlwaysMsgf(bIsErrorFree, TEXT("node failed to build"));

		// CREATE CHAIN OF ASSIGMENTS FOR CALLBACK
		for (FOutputPinInfo& OutputPair : VariableOutputs)
		{
			UEdGraphPin* PinWithData = CurrentCENode->FindPin(OutputPair.OutputPin->PinName);
			if (PinWithData == nullptr)
			{
				continue;
			}

			UK2Node_AssignmentStatement* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
			AssignNode->AllocateDefaultPins();

			bIsErrorFree &= Schema->TryCreateConnection(OutLastActivatedThenPin, AssignNode->GetExecPin());

			bIsErrorFree &= Schema->TryCreateConnection(OutputPair.TempVar->GetVariablePin(), AssignNode->GetVariablePin());
			AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());

			bIsErrorFree &= Schema->TryCreateConnection(AssignNode->GetValuePin(), PinWithData);
			AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());

			OutLastActivatedThenPin = AssignNode->GetThenPin();
		}

		bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*PinForCurrentDelegateProperty, *OutLastActivatedThenPin).CanSafeConnect();
	}

	return bIsErrorFree;
}

bool UK2Node_EnhancedAsyncTaskBase::HandleGetContextData(
	const TArray<FOutputPinInfo>& CaptureOutputs, UEdGraphPin* ContextHandlePin, UEdGraphPin*& InOutLastThenPin,
	UK2Node* Self, const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	bool bIsErrorFree = true;

	for (const FOutputPinInfo& OutputPair : CaptureOutputs)
	{
		// [temp var]      - [assign]
		// [context read]  - [value ]
		UK2Node_CallFunction* CallReadNode = nullptr;

		FName GetValueFunc;
		if (EAA::Internals::SelectAccessorForType(OutputPair.OutputPin, EAA::Internals::EAccessorRole::GETTER, GetValueFunc))
		{
			if (OutputPair.OutputPin->PinType.IsArray())
			{
				CallReadNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(Self, SourceGraph);
			}
			else
			{
				CallReadNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Self, SourceGraph);
			}
			CallReadNode->FunctionReference.SetExternalMember(GetValueFunc, UEnhancedAsyncContextLibrary::StaticClass());
			CallReadNode->AllocateDefaultPins();
			ensure(CallReadNode->GetTargetFunction() != nullptr);
		}

		if (!CallReadNode)
		{
			const FString FormattedMessage = FText::Format(
				LOCTEXT("AsyncTaskNoGetter", "EnhancedAsyncTaskBase: no GETTER for {0} in async task @@"),
				FText::FromName(OutputPair.OutputPin->PinType.PinCategory)
			).ToString();
			CompilerContext.MessageLog.Error(*FormattedMessage, Self);
			bIsErrorFree = false;
			continue;
		}

		bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, CallReadNode->GetExecPin());
		InOutLastThenPin = CallReadNode->GetThenPin();

		UK2Node_AssignmentStatement* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(Self, SourceGraph);
		AssignNode->AllocateDefaultPins();

		// Set the handle parameter
		UEdGraphPin* HandlePin = CallReadNode->FindPinChecked(EAA::Internals::PIN_Handle);
		if (!Schema->CanCreateConnection(ContextHandlePin, HandlePin).CanSafeConnect())
		{
			bIsErrorFree &= Schema->CreateAutomaticConversionNodeAndConnections(ContextHandlePin, HandlePin);
		}
		else
		{
			bIsErrorFree &= Schema->TryCreateConnection(ContextHandlePin, HandlePin);
		}
		CallReadNode->NotifyPinConnectionListChanged(HandlePin);

		// index
		Schema->TrySetDefaultValue(*CallReadNode->FindPinChecked(EAA::Internals::PIN_Index), FString::Printf(TEXT("%d"), OutputPair.CaptureIndex));
		// value
		bIsErrorFree &= Schema->TryCreateConnection(OutputPair.TempVar->GetVariablePin(), AssignNode->GetVariablePin());
		AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());
		// return
		UEdGraphPin* ValuePin = CallReadNode->FindPinChecked(EAA::Internals::PIN_Value);
		bIsErrorFree &= Schema->TryCreateConnection(AssignNode->GetValuePin(), ValuePin);
		AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());
		CallReadNode->NotifyPinConnectionListChanged(ValuePin);

		// exec
		bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, AssignNode->GetExecPin());
		InOutLastThenPin = AssignNode->GetThenPin();
	}

	return bIsErrorFree;
}

bool UK2Node_EnhancedAsyncTaskBase::HandleGetContextDataVariadic(
	const TArray<FOutputPinInfo>& CaptureOutputs, UEdGraphPin* ContextHandlePin, UEdGraphPin*& InOutLastThenPin,
	UK2Node* Self, const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	bool bIsErrorFree = true;

	// Create a "Make Array" node to compile the list of arguments into an array
	UK2Node_MakeArray* MakeArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(Self, SourceGraph);
	MakeArrayNode->NumInputs = CaptureOutputs.Num();
	MakeArrayNode->AllocateDefaultPins();

	int32 ArrayInputIndex = 0;
	for (const FOutputPinInfo& OutputPair : CaptureOutputs)
	{
		const FName VariadicPropertyName = EAA::Internals::IndexToName(OutputPair.CaptureIndex);

		// Create the "Make Literal String" node
		UK2Node_CallFunction* MakeLiteralStringNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Self, SourceGraph);
		MakeLiteralStringNode->FunctionReference.SetExternalMember(
			GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeLiteralString),
			UKismetSystemLibrary::StaticClass()
		);
		MakeLiteralStringNode->AllocateDefaultPins();

		// Set the literal value to the capture parameter name
		UEdGraphPin* MakeLiteralStringValuePin = MakeLiteralStringNode->FindPinChecked(TEXT("Value"), EGPD_Input);
		Schema->TrySetDefaultValue(*MakeLiteralStringValuePin, VariadicPropertyName.ToString());

		// Find the input pin on the "Make Array" node by index and link it to the literal string
		UEdGraphPin* ArrayIn = MakeArrayNode->FindPinChecked(MakeArrayNode->GetPinName(ArrayInputIndex++));
		bIsErrorFree &= Schema->TryCreateConnection(MakeLiteralStringNode->GetReturnValuePin(), ArrayIn);
	}

	auto CallGetVariadic = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Self, SourceGraph);
	CallGetVariadic->FunctionReference.SetExternalMember(
		GET_MEMBER_NAME_CHECKED(UEnhancedAsyncContextLibrary, Handle_GetValue_Variadic),
		UEnhancedAsyncContextLibrary::StaticClass()
	);
	CallGetVariadic->AllocateDefaultPins();

	// Set the handle parameter
	UEdGraphPin* HandlePin = CallGetVariadic->FindPinChecked(EAA::Internals::PIN_Handle);
	if (!Schema->CanCreateConnection(ContextHandlePin, HandlePin).CanSafeConnect())
	{
		bIsErrorFree &= Schema->CreateAutomaticConversionNodeAndConnections(ContextHandlePin, HandlePin);
	}
	else
	{
		bIsErrorFree &= Schema->TryCreateConnection(ContextHandlePin, HandlePin);
	}
	CallGetVariadic->NotifyPinConnectionListChanged(HandlePin);

	// Set the names parameter and force the makearray pin type
	UEdGraphPin* ArrayOut = MakeArrayNode->GetOutputPin();
	bIsErrorFree &= Schema->TryCreateConnection(ArrayOut, CallGetVariadic->FindPinChecked(TEXT("Names")));
	MakeArrayNode->PinConnectionListChanged(ArrayOut);

	bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, CallGetVariadic->GetExecPin());
	InOutLastThenPin = CallGetVariadic->GetThenPin();

	for (const FOutputPinInfo& OutputPair : CaptureOutputs)
	{
		const FName VariadicPropertyName = EAA::Internals::IndexToName(OutputPair.CaptureIndex);

		FEdGraphPinType PinType = OutputPair.OutputPin->PinType;
		PinType.bIsReference = true;
		PinType.bIsConst = false;

		// create variadic pin
		UEdGraphPin* const ValuePin = CallGetVariadic->CreatePin(EGPD_Output, PinType, VariadicPropertyName);

		if (OutputPair.TempVar)
		{
			// make assignment node to the hidden temp variable
			UK2Node_AssignmentStatement* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(Self, SourceGraph);
			AssignNode->AllocateDefaultPins();

			// value
			bIsErrorFree &= Schema->TryCreateConnection(OutputPair.TempVar->GetVariablePin(), AssignNode->GetVariablePin());
			AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());

			// return
			bIsErrorFree &= Schema->TryCreateConnection(AssignNode->GetValuePin(), ValuePin);
			AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());

			// exec
			bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, AssignNode->GetExecPin());
			InOutLastThenPin = AssignNode->GetThenPin();
		}
		else
		{
			// move connection from outside to vararg
			bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*OutputPair.OutputPin, *ValuePin).CanSafeConnect();
		}
	}

	return bIsErrorFree;
}

bool UK2Node_EnhancedAsyncTaskBase::HandleInvokeActivate(
	UEdGraphPin* ProxyObjectPin, UEdGraphPin*& LastThenPin,
	const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	bool bIsErrorFree = true;
	if (ProxyActivateFunctionName != NAME_None)
	{
		UK2Node_CallFunction* const CallActivateProxyObjectNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallActivateProxyObjectNode->FunctionReference.SetExternalMember(ProxyActivateFunctionName, ProxyClass);
		CallActivateProxyObjectNode->AllocateDefaultPins();

		// Hook up the self connection
		UEdGraphPin* ActivateCallSelfPin = Schema->FindSelfPin(*CallActivateProxyObjectNode, EGPD_Input);
		check(ActivateCallSelfPin);

		bIsErrorFree &= Schema->TryCreateConnection(ProxyObjectPin, ActivateCallSelfPin);

		// Hook the activate node up in the exec chain
		UEdGraphPin* ActivateExecPin = CallActivateProxyObjectNode->GetExecPin();
		UEdGraphPin* ActivateThenPin = CallActivateProxyObjectNode->GetThenPin();

		bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, ActivateExecPin);

		LastThenPin = ActivateThenPin;
	}
	return bIsErrorFree;
}

void UK2Node_EnhancedAsyncTaskBase::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	check(SourceGraph && Schema);

	// Is Context feature used on this node
	const bool bContextRequired = AnyCapturePinHasLinks();

	// If there is no need in context - just use default implementation of async node
	if (!bContextRequired)
	{
		OrphanCapturePins();
		Super::ExpandNode(CompilerContext, SourceGraph);
		return;
	}

	if (!ValidateDelegates(Schema, CompilerContext, SourceGraph))
	{
		// Error handled internally
		return;
	}

	if (!ValidateCaptures(Schema, CompilerContext, SourceGraph))
	{
		// Error handled internally
		return;
	}

	UEdGraphPin* LastThenPin = nullptr;
	bool bIsErrorFree = true;

	// Create a call to factory the proxy object

	UK2Node_CallFunction* const CallCreateProxyObjectNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallCreateProxyObjectNode->FunctionReference.SetExternalMember(ProxyFactoryFunctionName, ProxyFactoryClass);
	CallCreateProxyObjectNode->AllocateDefaultPins();
	if (CallCreateProxyObjectNode->GetTargetFunction() == nullptr)
	{
		const FText ClassName = ProxyFactoryClass ? FText::FromString(ProxyFactoryClass->GetName()) : LOCTEXT("MissingClassString", "Unknown Class");
		const FString FormattedMessage = FText::Format(
			LOCTEXT("AsyncTaskBadFactoryFunc", "EnhancedAsyncTaskBase: Missing function {0} from class {1} for async task @@"),
			FText::FromString(ProxyFactoryFunctionName.GetPlainNameString()),
			ClassName
		).ToString();

		CompilerContext.MessageLog.Error(*FormattedMessage, this);
		return;
	}

	// Move Outer execute to inner execute
	bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(
		*FindPinChecked(UEdGraphSchema_K2::PN_Execute),
		*CallCreateProxyObjectNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute)
	).CanSafeConnect();
	LastThenPin = CallCreateProxyObjectNode->GetThenPin();

	TArray<UEdGraphPin*> StandardInputPins;
	GetStandardPins(EGPD_Input, StandardInputPins);
	for (UEdGraphPin* CurrentPin : StandardInputPins)
	{
		if (FBaseAsyncTaskHelper::ValidDataPin(CurrentPin, EGPD_Input))
		{
			// match function inputs, to pass data to function from CallFunction node
			UEdGraphPin* DestPin = CallCreateProxyObjectNode->FindPin(CurrentPin->PinName);
			bIsErrorFree &= DestPin && CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *DestPin).CanSafeConnect();
		}
	}

	UEdGraphPin* const ProxyObjectPin = CallCreateProxyObjectNode->GetReturnValuePin();
	check(ProxyObjectPin);

	// Validate proxy object

	UK2Node_CallFunction* IsValidFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	IsValidFuncNode->FunctionReference.SetExternalMember(
		GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid),
		UKismetSystemLibrary::StaticClass());
	IsValidFuncNode->AllocateDefaultPins();

	bIsErrorFree &= Schema->TryCreateConnection(ProxyObjectPin, IsValidFuncNode->FindPinChecked(TEXT("Object")));

	UK2Node_IfThenElse* ValidateProxyNode = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
	ValidateProxyNode->AllocateDefaultPins();
	bIsErrorFree &= Schema->TryCreateConnection(IsValidFuncNode->GetReturnValuePin(), ValidateProxyNode->GetConditionPin());

	bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, ValidateProxyNode->GetExecPin());
	LastThenPin = ValidateProxyNode->GetThenPin();

	// Create a call to context creation function and obtain handle
	UK2Node_CallFunction* const CallCreateCaptureNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallCreateCaptureNode->FunctionReference.SetExternalMember(
		GET_MEMBER_NAME_CHECKED(UEnhancedAsyncContextLibrary, CreateContextForObject),
		UEnhancedAsyncContextLibrary::StaticClass()
	);
	CallCreateCaptureNode->AllocateDefaultPins();

	// Connect [CreateAction] and [CreateContext]
	bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, CallCreateCaptureNode->GetExecPin());
	LastThenPin = CallCreateCaptureNode->GetThenPin();

	// Set action object value
	bIsErrorFree &= Schema->TryCreateConnection(ProxyObjectPin, CallCreateCaptureNode->FindPinChecked(EAA::Internals::PIN_Action));

	// Set container property value
	UEdGraphPin* NamePin = CallCreateCaptureNode->FindPinChecked(TEXT("InnerContainerProperty"));
	Schema->TrySetDefaultValue(*NamePin, AsyncContextContainerProperty.ToString());

	UEdGraphPin* const CaptureContextHandlePin = CallCreateCaptureNode->GetReturnValuePin();
	check(CaptureContextHandlePin);

	// Configure context
	const bool bContextSetupRequired = bContextRequired && GetNumCaptures() > 0
		&& EAA::Switches::bWithSetupContext
		&& !EAA::Switches::bVariadicGetSet;

	if (bContextSetupRequired)
	{
		HandleSetupContext(CaptureContextHandlePin, LastThenPin, BuildContextConfigString(), this, Schema, CompilerContext, SourceGraph);
	}

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

	// Create capture setters
	if (EAA::Switches::bVariadicGetSet)
	{
		bIsErrorFree &= HandleSetContextDataVariadic(CaptureInputs, CaptureContextHandlePin, LastThenPin, this, Schema, CompilerContext, SourceGraph);
	}
	else
	{
		bIsErrorFree &= HandleSetContextData(CaptureInputs, CaptureContextHandlePin, LastThenPin, this, Schema, CompilerContext, SourceGraph);
	}
	ensureAlwaysMsgf(bIsErrorFree, TEXT("node failed to build"));

	UEdGraphPin* OutputAsyncTaskProxy = FindPin(FBaseAsyncTaskHelper::GetAsyncTaskProxyName());
	bIsErrorFree &= !OutputAsyncTaskProxy || CompilerContext.MovePinLinksToIntermediate(*OutputAsyncTaskProxy, *ProxyObjectPin).CanSafeConnect();

	bIsErrorFree &= ExpandDefaultToSelfPin(CompilerContext, SourceGraph, CallCreateProxyObjectNode);

	// FOR EACH DELEGATE DEFINE EVENT,
	//		CONNECT IT TO DELEGATE
	//		AND READ CONTEXT
	//		AND IMPLEMENT A CHAIN OF ASSIGMENTS
	UEdGraphPin * const PrevPin = LastThenPin;
	bIsErrorFree &= HandleActionDelegates(CallCreateProxyObjectNode, LastThenPin, Schema, SourceGraph, CompilerContext);

	if (PrevPin == LastThenPin)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("MissingDelegateProperties", "EnhancedAsyncTaskBase: Proxy has no delegates defined. @@").ToString(), this);
		return;
	}

	// Create a call to activate the proxy object if necessary
	bIsErrorFree &= HandleInvokeActivate(ProxyObjectPin, LastThenPin, Schema, CompilerContext, SourceGraph);

	// Move the connections from the original node then pin to the last internal then pin

	UEdGraphPin* OriginalThenPin = GetThenPin();

	if (OriginalThenPin)
	{
		bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*OriginalThenPin, *LastThenPin).CanSafeConnect();
	}
	bIsErrorFree &= CompilerContext.CopyPinLinksToIntermediate(*LastThenPin, *ValidateProxyNode->GetElsePin()).CanSafeConnect();

	if (!bIsErrorFree)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("InternalConnectionError", "EnhancedAsyncTaskBase: Internal connection error. @@").ToString(), this);
	}

	// Make sure we caught everything
	BreakAllNodeLinks();
}

#undef LOCTEXT_NAMESPACE
