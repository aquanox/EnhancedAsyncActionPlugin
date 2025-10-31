// Fill out your copyright notice in the Description page of Project Settings.


#include "EnhancedAsyncActionContextLibrary.h"

#include "Blueprint/BlueprintExceptionInfo.h"
#include "Engine/Engine.h"
#include "EnhancedAsyncActionManager.h"
#include "EnhancedAsyncActionContextImpl.h"
#include "EnhancedAsyncActionShared.h"
#include "Logging/StructuredLog.h"
#include "UObject/TextProperty.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnhancedAsyncActionContextLibrary)

FEnhancedAsyncActionContextHandle UEnhancedAsyncActionContextLibrary::CreateContextForObject(const UObject* Action, FName InnerContainerProperty)
{
	if (!IsValid(Action))
	{
		FFrame::KismetExecutionMessage(TEXT("CreateContextForObject failed for node"), ELogVerbosity::Error, TEXT("EAA_CreateContextForAction"));
		return FEnhancedAsyncActionContextHandle();
	}

	TSharedPtr<FEnhancedAsyncActionContext> Ctx;
	
	const FName AsyncContextContainerProperty = InnerContainerProperty;
	if (EAA::Internals::IsValidContainerProperty(Action, AsyncContextContainerProperty))
	{
		Ctx = MakeShared<FEnhancedAsyncActionContext_PropertyBagRef>(Action, AsyncContextContainerProperty);
	}
	else
	{
		Ctx = MakeShared<FEnhancedAsyncActionContext_PropertyBag>();
	}

	FEnhancedAsyncActionContextHandle Handle = FEnhancedAsyncActionManager::Get().SetContext(Action, Ctx.ToSharedRef());
	Handle.DataProperty = AsyncContextContainerProperty;
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
	auto ContextHandle = FEnhancedAsyncActionManager::Get().FindContextHandle(Context);
	if (!ContextHandle.IsValid())
	{
		ContextHandle = FEnhancedAsyncActionManager::Get().SetContext(Context, MakeShared<FEnhancedAsyncActionContext_PropertyBag>());
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

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Object(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UObject* Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->SetValueObject(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Class(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UClass* Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->SetValueClass(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Basic(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, EPropertyBagPropertyType Type, const int32 Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_Basic)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,Z_Param_Out_Handle);
	P_GET_PROPERTY(FIntProperty,Z_Param_Index);
	P_GET_ENUM(EPropertyBagPropertyType,Z_Param_Type);
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	
	const FProperty* ValueProp = CastField<FProperty>(Stack.MostRecentProperty);
	const void* ValuePtr = Stack.MostRecentPropertyAddress;
	
	P_FINISH;

	if (!ValueProp || !ValuePtr)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			INVTEXT("Failed to resolve the Value for Set Value Basic")
		);

		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		return;
	}
	
	P_NATIVE_BEGIN;

	bool bValidCall = false;
	
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(Z_Param_Out_Handle);
	switch (Z_Param_Type)
	{
	case EPropertyBagPropertyType::Bool:
		bValidCall = ValueProp && ValueProp->IsA(FBoolProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->SetValueBool(Z_Param_Index, *(const bool*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Byte:
		bValidCall = ValueProp && ValueProp->IsA(FByteProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->SetValueByte(Z_Param_Index, *(const uint8*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Int32:
		bValidCall = ValueProp && ValueProp->IsA(FIntProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->SetValueInt32(Z_Param_Index, *(const int32*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Int64:
		bValidCall = ValueProp && ValueProp->IsA(FInt64Property::StaticClass());
		if (bValidCall)
		{
			ContextSafe->SetValueInt64(Z_Param_Index, *(const int64*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Float:
		bValidCall = ValueProp && ValueProp->IsA(FFloatProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->SetValueFloat(Z_Param_Index, *(const float*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Double:
		bValidCall = ValueProp && ValueProp->IsA(FDoubleProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->SetValueDouble(Z_Param_Index, *(const double*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::String:
		bValidCall = ValueProp && ValueProp->IsA(FStrProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->SetValueString(Z_Param_Index, *(const FString*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Name:
		bValidCall = ValueProp && ValueProp->IsA(FNameProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->SetValueName(Z_Param_Index, *(const FName*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Text:
		bValidCall = ValueProp && ValueProp->IsA(FTextProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->SetValueText(Z_Param_Index, *(const FText*)ValuePtr);
		}
		break;
	default:
		bValidCall = false;
		break;
	}

	if (!bValidCall)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			INVTEXT("Unsupported property type the Value for Set Value Basic")
		);

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
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,Z_Param_Out_Handle);
	P_GET_PROPERTY(FIntProperty,Z_Param_Index);
	uint8 Z_Param_Out_Value = 0;
	Stack.StepCompiledIn<FEnumProperty>(&Z_Param_Out_Value);

	const FEnumProperty* Z_Param_Out_Type = CastFieldChecked<FEnumProperty>(Stack.MostRecentProperty);

	P_FINISH;
	
	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(Z_Param_Out_Handle);
	ContextSafe->SetValueByte(Z_Param_Index,Z_Param_Out_Value);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Struct(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_Struct)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,Z_Param_Out_Handle);
	P_GET_PROPERTY(FIntProperty,Z_Param_Index);
	
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	
	const FStructProperty* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	const void* ValuePtr = Stack.MostRecentPropertyAddress;
	
	P_FINISH;

	if (!ValueProp || !ValuePtr)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			INVTEXT("Failed to resolve the Value for Set Value Struct")
		);

		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		return;
	}

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(Z_Param_Out_Handle);
	ContextSafe->SetValueStruct(Z_Param_Index, ValueProp->Struct, FConstStructView(ValueProp->Struct, (const uint8*)ValuePtr));
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Array(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const TArray<int32>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_Array)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,Z_Param_Out_Handle);
	P_GET_PROPERTY(FIntProperty,Z_Param_Index);
	
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(NULL);
	void* SrcArrayAddr = Stack.MostRecentPropertyAddress;
	FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	if (!ArrayProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	P_FINISH;

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(Z_Param_Out_Handle);
	auto ElementType = EAA::Internals::GetValueTypeFromProperty(ArrayProperty->Inner);
	auto ElementTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(ArrayProperty->Inner);
	void* PropertyAddress = nullptr;
	ContextSafe->GetArrayRefForWrite(Z_Param_Index, ElementType, ElementTypeObject, PropertyAddress);
	if (PropertyAddress)
	{
		ArrayProperty->CopyCompleteValueFromScriptVM(PropertyAddress, SrcArrayAddr);
	}
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_SetValue_Set(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, const TSet<int32>& Value)
{
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_SetValue_Set)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,Z_Param_Out_Handle);
	P_GET_PROPERTY(FIntProperty,Z_Param_Index);
	
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FSetProperty>(NULL);
	void* SrcSetAddr = Stack.MostRecentPropertyAddress;
	FSetProperty* SetProperty = CastField<FSetProperty>(Stack.MostRecentProperty);
	if (!SetProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	P_FINISH;

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(Z_Param_Out_Handle);
	auto ElementType = EAA::Internals::GetValueTypeFromProperty(SetProperty->ElementProp);
	auto ElementTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(SetProperty->ElementProp);
	void* PropertyAddress = nullptr;
	ContextSafe->GetSetRefForWrite(Z_Param_Index, ElementType, ElementTypeObject, PropertyAddress);
	if (PropertyAddress)
	{
		SetProperty->CopyCompleteValueFromScriptVM(PropertyAddress, SrcSetAddr);
	}
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

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Object(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UObject*& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->GetValueObject(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Class(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, UClass*& Value)
{
	FEnhancedAsyncActionManager::Get().FindContextSafe(Handle)->GetValueClass(Index, Value);
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Basic(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, EPropertyBagPropertyType Type, int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_Basic)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,Z_Param_Out_Handle);
	P_GET_PROPERTY(FIntProperty,Z_Param_Index);
	P_GET_ENUM(EPropertyBagPropertyType,Z_Param_Type);
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	
	const FProperty* ValueProp = CastField<FProperty>(Stack.MostRecentProperty);
	void* ValuePtr = Stack.MostRecentPropertyAddress;
	
	P_FINISH;

	if (!ValueProp || !ValuePtr)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			INVTEXT("Failed to resolve the Value for Get Value Basic")
		);

		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		return;
	}
	
	P_NATIVE_BEGIN;

	bool bValidCall = false;
	
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(Z_Param_Out_Handle);
	switch (Z_Param_Type)
	{
	case EPropertyBagPropertyType::Bool:
		bValidCall = ValueProp && ValueProp->IsA(FBoolProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->GetValueBool(Z_Param_Index, *(bool*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Byte:
		bValidCall = ValueProp && ValueProp->IsA(FByteProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->GetValueByte(Z_Param_Index, *(uint8*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Int32:
		bValidCall = ValueProp && ValueProp->IsA(FIntProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->GetValueInt32(Z_Param_Index, *(int32*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Int64:
		bValidCall = ValueProp && ValueProp->IsA(FInt64Property::StaticClass());
		if (bValidCall)
		{
			ContextSafe->GetValueInt64(Z_Param_Index, *(int64*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Float:
		bValidCall = ValueProp && ValueProp->IsA(FFloatProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->GetValueFloat(Z_Param_Index, *(float*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Double:
		bValidCall = ValueProp && ValueProp->IsA(FDoubleProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->GetValueDouble(Z_Param_Index, *(double*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::String:
		bValidCall = ValueProp && ValueProp->IsA(FStrProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->GetValueString(Z_Param_Index, *(FString*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Name:
		bValidCall = ValueProp && ValueProp->IsA(FNameProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->GetValueName(Z_Param_Index, *(FName*)ValuePtr);
		}
		break;
	case EPropertyBagPropertyType::Text:
		bValidCall = ValueProp && ValueProp->IsA(FTextProperty::StaticClass());
		if (bValidCall)
		{
			ContextSafe->GetValueText(Z_Param_Index, *(FText*)ValuePtr);
		}
		break;
	default:
		bValidCall = false;
		break;
	}

	if (!bValidCall)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			INVTEXT("Unsupported property type the Value for Set Value Basic")
		);

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
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,Z_Param_Out_Handle);
	P_GET_PROPERTY(FIntProperty,Z_Param_Index);

	uint8 ExecResultTemp = 0;
	uint8& Z_Param_Out_Value = Stack.StepCompiledInRef<FEnumProperty, uint8>(&ExecResultTemp);

	const FEnumProperty* Z_Param_Out_Type = CastFieldChecked<FEnumProperty>(Stack.MostRecentProperty);

	P_FINISH;
	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(Z_Param_Out_Handle);
	ContextSafe->GetValueByte(Z_Param_Index,Z_Param_Out_Value);
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Struct(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_Struct)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,Z_Param_Out_Handle);
	P_GET_PROPERTY(FIntProperty,Z_Param_Index);
	
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	
	const FStructProperty* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* ValuePtr = Stack.MostRecentPropertyAddress;
	
	P_FINISH;

	if (!ValueProp || !ValuePtr)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			INVTEXT("Failed to resolve the Value for Set Value Struct")
		);

		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		return;
	}

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(Z_Param_Out_Handle);
	FStructView Result;
	ContextSafe->GetValueStruct(Z_Param_Index, ValueProp->Struct, Result);
	if (Result.IsValid() && Result.GetScriptStruct()->IsChildOf(ValueProp->Struct))
	{
		ValueProp->Struct->CopyScriptStruct(ValuePtr, Result.GetMemory());
	}
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Array(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TArray<int32>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_Array)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,Z_Param_Out_Handle);
	P_GET_PROPERTY(FIntProperty,Z_Param_Index);
	
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(NULL);
	void* SrcArrayAddr = Stack.MostRecentPropertyAddress;
	FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	if (!ArrayProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	P_FINISH;

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(Z_Param_Out_Handle);
	auto ElementType = EAA::Internals::GetValueTypeFromProperty(ArrayProperty->Inner);
	auto ElementTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(ArrayProperty->Inner);
	
	const void* PropertyAddress = nullptr;
	ContextSafe->GetArrayRefForRead(Z_Param_Index, ElementType, ElementTypeObject, PropertyAddress);
	if (PropertyAddress)
	{
		ArrayProperty->CopyCompleteValueToScriptVM(SrcArrayAddr, PropertyAddress);
	}
	P_NATIVE_END;
}

void UEnhancedAsyncActionContextLibrary::Handle_GetValue_Set(const FEnhancedAsyncActionContextHandle& Handle, int32 Index, TSet<int32>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncActionContextLibrary::execHandle_GetValue_Set)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,Z_Param_Out_Handle);
	P_GET_PROPERTY(FIntProperty,Z_Param_Index);
	
	// Read wildcard Value input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FSetProperty>(NULL);
	void* SrcAddr = Stack.MostRecentPropertyAddress;
	FSetProperty* SetProperty = CastField<FSetProperty>(Stack.MostRecentProperty);
	if (!SetProperty)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	P_FINISH;

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncActionManager::Get().FindContextSafe(Z_Param_Out_Handle);
	auto ElementType = EAA::Internals::GetValueTypeFromProperty(SetProperty->ElementProp);
	auto ElementTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(SetProperty->ElementProp);
	
	const void* PropertyAddress = nullptr;
	ContextSafe->GetSetRefForRead(Z_Param_Index, ElementType, ElementTypeObject, PropertyAddress);
	if (PropertyAddress)
	{
		SetProperty->CopyCompleteValueToScriptVM(SrcAddr, PropertyAddress);
	}
	P_NATIVE_END;
}