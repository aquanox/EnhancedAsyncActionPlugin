// Copyright 2025, Aquanox.

#include "K2Node_EnhancedAsyncTaskBase.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "EnhancedAsyncActionContextLibrary.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_TemporaryVariable.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "ScopedTransaction.h"
#include "EnhancedAsyncActionShared.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CustomEvent.h"
#include "EnhancedAsyncActionPrivate.h"
#include "K2Node_CallArrayFunction.h"
#include "StructUtils/PropertyBag.h"
#include "ToolMenu.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_EnhancedAsyncTaskBase)

#define LOCTEXT_NAMESPACE "UK2Node_EnhancedAsyncTaskBase"

namespace EAA::Switches
{
	// should fully rely on serialized array instead of querying and searching for pins every time?
	// possible optimization searching pins
	constexpr bool bRelyOnArray = true;
	
	// gather known types and call setup to optimize bakend updates after consecutive AddProperty
	constexpr bool bWithSetupContext = true;

	// debug tooltip
	constexpr bool bDebugTooltips = true;
}

UK2Node_EnhancedAsyncTaskBase::UK2Node_EnhancedAsyncTaskBase()
{
}

void UK2Node_EnhancedAsyncTaskBase::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	// Super::GetMenuActions(ActionRegistrar);
}

void UK2Node_EnhancedAsyncTaskBase::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	static const FName NodeSection = FName("EnhancedAsyncTaskBase");
	static const FText NodeSectionDesc = LOCTEXT("EnhancedAsyncTaskBase", "Task Node");

	auto* MutableThis = const_cast<UK2Node_EnhancedAsyncTaskBase*>(this);
	auto* MutablePin = const_cast<UEdGraphPin*>(Context->Pin);

	if (CanRemovePin(Context->Pin))
	{
		FToolMenuSection& Section = Menu->FindOrAddSection(NodeSection, NodeSectionDesc);
		Section.AddMenuEntry(
			"RemovePin",
			LOCTEXT("RemovePin", "Remove pin"),
			LOCTEXT("RemovePinTooltip", "Remove this capture pin"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateUObject(MutableThis, &ThisClass::RemoveInputPin, MutablePin)
			)
		);
	}
	else if (CanAddPin())
	{
		FToolMenuSection& Section = Menu->FindOrAddSection(NodeSection, NodeSectionDesc);
		Section.AddMenuEntry(
			"AddPin",
			LOCTEXT("AddPin", "Add pin"),
			LOCTEXT("AddPinTooltip", "Add another capture pin pair"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateUObject(MutableThis, &ThisClass::AddInputPin)
			)
		);
	}
	if (!Context->Pin && HasAnyCapturePins())
	{
		FToolMenuSection& Section = Menu->FindOrAddSection(NodeSection, NodeSectionDesc);
		Section.AddMenuEntry(
			"UnlinkAllCapture",
			LOCTEXT("UnlinkAllCapture", "Unlink all capture pins"),
			LOCTEXT("UnlinkAllCaptureTooltip", "Unlink all capture pins"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateUObject(MutableThis, &ThisClass::UnlinkAllDynamicPins)
			)
		);
		Section.AddMenuEntry(
			"RemoveAllCapture",
			LOCTEXT("RemoveAllCapture", "Remove all capture pins"),
			LOCTEXT("RemoveAllCaptureTooltip", "Remove all capture pins"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateUObject(MutableThis, &ThisClass::RemoveAllDynamicPins)
			)
		);
	}
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

bool UK2Node_EnhancedAsyncTaskBase::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const
{
	const bool bContainerResult = (AsyncContextContainerType != NULL);
	if (bContainerResult && OptionalOutput)
	{
		OptionalOutput->AddUnique(AsyncContextContainerType);
	}
	const bool bSuperResult = Super::HasExternalDependencies(OptionalOutput);
	return bContainerResult || bSuperResult;
}

FText UK2Node_EnhancedAsyncTaskBase::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText Base = Super::GetNodeTitle(TitleType);
	return FText::Format(INVTEXT("{0} (Capture)"), Base);
}

FText UK2Node_EnhancedAsyncTaskBase::GetTooltipText() const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("FunctionTooltip"), Super::GetTooltipText());
	
	if (!HasAnyLinkedCaptures())
		Args.Add(TEXT("CaptureString"), LOCTEXT("CaptureString", "Capture. This node supports capture context."));
	else
		Args.Add(TEXT("CaptureString"), LOCTEXT("CaptureString", "Capture. This node uses capture context."));
	
	if (EAA::Switches::bDebugTooltips)
	{
		FString Config = EAA::Internals::BuildContextConfigString(this, NumCaptures);
		Config.ReplaceInline(TEXT(";"), TEXT("\n"));
		Args.Add(TEXT("Extra"), FText::FromString(TEXT("\n\n") + Config));
	}
	else
	{
		Args.Add(TEXT("Extra"), INVTEXT(""));
	}
	return FText::Format(LOCTEXT("AsyncTaskTooltip", "{FunctionTooltip}\n\n{CaptureString}{Extra}"), Args);
}

