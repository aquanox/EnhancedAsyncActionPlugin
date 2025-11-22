// Copyright 2025, Aquanox.

#include "EnhancedAsyncContextLibrary.h"

#include "Blueprint/BlueprintExceptionInfo.h"
#include "Engine/Engine.h"
#include "EnhancedAsyncContextManager.h"
#include "EnhancedAsyncContextImpl.h"
#include "EnhancedAsyncContextShared.h"
#include "EnhancedAsyncActionHandle.h"
#include "EnhancedLatentActionHandle.h"
#include "Logging/StructuredLog.h"
#include "UObject/TextProperty.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnhancedAsyncContextLibrary)

#define EAA_KISMET_ENSURE(Expr, Message) \
	if (!ensureAlways(Expr)) { \
		FBlueprintExceptionInfo ExceptionInfo(EBlueprintExceptionType::AbortExecution,  INVTEXT(Message) );  \
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);  \
		return; \
	}

#define EAA_CONCAT_IMPL(A, B) A ## B
#define EAA_CONCAT(A, B) EAA_CONCAT_IMPL(A, B)

#define EAA_P_GET_DYNAMIC(PropertyType, Property, TmpName) \
	const int32 EAA_CONCAT(TmpName, PropertySize) = Property->GetElementSize() * Property->ArrayDim; \
	void* EAA_CONCAT(TmpName, StorageSpace) = FMemory_Alloca(EAA_CONCAT(TmpName, PropertySize)); \
	FMemory::Memzero(EAA_CONCAT(TmpName, StorageSpace), EAA_CONCAT(TmpName, PropertySize)); \
	InnerProp->InitializeValue(EAA_CONCAT(TmpName, StorageSpace)); \
	Stack.MostRecentPropertyAddress = nullptr; \
	Stack.MostRecentPropertyContainer = nullptr; \
	Stack.StepCompiledIn<PropertyType>(EAA_CONCAT(TmpName, StorageSpace)); \
	void* const TmpName = EAA_CONCAT(TmpName, StorageSpace);

#define EEA_P_GET_HANDLE_PTR(Type, Name) \
	Stack.MostRecentProperty = nullptr; \
	Stack.MostRecentPropertyAddress = nullptr; \
	Stack.StepCompiledIn<FStructProperty>(nullptr); \
	const FStructProperty* EAA_CONCAT(Name, Prop) = CastField<FStructProperty>(Stack.MostRecentProperty); \
	Type* const EAA_CONCAT(Name, Ptr) = reinterpret_cast<Type*>(Stack.MostRecentPropertyAddress);

// =================== RESOLVERS ===========================

template<typename T>
TSharedRef<FEnhancedAsyncActionContext> ResolveContext(const T& Handle) = delete;

template<>
TSharedRef<FEnhancedAsyncActionContext> ResolveContext(const FAsyncContextHandleBase& Handle)
{
	return FEnhancedAsyncContextManager::Get().FindContext(Handle/*.GetId()*/, EResolveErrorMode::Fallback).ToSharedRef();
}

template<>
TSharedRef<FEnhancedAsyncActionContext> ResolveContext(const FEnhancedAsyncActionContextHandle& Handle)
{
	return FEnhancedAsyncContextManager::Get().FindContext(Handle/*.GetId()*/, EResolveErrorMode::Fallback).ToSharedRef();
}

template<>
TSharedRef<FEnhancedAsyncActionContext> ResolveContext(const FEnhancedLatentActionContextHandle& Handle)
{
	return FEnhancedAsyncContextManager::Get().FindContext(Handle/*.GetId()*/, EResolveErrorMode::Fallback).ToSharedRef();
}

// =================== CORE ===========================

FEnhancedAsyncActionContextHandle UEnhancedAsyncContextLibrary::CreateContextForObject(const UObject* Action, FName InDataProperty)
{
	auto ValueOrError = FEnhancedAsyncContextManager::Get().CreateContext(Action, InDataProperty);
	if (ValueOrError.HasError())
	{
		UE_LOG(LogEnhancedAction, Warning, TEXT("CreateContextForObject failed for node: %s"), *ValueOrError.GetError());
		return FEnhancedAsyncActionContextHandle();
	}
	return ValueOrError.GetValue();
}

