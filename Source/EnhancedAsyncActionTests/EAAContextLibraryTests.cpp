// Copyright 2025, Aquanox.

#include "EAAContextLibraryTests.h"
#include "CoreMinimal.h"
#include "EnhancedAsyncActionContextLibrary.h"
#include "EnhancedAsyncActionContext.h"
#include "StructUtils/InstancedStruct.h"
#include "StructUtils/PropertyBag.h"
#include "EAADemoAsyncAction.h"
#include "Math/UnrealMathUtility.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "EnhancedAsyncActionManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/AssertionMacros.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAALibraryTestCreate,
	"EnhancedAsyncAction.Context.Create", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::HighPriority);

bool FEAALibraryTestCreate::RunTest(FString const&)
{
	FTestWorldScope Scope;
	auto* World = Scope.World;

	auto NullHandle = UEnhancedAsyncActionContextLibrary::CreateContextForObject(nullptr, NAME_None);
	UTEST_FALSE_EXPR(NullHandle.IsValid());
	UTEST_TRUE_EXPR(NullHandle.IsExternal());

	auto* Task = UEAADemoAsyncActionCapture::StartActionWithCapture(World, false, 42);
	auto TaskHandle = UEnhancedAsyncActionContextLibrary::CreateContextForObject(Task, NAME_None);
	UTEST_TRUE_EXPR(TaskHandle.IsValid());
	UTEST_TRUE_EXPR(TaskHandle.IsExternal());

	auto* TaskWithContainer = UEAADemoAsyncActionCaptureMember::StartActionWithCaptureFixed(World, false, 42);
	auto TaskContainerHandle = UEnhancedAsyncActionContextLibrary::CreateContextForObject(TaskWithContainer, TEXT("ContextData"));
	UTEST_TRUE_EXPR(TaskContainerHandle.IsValid());
	UTEST_FALSE_EXPR(TaskContainerHandle.IsExternal());

	auto* SampleObject = NewObject<UBlueprintAsyncActionBase>();
	auto TaskContainerHandleBadProp = UEnhancedAsyncActionContextLibrary::CreateContextForObject(SampleObject, TEXT("BAD"));
	UTEST_TRUE_EXPR(TaskContainerHandleBadProp.IsValid());
	UTEST_TRUE_EXPR(TaskContainerHandleBadProp.IsExternal());

	auto DebugHandle = UEnhancedAsyncActionContextLibrary::GetTestCaptureContext(World);
	UTEST_TRUE_EXPR(DebugHandle.IsValid());
	UTEST_TRUE_EXPR(DebugHandle.IsExternal());

	return true;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAALibraryOpsBasic,
	"EnhancedAsyncAction.Context.Basic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::HighPriority);

bool FEAALibraryOpsBasic::RunTest(FString const&)
{
	auto* Owner = NewObject<UBlueprintAsyncActionBase>();
	auto Handle = UEnhancedAsyncActionContextLibrary::CreateContextForObject(Owner, NAME_None);
	UTEST_TRUE_EXPR(Handle.IsValid());
	UTEST_TRUE_EXPR(Handle.IsExternal());

	using Lib = UEnhancedAsyncActionContextLibrary;
	Lib::Handle_SetValue_Bool(Handle, Test_Prop_Bool, true);
	Lib::Handle_SetValue_Byte(Handle, Test_Prop_Byte, 42);
	Lib::Handle_SetValue_Int32(Handle, Test_Prop_Int32, INT_MAX / 2);
	Lib::Handle_SetValue_Int64(Handle, Test_Prop_Int64, INT_MAX  * 2u);
	Lib::Handle_SetValue_Float(Handle, Test_Prop_Float, UE_PI);
	Lib::Handle_SetValue_Double(Handle, Test_Prop_Double, UE_EULERS_NUMBER);
	Lib::Handle_SetValue_String(Handle, Test_Prop_String, FString("foo"));
	Lib::Handle_SetValue_Name(Handle, Test_Prop_Name, FName("bar"));
	Lib::Handle_SetValue_Text(Handle, Test_Prop_Text, INVTEXT("baz"));

	{ bool V; Lib::Handle_GetValue_Bool(Handle, Test_Prop_Bool, V); UTEST_TRUE_EXPR(V == true); }
	{ uint8 V; Lib::Handle_GetValue_Byte(Handle, Test_Prop_Byte, V); UTEST_TRUE_EXPR(V == 42); }
	{ int32 V; Lib::Handle_GetValue_Int32(Handle, Test_Prop_Int32, V); UTEST_TRUE_EXPR(V == (INT_MAX / 2)); }
	{ int64 V; Lib::Handle_GetValue_Int64(Handle, Test_Prop_Int64, V); UTEST_TRUE_EXPR(V == (INT_MAX  * 2u)); }
	{ float V; Lib::Handle_GetValue_Float(Handle, Test_Prop_Float, V); UTEST_TRUE_EXPR(V == UE_PI); }
	{ double V; Lib::Handle_GetValue_Double(Handle, Test_Prop_Double, V); UTEST_TRUE_EXPR(V == UE_EULERS_NUMBER); }
	{ FString V; Lib::Handle_GetValue_String(Handle, Test_Prop_String, V); UTEST_TRUE_EXPR(V == FString("foo")); }
	{ FName V; Lib::Handle_GetValue_Name(Handle, Test_Prop_Name, V); UTEST_TRUE_EXPR(V == FName("bar")); }
	{ FText V; Lib::Handle_GetValue_Text(Handle, Test_Prop_Text, V); UTEST_TRUE_EXPR(V.ToString() == FString("baz")); }
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAALibraryOpsComplex,
	"EnhancedAsyncAction.Context.Complex", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::HighPriority);

bool FEAALibraryOpsComplex::RunTest(FString const&)
{
	auto* Owner = NewObject<UBlueprintAsyncActionBase>();
	auto Handle = UEnhancedAsyncActionContextLibrary::CreateContextForObject(Owner, NAME_None);
	UTEST_TRUE_EXPR(Handle.IsValid());
	UTEST_TRUE_EXPR(Handle.IsExternal());

	using Lib = UEnhancedAsyncActionContextLibrary;

	TSharedPtr<FEnhancedAsyncActionContext> Context = FEnhancedAsyncActionManager::Get().FindContext(Handle);
	UTEST_TRUE_EXPR(Context.IsValid());
	
	{
		const FEAACaptureContext StructValue { TEXT("foo"), 42, Owner };
		UScriptStruct* const StructType = StaticStruct<FEAACaptureContext>();
		
		Context->SetValueStruct(Test_Prop_Struct,
		                        StructType,
		                        reinterpret_cast<const uint8*>(&StructValue));

		const uint8* Output = nullptr;
		Context->GetValueStruct(Test_Prop_Struct, StructType,Output );
		UTEST_TRUE_EXPR(Output != nullptr);

		FEAACaptureContext OutputStruct;
		StructType->CopyScriptStruct(&OutputStruct, Output);
		
		UTEST_TRUE_EXPR(StructType->CompareScriptStruct(&StructValue, &OutputStruct, PPF_None) == true);
	}

	{
		const EEAAPayloadMode EnumValue = EEAAPayloadMode::TIMER;
		UEnum* const EnumType = StaticEnum<EEAAPayloadMode>();

		Context->SetValueEnum(Test_Prop_Enum, EnumType, static_cast<uint8>(EnumValue));

		uint8 Output;
		Context->GetValueEnum(Test_Prop_Enum, EnumType, Output);
		UTEST_TRUE_EXPR( EnumValue == static_cast<EEAAPayloadMode>(Output) ) ;
	}

	{
		UObject* const  Object = Owner;
		UClass* const ObjectType = UObject::StaticClass();
		
		Context->SetValueObject(Test_Prop_Object, ObjectType, Object);

		UObject* Output;
		Context->GetValueObject(Test_Prop_Object, ObjectType, Output);
		UTEST_TRUE_EXPR( Object == Output ) ;
	}

	{
		UClass* const  Object = Owner->GetClass();
		UClass* const MetaType = UBlueprintAsyncActionBase::StaticClass();

		Context->SetValueClass(Test_Prop_Class, MetaType, Object);

		UClass* Output;
		Context->GetValueClass(Test_Prop_Class, MetaType, Output);
		UTEST_TRUE_EXPR( Object == Output ) ;
	}

	return true;
}

#endif
