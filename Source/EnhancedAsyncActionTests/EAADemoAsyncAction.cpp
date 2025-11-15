// Copyright 2025, Aquanox.

#include "EAADemoAsyncAction.h"

#include "Async/Async.h"
#include "EnhancedAsyncActionShared.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Logging/StructuredLog.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EAADemoAsyncAction)

UEAADemoAsyncActionCapture* UEAADemoAsyncActionCapture::StartActionWithCapture(const UObject* WorldContextObject,  bool bDirectCall, const int32 UserIndex)
{
	auto Proxy = NewObject<ThisClass>();
	UE_LOGFMT(LogEnhancedAction, Verbose, "Construct Proxy {Func}", Proxy->GetName());
	Proxy->PayloadMode = bDirectCall ? EEAAPayloadMode::DIRECT : EEAAPayloadMode::TIMER;
	Proxy->LocalUserIndex = UserIndex;
	Proxy->World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	Proxy->RegisterWithGameInstance(WorldContextObject);
	return Proxy;
}

void UEAADemoAsyncActionCapture::Activate()
{
	auto Self = MakeWeakObjectPtr(this);
	
	UE_LOGFMT(LogEnhancedAction, Verbose, "Activate {Func}", Self->GetName());
	if (PayloadMode == EEAAPayloadMode::ASYNC)
	{
		AsyncTask(ENamedThreads::GameThread, [Self]() { ensure(Self.IsValid()); Self->InvokePayload(); });
	}
	else if (PayloadMode == EEAAPayloadMode::DIRECT)
	{
		Self->InvokePayload();
	}
	else if (PayloadMode == EEAAPayloadMode::TIMER)
	{
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, [Self](){ ensure(Self.IsValid()); Self->InvokePayload(); }, 1.0f, false);
	}
}

void UEAADemoAsyncActionCapture::InvokePayload()
{
	SetReadyToDestroy();
	
	UE_LOGFMT(LogEnhancedAction, Verbose, "Payload {Func}", GetName());
	OnCompleted.Broadcast(this, LocalUserIndex, TEXT("username"), TArray<int32>());
}

UEAADemoAsyncActionCaptureMember::UEAADemoAsyncActionCaptureMember()
{

}

UEAADemoAsyncActionCaptureMember* UEAADemoAsyncActionCaptureMember::StartActionWithCaptureFixed(const UObject* WorldContextObject,  bool bDirectCall, const int32 UserIndex)
{
	auto Proxy = NewObject<ThisClass>();
	UE_LOGFMT(LogEnhancedAction, Verbose, "Construct Proxy {Func}", Proxy->GetName());
	Proxy->PayloadMode = bDirectCall ? EEAAPayloadMode::DIRECT : EEAAPayloadMode::TIMER;
	Proxy->LocalUserIndex = UserIndex;
	Proxy->World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	Proxy->RegisterWithGameInstance(WorldContextObject);
	return Proxy;
}