FEnhancedLatentActionContextHandle UEnhancedAsyncContextLibrary::CreateContextForLatent(const UObject* Owner, int32 UUID, int32 CallUUID, bool bInitContainer, FEnhancedLatentActionDelegate Delegate)
{
	UE_LOG(LogEnhancedAction, Verbose, TEXT("CreateLatent Owner=%s UUID=%d call=%d init=%d"), *GetNameSafe(Owner), UUID, CallUUID, bInitContainer);

	const FLatentCallInfo CallInfo = FLatentCallInfo::Make(Owner, UUID, CallUUID, Delegate);
	if (!bInitContainer)
	{ // context handle without container requested (when there is nothing to capture)
		return FEnhancedLatentActionContextHandle(CallInfo);
	}

	auto ValueOrError = FEnhancedAsyncContextManager::Get().CreateContext(CallInfo);
	if (ValueOrError.HasError())
	{
		UE_LOG(LogEnhancedAction, Warning, TEXT("CreateContextForLatent failed for node: %s"), *ValueOrError.GetError());
		return FEnhancedLatentActionContextHandle();
	}
	return ValueOrError.GetValue();
}

void UEnhancedAsyncContextLibrary::DestroyContextForLatent(const FEnhancedLatentActionContextHandle& Handle)
{
	UE_LOG(LogEnhancedAction, Verbose, TEXT("DestroyLatentContext %s"), *Handle.GetDebugString());
	auto ValueOrError = FEnhancedAsyncContextManager::Get().DestroyContext(Handle.GetId());
	if (ValueOrError.HasError())
	{
		UE_LOG(LogEnhancedAction, Warning, TEXT("DestroyContextForLatent failed for node: %s"), *ValueOrError.GetError());
	}
}

void UEnhancedAsyncContextLibrary::SetupContextContainer(const FAsyncContextHandleBase& Handle, const FString& Config)
{
	ResolveContext(Handle)->SetupFromStringDefinition(Config);
}

FEnhancedAsyncActionContextHandle UEnhancedAsyncContextLibrary::GetContextForObject(const UObject* Action)
{
	auto Handle = FEnhancedAsyncContextManager::Get().FindContextHandle(Action);
	return Handle;
}

FEnhancedLatentActionContextHandle UEnhancedAsyncContextLibrary::GetContextForLatent(const UObject* Owner, int32 UUID, int32 CallUUID)
{
	const FLatentCallInfo CallInfo = FLatentCallInfo::Make(Owner, UUID, CallUUID);
	FEnhancedLatentActionContextHandle Handle = FEnhancedAsyncContextManager::Get().FindContextHandle(CallInfo);
	return Handle;
}

void UEnhancedAsyncContextLibrary::DumpContextForObject(const UObject* Action)
{
	auto Context = FEnhancedAsyncContextManager::Get().FindContext(FAsyncContextId::Make(Action));
	if (Context)
	{
		FStringBuilderBase Builder;
		Context->DebugDump(Builder);
		UE_LOG(LogEnhancedAction, Log, TEXT("Context for action %p [%s]:\n%s"), Action, *Context->GetDebugName(), Builder.ToString());
	}
}

void UEnhancedAsyncContextLibrary::DumpContext(const FAsyncContextHandleBase& Handle)
{
	auto Context = ResolveContext(Handle);
	{
		FStringBuilderBase Builder;
		Context->DebugDump(Builder);
		UE_LOG(LogEnhancedAction, Log, TEXT("Context for handle %p [%s]:\n%s"), &Handle, *Context->GetDebugName(), Builder.ToString());
	}
}

struct FAsyncContextHandleBase UEnhancedAsyncContextLibrary::GetHandleFromLatentHandle(const FEnhancedAsyncActionContextHandle& Handle)
{
	return FAsyncContextHandleBase(Handle);
}

struct FAsyncContextHandleBase UEnhancedAsyncContextLibrary::GetHandleFromActionHandle(const FEnhancedLatentActionContextHandle& Handle)
{
	return FAsyncContextHandleBase(Handle);
}

void UEnhancedAsyncContextLibrary::FindCaptureContextForObject(const UObject* Action, FEnhancedAsyncActionContextHandle& Handle, bool& Valid)
{
	Valid = false;
	if (IsValid(Action))
	{
		Handle = FEnhancedAsyncContextManager::Get().FindContextHandle(Action);
		Valid = Handle.IsValid();
	}
}

// =================== SETTERS ===========================

void UEnhancedAsyncContextLibrary::Handle_SetValue_Bool(const FAsyncContextHandleBase& Handle, int32 Index, const bool& Value)
{
	ResolveContext(Handle)->SetValueBool(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Byte(const FAsyncContextHandleBase& Handle, int32 Index, const uint8& Value)
{
	ResolveContext(Handle)->SetValueByte(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Int32(const FAsyncContextHandleBase& Handle, int32 Index, const int32& Value)
{
	ResolveContext(Handle)->SetValueInt32(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Int64(const FAsyncContextHandleBase& Handle, int32 Index, const int64& Value)
{
	ResolveContext(Handle)->SetValueInt64(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Float(const FAsyncContextHandleBase& Handle, int32 Index, const float& Value)
{
	ResolveContext(Handle)->SetValueFloat(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Double(const FAsyncContextHandleBase& Handle, int32 Index, const double& Value)
{
	ResolveContext(Handle)->SetValueDouble(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_String(const FAsyncContextHandleBase& Handle, int32 Index, const FString& Value)
{
	ResolveContext(Handle)->SetValueString(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Name(const FAsyncContextHandleBase& Handle, int32 Index, const FName& Value)
{
	ResolveContext(Handle)->SetValueName(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Text(const FAsyncContextHandleBase& Handle, int32 Index, const FText& Value)
{
	ResolveContext(Handle)->SetValueText(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Variadic(const FAsyncContextHandleBase& Handle, const TArray<FString>& Names)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_SetValue_Variadic)
{
	// Read the standard function arguments
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	const FStructProperty* HandleProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	const FAsyncContextHandleBase* HandlePtr = reinterpret_cast<const FAsyncContextHandleBase*>(Stack.MostRecentPropertyAddress);

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

	EAA_KISMET_ENSURE(HandleProp != nullptr && HandlePtr != nullptr, "Failed to resolve the Handle Property for Set Value Variadic");

	UE_LOG(LogEnhancedAction, Verbose, TEXT("Write context %s"), *HandlePtr->GetDebugString());

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(*HandlePtr, EResolveErrorMode::Fallback);
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

void UEnhancedAsyncContextLibrary::Handle_SetValue_Generic(const FAsyncContextHandleBase& Handle, int32 Index, const int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_SetValue_Generic)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase, ParamHandle);
	P_GET_PROPERTY(FIntProperty, ParamIndex);

	// Read wildcard Value input.
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);

	const FProperty* ValueProp = CastField<FProperty>(Stack.MostRecentProperty);
	const void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	EAA_KISMET_ENSURE(ValueProp != nullptr && ValuePtr != nullptr, "Failed to resolve the Value Property for Set Value Basic");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);

	FString Message;
	if (!ensureAlways(ContextSafe->SetValueByIndex(ParamIndex, ValueProp, ValuePtr, Message)))
	{
		FBlueprintExceptionInfo ExceptionInfo(EBlueprintExceptionType::AbortExecution,  FText::FromString(Message) );
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Enum(const FAsyncContextHandleBase& Handle, int32 Index, const int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_SetValue_Enum)
{
	P_GET_STRUCT_REF(FEnhancedAsyncActionContextHandle,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);

	uint8 ParamValue = 0;
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FEnumProperty>(&ParamValue);
	const FEnumProperty* ParamValueProp = CastField<FEnumProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Failed to resolve the Value Property for Set Value Enum");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	ContextSafe->SetValueEnum(ParamIndex, ParamValueProp->GetEnum(),  ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Struct(const FAsyncContextHandleBase& Handle, int32 Index, const int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_SetValue_Struct)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);

	// Read wildcard Value input.
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	const FStructProperty* ParamValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	const void* ParamValue = Stack.MostRecentPropertyAddress;
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr && ParamValue != nullptr, "Failed to resolve the Value Property for Set Value Struct");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	ContextSafe->SetValueStruct(ParamIndex, ParamValueProp->Struct, (const uint8*)ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Object(const FAsyncContextHandleBase& Handle, int32 Index, UObject* Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_SetValue_Object)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_OBJECT(UObject,ParamValue);
	const FObjectProperty* ParamValueProp = CastField<FObjectProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Failed to resolve the Value Property for Set Value Object");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	ContextSafe->SetValueObject(ParamIndex, ParamValueProp->PropertyClass, ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_SoftObject(const FAsyncContextHandleBase& Handle, int32 Index, TSoftObjectPtr<UObject> Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_SetValue_SoftObject)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase, ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_SOFTOBJECT(TSoftObjectPtr<UObject>,ParamValue);
	const FSoftObjectProperty* ParamValueProp = CastField<FSoftObjectProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Failed to resolve the Value Property for Set Value Soft Object");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	ContextSafe->SetValueSoftObject(ParamIndex, ParamValueProp->PropertyClass, ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Class(const FAsyncContextHandleBase& Handle, int32 Index, UClass* Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_SetValue_Class)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_OBJECT(UClass,ParamValue);
	const FClassProperty* ParamValueProp = CastField<FClassProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Failed to resolve the Value Property for Set Value Class");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	ContextSafe->SetValueClass(ParamIndex, ParamValueProp->MetaClass, ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_SoftClass(const FAsyncContextHandleBase& Handle, int32 Index, TSoftClassPtr<UObject> Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_SetValue_SoftClass)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_SOFTCLASS(TSoftClassPtr<UObject> ,ParamValue);
	const FSoftClassProperty* ParamValueProp = CastField<FSoftClassProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Failed to resolve the Value Property for Set Value Class");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	ContextSafe->SetValueSoftClass(ParamIndex, ParamValueProp->MetaClass, ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Array(const FAsyncContextHandleBase& Handle, int32 Index, const TArray<int32>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_SetValue_Array)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);

	// Read wildcard Value input.
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);

	FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	const void* ArrayAddr = Stack.MostRecentPropertyAddress;
	const void* ArrayContainerAddr = Stack.MostRecentPropertyContainer;
	P_FINISH;

	if (ArrayProperty == nullptr || ArrayAddr == nullptr)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	auto ValueType = EAA::Internals::GetValueTypeFromProperty(ArrayProperty);
	auto ValueTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(ArrayProperty);

	const void* ValuePtr = ArrayProperty->ContainerPtrToValuePtr<const void>(ArrayContainerAddr);
	ensureAlways(ValuePtr == ArrayAddr);
	ContextSafe->SetValueArray(ParamIndex, ValueType, ValueTypeObject, ArrayAddr);
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_SetValue_Set(const FAsyncContextHandleBase& Handle, int32 Index, const TSet<int32>& Value)
{
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_SetValue_Set)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);

	// Read wildcard Value input.
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FSetProperty>(nullptr);

	FSetProperty* SetProperty = CastField<FSetProperty>(Stack.MostRecentProperty);
	const void* SetAddr = Stack.MostRecentPropertyAddress;
	const void* SetContainerAddr = Stack.MostRecentPropertyContainer;
	P_FINISH;

	if (SetProperty == nullptr || SetAddr == nullptr)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	auto ValueType = EAA::Internals::GetValueTypeFromProperty(SetProperty);
	auto ValueTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(SetProperty);

	const void* ValuePtr = SetProperty->ContainerPtrToValuePtr<const void>(SetContainerAddr);
	ensureAlways(ValuePtr == SetAddr);
	ContextSafe->SetValueSet(ParamIndex, ValueType, ValueTypeObject, SetAddr);
	P_NATIVE_END;
}

// ==================== GETTERS =========================

void UEnhancedAsyncContextLibrary::Handle_GetValue_Bool(const FAsyncContextHandleBase& Handle, int32 Index, bool& Value)
{
	ResolveContext(Handle)->GetValueBool(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Byte(const FAsyncContextHandleBase& Handle, int32 Index, uint8& Value)
{
	ResolveContext(Handle)->GetValueByte(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Int32(const FAsyncContextHandleBase& Handle, int32 Index, int32& Value)
{
	ResolveContext(Handle)->GetValueInt32(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Int64(const FAsyncContextHandleBase& Handle, int32 Index, int64& Value)
{
	ResolveContext(Handle)->GetValueInt64(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Float(const FAsyncContextHandleBase& Handle, int32 Index, float& Value)
{
	ResolveContext(Handle)->GetValueFloat(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Double(const FAsyncContextHandleBase& Handle, int32 Index, double& Value)
{
	ResolveContext(Handle)->GetValueDouble(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_String(const FAsyncContextHandleBase& Handle, int32 Index, FString& Value)
{
	ResolveContext(Handle)->GetValueString(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Name(const FAsyncContextHandleBase& Handle, int32 Index, FName& Value)
{
	ResolveContext(Handle)->GetValueName(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Text(const FAsyncContextHandleBase& Handle, int32 Index, FText& Value)
{
	ResolveContext(Handle)->GetValueText(Index, Value);
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Variadic(const FAsyncContextHandleBase& Handle, const TArray<FString>& Names)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_GetValue_Variadic)
{
	// Read the standard function arguments
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	const FStructProperty* HandleProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	const FAsyncContextHandleBase* HandlePtr = reinterpret_cast<const FAsyncContextHandleBase*>(Stack.MostRecentPropertyAddress);

    P_GET_TARRAY_REF(FString, Parameters);

    struct FOutputParam { int32 Index; FName Name; const FProperty* Property; void* Value;  };
    TArray<FOutputParam, TInlineAllocator<16>> VarOutputs;

    // Read the input values and write them to the Python context
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

	EAA_KISMET_ENSURE(HandleProp != nullptr && HandlePtr != nullptr, "Failed to resolve the Handle Property for Get Value Variadic");

	UE_LOG(LogEnhancedAction, Verbose, TEXT("Read context %s"), *HandlePtr->GetDebugString());

    P_NATIVE_BEGIN;
    auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(*HandlePtr, EResolveErrorMode::Fallback);
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

void UEnhancedAsyncContextLibrary::Handle_GetValue_Generic(const FAsyncContextHandleBase& Handle, int32 Index, int32& Value)
{

}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_GetValue_Generic)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase, ParamHandle);
	P_GET_PROPERTY(FIntProperty, ParamIndex);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);

	const FProperty* ValueProp = CastField<FProperty>(Stack.MostRecentProperty);
	void* ValuePtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	EAA_KISMET_ENSURE(ValueProp != nullptr && ValuePtr != nullptr, "Failed to resolve the Value Property for Get Value Generic");

	P_NATIVE_BEGIN;

	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);

	FString Message;
	if (!ensureAlways(ContextSafe->GetValueByIndex(ParamIndex, ValueProp, ValuePtr, Message)))
	{
		FBlueprintExceptionInfo ExceptionInfo(EBlueprintExceptionType::AbortExecution,  FText::FromString(Message) );
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}

	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Enum(const FAsyncContextHandleBase& Handle, int32 Index, int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_GetValue_Enum)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);

	uint8 ExecResultTemp = 0;

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	uint8& ParamValue = Stack.StepCompiledInRef<FEnumProperty, uint8>(&ExecResultTemp);
	const FEnumProperty* ParamValueProp = CastField<FEnumProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Unsupported property type the Value for Set Value Enum");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	ContextSafe->GetValueEnum(ParamIndex, ParamValueProp->GetEnum(), ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Struct(const FAsyncContextHandleBase& Handle, int32 Index, int32& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_GetValue_Struct)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);

	// Read wildcard Value input.
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);

	const FStructProperty* ParamValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* ParamValue = Stack.MostRecentPropertyAddress;
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr && ParamValue != nullptr, "Unsupported property type the Value for Set Value Struct");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	const uint8* Result = nullptr;
	ContextSafe->GetValueStruct(ParamIndex, ParamValueProp->Struct, Result);
	if (Result)
	{
		ParamValueProp->Struct->CopyScriptStruct(ParamValue, Result);
	}
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Object(const FAsyncContextHandleBase& Handle, int32 Index, UObject*& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_GetValue_Object)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_OBJECT_REF(UObject,ParamValue);
	const FObjectProperty* ParamValueProp = CastField<FObjectProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Unsupported property type the Value for Set Value Object");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	ContextSafe->GetValueObject(ParamIndex, ParamValueProp->PropertyClass, P_ARG_GC_BARRIER(ParamValue));
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_SoftObject(const FAsyncContextHandleBase& Handle, int32 Index, TSoftObjectPtr<UObject>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_GetValue_SoftObject)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_SOFTOBJECT_REF(TSoftObjectPtr<UObject>,ParamValue);
	const FSoftObjectProperty* ParamValueProp = CastField<FSoftObjectProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Unsupported property type the Value for Get Value Soft Object");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	ContextSafe->GetValueSoftObject(ParamIndex, ParamValueProp->PropertyClass, ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Class(const FAsyncContextHandleBase& Handle, int32 Index, UClass*& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_GetValue_Class)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_OBJECT_REF(UClass,ParamValue);
	const FClassProperty* ParamValueProp = CastField<FClassProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Unsupported property type the Value for Set Value Class");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	ContextSafe->GetValueClass(ParamIndex, ParamValueProp->MetaClass, P_ARG_GC_BARRIER(ParamValue));
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_SoftClass(const FAsyncContextHandleBase& Handle, int32 Index, TSoftClassPtr<UObject>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_GetValue_SoftClass)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);
	P_GET_SOFTCLASS_REF(TSoftClassPtr<UObject>,ParamValue);
	const FSoftClassProperty* ParamValueProp = CastField<FSoftClassProperty>(Stack.MostRecentProperty);
	P_FINISH;

	EAA_KISMET_ENSURE(ParamValueProp != nullptr, "Unsupported property type the Value for Get Value Soft Class");

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	ContextSafe->GetValueSoftClass(ParamIndex, ParamValueProp->MetaClass, ParamValue);
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Array(const FAsyncContextHandleBase& Handle, int32 Index, TArray<int32>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_GetValue_Array)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);

	// Read wildcard Value input.
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);

	FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	void* ArrayAddr = Stack.MostRecentPropertyAddress;
	void* ArrayContainerAddr = Stack.MostRecentPropertyContainer;
	P_FINISH;

	if (ArrayProperty == nullptr || ArrayAddr == nullptr)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	auto ValueType = EAA::Internals::GetValueTypeFromProperty(ArrayProperty);
	auto ValueTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(ArrayProperty);

	void* ValuePtr = ArrayProperty->ContainerPtrToValuePtr<void>(ArrayContainerAddr);
	ensureAlways(ValuePtr == ArrayAddr);
	ContextSafe->GetValueArray(ParamIndex, ValueType, ValueTypeObject, ArrayAddr);
	P_NATIVE_END;
}

void UEnhancedAsyncContextLibrary::Handle_GetValue_Set(const FAsyncContextHandleBase& Handle, int32 Index, TSet<int32>& Value)
{
	checkNoEntry();
}

DEFINE_FUNCTION(UEnhancedAsyncContextLibrary::execHandle_GetValue_Set)
{
	P_GET_STRUCT_REF(FAsyncContextHandleBase,ParamHandle);
	P_GET_PROPERTY(FIntProperty,ParamIndex);

	// Read wildcard Value input.
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FSetProperty>(nullptr);

	FSetProperty* SetProperty = CastField<FSetProperty>(Stack.MostRecentProperty);
	void* SetAddr = Stack.MostRecentPropertyAddress;
	void* SetContainerAddr = Stack.MostRecentPropertyContainer;
	P_FINISH;

	if (SetProperty == nullptr || SetAddr == nullptr)
	{
		Stack.bArrayContextFailed = true;
		return;
	}

	P_NATIVE_BEGIN;
	auto ContextSafe = FEnhancedAsyncContextManager::Get().FindContext(ParamHandle);
	auto ValueType = EAA::Internals::GetValueTypeFromProperty(SetProperty);
	auto ValueTypeObject = EAA::Internals::GetValueTypeObjectFromProperty(SetProperty);

	void* ValuePtr = SetProperty->ContainerPtrToValuePtr<void>(SetContainerAddr);
	ensureAlways(ValuePtr == SetAddr);
	ContextSafe->GetValueSet(ParamIndex, ValueType, ValueTypeObject, SetAddr);
	P_NATIVE_END;
}

#undef EAA_KISMET_ARRAY_ENSURE
#undef EAA_KISMET_ENSURE
