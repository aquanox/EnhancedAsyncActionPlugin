// Copyright 2025, Aquanox.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "EnhancedAsyncActionSettings.generated.h"

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

USTRUCT()
struct FExternalLatentFunctionSpec
{
	GENERATED_BODY()

	// Async action class for registration
	UPROPERTY(EditAnywhere, Category=Spec, meta=(AllowAbstract=true, HideViewOptions=true, DisplayThumbnail=false))
	TSoftClassPtr<class UObject> FactoryClass;
	// Name of function
	UPROPERTY(EditAnywhere, Category=Spec)
	FName FunctionName;
};

/**
 *
 */
UCLASS(MinimalAPI, Config=Engine, DefaultConfig, DisplayName="Enhanced Async Actions")
class UEnhancedAsyncActionSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UE_API static const UEnhancedAsyncActionSettings* Get();

	UE_API virtual FName GetContainerName() const override { return TEXT("Editor"); }
	UE_API virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	UE_API virtual FName GetSectionName() const override { return TEXT("EnhancedAsyncAction"); }

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

	/**
	 * List of manually registered latent functions to use EnhancedCallLatentFunction node.
	 *
	 * This option forces registration of functions that do not have `HasLatentContext` metadata specifier but compatible with plugin
	 *
	 * IMPORTANT: Use this setting only if you have a class you can not modify to add plugin metadata.
	 */
	UPROPERTY(Config, EditAnywhere, Category=General, meta=(ConfigRestartRequired=true))
	TArray<FExternalLatentFunctionSpec> ExternalLatentFunctions;
};

#undef UE_API