void UK2Node_EnhancedAsyncTaskBase::ImportCaptureConfigFromProxyClass()
{
	const FString* ContextParamName = EAA::Internals::FindMetadataHierarchical(ProxyClass, EAA::Internals::MD_HasAsyncContext);
	check(ContextParamName);
	AsyncContextParameterName = **ContextParamName;

	AsyncContextContainerType = FInstancedPropertyBag::StaticStruct();
	AsyncContextContainerProperty = NAME_None;

	const FString* ContextPropName = EAA::Internals::FindMetadataHierarchical(ProxyClass, EAA::Internals::MD_AsyncContextContainer);
	FName PossibleContextProp = ContextPropName ? **ContextPropName : AsyncContextParameterName;
	{
		// search for member property with explicitly specified with AsyncContextContainer or having same name as AsyncContext
		auto* MemberProperty = CastField<FStructProperty>(ProxyClass->FindPropertyByName(PossibleContextProp));
		if (MemberProperty  && MemberProperty->Struct->IsChildOf(FInstancedPropertyBag::StaticStruct()))
		{
			AsyncContextContainerType = MemberProperty->Struct;
			AsyncContextContainerProperty = MemberProperty->GetFName();
		}
	}
	
	bExposeContextParameter = EAA::Internals::FindMetadataHierarchical(ProxyClass, EAA::Internals::MD_ExposedAsyncContext) != nullptr;
}

void UK2Node_EnhancedAsyncTaskBase::AllocateDefaultPins()
{
	UE_LOG(LogEnhancedAction, Log, TEXT("%p:AllocateDefaultPins"), this);

	Super::AllocateDefaultPins();

	// Optionally hide the async context pin if requested
	SyncContextPinState();

	Captures.Empty();

	const UScriptStruct* Skeleton = nullptr;
	if (HasDefaultDynamicPins(Skeleton))
	{
		AllocateDefaultCapturePinsForStruct(Skeleton);
	}
	else
	{
		for (int32 Index = 0; Index < NumCaptures; ++Index)
		{
			auto In = CreatePin(EEdGraphPinDirection::EGPD_Input,
								   UEdGraphSchema_K2::PC_Wildcard,
								   GetPinName(EEdGraphPinDirection::EGPD_Input, Index)
			);
			In->SourceIndex = Index;
			In->bDefaultValueIsIgnored = true;
			auto Out = CreatePin(EEdGraphPinDirection::EGPD_Output,
									UEdGraphSchema_K2::PC_Wildcard,
									GetPinName(EEdGraphPinDirection::EGPD_Output, Index)
			);
			Out->SourceIndex = Index;

			Captures.Add(FEATCapturePinPair(Index, In, Out));
		}
	}
}

void UK2Node_EnhancedAsyncTaskBase::AllocateDefaultCapturePinsForStruct(const UScriptStruct* Struct)
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	int32 Index = 0;
	for (TFieldIterator<FProperty> PropertyIt(Struct); PropertyIt; ++PropertyIt, ++Index)
	{
		const FProperty* Property = *PropertyIt;
		if (!Property->HasAnyPropertyFlags(CPF_BlueprintVisible)) continue;

		auto In = CreatePin(EEdGraphPinDirection::EGPD_Input, NAME_None, GetPinName(EEdGraphPinDirection::EGPD_Input, Index) );
		In->SourceIndex = Index;
		In->bDefaultValueIsIgnored = true;
		In->PinToolTip = Property->GetToolTipText().ToString();
		In->PinFriendlyName = Property->GetDisplayNameText();
		bool bInGood = Schema->ConvertPropertyToPinType(Property, /*out*/ In->PinType);

		auto Out = CreatePin(EEdGraphPinDirection::EGPD_Output, NAME_None, GetPinName(EEdGraphPinDirection::EGPD_Output, Index) );
		Out->SourceIndex = Index;
		Out->PinToolTip = Property->GetToolTipText().ToString();
		Out->PinFriendlyName = Property->GetDisplayNameText();
		bool bOutGood = Schema->ConvertPropertyToPinType(Property, /*out*/ Out->PinType);

		if (bInGood && bOutGood)
		{
			Captures.Add(FEATCapturePinPair(Index, Property->GetFName(), In, Out));
		}
	}

	NumCaptures = Captures.Num();
}

bool UK2Node_EnhancedAsyncTaskBase::HasAnyLinkedCaptures() const
{
	if (!NumCaptures) return false;
	
	for (int32 Index = 0; Index < NumCaptures; ++Index)
	{
		auto InPin = FindDynamicPin(EGPD_Input, Index);
		if (InPin && InPin->LinkedTo.Num())
			return true;

		auto OutPin = FindDynamicPin(EGPD_Output, Index);
		if (OutPin && OutPin->LinkedTo.Num())
			return true;
	}

	return false;
}

bool UK2Node_EnhancedAsyncTaskBase::HasContextExposed() const
{
	return bExposeContextParameter;
}

bool UK2Node_EnhancedAsyncTaskBase::NeedSetupCaptureContext() const
{
	return HasAnyCapturePins() && EAA::Switches::bWithSetupContext;
}

TArray<UEdGraphPin*> UK2Node_EnhancedAsyncTaskBase::GetStandardPins(EEdGraphPinDirection Dir) const
{
	TArray<UEdGraphPin*>  Result;
	Result.Reserve(Pins.Num());
	for (auto& Pin : Pins)
	{
		if (Pin->Direction == Dir && !IsDynamicPin(Pin))
		{
			Result.Add(Pin);
		}
	}
	return Result;
}

