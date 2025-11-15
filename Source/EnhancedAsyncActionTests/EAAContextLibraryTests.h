// Copyright 2025, Aquanox.

#pragma once

#include "Engine/Engine.h"
#include "EAADemoAsyncAction.h"
#include "EAAContextLibraryTests.generated.h"

struct FTestWorldScope
{
	FWorldContext WorldContext;
	UWorld* World = nullptr;
	UWorld* PrevWorld = nullptr;

	FTestWorldScope()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);

		WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);

		PrevWorld = &*GWorld;
		GWorld = World;
	}

	~FTestWorldScope()
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(true);
		GWorld = PrevWorld;
	}

	UWorld* operator->() const { return World; }
};


enum
{
	Test_Prop_Bool,
	
	Test_Prop_Byte,
	Test_Prop_Int32,
	Test_Prop_Int64,
	Test_Prop_Float,
	Test_Prop_Double,
	
	Test_Prop_String,
	Test_Prop_Name,
	Test_Prop_Text,
	
	Test_Prop_Struct,
	Test_Prop_Enum,
	Test_Prop_Object,
	Test_Prop_Class,
	
	Test_Prop_Array,
	Test_Prop_Set,
};

USTRUCT()
struct FEAAContextTestStruct
{
	GENERATED_BODY()
	
};
