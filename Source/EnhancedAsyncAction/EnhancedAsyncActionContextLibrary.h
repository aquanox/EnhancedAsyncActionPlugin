// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnhancedAsyncActionContext.h"
#include "EnhancedAsyncActionContextLibrary.generated.h"

#define UE_API ENHANCEDASYNCACTION_API

/**
 * Blueprint integration helpers
 */
UCLASS(MinimalAPI)
class UEnhancedAsyncActionContextLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Create capture context handle for async action instance
	 * @param Action Action proxy object
	 * @return Context handle
	 */
	UFUNCTION(BlueprintCallable, Category="EAA|Core", meta=(BlueprintInternalUseOnly=false, AdvancedDisplay=1))
	static UE_API FEnhancedAsyncActionContextHandle CreateContextForObject(const UObject* Action, FName InnerContainerProperty);

	/**
	 * Setup context container according to spec string.
	 * 
	 * This is a possible optimization to early configure FInstancedPropertyBag before setting properties,
	 * otherwise each AddProperty will result in data copying
	 *
	 * @see FPropertyTypeInfo::EncodeTypeInfo
	 * @see FPropertyTypeInfo::ParseTypeInfo
	 * 
	 * @param Handle Context handle
	 * @param Config Context container spec string
	 */
	UFUNCTION(BlueprintCallable, Category="EAA|Core", meta=(BlueprintInternalUseOnly=true))
	static UE_API void SetupContextContainer(const FEnhancedAsyncActionContextHandle& Handle, FString Config);

	/**
	 * Acquire capture context handle for async action and check validity.
	 *
	 * Called from generated delegates.
	 * 
	 * @param Action 
	 * @param Handle 
	 * @param Valid 
	 */
	UFUNCTION(BlueprintCallable, Category="EAA|Core", meta=(BlueprintInternalUseOnly=true))
	static UE_API void FindCaptureContextForObject(const UObject* Action, FEnhancedAsyncActionContextHandle& Handle, bool& Valid);

	/**
	 * Test if capture context handle valid
	 * @param Handle  Capture ontext handle
	 * @return true if handle valid
	 */
	UFUNCTION(BlueprintPure, Category="EAA|Core")
	static UE_API bool IsValidContext(const FEnhancedAsyncActionContextHandle& Handle);

	/**
	 * Acquire capture context handle for async action
	 * @param Action Action proxy object
	 * @return Context handle
	 */
	UFUNCTION(BlueprintCallable, Category="EAA|Core")
	static UE_API FEnhancedAsyncActionContextHandle GetContextForObject(const UObject* Action);

	/**
	 * Acquire unique capture context handle for testing purposes (context is tied to current world)
	 * @param WorldContextObject World context object
	 * @return Context handle
	 */
	UFUNCTION(BlueprintCallable, Category="EAA|Debug", meta=(DevelopmentOnly, WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static UE_API FEnhancedAsyncActionContextHandle GetTestCaptureContext(const UObject* WorldContextObject);

	/**
	 * Dump context information to log for debugging purposes
	 */
	UFUNCTION(BlueprintCallable, Category="EAA|Debug", meta=(DevelopmentOnly))
	static UE_API void DumpContextForObject(const UObject* Action);

	/**
	 * Dump context information to log for debugging purposes
	 */
	UFUNCTION(BlueprintCallable, Category="EAA|Debug", meta=(DevelopmentOnly))
	static UE_API void DumpContext(const FEnhancedAsyncActionContextHandle& Handle);


	// ========= SETTERS [ STANDARD ] ============

	UFUNCTION(BlueprintCallable, Category="EAA|Setters", DisplayName="Set Bool Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Bool(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const bool& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Setters", DisplayName="Set Byte Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Byte(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const uint8& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Setters", DisplayName="Set Int32 Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Int32(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int32& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Setters", DisplayName="Set Int64 Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Int64(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int64& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Setters", DisplayName="Set Float Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Float(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const float& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Setters", DisplayName="Set Double Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Double(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const double& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Setters", DisplayName="Set Name Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Name(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const FName& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Setters", DisplayName="Set String Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_String(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const FString& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Setters", DisplayName="Set Text Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Text(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const FText& Value);

	// ======== SETTERS [ GENERIC ] ============

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Setters", DisplayName="Set Generic Value", meta=(BlueprintInternalUseOnly=false, SetParam="Value"))
	static UE_API void Handle_SetValue_Basic(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, EPropertyBagPropertyType Type, const int32 Value);

	DECLARE_FUNCTION(execHandle_SetValue_Basic);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Setters", DisplayName="Set Enum Value", meta=(BlueprintInternalUseOnly=false, SetParam="Value"))
	static UE_API void Handle_SetValue_Enum(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int32& Value);

	DECLARE_FUNCTION(execHandle_SetValue_Enum);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Setters", DisplayName="Set Struct Value", meta=(BlueprintInternalUseOnly=false, CustomStructureParam = "Value"))
	static UE_API void Handle_SetValue_Struct(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int32& Value);

	DECLARE_FUNCTION(execHandle_SetValue_Struct);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Setters", DisplayName="Set Object Value", meta=(BlueprintInternalUseOnly=false))
	static UE_API void Handle_SetValue_Object(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UObject* Value);

	DECLARE_FUNCTION(execHandle_SetValue_Object);
	
	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Setters", DisplayName="Set Soft Object Value", meta=(BlueprintInternalUseOnly=false))
	static UE_API void Handle_SetValue_SoftObject(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSoftObjectPtr<UObject> Value);

	DECLARE_FUNCTION(execHandle_SetValue_SoftObject);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Setters", DisplayName="Set Class Value", meta=(BlueprintInternalUseOnly=false))
	static UE_API void Handle_SetValue_Class(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UClass* Value);

	DECLARE_FUNCTION(execHandle_SetValue_Class);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Setters", DisplayName="Set Soft Class Value", meta=(BlueprintInternalUseOnly=false))
	static UE_API void Handle_SetValue_SoftClass(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSoftClassPtr<UObject> Value);

	DECLARE_FUNCTION(execHandle_SetValue_SoftClass);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Setters", DisplayName="Set Array Value", meta=(BlueprintInternalUseOnly=false, ArrayParm = "Value"))
	static UE_API void Handle_SetValue_Array(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const TArray<int32>& Value);

	DECLARE_FUNCTION(execHandle_SetValue_Array);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Setters", DisplayName="Set Set Value", meta=(BlueprintInternalUseOnly=false, SetParam = "Value"))
	static UE_API void Handle_SetValue_Set(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const TSet<int32>& Value);

	DECLARE_FUNCTION(execHandle_SetValue_Set);

	// ======== GETTERS [ GENERIC ] ============

	UFUNCTION(BlueprintCallable, Category="EAA|Getters", DisplayName="Get Bool Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Bool(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, bool& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Getters", DisplayName="Get Byte Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Byte(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, uint8& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Getters", DisplayName="Get Int32 Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Int32(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int32& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Getters", DisplayName="Get Int64 Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Int64(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int64& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Getters", DisplayName="Get Float Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Float(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, float& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Getters", DisplayName="Get Double Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Double(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, double& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Getters", DisplayName="Get String Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_String(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, FString& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Getters", DisplayName="Get Name Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Name(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, FName& Value);

	UFUNCTION(BlueprintCallable, Category="EAA|Getters", DisplayName="Get Text Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Text(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, FText& Value);

	// ========= GETTERS [ GENERIC ] ============

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Getters", DisplayName="Get Generic Value", meta=(BlueprintInternalUseOnly=false, SetParam="Value"))
	static UE_API void Handle_GetValue_Basic(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, EPropertyBagPropertyType Type, int32& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Basic);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Getters", DisplayName="Get Enum Value", meta=(BlueprintInternalUseOnly=false))
	static UE_API void Handle_GetValue_Enum(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int32& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Enum);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Getters", DisplayName="Get Struct Value", meta=(BlueprintInternalUseOnly=false, CustomStructureParam = "Value"))
	static UE_API void Handle_GetValue_Struct(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int32& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Struct);

	UFUNCTION(BlueprintCallable,  CustomThunk, Category="EAA|Getters", DisplayName="Get Object Value", meta=(BlueprintInternalUseOnly=false, DynamicOutputParam="Value"))
	static UE_API void Handle_GetValue_Object(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UObject*& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Object);
	
	UFUNCTION(BlueprintCallable,  CustomThunk, Category="EAA|Getters", DisplayName="Get Object Value", meta=(BlueprintInternalUseOnly=false, DynamicOutputParam="Value"))
	static UE_API void Handle_GetValue_SoftObject(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSoftObjectPtr<UObject>& Value);

	DECLARE_FUNCTION(execHandle_GetValue_SoftObject);
	
	UFUNCTION(BlueprintCallable,  CustomThunk, Category="EAA|Getters", DisplayName="Get Class Value", meta=(BlueprintInternalUseOnly=false, DynamicOutputParam="Value"))
	static UE_API void Handle_GetValue_Class(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UClass*& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Class);
	
	UFUNCTION(BlueprintCallable,  CustomThunk, Category="EAA|Getters", DisplayName="Get Class Value", meta=(BlueprintInternalUseOnly=false, DynamicOutputParam="Value"))
	static UE_API void Handle_GetValue_SoftClass(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSoftClassPtr<UObject>& Value);

	DECLARE_FUNCTION(execHandle_GetValue_SoftClass);
	
	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Getters", DisplayName="Get Array Value", meta=(BlueprintInternalUseOnly=false, ArrayParm = "Value"))
	static UE_API void Handle_GetValue_Array(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TArray<int32>& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Array);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EAA|Getters", DisplayName="Get Set Value", meta=(BlueprintInternalUseOnly=false, SetParam = "Value"))
	static UE_API void Handle_GetValue_Set(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSet<int32>& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Set);
};

#undef UE_API
