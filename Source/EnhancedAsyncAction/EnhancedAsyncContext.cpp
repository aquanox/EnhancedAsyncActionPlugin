// Copyright 2025, Aquanox.

#include "EnhancedAsyncContext.h"

#include "EnhancedAsyncContextShared.h"

bool FEnhancedAsyncActionContext::SetValueByIndex(int32 Index, const FProperty* Property, const void* Value, FString& Message)
{
	return SetValueByName(EAA::Internals::IndexToName(Index), Property, Value, Message);
}

bool FEnhancedAsyncActionContext::GetValueByIndex(int32 Index, const FProperty* Property, void* OutValue, FString& Message)
{
	return GetValueByName(EAA::Internals::IndexToName(Index), Property, OutValue, Message);
}
