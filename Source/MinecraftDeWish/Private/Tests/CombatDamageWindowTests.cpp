#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ToolAnimNotifies.h"

/**
 * Smoke-tests the combat damage window notify: default property value and null-safety.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatDamageWindowTest,
	"MinecraftDeWish.Combat.DamageWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDamageWindowTest::RunTest(const FString& Parameters)
{
	UAnimNotify_ActionWindow* DamageWindow = NewObject<UAnimNotify_ActionWindow>(GetTransientPackage());
	TestNotNull(TEXT("damage window notify created"), DamageWindow);
	if (!DamageWindow)
	{
		return false;
	}

	TestEqual(TEXT("default action tag is None"), DamageWindow->ActionTag, NAME_None);

	// Assigning a tag round-trips.
	DamageWindow->ActionTag = FName(TEXT("Melee.Impact"));
	TestEqual(TEXT("action tag assignable"), DamageWindow->ActionTag, FName(TEXT("Melee.Impact")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS