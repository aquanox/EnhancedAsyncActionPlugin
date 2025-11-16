// Copyright 2025, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EnhancedAsyncActionSettings.generated.h"

/**
 *
 */
UCLASS(Abstract, Config=Engine, DefaultConfig)
class ENHANCEDASYNCACTION_API UEnhancedAsyncActionSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("EnhancedAsyncAction"); }
	virtual bool SupportsAutoRegistration() const override { return false; }

	static const UEnhancedAsyncActionSettings* Get()
	{
		return GetDefault<UEnhancedAsyncActionSettings>();
	}
private:

};
