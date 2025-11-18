// Copyright 2025, Aquanox.

#include "EnhancedAsyncContextShared.h"

#include "StructUtils/PropertyBag.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/TextProperty.h"
#include "EnhancedAsyncContextLibrary.h"

DEFINE_LOG_CATEGORY(LogEnhancedAction);

struct FLocalNameTable
{
	enum { MaxIndex = std::max(EAA::Internals::MaxCapturePins, 64) };

	TArray<FName, TInlineAllocator<MaxIndex>> CapturePropertyNames;
	TArray<FName, TInlineAllocator<MaxIndex>> InputPinNames;
	TArray<FName, TInlineAllocator<MaxIndex>> OutputPinNames;
	TArray<TPair<FName, FName>, TInlineAllocator<MaxIndex>> MirrorTable;

	static FLocalNameTable& Get()
	{
		static FLocalNameTable Instance;
		return Instance;
	}
private:
	FLocalNameTable()
	{
		CapturePropertyNames.SetNum(MaxIndex);
		InputPinNames.SetNum(MaxIndex);
		OutputPinNames.SetNum(MaxIndex);
		MirrorTable.SetNum(MaxIndex);
		for (int32 I = 0; I < MaxIndex; I++)
		{
			CapturePropertyNames[I] = *FString::Printf(TEXT("ctx%02d"), I);
			InputPinNames[I] = *FString::Printf(TEXT("In[%d]"), I);
			OutputPinNames[I] = *FString::Printf(TEXT("Out[%d]"), I);

			MirrorTable[I] = TPair<FName, FName>(InputPinNames[I], OutputPinNames[I]);
		}
	}
};

FName EAA::Internals::IndexToName(int32 Index)
{
	auto& CapturePropertyNames = FLocalNameTable::Get().CapturePropertyNames;
	return CapturePropertyNames.IsValidIndex(Index) ? CapturePropertyNames[Index] : NAME_Name;
}

int32 EAA::Internals::NameToIndex(const FName& Name)
{
	auto& CapturePropertyNames = FLocalNameTable::Get().CapturePropertyNames;
	int32 Idx = CapturePropertyNames.IndexOfByKey(Name);
	check(Idx != INDEX_NONE);
	return Idx;
}

FName EAA::Internals::IndexToPinName(int32 Index, bool bIsInput)
{
	FLocalNameTable& Table = FLocalNameTable::Get();
	const auto& Container = bIsInput ? Table.InputPinNames : Table.OutputPinNames;
	check(Container.IsValidIndex(Index));
	return Container[Index];
}

int32 EAA::Internals::PinNameToIndex(const FName& Name, bool bIsInput)
{
	FLocalNameTable& Table = FLocalNameTable::Get();
	const int32 Idx = (bIsInput ? Table.InputPinNames : Table.OutputPinNames).IndexOfByKey(Name);
	check(Idx != INDEX_NONE);
	return Idx;
}

int32 EAA::Internals::FindPinIndex(const FName& Name)
{
	FLocalNameTable& Table = FLocalNameTable::Get();
	const int32 InputIndex = Table.InputPinNames.IndexOfByKey(Name);
	if (InputIndex != INDEX_NONE)
		return InputIndex;
	const int32 OutputIndex = Table.OutputPinNames.IndexOfByKey(Name);
	if (OutputIndex != INDEX_NONE)
		return OutputIndex;
	checkNoEntry();
	return INDEX_NONE;
}

FName EAA::Internals::MirrorPinName(const FName& Name)
{
	FLocalNameTable& Table = FLocalNameTable::Get();
	const int32 InputIndex = Table.InputPinNames.IndexOfByKey(Name);
	if (InputIndex != INDEX_NONE)
		return Table.OutputPinNames[InputIndex];
	const int32 OutputIndex = Table.OutputPinNames.IndexOfByKey(Name);
	if (OutputIndex != INDEX_NONE)
		return Table.InputPinNames[OutputIndex];
	checkNoEntry();
	return NAME_None;
}

bool EAA::Internals::IsValidContainerProperty(const UObject* Object, const FName& Property)
{
	if (Property.IsNone() || !IsValid(Object))
		return false;

	const UClass* Class = Object->IsA(UClass::StaticClass()) ? CastChecked<const UClass>(Object) : Object->GetClass();

	FStructProperty* StructProperty = CastField<FStructProperty>(Class->FindPropertyByName(Property));
	if (!StructProperty)
		return false;

	return StructProperty->Struct && StructProperty->Struct->IsChildOf(FInstancedPropertyBag::StaticStruct());
}
