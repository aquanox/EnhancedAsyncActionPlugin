// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EnhancedAsyncActionSettings.generated.h"

/**
 * 
 */
UCLASS(Config=Engine, DefaultConfig)
class ENHANCEDASYNCACTION_API UEnhancedAsyncActionSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("EnhancedAsyncAction"); }
	virtual bool SupportsAutoRegistration() const override { return false; }
	
	static const UEnhancedAsyncActionSettings* Get() { return GetDefault<UEnhancedAsyncActionSettings>();  }
	bool IsClassAllowed(UClass* InClass) const;
	
private:

	UPROPERTY(Config, EditAnywhere, Category=General)
	TArray<TSoftClassPtr<class UBlueprintAsyncActionBase>> AllowedNodes;
};
