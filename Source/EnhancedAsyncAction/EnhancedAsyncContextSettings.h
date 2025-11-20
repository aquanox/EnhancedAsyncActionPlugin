// Copyright 2025, Aquanox.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "EnhancedAsyncContextSettings.generated.h"

#define UE_API ENHANCEDASYNCACTION_API

class UBlueprintAsyncActionBase;

USTRUCT()
struct FExternalAsyncActionSpec
{
	GENERATED_BODY()

	// Async action class for registration
	UPROPERTY(EditAnywhere, Category=Spec, meta=(AllowAbstract=false, HideViewOptions=true, DisplayThumbnail=false))
	TSoftClassPtr<UBlueprintAsyncActionBase> ActionClass;
	// Name of context property (value of metadata HasAsyncContext="Name")
	UPROPERTY(EditAnywhere, Category=Spec)
	FName ContextPropertyName;
	// Name of context container property (value of metadata AsyncContextContainer="Name")
	UPROPERTY(EditAnywhere, Category=Spec)
	FName ContainerPropertyName;
	// Should display context pin
	UPROPERTY(EditAnywhere, Category=Spec)
	bool bExposedContext = false;
};

/**
 *
 */
UCLASS(MinimalAPI, Config=Engine, DefaultConfig, DisplayName="Enhanced Async Actions")
class UEnhancedAsyncContextSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UE_API static const UEnhancedAsyncContextSettings* Get();

	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("EnhancedAsyncAction"); }

	UE_API const FExternalAsyncActionSpec* FindActionSpecForClass(UClass* Class) const;

private:
	/**
	 * List of manually registered actions to use EnhancedAsyncAction node.
	 *
	 * This option forces registration of proxies that do not have `HasAsyncContext` metadata specifier but compatible with plugin
	 *
	 * IMPORTANT: Use this setting only if you have a class you can not modify to add plugin metadata.
	 */
	UPROPERTY(Config, EditAnywhere, Category=General, meta=(ConfigRestartRequired=true))
	TArray<FExternalAsyncActionSpec> ExternalAsyncActions;
};

#undef UE_API
