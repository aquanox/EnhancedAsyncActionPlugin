// Copyright 2025, Aquanox.

#include "EnhancedAsyncActionContextLibrary.h"

#include "Blueprint/BlueprintExceptionInfo.h"
#include "Engine/Engine.h"
#include "EnhancedAsyncActionManager.h"
#include "EnhancedAsyncActionContextImpl.h"
#include "EnhancedAsyncActionShared.h"
#include "Logging/StructuredLog.h"
#include "UObject/TextProperty.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnhancedAsyncActionContextLibrary)

#define EAA_KISMET_ARRAY_ENSURE(Expr) \
	if (!ensureAlways(Expr))  { \
		Stack.bArrayContextFailed = true;  \
		return;  \
	}

#define EAA_KISMET_ENSURE(Expr, Message) \
	if (!ensureAlways(Expr)) { \
		FBlueprintExceptionInfo ExceptionInfo(EBlueprintExceptionType::AbortExecution,  INVTEXT(Message) );  \
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);  \
		return; \
	}

FEnhancedAsyncActionContextHandle UEnhancedAsyncActionContextLibrary::CreateContextForObject(const UObject* Action, FName InDataProperty)
{
	if (!IsValid(Action))
	{
		// FFrame::KismetExecutionMessage(TEXT("CreateContextForObject failed for node"), ELogVerbosity::Error, TEXT("EAA_CreateContextForAction"));
		UE_LOG(LogEnhancedAction, Log, TEXT("CreateContextForObject failed for node"));
		return FEnhancedAsyncActionContextHandle();
	}

	TSharedPtr<FEnhancedAsyncActionContext> Context;
	
	if (EAA::Internals::IsValidContainerProperty(Action, InDataProperty))
	{
		Context = MakeShared<FEnhancedAsyncActionContext_PropertyBagRef>(Action, InDataProperty);
	}
	else
	{
		Context = MakeShared<FEnhancedAsyncActionContext_PropertyBag>(Action);
		if (!InDataProperty.IsNone())
		{
			UE_LOG(LogEnhancedAction, Log, TEXT("Missing expected member property %s:%s"), *Action->GetClass()->GetName(), *InDataProperty.ToString());
			InDataProperty = NAME_None;
		}
	}

	FEnhancedAsyncActionContextHandle Handle = FEnhancedAsyncActionManager::Get().SetContext(Action, Context.ToSharedRef());
	Handle.DataProperty = InDataProperty;
	return Handle;
}

void UEnhancedAsyncActionContextLibrary::SetupContextContainer(const FEnhancedAsyncActionContextHandle& Handle, FString Config)
{
	if (!Handle.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("SetupContextContainer failed for node"), ELogVerbosity::Error, TEXT("EAA_SetupContextContainer"));
		return;
	}

	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->SetupFromStringDefinition(Config);
}

FEnhancedAsyncActionContextHandle UEnhancedAsyncActionContextLibrary::GetContextForObject(const UObject* Action)
{
	auto Context = FEnhancedAsyncActionManager::Get().FindContextHandle(Action);
	if (!Context.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("GetContextForObject failed for node"), ELogVerbosity::Error, TEXT("EAA_GetCaptureContext"));
	}
	return Context;
}

FEnhancedAsyncActionContextHandle UEnhancedAsyncActionContextLibrary::GetTestCaptureContext(const UObject* WorldContextObject)
{
	const UWorld* Context = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	FEnhancedAsyncActionContextHandle ContextHandle = FEnhancedAsyncActionManager::Get().FindContextHandle(Context);
	if (!ContextHandle.IsValid())
	{
		ContextHandle = FEnhancedAsyncActionManager::Get().SetContext(Context, MakeShared<FEnhancedAsyncActionContext_PropertyBag>(Context));
	}
	return ContextHandle;
}

void UEnhancedAsyncActionContextLibrary::DumpContextForObject(const UObject* Action)
{
	auto Context = FEnhancedAsyncActionManager::Get().FindContextSafe(Action);
	{
		FStringBuilderBase Builder;
		Context->DebugDump(Builder);
		UE_LOG(LogEnhancedAction, Log, TEXT("Context for action %p [%s]:\n%s"), Action, *Context->GetDebugName(), Builder.ToString());
	}
}

