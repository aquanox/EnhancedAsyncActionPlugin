// Fill out your copyright notice in the Description page of Project Settings.


#include "EAADemoAsyncActionStandard.h"
#include "EnhancedAsyncActionShared.h"
#include "Engine/Engine.h"
#include "Logging/StructuredLog.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EAADemoAsyncActionStandard)

UEAADemoAsyncActionStandard* UEAADemoAsyncActionStandard::StartActionPseudoCapture(const UObject* WorldContextObject, int32 UserIndex, FInstancedStruct Captures)
{
	auto* Proxy = NewObject<ThisClass>();
	UE_LOGFMT(LogEnhancedAction, Verbose, "Construct Proxy {Func}", Proxy->GetName());
	Proxy->LocalCaptures = MoveTemp(Captures);
	Proxy->LocalUserIndex = UserIndex;
	Proxy->LocalWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	return Proxy;
}

void UEAADemoAsyncActionStandard::Activate()
{
	FTimerHandle Handle;
	LocalWorld->GetTimerManager().SetTimer(Handle, [Owner = MakeWeakObjectPtr(this)]()
	{
		if (ThisClass* Self = Owner.Get())
		{
			Self->OnCompleted.Broadcast(Self->LocalCaptures, Self->LocalUserIndex, TEXT("Username"), TArray<int32>());
		}
	}, 1.0f, false);
}

UEAADemoAsyncAction* UEAADemoAsyncAction::StartDemoAction(const UObject* WorldContextObject, int32 UserIndex)
{
	auto* Proxy = NewObject<ThisClass>();
	UE_LOGFMT(LogEnhancedAction, Verbose, "Construct Proxy {Func}", Proxy->GetName());
	Proxy->LocalUserIndex = UserIndex;
	Proxy->LocalWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	return Proxy;
}

void UEAADemoAsyncAction::Activate()
{
	UE_LOGFMT(LogEnhancedAction, Verbose, "Activate {Func}", GetName());
	
	FTimerHandle Handle;
	LocalWorld->GetTimerManager().SetTimer(Handle, [Owner = MakeWeakObjectPtr(this)]()
	{
		if (ThisClass* Self = Owner.Get())
		{
			Self->OnCompleted.Broadcast(Self->LocalUserIndex, TEXT("Username"), TArray<int32>());
		}
	}, 1.0f, false);
}
