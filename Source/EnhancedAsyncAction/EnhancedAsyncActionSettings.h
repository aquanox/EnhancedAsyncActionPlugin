// Copyright 2025, Aquanox.

#pragma once

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
	static const UEnhancedAsyncActionSettings* Get()
	{
		return GetDefault<UEnhancedAsyncActionSettings>();
	}

	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("EnhancedAsyncAction"); }
	virtual bool SupportsAutoRegistration() const override { return false; }

private:

};
