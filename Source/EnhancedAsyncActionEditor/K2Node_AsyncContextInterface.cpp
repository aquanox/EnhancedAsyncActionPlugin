// Copyright 2025, Aquanox.

#include "K2Node_AsyncContextInterface.h"
#include "EnhancedAsyncContextShared.h"
#include "EnhancedAsyncContextPrivate.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"
#include "ToolMenu.h"

#define LOCTEXT_NAMESPACE "FEnhancedAsyncTaskCapture"

FEnhancedAsyncTaskCapture::FEnhancedAsyncTaskCapture(int32 InIndex, UEdGraphPin* Input, UEdGraphPin* Output)
{
	Index = InIndex;
	InputPinName = Input->PinName;
	OutputPinName = Output->PinName;
}

UK2Node* IK2Node_AsyncContextInterface::GetNodeObject() const
{
	return CastChecked<UK2Node>(_getUObject());
}

FName IK2Node_AsyncContextInterface::GetCapturePinName(int32 PinIndex, EEdGraphPinDirection Dir) const
{
	return EAA::Internals::IndexToPinName(PinIndex, Dir == EGPD_Input);
}

int32 IK2Node_AsyncContextInterface::GetMaxCapturePins() const
{
	return EAA::Internals::MaxCapturePins;
}

bool IK2Node_AsyncContextInterface::CanAddPin() const
{
	return GetNumCaptures() < GetMaxCapturePins();
}

void IK2Node_AsyncContextInterface::AddInputPin()
{
	FScopedTransaction Transaction(LOCTEXT("AddPinTx", "AddPin"));
	GetNodeObject()->Modify();
	AddCaptureInternal();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetNodeObject()->GetBlueprint());
}

bool IK2Node_AsyncContextInterface::CanRemovePin(const UEdGraphPin* Pin) const
{
	return Pin && IsCapturePin(Pin);
}

void IK2Node_AsyncContextInterface::RemoveInputPin(UEdGraphPin* Pin)
{
	check(Pin->ParentPin == nullptr);

	const int32 CaptureIndex = IndexOfCapturePin(Pin);
	if (CaptureIndex == INDEX_NONE)
	{
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("RemovePinTx", "RemovePin"));
	GetNodeObject()->Modify();

	RemoveCaptureInternal(CaptureIndex);

	SyncPinIndexesAndNames();

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetNodeObject()->GetBlueprint());
}

void IK2Node_AsyncContextInterface::AddCaptureInternal()
{
	auto& Captures = GetMutableCapturesArray();
	const int32 Index = Captures.Num();

	UEdGraphNode::FCreatePinParams PinParams;
	PinParams.bIsConst = true;
	PinParams.bIsReference = true;

	UEdGraphPin* In = GetNodeObject()->CreatePin(EEdGraphPinDirection::EGPD_Input,
								UEdGraphSchema_K2::PC_Wildcard,
								GetCapturePinName(Index, EEdGraphPinDirection::EGPD_Input),
								PinParams);
	In->SourceIndex = Index;
	In->bDefaultValueIsIgnored = true;
	UEdGraphPin* Out = GetNodeObject()->CreatePin(EEdGraphPinDirection::EGPD_Output,
								 UEdGraphSchema_K2::PC_Wildcard,
								 GetCapturePinName(Index, EEdGraphPinDirection::EGPD_Output)
	);
	Out->SourceIndex = Index;

	Captures.Add(FEnhancedAsyncTaskCapture(Index, In, Out));
}

