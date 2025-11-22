// Copyright 2025, Aquanox.

#pragma once

#include "UObject/Object.h"
#include "UObject/GeneratedCppIncludes.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"

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

struct FTestBlueprintScope : public FTestWorldScope
{
	UObject* MostRecentObject = nullptr;

	template<typename T = UObject>
	T* Create(const FSoftClassPath& InPath)
	{
		auto* Object = NewObject<UObject>(GetTransientPackage(), InPath.TryLoadClass<T>() );
		ensureAlways(Object != nullptr);
		MostRecentObject = Object;
		return CastChecked<T>(Object);
	}

	template<typename T = AActor>
	T* SpawnActor(const FSoftClassPath& InPath)
	{
		auto* Object = World->SpawnActor(InPath.TryLoadClass<AActor>());
		ensureAlways(Object != nullptr);
		MostRecentObject = Object;
		return CastChecked<T>(Object);
	}

	template<typename TNative, typename TProperty = FProperty>
	static TNative* FindProperty(const UObject* Object, const TCHAR* InName, int32 Index = 0)
	{
		auto* Prop = FindFProperty<TProperty>(Object->GetClass(), InName);
		if (!ensureAlways(Prop)) return nullptr;
		return Prop->template ContainerPtrToValuePtr<TNative>(Object, Index);
	}
};

#if 1
#define XTEST_TRUE_EXPR(Expression)\
	if (!TestTrue(TEXT(#Expression), Expression))\
	{\
	ensureAlwaysMsgf(false, TEXT("Condition failed: %s"), TEXT(#Expression)); \
	return false;\
	}

#define XTEST_FALSE_EXPR(Expression)\
	if (!TestFalse(TEXT(#Expression), Expression))\
	{\
	ensureAlwaysMsgf(false, TEXT("Condition failed: %s"), TEXT(#Expression)); \
	return false;\
	}

#define XTEST_VALID_EXPR(Value)\
	if (!TestValid(TEXT(#Value), Value))\
	{\
	ensureAlwaysMsgf(false, TEXT("Condition failed: %s"), TEXT(#Value)); \
	return false;\
	}

#define XTEST_INVALID_EXPR(Value)\
	if (!TestInvalid(TEXT(#Value), Value))\
	{\
	ensureAlwaysMsgf(false, TEXT("Condition failed: %s"), TEXT(#Value)); \
	return false;\
	}

#else
#define XTEST_TRUE_EXPR(Expression) UTEST_TRUE_EXPR(Expression)
#define XTEST_FALSE_EXPR(Expression) UTEST_FALSE_EXPR(Expression)
#define XTEST_VALID_EXPR(Expression) UTEST_VALID_EXPR(Expression)
#define XTEST_INVALID_EXPR(Expression) UTEST_INVALID_EXPR(Expression)
#endif