void UEnhancedAsyncActionContextLibrary::DumpContext(const FEnhancedAsyncActionContextHandle& Handle)
{
	auto Context = FEnhancedAsyncActionManager::Get().FindContextSafe(Handle);
	{
		FStringBuilderBase Builder;
		Context->DebugDump(Builder);
		UE_LOG(LogEnhancedAction, Log, TEXT("Context for handle %p [%s]:\n%s"), &Handle, *Context->GetDebugName(), Builder.ToString());
	}
}

void UEnhancedAsyncActionContextLibrary::FindCaptureContextForObject(const UObject* Action, FEnhancedAsyncActionContextHandle& Handle, bool& Valid)
{
	Handle = FEnhancedAsyncActionManager::Get().FindContextHandle(Action);
	Valid = Handle.IsValid();
}

bool UEnhancedAsyncActionContextLibrary::IsValidContext(const FEnhancedAsyncActionContextHandle& Handle)
{
	return Handle.IsValid();
}

// UE_LOGFMT(LogEnhancedAction, Log, "{Func} set {Index} => {Value}", __FUNCTIONW__, Index, Value);
//
// UE_LOGFMT(LogEnhancedAction, Log, "{Func} get {Index} => {Value}", __FUNCTIONW__, Index, Value);

// =================== SETTERS ===========================

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Bool(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const bool& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->SetValueBool(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Byte(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const uint8& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->SetValueByte(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Int32(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int32& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->SetValueInt32(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Int64(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int64& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->SetValueInt64(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Float(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const float& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->SetValueFloat(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Double(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const double& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->SetValueDouble(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_String(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const FString& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->SetValueString(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Name(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const FName& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->SetValueName(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Text(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const FText& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->SetValueText(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Variadic(const FEnhancedAsyncActionContextHandle& Handle, const TArray<FString>& Names)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_Variadic)
{
	// Read the standard function arguments
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle, ParamHandle);
	P_GET_TARRAY_REF(FString, Parameters);

	struct FInputParam { int32 Index; FName Name; const FProperty* Property; const void* Value;  };
	TArray<FInputParam, TInlineAllocator<16>> VarInputs;
	TArray<TPair<FName, const FProperty*>, TInlineAllocator<16>> SetupInputs;

	// Read the input values
	for (int32 Index = 0; Index < Parameters.Num(); ++Index)
	{
		const FString& InputName = Parameters[Index];
		
		Stack.MostRecentProperty = nullptr;
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.StepCompiledIn<FProperty>(nullptr);
		check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

		FInputParam& Input = VarInputs.AddDefaulted_GetRef();
		Input.Name = *InputName;
		Input.Index = EAA::Internals::NameToIndex(Input.Name);
		Input.Property = Stack.MostRecentProperty;
		Input.Value = Stack.MostRecentPropertyAddress;

		SetupInputs.Emplace(Input.Name, Input.Property);
	}

	P_FINISH;

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	if (ContextSafe->CanSetupContext())
	{
		ContextSafe->SetupFromProperties(SetupInputs);
	}
	
	for (const FInputParam& Input : VarInputs)
	{
		FString Message;
		if (!ensureAlways(ContextSafe->SetValueByName(Input.Name, Input.Property, Input.Value, Message)))
		{
			auto PropertyContainerString = UEnum::GetValueAsString(EAA::Internals::GetContainerTypeFromProperty(Input.Property));
			auto PropertyTypeString = UEnum::GetValueAsString(EAA::Internals::GetValueTypeFromProperty(Input.Property));
			FBlueprintExceptionInfo ExceptionInfo(EBlueprintExceptionType::AbortExecution, FText::FromString(
				FString::Printf(TEXT("Failed to set context property at index %d (%s:%s) : %s"),
					Input.Index, *PropertyContainerString, *PropertyTypeString, *Message )
			) );
			FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		}
	}
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Generic(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_Generic)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle, ParamHandle);
	P_GET_PROPERTY(FIntProperty, ParamIndex);

	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);

	const FProperty* ValueProp = CastField<FProperty>(Stack.MostRecentProperty);
	const void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	EAA_KISMET_ENSURE(ValueProp != nullptr && ValuePtr != nullptr, "Failed to resolve the Value Property for Set Value Basic");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);

	FString Message;
	if (!ensureAlways(ContextSafe->SetValueByIndex(ParamIndex, ValueProp, ValuePtr, Message)))
	{
		FBlueprintExceptionInfo ExceptionInfo(EBlueprintExceptionType::AbortExecution,  FText::FromString(Message) );
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Enum(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_Enum)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	
	uint8 ParamValue = 0;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FEnumProperty>(&ParamValue);
	const FEnumProperty* ParamValueProp = CastField<FEnumProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Failed to resolve the Value Property for Set Value Enum");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	ContextSafe->SetValueEnum(ParamIndex, ParamValueProp->GetEnum(),  ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Struct(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_Struct)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	const FStructProperty* ParamValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	const void* ParamValue = Stack.MostRecentPropertyAddress;
	P_FINISH;
	
	EAA_KISMET_ENSURE(ParamValueProp != nullptr && ParamValue != nullptr, "Failed to resolve the Value Property for Set Value Struct");
	
	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	ContextSafe->SetValueStruct(ParamIndex, ParamValueProp->Struct, (const uint8*)ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Object(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UObject* Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_Object)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_OBJECT(UObject,ParamValue);
	const FObjectProperty* ParamValueProp = CastField<FObjectProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Failed to resolve the Value Property for Set Value Object");
	
	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	ContextSafe->SetValueObject(ParamIndex, ParamValueProp->PropertyClass, ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_SoftObject(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSoftObjectPtr<UObject> Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_SoftObject)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle, ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_SOFTOBJECT(TSoftObjectPtr<UObject>,ParamValue);
	const FSoftObjectProperty* ParamValueProp = CastField<FSoftObjectProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Failed to resolve the Value Property for Set Value Soft Object");
	
	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	ContextSafe->SetValueSoftObject(ParamIndex, ParamValueProp->PropertyClass, ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Class(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UClass* Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_Class)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_OBJECT(UClass,ParamValue);
	const FClassProperty* ParamValueProp = CastField<FClassProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Failed to resolve the Value Property for Set Value Class");
	
	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	ContextSafe->SetValueClass(ParamIndex, ParamValueProp->MetaClass, ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_SoftClass(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSoftClassPtr<UObject> Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_SoftClass)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_SOFTCLASS(TSoftClassPtr<UObject> ,ParamValue);
	const FSoftClassProperty* ParamValueProp = CastField<FSoftClassProperty>(Stack.MostRecentProperty);
	P_FINISH;
	
	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Failed to resolve the Value Property for Set Value Class");
	
	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	ContextSafe->SetValueSoftClass(ParamIndex, ParamValueProp->MetaClass, ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Array(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const TArray<int32>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_Array)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);
	
	FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	const void* ArrayAddr = Stack.MostRecentPropertyAddress;
	const void* ArrayContainerAddr = Stack.MostRecentPropertyContainer;
	P_FINISH;

	EAA_KISMET_ARRAY_ENSURE(ArrayProperty != nullptr && ArrayAddr != nullptr);

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	auto ValueType = EAA::Internals::GetValueTypeFromProperty(ArrayProperty);
	auto ValueTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(ArrayProperty);

	const void* ValuePtr = ArrayProperty->ContainerPtrToValuePtr<const void>(ArrayContainerAddr);
	ensureAlways(ValuePtr == ArrayAddr);
	ContextSafe->SetValueArray(ParamIndex, ValueType, ValueTypeObject, ValuePtr);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Set(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const TSet<int32>& Value)
{
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_Set)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FSetProperty>(NULL);
	
	FSetProperty* SetProperty = CastField<FSetProperty>(Stack.MostRecentProperty);
	const void* SetAddr = Stack.MostRecentPropertyAddress;
	const void* SetContainerAddr = Stack.MostRecentPropertyContainer;
	P_FINISH;
	
	EAA_KISMET_ARRAY_ENSURE(SetProperty != nullptr && SetAddr != nullptr);

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	auto ValueType = EAA::Internals::GetValueTypeFromProperty(SetProperty);
	auto ValueTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(SetProperty);

	const void* ValuePtr = SetProperty->ContainerPtrToValuePtr<const void>(SetContainerAddr);
	ensureAlways(ValuePtr == SetAddr);
	ContextSafe->SetValueSet(ParamIndex, ValueType, ValueTypeObject, ValuePtr);
	P_NATIVE_END;
}

// ==================== GETTERS =========================

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Bool(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, bool& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->GetValueBool(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Byte(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, uint8& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->GetValueByte(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Int32(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int32& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->GetValueInt32(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Int64(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int64& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->GetValueInt64(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Float(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, float& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->GetValueFloat(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Double(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, double& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->GetValueDouble(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_String(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, FString& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->GetValueString(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Name(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, FName& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->GetValueName(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Text(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, FText& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->GetValueText(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Variadic(const FEnhancedAsyncActionContextHandle& Handle, const TArray<FString>& Names)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_Variadic)
{
	// Read the standard function arguments
    P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle, ParamHandle);
    P_GET_TARRAY_REF(FString, Parameters);

    struct FOutputParam { int32 Index; FName Name; const FProperty* Property; void* Value;  };
    TArray<FOutputParam, TInlineAllocator<16>> VarOutputs;

    // Read the input values and write them to the Python context
    bool bHasValidInputValues = true;
    for (int32 Index = 0; Index < Parameters.Num(); ++Index)
    {
    	const FString& InputName = Parameters[Index];
    	
    	Stack.MostRecentProperty = nullptr;
    	Stack.MostRecentPropertyAddress = nullptr;
    	Stack.StepCompiledIn<FProperty>(nullptr);
    	check(Stack.MostRecentProperty && Stack.MostRecentPropertyAddress);

    	FOutputParam& Output = VarOutputs.AddDefaulted_GetRef();
    	Output.Name = *InputName;
    	Output.Index = EAA::Internals::NameToIndex(Output.Name);
    	Output.Property = Stack.MostRecentProperty;
    	Output.Value = Stack.MostRecentPropertyAddress;
    }

    P_FINISH;

    P_NATIVE_BEGIN;
    auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	for (const FOutputParam& Input : VarOutputs)
	{
		FString Message;
		if (!ensureAlways(ContextSafe->GetValueByName(Input.Name, Input.Property, Input.Value, Message)))
		{
			auto PropertyContainerString = UEnum::GetValueAsString(EAA::Internals::GetContainerTypeFromProperty(Input.Property));
			auto PropertyTypeString = UEnum::GetValueAsString(EAA::Internals::GetValueTypeFromProperty(Input.Property));
			FBlueprintExceptionInfo ExceptionInfo(EBlueprintExceptionType::AbortExecution, FText::FromString(
				FString::Printf(TEXT("Failed to get context property at index %d (%s:%s) : %s"),
					Input.Index, *PropertyContainerString, *PropertyTypeString, *Message )
			) );
			FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		}
	}
    P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Generic(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int32& Value)
{
	
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_Generic)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle, ParamHandle);
	P_GET_PROPERTY(FIntProperty, ParamIndex);
	
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	
	const FProperty* ValueProp = CastField<FProperty>(Stack.MostRecentProperty);
	void* ValuePtr = Stack.MostRecentPropertyAddress;
	
	P_FINISH;

	EAA_KISMET_ENSURE(ValueProp != nullptr && ValuePtr != nullptr, "Failed to resolve the Value Property for Get Value Generic");
	
	P_NATIVE_BEGIN;
	
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	
	FString Message;
	if (!ensureAlways(ContextSafe->GetValueByIndex(ParamIndex, ValueProp, ValuePtr, Message)))
	{
		FBlueprintExceptionInfo ExceptionInfo(EBlueprintExceptionType::AbortExecution,  FText::FromString(Message) );
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}
	
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Enum(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_Enum)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);

	uint8 ExecResultTemp = 0;
	
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	uint8& ParamValue = Stack.StepCompiledInRef<FEnumProperty, uint8>(&ExecResultTemp);
	const FEnumProperty* ParamValueProp = CastField<FEnumProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Unsupported property type the Value for Set Value Enum");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	ContextSafe->GetValueEnum(ParamIndex, ParamValueProp->GetEnum(), ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Struct(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_Struct)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	
	const FStructProperty* ParamValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* ParamValue = Stack.MostRecentPropertyAddress;
	P_FINISH;
	
	EAA_KISMET_ENSURE(ParamValueProp != nullptr && ParamValue != nullptr, "Unsupported property type the Value for Set Value Struct");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	const uint8* Result = nullptr;
	ContextSafe->GetValueStruct(ParamIndex, ParamValueProp->Struct, Result);
	if (Result)
	{
		ParamValueProp->Struct->CopyScriptStruct(ParamValue, Result);
	}
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Object(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UObject*& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_Object)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_OBJECT_REF(UObject,ParamValue);
	const FObjectProperty* ParamValueProp = CastField<FObjectProperty>(Stack.MostRecentProperty);
	P_FINISH;
	
	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Unsupported property type the Value for Set Value Object");
	
	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	ContextSafe->GetValueObject(ParamIndex, ParamValueProp->PropertyClass, P_ARG_GC_BARRIER(ParamValue));
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_SoftObject(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSoftObjectPtr<UObject>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_SoftObject)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_SOFTOBJECT_REF(TSoftObjectPtr<UObject>,ParamValue);
	const FSoftObjectProperty* ParamValueProp = CastField<FSoftObjectProperty>(Stack.MostRecentProperty);
	P_FINISH;
	
	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Unsupported property type the Value for Get Value Soft Object");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	ContextSafe->GetValueSoftObject(ParamIndex, ParamValueProp->PropertyClass, ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Class(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UClass*& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_Class)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_OBJECT_REF(UClass,ParamValue);
	const FClassProperty* ParamValueProp = CastField<FClassProperty>(Stack.MostRecentProperty);
	P_FINISH;
	
	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Unsupported property type the Value for Set Value Class");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	ContextSafe->GetValueClass(ParamIndex, ParamValueProp->MetaClass, P_ARG_GC_BARRIER(ParamValue));
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_SoftClass(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSoftClassPtr<UObject>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_SoftClass)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_SOFTCLASS_REF(TSoftClassPtr<UObject>,ParamValue);
	const FSoftClassProperty* ParamValueProp = CastField<FSoftClassProperty>(Stack.MostRecentProperty);
	P_FINISH;
	
	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Unsupported property type the Value for Get Value Soft Class");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	ContextSafe->GetValueSoftClass(ParamIndex, ParamValueProp->MetaClass, ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Array(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TArray<int32>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_Array)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);
	
	FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	void* ArrayAddr = Stack.MostRecentPropertyAddress;
	void* ArrayContainerAddr = Stack.MostRecentPropertyContainer;
	P_FINISH;
	
	EAA_KISMET_ARRAY_ENSURE(ArrayProperty != nullptr && ArrayAddr != nullptr);

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	auto ValueType = EAA::Internals::GetValueTypeFromProperty(ArrayProperty);
	auto ValueTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(ArrayProperty);

	void* ValuePtr = ArrayProperty->ContainerPtrToValuePtr<void>(ArrayContainerAddr);
	ensureAlways(ValuePtr == ArrayAddr);
	ContextSafe->GetValueArray(ParamIndex, ValueType, ValueTypeObject, ValuePtr);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Set(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSet<int32>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_Set)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FSetProperty>(nullptr);

	FSetProperty* SetProperty = CastField<FSetProperty>(Stack.MostRecentProperty);
	void* SetAddr = Stack.MostRecentPropertyAddress;
	void* SetContainerAddr = Stack.MostRecentPropertyContainer;
	P_FINISH;

	EAA_KISMET_ARRAY_ENSURE(SetProperty != nullptr && SetAddr != nullptr);

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(ParamHandle);
	auto ValueType = EAA::Internals::GetValueTypeFromProperty(SetProperty);
	auto ValueTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(SetProperty);

	void* ValuePtr = SetProperty->ContainerPtrToValuePtr<void>(SetContainerAddr);
	ensureAlways(ValuePtr == SetAddr);
	ContextSafe->GetValueSet(ParamIndex, ValueType, ValueTypeObject, ValuePtr);
	P_NATIVE_END;
}

#undef EAA_KISMET_ARRAY_ENSURE
#undef EAA_KISMET_ENSURE
