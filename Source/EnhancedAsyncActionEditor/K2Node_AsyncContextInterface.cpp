// Copyright 2025, Aquanox.

#include "K2Node_AsyncContextInterface.h"
#include "EnhancedAsyncContextShared.h"
#include "EnhancedAsyncContextPrivate.h"
#include "EdGraph/EdGraphPin.h"

FEnhancedAsyncTaskCapture::FEnhancedAsyncTaskCapture(int32 InIndex, UEdGraphPin* Input, UEdGraphPin* Output)
{
	Index = InIndex;
	InputPinName = Input->PinName;
	OutputPinName = Output->PinName;
}

FName IK2Node_AsyncContextInterface::GetCapturePinName(int32 PinIndex, EEdGraphPinDirection Dir) const
{
	return EAA::Internals::IndexToPinName(PinIndex, Dir == EGPD_Input);
}

int32 IK2Node_AsyncContextInterface::GetMaxCapturePins() const
{
	return EAA::Internals::MaxCapturePins;
}