void IK2Node_AsyncContextInterface::RemoveCaptureInternal(int32 ArrayIndex)
{
	auto& Captures = GetMutableCapturesArray();
	check(Captures.IsValidIndex(ArrayIndex));

	auto& Pins = GetNodeObject()->Pins;

	TFunction<void(UEdGraphPin*)> RemovePinLambda = [&Pins, &RemovePinLambda](UEdGraphPin* PinToRemove)
	{
		for (int32 SubPinIndex = PinToRemove->SubPins.Num() - 1; SubPinIndex >= 0; --SubPinIndex)
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

	const FEnhancedAsyncTaskCapture CapureInfo = Captures[ArrayIndex];
	Captures.RemoveAt(ArrayIndex);

	if (UEdGraphPin* InPin = GetNodeObject()->FindPin(CapureInfo.InputPinName, EEdGraphPinDirection::EGPD_Input))
	{
		InPin->BreakAllPinLinks(false);
		RemovePinLambda(InPin);
	}
	if (UEdGraphPin* OutPin = GetNodeObject()->FindPin(CapureInfo.OutputPinName, EEdGraphPinDirection::EGPD_Output))
	{
		OutPin->BreakAllPinLinks(false);
		RemovePinLambda(OutPin);
	}
}

bool IK2Node_AsyncContextInterface::TestConnectPins(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	if (IsCapturePin(MyPin) && OtherPin)
	{
		if (OtherPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec
		 || OtherPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Delegate)
		{
			OutReason = FString::Printf(TEXT("Can not capture pin"));
			return false;
		}
		if (OtherPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
		{
			if (!EAA::Internals::IsCapturableType(OtherPin->PinType))
			{
				OutReason = FString::Printf(TEXT("Can not capture pin of type %s"), *OtherPin->PinType.PinCategory.ToString());
				return false;
			}
		}
	}
	return true;
}

int32 IK2Node_AsyncContextInterface::IndexOfCapturePin(const UEdGraphPin* Pin) const
{
	if (Pin->SourceIndex == INDEX_NONE)
		return false;

	auto& Captures = GetCapturesArray();
	return Captures.IndexOfByPredicate([&Pin](const FEnhancedAsyncTaskCapture& Capture)
	{
		return Capture.NameOf(Pin->Direction) == Pin->PinName;
	});
}

UEdGraphPin* IK2Node_AsyncContextInterface::FindCapturePin(int32 PinIndex, EEdGraphPinDirection Dir) const
{
	auto& Captures = GetCapturesArray();
	ensure(Captures.IsValidIndex(PinIndex));
	const FName CapturePinName = GetCapturePinName(PinIndex, Dir);
	ensure(Captures[PinIndex].NameOf(Dir) == CapturePinName);
	return GetNodeObject()->FindPin(CapturePinName, Dir);
}

UEdGraphPin* IK2Node_AsyncContextInterface::FindCapturePinChecked(int32 PinIndex, EEdGraphPinDirection Dir) const
{
	auto& Captures = GetCapturesArray();
	ensure(Captures.IsValidIndex(PinIndex));
	const FName CapturePinName = GetCapturePinName(PinIndex, Dir);
	ensure(Captures[PinIndex].NameOf(Dir) == CapturePinName);
	return GetNodeObject()->FindPinChecked(CapturePinName, Dir);
}

void IK2Node_AsyncContextInterface::ForEachCapturePin(EEdGraphPinDirection Dir, TFunction<bool(int32, UEdGraphPin*)> const& Func) const
{
	auto& Captures = GetCapturesArray();
	for (int32 Index = 0; Index < Captures.Num(); ++Index)
	{
		const FEnhancedAsyncTaskCapture& CaptureInfo = Captures[Index];

		UEdGraphPin* Pin = GetNodeObject()->FindPin(CaptureInfo.NameOf(Dir), Dir);
		ensureAlways(Pin && Pin->SourceIndex == CaptureInfo.Index);
		if (!Func(CaptureInfo.Index, Pin))
		{
			break;
		}
	}
}

void IK2Node_AsyncContextInterface::ForEachCapturePinPair(TFunction<bool(int32, UEdGraphPin*, UEdGraphPin*)> const& Func) const
{
	auto& Captures = GetCapturesArray();
	for (int32 Index = 0; Index < Captures.Num(); ++Index)
	{
		const FEnhancedAsyncTaskCapture& CaptureInfo = Captures[Index];

		UEdGraphPin* InPin = GetNodeObject()->FindPin(CaptureInfo.InputPinName, EGPD_Input);
		ensureAlways(InPin && InPin->SourceIndex == CaptureInfo.Index);
		UEdGraphPin* OutPin = GetNodeObject()->FindPin(CaptureInfo.OutputPinName, EGPD_Output);
		ensureAlways(OutPin && OutPin->SourceIndex == CaptureInfo.Index);
		if (!Func(CaptureInfo.Index, InPin, OutPin))
		{
			break;
		}
	}
}

bool IK2Node_AsyncContextInterface::IsCapturePin(const UEdGraphPin* Pin) const
{
	if (Pin->ParentPin != nullptr || Pin->bOrphanedPin || Pin->SourceIndex == INDEX_NONE)
		return false;
	return IndexOfCapturePin(Pin) != INDEX_NONE;
}


void IK2Node_AsyncContextInterface::SyncPinIndexesAndNames()
{
	auto SyncPin = [this](FEnhancedAsyncTaskCapture& Info, UEdGraphPin* CurrentPin, int32 CurrentIndex)
	{
		check(CurrentPin);

		const FName OldName = CurrentPin->PinName;
		const FName ElementName = GetCapturePinName(CurrentIndex, CurrentPin->Direction);

		CurrentPin->Modify();
		CurrentPin->PinName = ElementName;
		CurrentPin->SourceIndex = CurrentIndex;

		Info.NameOf(CurrentPin->Direction) = ElementName;
	};

	auto& Captures = GetMutableCapturesArray();
	for (int32 Index = 0; Index < Captures.Num(); ++Index)
	{
		FEnhancedAsyncTaskCapture& CaptureInfo = Captures[Index];
		const int32 OldIndex = CaptureInfo.Index;

		CaptureInfo.Index = Index;
		if (UEdGraphPin* InPin = GetNodeObject()->FindPinChecked(CaptureInfo.InputPinName, EGPD_Input))
			SyncPin(CaptureInfo, InPin, Index);

		if (UEdGraphPin* OutPin = GetNodeObject()->FindPinChecked(CaptureInfo.OutputPinName, EGPD_Output))
			SyncPin(CaptureInfo, OutPin, Index);
	}
}

void IK2Node_AsyncContextInterface::SyncCapturePinTypes()
{
	ForEachCapturePinPair([this](int32 Index, UEdGraphPin* In, UEdGraphPin* Out)
	{
		SynchronizeCapturePinType(In, Out);
		return true;
	});
}

void IK2Node_AsyncContextInterface::SynchronizeCapturePinType(UEdGraphPin* Pin)
{
	int32 CaptureIndex = IndexOfCapturePin(Pin);
	check(CaptureIndex != INDEX_NONE);

	UEdGraphPin* InputPin = FindCapturePinChecked(CaptureIndex, EGPD_Input);
	UEdGraphPin* OutputPin = FindCapturePinChecked(CaptureIndex, EGPD_Output);

	SynchronizeCapturePinType(InputPin, OutputPin);
}

void IK2Node_AsyncContextInterface::SynchronizeCapturePinType(UEdGraphPin* InputPin, UEdGraphPin* OutputPin)
{
	check(InputPin && OutputPin);

	FEdGraphPinType NewType = EAA::Internals::DetermineCommonPinType(InputPin, OutputPin);

	bool bPinTypeChanged = false;
	auto ApplyPinType = [&bPinTypeChanged](UEdGraphPin* LocalPin, FEdGraphPinType Type)
	{
		Type.bIsReference = LocalPin->Direction == EGPD_Input;
		Type.bIsConst = LocalPin->Direction == EGPD_Input;

		if (LocalPin->PinType != Type)
		{
			LocalPin->PinType = Type;
			bPinTypeChanged = true;
		}
	};
	ApplyPinType(InputPin, NewType);
	ApplyPinType(OutputPin, NewType);

	if (bPinTypeChanged)
	{
		InputPin->Modify();
		OutputPin->Modify();

		// Let the graph know to refresh
		GetNodeObject()->GetGraph()->NotifyNodeChanged(GetNodeObject());

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetNodeObject()->GetBlueprint());
	}
}

bool IK2Node_AsyncContextInterface::AnyCapturePinHasLinks() const
{
	bool bHasLinks = false;
	ForEachCapturePinPair([&](int32 Index, UEdGraphPin* InPin, UEdGraphPin* OutPin)
	{
		if (InPin->LinkedTo.Num() || OutPin->LinkedTo.Num())
		{
			bHasLinks = true;
			return false;
		}
		return true;
	});
	return bHasLinks;
}


void IK2Node_AsyncContextInterface::GetStandardPins(EEdGraphPinDirection Dir, TArray<UEdGraphPin*> &OutPins) const
{
	auto& Pins = GetNodeObject()->Pins;
	OutPins.Reserve(Pins.Num());
	for (UEdGraphPin* const Pin : Pins)
	{
		if (Pin->Direction == Dir && !IsCapturePin(Pin))
		{
			OutPins.Add(Pin);
		}
	}
}

void IK2Node_AsyncContextInterface::OrphanCapturePins()
{
	// When context is not used by making them execs can make base implementation ignore them as they will be not a "Data pins" :D

	const FEdGraphPinType ExecPinType(UEdGraphSchema_K2::PC_Exec, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType());

	ForEachCapturePinPair([&](int32 Index, UEdGraphPin* InPin, UEdGraphPin* OutPin)
	{
		InPin->PinType = ExecPinType;
		OutPin->PinType = ExecPinType;
		return true;
	});
}

void FK2Node_AsyncContextMenuActions::SetupActions(UK2Node* Node, UToolMenu* Menu, UGraphNodeContextMenuContext* Context)
{
	static const FName NodeSection = FName("AsyncContextNode");
	static const FText NodeSectionDesc = LOCTEXT("AsyncContextNode", "Async Context Node");

	IK2Node_AsyncContextInterface* Obj = Cast<IK2Node_AsyncContextInterface>(Node);
	check(Obj);

	const TWeakObjectPtr<UK2Node> WeakThis = MakeWeakObjectPtr(Node);

	if (Obj->CanRemovePin(Context->Pin))
	{
		FToolMenuSection& Section = Menu->FindOrAddSection(NodeSection, NodeSectionDesc);
		Section.AddMenuEntry(
			"RemovePin",
			LOCTEXT("RemovePin", "Remove capture pin"),
			LOCTEXT("RemovePinTooltip", "Remove selected capture pin"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&FK2Node_AsyncContextMenuActions::RemoveCapturePin, WeakThis, Context->Pin)
			)
		);
	}
	else if (Obj->CanAddPin())
	{
		FToolMenuSection& Section = Menu->FindOrAddSection(NodeSection, NodeSectionDesc);
		Section.AddMenuEntry(
			"AddPin",
			LOCTEXT("AddPin", "Add capture pin"),
			LOCTEXT("AddPinTooltip", "Add capture pin pair"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&FK2Node_AsyncContextMenuActions::AddCapturePin, WeakThis)
			)
		);
	}
	if (!Context->Pin && Obj->GetNumCaptures() > 0)
	{
		FToolMenuSection& Section = Menu->FindOrAddSection(NodeSection, NodeSectionDesc);
		Section.AddMenuEntry(
			"UnlinkAllCapture",
			LOCTEXT("UnlinkAllCapture", "Unlink all capture pins"),
			LOCTEXT("UnlinkAllCaptureTooltip", "Unlink all capture pins"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&FK2Node_AsyncContextMenuActions::BreakAllCapturePins, WeakThis)
			)
		);
		Section.AddMenuEntry(
			"RemoveAllCapture",
			LOCTEXT("RemoveAllCapture", "Remove all capture pins"),
			LOCTEXT("RemoveAllCaptureTooltip", "Remove all capture pins"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&FK2Node_AsyncContextMenuActions::RemoveAllCapturePins, WeakThis)
			)
		);
		Section.AddMenuEntry(
			"RemoveUnusedCapture",
			LOCTEXT("RemoveUnusedCapture", "Remove unused capture pins"),
			LOCTEXT("RemoveUnusedCaptureTooltip", "unused unlinked capture pins"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&FK2Node_AsyncContextMenuActions::RemoveUnusedCapturePins, WeakThis)
			)
		);
	}
}

