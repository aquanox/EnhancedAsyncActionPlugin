// Copyright 2025, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "StructUtils/InstancedStruct.h"
#include "EAADemoAsyncActionStandard.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEAADemoStandardResult,
	int32, UserIndex,
	FString, UserName,
	const TArray<int32>&, Scores
);

/**
 * Trivial async action "loading user data" 
 */
UCLASS(MinimalAPI)
class UEAADemoAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FEAADemoStandardResult OnCompleted;
	UPROPERTY(BlueprintAssignable)
	FEAADemoStandardResult OnFailed;

	UFUNCTION(BlueprintCallable, Category="EAA|Demo", DisplayName="LoadStatsForUser", meta=( BlueprintInternalUseOnly=true,  WorldContext = "WorldContextObject"))
	static UEAADemoAsyncAction* StartDemoAction(const UObject* WorldContextObject, int32 UserIndex);
	
	virtual void Activate() override;
protected:
	
	UPROPERTY()
	TObjectPtr<UWorld> LocalWorld;
	UPROPERTY()
	int32 LocalUserIndex;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FEAADemoStandardCtxResult,
	FInstancedStruct, Captures,
	int32, UserIndex,
	FString, UserName,
	const TArray<int32>&, Scores);

/**
 * Trivial async action "loading user data" but allows capturing additional properties
 *
 * Using a FInstancedStruct or a USTRUCT as a container for the captured data.
 */
UCLASS(MinimalAPI)
class UEAADemoAsyncActionStandard : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable)
	FEAADemoStandardCtxResult OnCompleted;
	UPROPERTY(BlueprintAssignable)
	FEAADemoStandardCtxResult OnFailed;

	UFUNCTION(BlueprintCallable,Category="EAA|Demo", DisplayName="LoadStatsForUser (IS)", meta=( BlueprintInternalUseOnly=true,  WorldContext = "WorldContextObject"))
	static UEAADemoAsyncActionStandard* StartActionPseudoCapture(const UObject* WorldContextObject, int32 UserIndex, FInstancedStruct Captures);

	virtual void Activate() override;
protected:
	UPROPERTY()
	TObjectPtr<UWorld> LocalWorld;
	UPROPERTY()
	int32 LocalUserIndex;
	UPROPERTY()
	FInstancedStruct LocalCaptures;
};
