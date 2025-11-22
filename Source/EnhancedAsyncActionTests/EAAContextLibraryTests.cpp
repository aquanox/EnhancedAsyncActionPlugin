// Copyright 2025, Aquanox.

#include "EAAContextLibraryTests.h"
#include "CoreMinimal.h"
#include "EAATestsShared.h"
#include "EnhancedAsyncContextLibrary.h"
#include "EnhancedAsyncContext.h"
#include "EnhancedAsyncActionHandle.h"
#include "StructUtils/InstancedStruct.h"
#include "StructUtils/PropertyBag.h"
#include "EnhancedAsyncContextManager.h"
#include "EAADemoAsyncAction.h"
#include "EnhancedLatentActionHandle.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/AutomationTest.h"
#include "Misc/AssertionMacros.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAALibraryTestCreateAsync,
	"EnhancedAsyncAction.Context.CreateAsync",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::HighPriority);

bool FEAALibraryTestCreateAsync::RunTest(FString const&)
{
	FTestWorldScope Scope;

	auto* World = Scope.World;

	auto* Task = UEAADemoAsyncActionCapture::StartActionWithCapture(World, false, 42);
	auto* TaskWithContainer = UEAADemoAsyncActionCaptureMember::StartActionWithCaptureFixed(World, false, 42);
	auto* TaskGeneric = NewObject<UBlueprintAsyncActionBase>();

	{
		AddExpectedError(TEXT("CreateContextForObject failed"));
		auto Handle = UEnhancedAsyncContextLibrary::CreateContextForObject(nullptr, NAME_None);
		XTEST_FALSE_EXPR(Handle.IsValid());
		XTEST_FALSE_EXPR(Handle.GetContext().IsValid());
	}

	{
		auto Handle = UEnhancedAsyncContextLibrary::CreateContextForObject(Task, NAME_None);
		XTEST_TRUE_EXPR(Handle.IsValid());
		XTEST_TRUE_EXPR(Handle.GetContext().IsValid());

		auto ResolvedHandle = UEnhancedAsyncContextLibrary::GetContextForObject(Task);
		XTEST_TRUE_EXPR(Handle.GetId() == ResolvedHandle.GetId());
	}

	{
		auto Handle = UEnhancedAsyncContextLibrary::CreateContextForObject(TaskWithContainer, TEXT("ContextData"));
		XTEST_TRUE_EXPR(Handle.IsValid());
		XTEST_TRUE_EXPR(Handle.GetContext().IsValid());

		auto ResolvedHandle = UEnhancedAsyncContextLibrary::GetContextForObject(TaskWithContainer);
		XTEST_TRUE_EXPR(Handle.GetId() == ResolvedHandle.GetId());
	}

	{
		auto Handle = UEnhancedAsyncContextLibrary::CreateContextForObject(TaskGeneric, TEXT("BAD"));
		XTEST_TRUE_EXPR(Handle.IsValid());
		XTEST_TRUE_EXPR(Handle.GetContext().IsValid());

		auto ResolvedHandle = UEnhancedAsyncContextLibrary::GetContextForObject(TaskGeneric);
		XTEST_TRUE_EXPR(Handle.GetId() == ResolvedHandle.GetId());
	}

	{
		auto Handle = UEnhancedAsyncContextLibrary::CreateContextForObject(World);
		XTEST_TRUE_EXPR(Handle.IsValid());
		XTEST_TRUE_EXPR(Handle.GetContext().IsValid());

		auto ResolvedHandle = UEnhancedAsyncContextLibrary::GetContextForObject(World);
		XTEST_TRUE_EXPR(Handle.GetId() == ResolvedHandle.GetId());
	}

	return true;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAALibraryTestCreateLatent,
	"EnhancedAsyncAction.Context.CreateLatent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::HighPriority);

bool FEAALibraryTestCreateLatent::RunTest(FString const&)
{
	FTestWorldScope Scope;

	auto* World = Scope.World;

	{
		auto Handle = UEnhancedAsyncContextLibrary::CreateContextForLatent(World, 42, 142, true, FEnhancedLatentActionDelegate());
		XTEST_TRUE_EXPR(Handle.IsValid());
		XTEST_TRUE_EXPR(Handle.GetContext().IsValid());

		auto ResolvedHandle = UEnhancedAsyncContextLibrary::GetContextForLatent(World, 42, 142);
		XTEST_TRUE_EXPR(Handle.GetId() == ResolvedHandle.GetId());

		UEnhancedAsyncContextLibrary::DestroyContextForLatent(Handle);

		auto ReResolvedHandle = UEnhancedAsyncContextLibrary::GetContextForLatent(World, 42, 142);
		XTEST_FALSE_EXPR(ReResolvedHandle.GetContext().IsValid());
	}

	{
		auto Handle = UEnhancedAsyncContextLibrary::CreateContextForLatent(World, 142, 1142, false, FEnhancedLatentActionDelegate());
		XTEST_FALSE_EXPR(Handle.IsValid());
		XTEST_FALSE_EXPR(Handle.GetContext().IsValid());

		auto ResolvedHandle = UEnhancedAsyncContextLibrary::GetContextForLatent(World, 142, 1142);
		XTEST_TRUE_EXPR(Handle.GetId() == ResolvedHandle.GetId());
	}

	return true;
}

#define PropertyIndexOf(InType) (static_cast<int32>(EPropertyBagPropertyType::InType))
#define PropertyIndexOfContainer(InContainer, InType) (static_cast<int32>(EPropertyBagPropertyType::InType) | (static_cast<int32>(EPropertyBagContainerType::InContainer) << 16))

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAALibraryOpsBasic,
	"EnhancedAsyncAction.Context.Basic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::HighPriority);

bool FEAALibraryOpsBasic::RunTest(FString const&)
{
	auto* Owner = NewObject<UBlueprintAsyncActionBase>();
	auto Handle = UEnhancedAsyncContextLibrary::CreateContextForObject(Owner, NAME_None);
	XTEST_TRUE_EXPR(Handle.IsValid());

	using Lib = UEnhancedAsyncContextLibrary;
	Lib::Handle_SetValue_Bool(Handle, PropertyIndexOf(Bool), true);
	Lib::Handle_SetValue_Byte(Handle, PropertyIndexOf(Byte), 42);
	Lib::Handle_SetValue_Int32(Handle, PropertyIndexOf(Int32), INT_MAX / 2);
	Lib::Handle_SetValue_Int64(Handle, PropertyIndexOf(Int64), INT_MAX  * 2u);
	Lib::Handle_SetValue_Float(Handle, PropertyIndexOf(Float), UE_PI);
	Lib::Handle_SetValue_Double(Handle, PropertyIndexOf(Double), UE_EULERS_NUMBER);
	Lib::Handle_SetValue_String(Handle, PropertyIndexOf(String), FString("foo"));
	Lib::Handle_SetValue_Name(Handle, PropertyIndexOf(Name), FName("bar"));
	Lib::Handle_SetValue_Text(Handle, PropertyIndexOf(Text), INVTEXT("baz"));

	{ bool V; Lib::Handle_GetValue_Bool(Handle, PropertyIndexOf(Bool), V); XTEST_TRUE_EXPR(V == true); }
	{ uint8 V; Lib::Handle_GetValue_Byte(Handle, PropertyIndexOf(Byte), V); XTEST_TRUE_EXPR(V == 42); }
	{ int32 V; Lib::Handle_GetValue_Int32(Handle, PropertyIndexOf(Int32), V); XTEST_TRUE_EXPR(V == (INT_MAX / 2)); }
	{ int64 V; Lib::Handle_GetValue_Int64(Handle, PropertyIndexOf(Int64), V); XTEST_TRUE_EXPR(V == (INT_MAX  * 2u)); }
	{ float V; Lib::Handle_GetValue_Float(Handle, PropertyIndexOf(Float), V); XTEST_TRUE_EXPR(V == UE_PI); }
	{ double V; Lib::Handle_GetValue_Double(Handle, PropertyIndexOf(Double), V); XTEST_TRUE_EXPR(V == UE_EULERS_NUMBER); }
	{ FString V; Lib::Handle_GetValue_String(Handle, PropertyIndexOf(String), V); XTEST_TRUE_EXPR(V == FString("foo")); }
	{ FName V; Lib::Handle_GetValue_Name(Handle, PropertyIndexOf(Name), V); XTEST_TRUE_EXPR(V == FName("bar")); }
	{ FText V; Lib::Handle_GetValue_Text(Handle, PropertyIndexOf(Text), V); XTEST_TRUE_EXPR(V.ToString() == FString("baz")); }

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEAALibraryOpsComplex,
	"EnhancedAsyncAction.Context.Complex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::HighPriority);

bool FEAALibraryOpsComplex::RunTest(FString const&)
{
	auto* Owner = NewObject<UBlueprintAsyncActionBase>();
	auto Handle = UEnhancedAsyncContextLibrary::CreateContextForObject(Owner, NAME_None);
	XTEST_TRUE_EXPR(Handle.IsValid());

	using Lib = UEnhancedAsyncContextLibrary;

	TSharedPtr<FEnhancedAsyncActionContext> Context = FEnhancedAsyncContextManager::Get().FindContext(Handle);
	XTEST_TRUE_EXPR(Context.IsValid());

	{
		const FEAACaptureContext StructValue { TEXT("foo"), 42, Owner };
		UScriptStruct* const StructType = StaticStruct<FEAACaptureContext>();

		Context->SetValueStruct(PropertyIndexOf(Struct),
		                        StructType,
		                        reinterpret_cast<const uint8*>(&StructValue));

		const uint8* Output = nullptr;
		Context->GetValueStruct(PropertyIndexOf(Struct), StructType,Output );
		XTEST_TRUE_EXPR(Output != nullptr);

		FEAACaptureContext OutputStruct;
		StructType->CopyScriptStruct(&OutputStruct, Output);

		XTEST_TRUE_EXPR(StructType->CompareScriptStruct(&StructValue, &OutputStruct, PPF_None) == true);
	}

	{
		const EEAAPayloadMode EnumValue = EEAAPayloadMode::TIMER;
		UEnum* const EnumType = StaticEnum<EEAAPayloadMode>();

		Context->SetValueEnum(PropertyIndexOf(Enum), EnumType, static_cast<uint8>(EnumValue));

		uint8 Output;
		Context->GetValueEnum(PropertyIndexOf(Enum), EnumType, Output);
		XTEST_TRUE_EXPR( EnumValue == static_cast<EEAAPayloadMode>(Output) ) ;
	}

	{
		UObject* const  Object = Owner;
		UClass* const ObjectType = UObject::StaticClass();

		Context->SetValueObject(PropertyIndexOf(Object), ObjectType, Object);

		UObject* Output;
		Context->GetValueObject(PropertyIndexOf(Object), ObjectType, Output);
		XTEST_TRUE_EXPR( Object == Output ) ;
	}

	{
		UClass* const  Object = Owner->GetClass();
		UClass* const MetaType = UBlueprintAsyncActionBase::StaticClass();

		Context->SetValueClass(PropertyIndexOf(Class), MetaType, Object);

		UClass* Output;
		Context->GetValueClass(PropertyIndexOf(Class), MetaType, Output);
		XTEST_TRUE_EXPR( Object == Output ) ;
	}

	return true;
}