void FK2Node_AsyncContextMenuActions::AddCapturePin(TWeakObjectPtr<UK2Node> NodeRef)
{
	if (IK2Node_AsyncContextInterface* Obj = Cast<IK2Node_AsyncContextInterface>(NodeRef.Get()))
	{
		FScopedTransaction Transaction(LOCTEXT("AddPinTx", "AddPin"));
		Obj->GetNodeObject()->Modify();
		Obj->AddCaptureInternal();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Obj->GetNodeObject()->GetBlueprint());
	}
}

void FK2Node_AsyncContextMenuActions::RemoveCapturePin(TWeakObjectPtr<UK2Node> NodeRef, const UEdGraphPin* Pin)
{
	if (IK2Node_AsyncContextInterface* Obj = Cast<IK2Node_AsyncContextInterface>(NodeRef.Get()))
	{
		const int32 CaptureIndex = Obj->IndexOfCapturePin(Pin);
		if (CaptureIndex != INDEX_NONE)
		{
			FScopedTransaction Transaction(LOCTEXT("RemovePinTx", "RemovePin"));
			Obj->GetNodeObject()->Modify();
			Obj->RemoveCaptureInternal(CaptureIndex);
			Obj->SyncPinIndexesAndNames();
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Obj->GetNodeObject()->GetBlueprint());
		}
	}
}

void FK2Node_AsyncContextMenuActions::BreakAllCapturePins(TWeakObjectPtr<UK2Node> NodeRef)
{
	if (IK2Node_AsyncContextInterface* Obj = Cast<IK2Node_AsyncContextInterface>(NodeRef.Get()))
	{
		UK2Node* const Node = Obj->GetNodeObject();

		FScopedTransaction Transaction(LOCTEXT("UnlinkAllDynamicPinsTx", "UnlinkAllCapturePins"));
		Node->Modify();

		Obj->ForEachCapturePinPair([](int32 Index, UEdGraphPin* InPin, UEdGraphPin* OutPin)
		{
			if (InPin)
			{
				InPin->BreakAllPinLinks(true);
				InPin->PinType = EAA::Internals::GetWildcardType();
			}
			if (OutPin)
			{
				OutPin->BreakAllPinLinks(true);
				OutPin->PinType = EAA::Internals::GetWildcardType();
			}
			return true;
		});

		Node->GetGraph()->NotifyNodeChanged(Node);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Node->GetBlueprint());
	}
}

void FK2Node_AsyncContextMenuActions::RemoveAllCapturePins(TWeakObjectPtr<UK2Node> NodeRef)
{
	if (IK2Node_AsyncContextInterface* Obj = Cast<IK2Node_AsyncContextInterface>(NodeRef.Get()))
	{
		UK2Node* const Node = Obj->GetNodeObject();

		FScopedTransaction Transaction(LOCTEXT("RemoveAllCapturePinsTx", "RemoveAllCapturePins"));
		Node->Modify();

		for (int Index = Obj->GetNumCaptures() - 1; Index >= 0; --Index)
		{
			Obj->RemoveCaptureInternal(Index);
		}

		check(Obj->GetNumCaptures() == 0);

		Node->GetGraph()->NotifyNodeChanged(Node);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Node->GetBlueprint());
	}
}