bool UK2Node_EnhancedAsyncTaskBase::HasDefaultDynamicPins(const UScriptStruct*& Skeleton) const
{
	if (!AsyncContextContainerProperty.IsNone())
	{
		auto* MemberProperty = CastField<FStructProperty>(ProxyClass->FindPropertyByName(AsyncContextContainerProperty));
		check(MemberProperty);
		auto* ProxyCDO = ProxyClass->GetDefaultObject();

		if (MemberProperty->Struct->IsChildOf(FInstancedPropertyBag::StaticStruct()))
		{ // Property bag
			auto* Value = MemberProperty->ContainerPtrToValuePtrForDefaults<FInstancedPropertyBag>(ProxyClass, ProxyCDO);
			Skeleton = Value->GetPropertyBagStruct();
		}
		else
		{ // arbitrary struct
			Skeleton = MemberProperty->Struct;
		}
	}

	return false;
}

bool UK2Node_EnhancedAsyncTaskBase::IsDynamicContainerType() const
{
	return AsyncContextContainerType == nullptr
		|| AsyncContextContainerType->IsChildOf(FInstancedPropertyBag::StaticStruct());
}

FName UK2Node_EnhancedAsyncTaskBase::GetPinName(EEdGraphPinDirection Dir, int32 PinIndex) const
{
	if (Dir == EGPD_Input)
		return *FString::Printf(TEXT("In[%d]"), PinIndex);
	else
		return *FString::Printf(TEXT("Out[%d]"), PinIndex);
}

bool UK2Node_EnhancedAsyncTaskBase::IsContextPin(const UEdGraphPin* Pin) const
{
	return Pin && Pin->PinName == AsyncContextParameterName
		&& Pin->Direction == EGPD_Output
		&& Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object;
}

bool UK2Node_EnhancedAsyncTaskBase::CanSplitPin(const UEdGraphPin* Pin) const
{
	return Super::CanSplitPin(Pin) && !IsDynamicPin(Pin);
}

