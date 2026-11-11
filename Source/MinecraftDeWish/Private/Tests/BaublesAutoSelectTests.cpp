#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "BaublesSystem.h"
#include "ToolTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBaublesAutoSelectImmediateCommitTest,
	"MinecraftDeWish.Baubles.AutoSelectCommitsWhenNoWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBaublesAutoSelectImmediateCommitTest::RunTest(const FString& Parameters)
{
	UBaublesSystem* Baubles = NewObject<UBaublesSystem>(GetTransientPackage());
	TestNotNull(TEXT("baubles created"), Baubles);
	if (!Baubles)
	{
		return false;
	}

	TestEqual(TEXT("initial active tool is None"), Baubles->GetActiveToolType(), EToolType::None);

	Baubles->RequestAutoToolSelection(EToolType::Pickaxe);
	TestEqual(TEXT("request commits immediately without world"), Baubles->GetActiveToolType(), EToolType::Pickaxe);

	// Requesting the same candidate again should not change state.
	Baubles->RequestAutoToolSelection(EToolType::Pickaxe);
	TestEqual(TEXT("same candidate keeps active tool"), Baubles->GetActiveToolType(), EToolType::Pickaxe);

	// A different candidate becomes pending (no world -> immediate commit).
	Baubles->RequestAutoToolSelection(EToolType::Shovel);
	TestEqual(TEXT("new candidate commits"), Baubles->GetActiveToolType(), EToolType::Shovel);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBaublesAutoSelectCancelTest,
	"MinecraftDeWish.Baubles.AutoSelectCancelClearsPending",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBaublesAutoSelectCancelTest::RunTest(const FString& Parameters)
{
	UBaublesSystem* Baubles = NewObject<UBaublesSystem>(GetTransientPackage());
	TestNotNull(TEXT("baubles created"), Baubles);
	if (!Baubles)
	{
		return false;
	}

	Baubles->RequestAutoToolSelection(EToolType::Axe);
	// Without world it already committed; cancel should still be safe.
	Baubles->CancelAutoToolSelection();
	TestEqual(TEXT("cancel does not revert committed tool"), Baubles->GetActiveToolType(), EToolType::Axe);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBaublesAutoSelectSmokeTest,
	"MinecraftDeWish.Baubles.AutoSelect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBaublesAutoSelectSmokeTest::RunTest(const FString& Parameters)
{
	UBaublesSystem* Baubles = NewObject<UBaublesSystem>(GetTransientPackage());
	TestNotNull(TEXT("baubles created"), Baubles);
	if (!Baubles)
	{
		return false;
	}

	TestEqual(TEXT("initial active tool is None"), Baubles->GetActiveToolType(), EToolType::None);

	Baubles->RequestAutoToolSelection(EToolType::Pickaxe);
	TestEqual(TEXT("request commits immediately without world"), Baubles->GetActiveToolType(), EToolType::Pickaxe);

	Baubles->CancelAutoToolSelection();
	TestEqual(TEXT("cancel keeps committed tool"), Baubles->GetActiveToolType(), EToolType::Pickaxe);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS