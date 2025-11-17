// Copyright 2025, Aquanox.

#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "StructUtils/PropertyBag.h"
#include "EAADemoAsyncAction.generated.h"

UENUM(BlueprintType)
enum class EEAAPayloadMode : uint8
{
	DIRECT,
	ASYNC,
	TIMER
};

USTRUCT(BlueprintType)
struct FEAACaptureContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString ParamA;
	UPROPERTY(BlueprintReadWrite)
	int32 ParamB = 0;
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UObject> ParamC;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FEAADemoResult,
	const UObject*, Context,
	int32, UserIndex,
	FString, UserName,
	const TArray<int32>&, Scores);

/**
 * This is an example of using "async with capture" using plugin feature.
 *
 * The requirements:
 * - `HasDedicatedAsyncNode` to hide default UE async node from search
 * - `HasAsyncContext=Context` to mark property in delegate and mark this type supported
 *
 * Then delegate still passes the capture to the subscriber
 */
UCLASS(MinimalAPI, meta=(HasDedicatedAsyncNode, HasAsyncContext=Context))
class UEAADemoAsyncActionCapture : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FEAADemoResult OnCompleted;

	UFUNCTION(BlueprintCallable, Category="EAA|Demo", DisplayName="Load Stats (Basic)", meta=( BlueprintInternalUseOnly=true, WorldContext = "WorldContextObject"))
	static UEAADemoAsyncActionCapture* StartActionWithCapture(const UObject* WorldContextObject, bool bDirectCall, const int32 UserIndex);

	virtual void Activate() override;

	void InvokePayload();

protected:
	UPROPERTY()
	TObjectPtr<UWorld> World;
	UPROPERTY()
	int32 LocalUserIndex;
	UPROPERTY()
	EEAAPayloadMode PayloadMode;
};

/**
 * This is an example of using "async with capture" using plugin feature AND localized data.
 *
 * The benefits are:
 * - data stored within action and has all UI features
 * - GC exposure by UPROPERTY (not by Capture Manager)
 *
 * The requirements:
 * - `HasDedicatedAsyncNode` to hide from default UE nodes
 * - `HasAsyncContext=Context` to mark property in delegate and mark this type supported
 *
 * - `AsyncContextContainer=MemberName` to mark member property as a container for context
 *
 * Then delegate still passes the capture to the subscriber
 */
UCLASS(MinimalAPI, meta=(HasDedicatedAsyncNode, HasAsyncContext=Context, AsyncContextContainer="ContextData"))
class UEAADemoAsyncActionCaptureMember : public UEAADemoAsyncActionCapture
{
	GENERATED_BODY()

public:
	UEAADemoAsyncActionCaptureMember();

	UFUNCTION(BlueprintCallable, Category="EAA|Demo",DisplayName="Load Stats (Embedded)", meta=( BlueprintInternalUseOnly=true, WorldContext = "WorldContextObject"))
	static UEAADemoAsyncActionCaptureMember* StartActionWithCaptureFixed(const UObject* WorldContextObject, bool bDirectCall, const int32 UserIndex);

private:
	// any number of properties backed by bag
	UPROPERTY()
	FInstancedPropertyBag ContextData;
};

/**
 * Async action without any plugin metadata but compatible to certain degree (has a parameter for context and optionally container)
 */
UCLASS(MinimalAPI)
class UEAADemoAsyncActionExternal : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FEAADemoResult OnCompleted;

	UEAADemoAsyncActionExternal();

	UFUNCTION(BlueprintCallable, Category="EAA|Demo",DisplayName="Load Stats (External)", meta=( BlueprintInternalUseOnly=true, WorldContext = "WorldContextObject"))
	static UEAADemoAsyncActionExternal* StartActionExternal(const UObject* WorldContextObject);

	virtual void Activate() override;
private:
	UPROPERTY()
	FInstancedPropertyBag ContextData;
};