bool UK2Node_EnhancedAsyncTaskBase::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	if (IsDynamicPin(MyPin) && OtherPin)
	{
		if (OtherPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec
		 || OtherPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Delegate)
		{
			OutReason = FString::Printf(TEXT("Can not capture pin"));
			return true;
		}
		if (OtherPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
		{
			if (!EAA::Internals::IsCapturableType(OtherPin->PinType))
			{
				OutReason = FString::Printf(TEXT("Can not capture pin of type %s"), *OtherPin->PinType.PinCategory.ToString());
				return true;
			}
		}
	}
	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_EnhancedAsyncTaskBase::SyncPinNames()
{
	auto SyncPin = [this](UEdGraphPin* CurrentPin, int32 CurrentIndex)
	{
		check(CurrentPin);
		
		const FName OldName = CurrentPin->PinName;
		const FName ElementName = GetPinName(CurrentPin->Direction, CurrentIndex);

		CurrentPin->Modify();
		CurrentPin->PinName = ElementName;
		CurrentPin->SourceIndex = CurrentIndex;
	};

	if (!EAA::Switches::bRelyOnArray)
	{
		TArray<UEdGraphPin*> InPins, OutPins;
		for (UEdGraphPin* Pin : Pins)
		{
			auto& Target = Pin->Direction == EGPD_Input ? InPins : OutPins;
			if (IsDynamicPin(Pin))
			{
				Target.Add(Pin);
			}
		}
		check(InPins.Num() == OutPins.Num());

		for (int32 Index = 0; Index < InPins.Num(); ++Index)
		{
			auto& CaptureInfo = Captures[Index];

			ensure(CaptureInfo.InputPinName == InPins[Index]->PinName);
			ensure(CaptureInfo.OutputPinName == OutPins[Index]->PinName);
		
			SyncPin( InPins[Index], Index );
			SyncPin( OutPins[Index], Index );

			CaptureInfo.Index = Index;
			CaptureInfo.InputPinName = InPins[Index]->PinName;
			CaptureInfo.OutputPinName = OutPins[Index]->PinName;
		}
	}
	else
	{
		for (int32 Index = 0; Index < Captures.Num(); ++Index)
		{
			FEATCapturePinPair& CaptureInfo = Captures[Index];
			
			UEdGraphPin* InPin = CaptureInfo.FindPinChecked(EGPD_Input, this);
			UEdGraphPin* OutPin = CaptureInfo.FindPinChecked(EGPD_Output, this);
			
			SyncPin( InPin, Index);
			SyncPin( OutPin, Index);

			CaptureInfo.Index = Index;
			CaptureInfo.InputPinName = InPin->PinName;
			CaptureInfo.OutputPinName = OutPin->PinName;
		}
	}
}

void UK2Node_EnhancedAsyncTaskBase::SyncPinTypes()
{
	for (int32 PinIndex = 0; PinIndex < NumCaptures; ++PinIndex)
	{
		SynchronizeArgumentPinType(FindDynamicPin(EGPD_Input, PinIndex));
	}
}

UEdGraphPin* UK2Node_EnhancedAsyncTaskBase::FindDynamicPin(EEdGraphPinDirection Dir, int32 PinIndex) const
{
	if (!EAA::Switches::bRelyOnArray)
	{
		ensure(PinIndex < NumCaptures);
		return FindPin(GetPinName(Dir, PinIndex), Dir);
	}
	else
	{
		ensure(Captures.IsValidIndex(PinIndex));
		ensure(Captures[PinIndex].NameOf(Dir) == GetPinName(Dir, PinIndex));
		return Captures[PinIndex].FindPin(Dir, this);
	}
}

UEdGraphPin* UK2Node_EnhancedAsyncTaskBase::FindMatchingPin(const UEdGraphPin* Pin, EEdGraphPinDirection Dir) const
{
	// Pin is already of expected type
	if (Pin->Direction == Dir) return const_cast<UEdGraphPin*>(Pin);
	
	if (!EAA::Switches::bRelyOnArray)
	{
		const TCHAR* CurrentPinPrefix = Pin->Direction == EGPD_Input ? TEXT("In[") : TEXT("Out[");
		const TCHAR* MirrorPinPrefix = Dir == EGPD_Input ? TEXT("In[") : TEXT("Out[");

		FString String = Pin->PinName.ToString();
		check(String.StartsWith(CurrentPinPrefix));
		String.ReplaceInline(CurrentPinPrefix, MirrorPinPrefix, ESearchCase::CaseSensitive);
		return FindPin(String);
	}
	else
	{
		int32 Index = Captures.IndexOfByKey(Pin->PinName);
		ensure(Index != INDEX_NONE);
		return Captures[Index].FindPin(Dir, this);
	}
}

UEdGraphPin* UK2Node_EnhancedAsyncTaskBase::FindMatchingPin(const UEdGraphPin* Pin) const
{
	return FindMatchingPin(Pin, Pin->Direction == EGPD_Input ? EGPD_Output : EGPD_Input);
}

bool UK2Node_EnhancedAsyncTaskBase::IsDynamicPin(const UEdGraphPin* Pin) const
{
	// dynamic pins never split
	if (Pin->ParentPin != nullptr || Pin->bOrphanedPin) return false;

	if (!EAA::Switches::bRelyOnArray)
	{
		const TCHAR* CurrentPinPrefix = Pin->Direction == EGPD_Input ? TEXT("In[") : TEXT("Out[");
		return Pin->PinName.ToString().StartsWith(CurrentPinPrefix, ESearchCase::CaseSensitive);
	}
	else
	{
		return Captures.IndexOfByKey(Pin->PinName) != INDEX_NONE;
	}
}

int32 UK2Node_EnhancedAsyncTaskBase::GetMaxCapturePinsNum()
{
	return EAA::Internals::MaxCapturePins;
}

bool UK2Node_EnhancedAsyncTaskBase::CanAddPin() const
{
	return IsDynamicContainerType()
		&& NumCaptures < GetMaxCapturePinsNum();
}

void UK2Node_EnhancedAsyncTaskBase::AddInputPin()
{
	check(IsDynamicContainerType());
	
	Modify();

	const int32 Index = NumCaptures;
	++NumCaptures;

	auto In = CreatePin(EEdGraphPinDirection::EGPD_Input,
						   UEdGraphSchema_K2::PC_Wildcard,
						   GetPinName(EEdGraphPinDirection::EGPD_Input, Index)
	);
	In->bDefaultValueIsIgnored = true;
	auto Out = CreatePin(EEdGraphPinDirection::EGPD_Output,
							UEdGraphSchema_K2::PC_Wildcard,
							GetPinName(EEdGraphPinDirection::EGPD_Output, Index)
	);
	
	Captures.Add(FEATCapturePinPair(Index, In, Out));

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

bool UK2Node_EnhancedAsyncTaskBase::CanRemovePin(const UEdGraphPin* Pin) const
{
	return IsDynamicContainerType()
		&& Pin
		&& HasAnyCapturePins()
		&& IsDynamicPin(Pin);
}

void UK2Node_EnhancedAsyncTaskBase::RemoveInputPin(UEdGraphPin* Pin)
{
	check(IsDynamicContainerType());
	check(Pin->ParentPin == nullptr);
	checkSlow(Pins.Contains(Pin));

	if (!IsDynamicPin(Pin))
		 return;

	FScopedTransaction Transaction(LOCTEXT("RemovePinTx", "RemovePin"));
	Modify();

	UEdGraphPin* InPin = FindMatchingPin(Pin, EEdGraphPinDirection::EGPD_Input);
	UEdGraphPin* OutPin = FindMatchingPin(Pin, EEdGraphPinDirection::EGPD_Output);
	check(InPin && OutPin);

	int32 CaptureIndex = Captures.IndexOfByPredicate([InPin, OutPin](const FEATCapturePinPair& Pair) {
		return Pair.InputPinName == InPin->PinName && Pair.OutputPinName == OutPin->PinName;
	});
	check(CaptureIndex != INDEX_NONE);

	TFunction<void(UEdGraphPin*)> RemovePinLambda = [this, &RemovePinLambda](UEdGraphPin* PinToRemove)
	{
		for (int32 SubPinIndex = PinToRemove->SubPins.Num()-1; SubPinIndex >= 0; --SubPinIndex)
		{
			RemovePinLambda(PinToRemove->SubPins[SubPinIndex]);
		}

		int32 PinRemovalIndex = INDEX_NONE;
		if (Pins.Find(PinToRemove, PinRemovalIndex))
		{
			Pins.RemoveAt(PinRemovalIndex);
			PinToRemove->MarkAsGarbage();
		}
	};

	RemovePinLambda(InPin);
	RemovePinLambda(OutPin);

	Captures.RemoveAt(CaptureIndex);

	PinConnectionListChanged(InPin);
	PinConnectionListChanged(OutPin);

	--NumCaptures;
	SyncPinNames();
	
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

void UK2Node_EnhancedAsyncTaskBase::UnlinkAllDynamicPins()
{
	const FEdGraphPinType WildcardPinType(UEdGraphSchema_K2::PC_Wildcard, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());

	FScopedTransaction Transaction(LOCTEXT("UnlinkAllDynamicPinsTx", "UnlinkAllDynamicPins"));
	Modify();
	
	for (int32 Index = 0; Index < NumCaptures; ++Index)
	{
		auto InPin = FindDynamicPin(EGPD_Input, Index);
		if (ensure(InPin))
		{
			InPin->BreakAllPinLinks(true);
			InPin->PinType = WildcardPinType;
		}

		auto OutPin = FindDynamicPin(EGPD_Output, Index);
		if (ensure(OutPin))
		{
			OutPin->BreakAllPinLinks(true);
			OutPin->PinType = WildcardPinType;
		}
	}

	GetGraph()->NotifyNodeChanged(this);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

void UK2Node_EnhancedAsyncTaskBase::RemoveAllDynamicPins()
{
	FScopedTransaction Transaction(LOCTEXT("RemoveAllDynamicPinsTx", "RemoveAllDynamicPins"));
	Modify();

	TFunction<void(UEdGraphPin*)> RemovePinLambda = [this, &RemovePinLambda](UEdGraphPin* PinToRemove)
	{
		for (int32 SubPinIndex = PinToRemove->SubPins.Num()-1; SubPinIndex >= 0; --SubPinIndex)
		{
			RemovePinLambda(PinToRemove->SubPins[SubPinIndex]);
		}

		int32 PinRemovalIndex = INDEX_NONE;
		if (Pins.Find(PinToRemove, PinRemovalIndex))
		{
			Pins.RemoveAt(PinRemovalIndex);
			PinToRemove->MarkAsGarbage();
		}
	};

	for (int Index = NumCaptures - 1; Index >= 0; --Index)
	{
		RemovePinLambda(FindDynamicPin(EGPD_Input, Index));
		RemovePinLambda(FindDynamicPin(EGPD_Output, Index));
	}
	
	NumCaptures = 0;
	Captures.Empty();
	
	GetGraph()->NotifyNodeChanged(this);
	
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
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
			Modify();
			
			ContextPin->Modify();
			ContextPin->bHidden = bHidden;
			ContextPin->bNotConnectable = bHidden;

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

	if (IsDynamicPin(Pin) && !Pin->IsPendingKill())
	{
		SynchronizeArgumentPinType(Pin);
	}
}

void UK2Node_EnhancedAsyncTaskBase::PinTypeChanged(UEdGraphPin* Pin)
{
	UE_LOG(LogEnhancedAction, Log, TEXT("%p:PinTypeChanged"), this);
	if (IsDynamicPin(Pin) && !Pin->IsPendingKill())
	{
		SynchronizeArgumentPinType(Pin);
	}
	Super::PinTypeChanged(Pin);
}

void UK2Node_EnhancedAsyncTaskBase::PostReconstructNode()
{
	UE_LOG(LogEnhancedAction, Log, TEXT("%p:PostReconstructNode"), this);
	Super::PostReconstructNode();

	if (!IsTemplate())
	{
		SyncContextPinState();
		
		UEdGraph* OuterGraph = GetGraph();
		if (OuterGraph && OuterGraph->Schema)
		{
			for (UEdGraphPin* CurrentPin : Pins)
			{
				if (IsDynamicPin(CurrentPin))
				{
					SynchronizeArgumentPinType(CurrentPin);
				}
			}
		}
	}
}

void UK2Node_EnhancedAsyncTaskBase::SynchronizeArgumentPinType(UEdGraphPin* Pin)
{
	check(Pin);
	
	const UEdGraphSchema_K2* Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
	
	UEdGraphPin* InputPin = FindMatchingPin(Pin, EGPD_Input);
	UEdGraphPin* OutputPin = FindMatchingPin(Pin, EGPD_Output);
	check(InputPin && OutputPin);

	FEdGraphPinType NewType = EAA::Internals::DeterminePinType(InputPin, OutputPin);

	bool bPinTypeChanged = false;
	auto ApplyPinType = [&bPinTypeChanged](UEdGraphPin* LocalPin, const FEdGraphPinType& Type)
	{
		if (LocalPin->PinType != Type)
		{
			LocalPin->PinType = Type;
			bPinTypeChanged = true;
		}
	};
	ApplyPinType(InputPin, NewType);
	ApplyPinType(OutputPin, NewType);

	UE_LOG(LogEnhancedAction, Verbose, TEXT("SyncType %s => %s"), *EAA::Internals::ToDebugString(Pin), *EAA::Internals::ToDebugString(NewType))
	
	if (bPinTypeChanged)
	{
		InputPin->Modify();
		OutputPin->Modify();
		
		// Let the graph know to refresh
		GetGraph()->NotifyNodeChanged(this);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

bool UK2Node_EnhancedAsyncTaskBase::ValidateDelegates(
            const UEdGraphSchema_K2* Schema, class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	const FName& ContextPinName = AsyncContextParameterName;
	if (ContextPinName.IsNone())
	{
		const FString FormattedMessage = FText::Format(
			LOCTEXT("AsyncTaskErrorFmt", "EnhancedAsyncAction: {0} Context property is missing in @@"),
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
				LOCTEXT("AsyncTaskErrorFmt", "EnhancedAsyncAction: Function signature of {0}::{1} does not have required context parameter {2} in @@"),
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

	for (int32 Index = 0; Index < NumCaptures; ++Index)
	{
		UEdGraphPin* ArgumentTypePin =  FindDynamicPin(EGPD_Input, Index);

		if (EAA::Internals::IsWildcardType(ArgumentTypePin->PinType))
			continue;

		if (!EAA::Internals::IsCapturableType(ArgumentTypePin->PinType))
		{
			const FString FormattedMessage = FText::Format(
				LOCTEXT("AsyncTaskErrorFmt", "EnhancedAsyncTaskBase: {0} is unsupported capture for async task @@"),
				FText::FromName(ArgumentTypePin->PinType.PinCategory)
			).ToString();
			CompilerContext.MessageLog.Error(*FormattedMessage, this);
			bIsErrorFree = false;
			continue;
		}
	}
	
	return bIsErrorFree;
}

bool UK2Node_EnhancedAsyncTaskBase::HandleSetContextData(
	UEdGraphPin* InContextPin, UEdGraphPin*& LastThenPin,
	const UEdGraphSchema_K2* Schema, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	bool bIsErrorFree = true;
	for (int32 Index = 0; Index < NumCaptures; ++Index)
	{
		UEdGraphPin* ArgumentTypePin =  FindDynamicPin(EGPD_Input, Index);
		
		if (EAA::Internals::IsWildcardType(ArgumentTypePin->PinType))
			continue;
		if (!ArgumentTypePin->LinkedTo.Num())
			continue;

		UK2Node_CallFunction* CallNode = nullptr;

		FName SetValueFunc;
		if (EAA::Internals::SelectAccessorForType(ArgumentTypePin, EAA::Internals::EAccessorRole::SETTER, SetValueFunc))
		{
			if (ArgumentTypePin->PinType.IsArray())
			{
				CallNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(this, SourceGraph);
			}
			else
			{
				CallNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			}
			
			CallNode->SetFromFunction(UEnhancedAsyncActionContextLibrary::StaticClass()->FindFunctionByName(SetValueFunc));
			CallNode->AllocateDefaultPins();
			ensure(CallNode->GetTargetFunction() != nullptr);
		}

		if (!CallNode)
		{
			const FString FormattedMessage = FText::Format(
				LOCTEXT("AsyncTaskErrorFmt", "EnhancedAsyncTaskBase: no SETTER for {0} in async task @@"),
				FText::FromName(ArgumentTypePin->PinType.PinCategory)
			).ToString();
			CompilerContext.MessageLog.Error(*FormattedMessage, this);
			bIsErrorFree = false;
			continue;
		}

		// match function inputs, to pass data to function from CallFunction node

		UEdGraphPin* HandlePin = CallNode->FindPinChecked(EAA::Internals::PIN_Handle);
		bIsErrorFree &=  Schema->TryCreateConnection(InContextPin, HandlePin);
			
		UEdGraphPin* IndexPin = CallNode->FindPinChecked(EAA::Internals::PIN_Index);
		Schema->TrySetDefaultValue(*IndexPin, FString::Printf(TEXT("%d"), Index));
			
		UEdGraphPin* ValuePin = CallNode->FindPinChecked(EAA::Internals::PIN_Value);
		bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*ArgumentTypePin, *ValuePin).CanSafeConnect();
		CallNode->PinConnectionListChanged(ValuePin);

		// Connect [Last] and [Set]
		bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, CallNode->GetExecPin());
		LastThenPin = CallNode->GetThenPin();
	}

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
		for (UEdGraphPin* CurrentPin : GetStandardPins(EGPD_Output))
		{
			const bool bIsProxyPin  = (OutputAsyncTaskProxy == CurrentPin);
			const bool bIsContextPin = IsContextPin(CurrentPin);
			
			UE_LOG(LogEnhancedAction, Log, TEXT("Gather Delegate %s"), *EAA::Internals::ToDebugString(CurrentPin));

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
					UK2Node_TemporaryVariable* TempVarOutput = CompilerContext.SpawnInternalVariable(this,
						PinType.PinCategory, PinType.PinSubCategory, PinType.PinSubCategoryObject.Get(),
						PinType.ContainerType, PinType.PinValueType);
					// move external pins to the temporary variable
					bIsErrorFree &= TempVarOutput->GetVariablePin()
						&& CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *TempVarOutput->GetVariablePin()).CanSafeConnect();
					VariableOutputs.Add(FOutputPinInfo(FOutputPinInfo::DELEGATE, CurrentPin, TempVarOutput));
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
		for (int32 Index = 0; Index < NumCaptures; ++Index)
		{
			UEdGraphPin* const CurrentPin  = FindDynamicPin(EGPD_Output, Index);

			if (!CurrentPin->LinkedTo.Num())
				continue;
			if (EAA::Internals::IsWildcardType(CurrentPin->PinType))
				continue;
			
			UE_LOG(LogEnhancedAction, Log, TEXT("Gather Capture %s"), *EAA::Internals::ToDebugString(CurrentPin));

			const FEdGraphPinType& PinType = CurrentPin->PinType;
			UK2Node_TemporaryVariable* TempVarOutput = CompilerContext.SpawnInternalVariable(this,
				PinType.PinCategory, PinType.PinSubCategory, PinType.PinSubCategoryObject.Get(), PinType.ContainerType, PinType.PinValueType);
			// move external pins to the temporary variable
			bIsErrorFree &= TempVarOutput->GetVariablePin()
				&& CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *TempVarOutput->GetVariablePin()).CanSafeConnect();

			CaptureOutputs.Add(FOutputPinInfo(FOutputPinInfo::CAPTURE, CurrentPin, TempVarOutput, Index));
		}
	}
	
	for (TFieldIterator<FMulticastDelegateProperty> PropertyIt(ProxyClass); PropertyIt && bIsErrorFree; ++PropertyIt)
	{
		FMulticastDelegateProperty* const CurrentProperty = *PropertyIt;

		UE_LOG(LogEnhancedAction, Log, TEXT("Process delegate %s"), *EAA::Internals::ToDebugString(CurrentProperty));
		
		UEdGraphPin* OutLastActivatedThenPin = nullptr;

		UEdGraphPin* PinForCurrentDelegateProperty = static_cast<UK2Node_EnhancedAsyncTaskBase*const>(this)->FindPin(CurrentProperty->GetFName());
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
			const FName GetContextFn = GET_MEMBER_NAME_CHECKED(UEnhancedAsyncActionContextLibrary, FindCaptureContextForObject);
			CallGetCtx->FunctionReference.SetExternalMember(GetContextFn, UEnhancedAsyncActionContextLibrary::StaticClass());
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

			if (!CallGetCtx->IsNodePure())
			{
				bIsErrorFree &= Schema->TryCreateConnection(OutLastActivatedThenPin, CallGetCtx->GetExecPin());
				OutLastActivatedThenPin = CallGetCtx->GetThenPin();
			}
		}

		check(ContextHandlePin);
		
		// CREATE CONTEXT ACCESSORS
		for (FOutputPinInfo& OutputPair : CaptureOutputs)
		{
			ensure(OutputPair.CaptureIndex != -1);

			// [temp var]      - [assign]
			// [context read]  - [value ]
			UK2Node_CallFunction* CallReadNode = nullptr;
			
			FName GetValueFunc;
			if (EAA::Internals::SelectAccessorForType(OutputPair.OutputPin, EAA::Internals::EAccessorRole::GETTER, GetValueFunc))
			{
				if (OutputPair.OutputPin->PinType.IsArray())
				{
					CallReadNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(this, SourceGraph);
				}
				else
				{
					CallReadNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
				}
				CallReadNode->FunctionReference.SetExternalMember(GetValueFunc, UEnhancedAsyncActionContextLibrary::StaticClass());
				CallReadNode->AllocateDefaultPins();
				ensure(CallReadNode->GetTargetFunction() != nullptr);
				ensure(!CallReadNode->IsNodePure());
			}

			if (!CallReadNode)
			{
				const FString FormattedMessage = FText::Format(
					LOCTEXT("AsyncTaskErrorFmt", "EnhancedAsyncTaskBase: no GETTER for {0} in async task @@"),
					FText::FromName(OutputPair.OutputPin->PinType.PinCategory)
				).ToString();
				CompilerContext.MessageLog.Error(*FormattedMessage, this);
				bIsErrorFree = false;
				continue;
			}

			bIsErrorFree &= Schema->TryCreateConnection(OutLastActivatedThenPin, CallReadNode->GetExecPin());
			OutLastActivatedThenPin = CallReadNode->GetThenPin();
			
			UK2Node_AssignmentStatement* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
			AssignNode->AllocateDefaultPins();

			// handle
			bIsErrorFree &= Schema->TryCreateConnection(CallReadNode->FindPinChecked(EAA::Internals::PIN_Handle), ContextHandlePin);
			// index
			Schema->TrySetDefaultValue(*CallReadNode->FindPinChecked(EAA::Internals::PIN_Index), FString::Printf(TEXT("%d"), OutputPair.CaptureIndex));
			// value
			bIsErrorFree &= Schema->TryCreateConnection(OutputPair.TempVar->GetVariablePin(), AssignNode->GetVariablePin());
			AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());
			// return
			UEdGraphPin* ValuePin = CallReadNode->FindPinChecked(EAA::Internals::PIN_Value);
			// todo: remove this hack and solve how to conform these two
			// GET_Object and GET_Class return UObject*& and has no PinCategoryObject set (should be forced to be wildcard?)
			// so TryCreateConnection fails. as it has PinCategoryObject known.
			// Force ValuePin PinCategoryObject to known value, in case types mismatch it will result in assert in execGetObject/GetValueObject
			ConformDynamicOutputPin(Schema, CallReadNode, ValuePin, AssignNode->GetVariablePin()->PinType);
			bIsErrorFree &= Schema->TryCreateConnection(AssignNode->GetValuePin(), ValuePin);
			AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());
			CallReadNode->NotifyPinConnectionListChanged(ValuePin);
			
			// exec
			bIsErrorFree &= Schema->TryCreateConnection(OutLastActivatedThenPin, AssignNode->GetExecPin());
			OutLastActivatedThenPin = AssignNode->GetThenPin();

			//
			ensure(bIsErrorFree);
		}

		// CREATE CHAIN OF ASSIGMENTS FOR CALLBACK
		for (FOutputPinInfo& OutputPair : VariableOutputs) 
		{
			UEdGraphPin* PinWithData = CurrentCENode->FindPin(OutputPair.GetName());
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

void UK2Node_EnhancedAsyncTaskBase::ConformDynamicOutputPin(const UEdGraphSchema_K2* Schema, UK2Node_CallFunction* Func, UEdGraphPin* Pin, const FEdGraphPinType& VarType)
{
	static const FName MD_GenericParam(TEXT("GenericParam"));

	const FString* GenericParam = Func->GetTargetFunction()->FindMetaData(MD_GenericParam);
	if (GenericParam &&
		(!GenericParam->Len() && Pin->GetName() == TEXT("Value") || (Pin->GetName() == *GenericParam))
	)
	{
		Pin->PinType = VarType;
		return;
	}

	auto DynamicPin = Func->GetTargetFunction()->FindMetaData(FBlueprintMetadata::MD_DynamicOutputParam);
	if (DynamicPin && *DynamicPin == Pin->GetName() && Pin->PinType.PinCategory == VarType.PinCategory)
	{
		const FName PinCategory = Pin->PinType.PinCategory;
		if ((PinCategory == UEdGraphSchema_K2::PC_Object) ||
			(PinCategory == UEdGraphSchema_K2::PC_Interface) ||
			(PinCategory == UEdGraphSchema_K2::PC_Class) ||
			(PinCategory == UEdGraphSchema_K2::PC_SoftObject) || 
			(PinCategory == UEdGraphSchema_K2::PC_SoftClass))
		{
			//UClass* ExpectedClass = CastChecked<UClass>(VarType.PinSubCategoryObject.Get());
			//UClass* GetterClass = CastChecked<UClass>(Pin->PinType.PinSubCategoryObject.Get());
			Pin->PinType = VarType;
		}
	}
}

void UK2Node_EnhancedAsyncTaskBase::OrphanCapturePins()
{
	// When context is not used by making them execs can make base implementation ignore them as they will be not a "Data pins" :D

	const FEdGraphPinType ExecPinType(UEdGraphSchema_K2::PC_Exec, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());
	
	for (int32 Index = 0; Index < NumCaptures; ++Index)
	{
		auto InPin = FindDynamicPin(EGPD_Input, Index);
		if (InPin)
		{
			//InPin->bOrphanedPin = true;
			InPin->PinType = ExecPinType;
		}

		auto OutPin = FindDynamicPin(EGPD_Output, Index);
		if (OutPin)
		{
			//OutPin->bOrphanedPin = true;
			OutPin->PinType = ExecPinType;
		}
	}
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

	if (!EAA::Internals::IsValidProxyClass(ProxyClass))
	{
		const FText ClassName = ProxyFactoryClass ? FText::FromString(ProxyFactoryClass->GetName()) : LOCTEXT("BadClassString", "Bad Class");
		const FString FormattedMessage = FText::Format(
			LOCTEXT("AsyncTaskErrorFmt", "EnhancedAsyncAction: {0} is invalid async task @@"),
			ClassName
		).ToString();

		CompilerContext.MessageLog.Error(*FormattedMessage, this);
		return;
	}
	
	// Is Context feature used on this node
	const bool bContextRequired = HasAnyLinkedCaptures();
	// Is Context setup needed on this node
	const bool bContextSetupRequired = bContextRequired && NeedSetupCaptureContext();

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
			LOCTEXT("AsyncTaskErrorFmt", "EnhancedAsyncTaskBase: Missing function {0} from class {1} for async task @@"),
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

	for (UEdGraphPin* CurrentPin : GetStandardPins(EGPD_Input))
	{
		if (FBaseAsyncTaskHelper::ValidDataPin(CurrentPin, EGPD_Input))
		{
			// match function inputs, to pass data to function from CallFunction node
			UEdGraphPin* DestPin = CallCreateProxyObjectNode->FindPin(CurrentPin->PinName);
			bIsErrorFree &= DestPin && CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *DestPin).CanSafeConnect();
			ensure(bIsErrorFree);
		}
	}

	UEdGraphPin* const ProxyObjectPin = CallCreateProxyObjectNode->GetReturnValuePin();
	check(ProxyObjectPin);

	// Validate proxy object
	
	UK2Node_CallFunction* IsValidFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	const FName IsValidFuncName = GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid);
	IsValidFuncNode->FunctionReference.SetExternalMember(IsValidFuncName, UKismetSystemLibrary::StaticClass());
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
		GET_MEMBER_NAME_CHECKED(UEnhancedAsyncActionContextLibrary, CreateContextForObject),
		UEnhancedAsyncActionContextLibrary::StaticClass()
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

	if (bContextSetupRequired)
	{
		const FString CapturesConfigString = EAA::Internals::BuildContextConfigString(this, NumCaptures);
		//UE_LOG(LogEnhancedAction, Log, TEXT("%p:Config=%s"), this, *CapturesConfigString);
	
		// Create a call to context setup
		UK2Node_CallFunction* const CallSetupCaptureNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallSetupCaptureNode->FunctionReference.SetExternalMember(
			GET_MEMBER_NAME_CHECKED(UEnhancedAsyncActionContextLibrary, SetupContextContainer),
			UEnhancedAsyncActionContextLibrary::StaticClass()
		);
		CallSetupCaptureNode->AllocateDefaultPins();

		bIsErrorFree &= Schema->TryCreateConnection(CaptureContextHandlePin, CallSetupCaptureNode->FindPinChecked(EAA::Internals::PIN_Handle));

		// Set config value
		UEdGraphPin* ConfigPin = CallSetupCaptureNode->FindPinChecked(TEXT("Config"));
		Schema->TrySetDefaultValue(*ConfigPin, CapturesConfigString);

		// Connect [CreateContext] and [SetupContext]
		bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, CallSetupCaptureNode->GetExecPin());
		LastThenPin = CallSetupCaptureNode->GetThenPin();
	}

	// Create capture setters 
	bIsErrorFree &= HandleSetContextData(CaptureContextHandlePin, LastThenPin, Schema, CompilerContext, SourceGraph);

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
