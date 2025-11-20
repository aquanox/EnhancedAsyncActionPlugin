// Copyright 2025, Aquanox.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnhancedAsyncContextLibrary.generated.h"

#define UE_API ENHANCEDASYNCACTION_API

struct FEnhancedAsyncActionContextHandle;
struct FEnhancedLatentActionContextHandle;
struct FLatentContextInfo;
class FEnhancedLatentActionDelegate;

/**
 * Blueprint integration helpers
 */
UCLASS(MinimalAPI)
class UEnhancedAsyncContextLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Create capture context handle for async action instance. Called by UK2Node_EnhancedAsyncAction.
	 *
	 * @param Action Action proxy object
	 * @param InnerContainerProperty Name of container property (none for default)
	 *
	 * @return Context handle
	 */
	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Core", meta=(BlueprintInternalUseOnly=true, AdvancedDisplay=1))
	static UE_API FEnhancedAsyncActionContextHandle CreateContextForObject(const UObject* Action, FName InnerContainerProperty = NAME_None);

	/**
	 * Create capture context handle for latent function. Called by UK2Node_EnhancedCallLatentFunction.
	 *
	 * @return Context handle
	 */
	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Core", meta=(BlueprintInternalUseOnly=true))
	static UE_API FEnhancedLatentActionContextHandle CreateContextForLatent(const UObject* Owner, int32 UUID, int32 CallUUID, FEnhancedLatentActionDelegate Delegate);

	/**
	 * Create dummy context for latent. Called by UK2Node_EnhancedCallLatentFunction.
	 *
	 * @return Context handle
	 */
	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Core", meta=(BlueprintInternalUseOnly=true))
	static UE_API FEnhancedLatentActionContextHandle CreateEmptyContextForLatent(const UObject* Owner, int32 UUID, int32 CallUUID, FEnhancedLatentActionDelegate Delegate);

	/**
	 * Destroy capture context used by latent function. Called by UK2Node_EnhancedCallLatentFunction.
	 *
	 * @return Context handle
	 */
	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Core", meta=(BlueprintInternalUseOnly=true))
	static UE_API void DestroyContextForLatent(const FEnhancedLatentActionContextHandle& Handle);

	/**
	 * Setup context container according to spec string. Called by UK2Node_EnhancedAsyncAction.
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
	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Core", meta=(BlueprintInternalUseOnly=true))
	static UE_API void SetupContextContainer(const FEnhancedAsyncActionContextHandle& Handle, FString Config);

	/**
	 * Acquire capture context handle for async action and check validity.
	 *
	 * Called by UK2Node_EnhancedAsyncAction from generated delegates.
	 *
	 * @param Action Object to search context for
	 * @param Handle Found handle
	 * @param Valid Internal test that handle is found and valid
	 */
	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Core", meta=(BlueprintInternalUseOnly=true))
	static UE_API void FindCaptureContextForObject(const UObject* Action, FEnhancedAsyncActionContextHandle& Handle, bool& Valid);

	/**
	 * Acquire capture context handle for async action. Called by UK2Node_EnhancedAsyncAction or Tests.
	 *
	 * @param Action Action proxy object
	 * @return Context handle
	 */
	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Core")
	static UE_API FEnhancedAsyncActionContextHandle GetContextForObject(const UObject* Action);

	/**
	 * Dump context information to log for debugging purposes
	 */
	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Debug", meta=(DevelopmentOnly))
	static UE_API void DumpContextForObject(const UObject* Action);

	/**
	 * Dump context information to log for debugging purposes
	 */
	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Debug", meta=(DevelopmentOnly))
	static UE_API void DumpContext(const FAsyncContextHandleBase& Handle);

	// ========== CASTS =============

	UFUNCTION(BlueprintPure, Category="EnhancedAsyncAction|Casts", meta=(CompactNodeTitle = "->", BlueprintAutocast, BlueprintInternalUseOnly=true))
	static UE_API struct FAsyncContextHandleBase GetHandleFromLatentHandle(const FEnhancedAsyncActionContextHandle& Handle);

	UFUNCTION(BlueprintPure, Category="EnhancedAsyncAction|Casts", meta=(CompactNodeTitle = "->", BlueprintAutocast, BlueprintInternalUseOnly=true))
	static UE_API struct FAsyncContextHandleBase GetHandleFromActionHandle(const FEnhancedLatentActionContextHandle& Handle);

	// ========= SETTERS [ STANDARD ] ============

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Setters", DisplayName="Set Bool Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Bool(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const bool& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Setters", DisplayName="Set Byte Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Byte(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const uint8& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Setters", DisplayName="Set Int32 Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Int32(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int32& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Setters", DisplayName="Set Int64 Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Int64(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int64& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Setters", DisplayName="Set Float Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Float(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const float& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Setters", DisplayName="Set Double Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Double(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const double& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Setters", DisplayName="Set Name Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Name(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const FName& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Setters", DisplayName="Set String Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_String(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const FString& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Setters", DisplayName="Set Text Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_SetValue_Text(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const FText& Value);

	// ======== SETTERS [ GENERIC ] ============

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Getters", DisplayName="Set Value Variadic", meta=(Variadic, BlueprintInternalUseOnly=true, CustomStructureParam="Handle"))
	static UE_API void Handle_SetValue_Variadic(const FAsyncContextHandleBase& Handle, const TArray<FString>& Names);

	DECLARE_FUNCTION(execHandle_SetValue_Variadic);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Getters", DisplayName="Set Generic Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_SetValue_Generic(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int32& Value);

	DECLARE_FUNCTION(execHandle_SetValue_Generic);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Setters", DisplayName="Set Enum Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_SetValue_Enum(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int32& Value);

	DECLARE_FUNCTION(execHandle_SetValue_Enum);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Setters", DisplayName="Set Struct Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_SetValue_Struct(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int32& Value);

	DECLARE_FUNCTION(execHandle_SetValue_Struct);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Setters", DisplayName="Set Object Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_SetValue_Object(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UObject* Value);

	DECLARE_FUNCTION(execHandle_SetValue_Object);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Setters", DisplayName="Set Soft Object Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_SetValue_SoftObject(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSoftObjectPtr<UObject> Value);

	DECLARE_FUNCTION(execHandle_SetValue_SoftObject);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Setters", DisplayName="Set Class Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_SetValue_Class(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UClass* Value);

	DECLARE_FUNCTION(execHandle_SetValue_Class);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Setters", DisplayName="Set Soft Class Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_SetValue_SoftClass(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSoftClassPtr<UObject> Value);

	DECLARE_FUNCTION(execHandle_SetValue_SoftClass);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Setters", DisplayName="Set Array Value", meta=(BlueprintInternalUseOnly=true, ArrayParm="Value"))
	static UE_API void Handle_SetValue_Array(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const TArray<int32>& Value);

	DECLARE_FUNCTION(execHandle_SetValue_Array);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Setters", DisplayName="Set Set Value", meta=(BlueprintInternalUseOnly=true, SetParam="Value"))
	static UE_API void Handle_SetValue_Set(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const TSet<int32>& Value);

	DECLARE_FUNCTION(execHandle_SetValue_Set);

	// ======== GETTERS [ GENERIC ] ============

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Getters", DisplayName="Get Bool Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Bool(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, bool& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Getters", DisplayName="Get Byte Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Byte(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, uint8& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Getters", DisplayName="Get Int32 Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Int32(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int32& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Getters", DisplayName="Get Int64 Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Int64(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int64& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Getters", DisplayName="Get Float Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Float(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, float& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Getters", DisplayName="Get Double Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Double(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, double& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Getters", DisplayName="Get String Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_String(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, FString& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Getters", DisplayName="Get Name Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Name(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, FName& Value);

	UFUNCTION(BlueprintCallable, Category="EnhancedAsyncAction|Getters", DisplayName="Get Text Value", meta=(BlueprintInternalUseOnly=true))
	static UE_API void Handle_GetValue_Text(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, FText& Value);

	// ========= GETTERS [ GENERIC ] ============

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Getters", DisplayName="Get Value Variadic", meta=(Variadic, BlueprintInternalUseOnly=true, CustomStructureParam="Handle"))
	static UE_API void Handle_GetValue_Variadic(const FAsyncContextHandleBase& Handle, const TArray<FString>& Names);

	DECLARE_FUNCTION(execHandle_GetValue_Variadic);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Getters", DisplayName="Get Generic Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_GetValue_Generic(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int32& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Generic);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Getters", DisplayName="Get Enum Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_GetValue_Enum(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int32& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Enum);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Getters", DisplayName="Get Struct Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_GetValue_Struct(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int32& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Struct);

	UFUNCTION(BlueprintCallable,  CustomThunk, Category="EnhancedAsyncAction|Getters", DisplayName="Get Object Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_GetValue_Object(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UObject*& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Object);

	UFUNCTION(BlueprintCallable,  CustomThunk, Category="EnhancedAsyncAction|Getters", DisplayName="Get Soft Object Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_GetValue_SoftObject(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSoftObjectPtr<UObject>& Value);

	DECLARE_FUNCTION(execHandle_GetValue_SoftObject);

	UFUNCTION(BlueprintCallable,  CustomThunk, Category="EnhancedAsyncAction|Getters", DisplayName="Get Class Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_GetValue_Class(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UClass*& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Class);

	UFUNCTION(BlueprintCallable,  CustomThunk, Category="EnhancedAsyncAction|Getters", DisplayName="Get Soft Class Value", meta=(BlueprintInternalUseOnly=true, CustomStructureParam="Value"))
	static UE_API void Handle_GetValue_SoftClass(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSoftClassPtr<UObject>& Value);

	DECLARE_FUNCTION(execHandle_GetValue_SoftClass);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Getters", DisplayName="Get Array Value", meta=(BlueprintInternalUseOnly=true, ArrayParm="Value"))
	static UE_API void Handle_GetValue_Array(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TArray<int32>& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Array);

	UFUNCTION(BlueprintCallable, CustomThunk, Category="EnhancedAsyncAction|Getters", DisplayName="Get Set Value", meta=(BlueprintInternalUseOnly=true, SetParam="Value"))
	static UE_API void Handle_GetValue_Set(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSet<int32>& Value);

	DECLARE_FUNCTION(execHandle_GetValue_Set);
};

#undef UE_API