void FK2Node_AsyncContextMenuActions::RemoveUnusedCapturePins(TWeakObjectPtr<UK2Node> NodeRef)
{
	if (IK2Node_AsyncContextInterface* Obj = Cast<IK2Node_AsyncContextInterface>(NodeRef.Get()))
	{
		UK2Node* const Node = Obj->GetNodeObject();

		FScopedTransaction Transaction(LOCTEXT("RemoveUnusedCapturePinsTx", "RemoveUnusedCapturePins"));
		Node->Modify();

		TArray<int32, TInlineAllocator<16>> IndexesToRemove;
		Obj->ForEachCapturePinPair([&IndexesToRemove](int32 Index, UEdGraphPin* InPin, UEdGraphPin* OutPin)
		{
			if (InPin->LinkedTo.Num() == 0 && OutPin->LinkedTo.Num() == 0)
			{
				IndexesToRemove.Add(Index);
			}
			return true;
		});

		for (int Index = IndexesToRemove.Num() - 1; Index >= 0; --Index)
		{
			Obj->RemoveCaptureInternal(IndexesToRemove[Index]);
		}

		Obj->SyncPinIndexesAndNames();

		Node->GetGraph()->NotifyNodeChanged(Node);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Node->GetBlueprint());
	}
}

#undef LOCTEXT_NAMESPACE
