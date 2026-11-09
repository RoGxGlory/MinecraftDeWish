#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "WorldGenerator.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"

/**
 * The foliage wind is one global multiplier (MPC_World's WindStrength scalar), so the only
 * rule worth testing is the one the player setting relies on: clamp to the supported range,
 * and persist whatever was actually applied. A value outside the range must never reach the
 * material, because a negative multiplier would push the leaves against the wind direction
 * and an unbounded one would tear the canopy apart.
 */
namespace WindSettingTest
{
	/** Mirrors the section/key AWorldGenerator persists the wind strength under. */
	const TCHAR* const SettingSection = TEXT("MinecraftDeWish.GameSettings");
	const TCHAR* const SettingKey = TEXT("WindStrength");

	/** A throwaway game world, destroyed on scope exit, so tests never leak actors into each other. */
	struct FScopedTestWorld
	{
		FScopedTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/false, TEXT("WindSettingTest"));
			WorldContext = &GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext->SetCurrentWorld(World);
		}

		~FScopedTestWorld()
		{
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(/*bInformEngineOfWorld=*/false);
				World = nullptr;
			}
		}

		FScopedTestWorld(const FScopedTestWorld&) = delete;
		FScopedTestWorld& operator=(const FScopedTestWorld&) = delete;

		AWorldGenerator* SpawnGenerator()
		{
			return World ? World->SpawnActor<AWorldGenerator>() : nullptr;
		}

		UWorld* World = nullptr;
		FWorldContext* WorldContext = nullptr;
	};

	/**
	 * Puts the developer's own GameUserSettings.ini value back when the test ends. The setter
	 * writes to the real file, so a test that left its value behind would silently change how
	 * the next editor session renders leaves.
	 */
	struct FScopedSettingRestore
	{
		FScopedSettingRestore()
		{
			bHadValue = GConfig && GConfig->GetFloat(SettingSection, SettingKey, SavedValue, GGameUserSettingsIni);
		}

		~FScopedSettingRestore()
		{
			if (!GConfig)
			{
				return;
			}

			if (bHadValue)
			{
				GConfig->SetFloat(SettingSection, SettingKey, SavedValue, GGameUserSettingsIni);
			}
			else
			{
				GConfig->RemoveKey(SettingSection, SettingKey, GGameUserSettingsIni);
			}
			GConfig->Flush(false, GGameUserSettingsIni);
		}

		float SavedValue = 0.0f;
		bool bHadValue = false;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWindStrengthClampTest,
	"MinecraftDeWish.WorldGenerator.WindStrengthClampsToRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWindStrengthClampTest::RunTest(const FString& Parameters)
{
	WindSettingTest::FScopedSettingRestore Restore;
	WindSettingTest::FScopedTestWorld Scoped;

	AWorldGenerator* Generator = Scoped.SpawnGenerator();
	TestNotNull(TEXT("world generator was spawned"), Generator);
	if (!Generator)
	{
		return false;
	}

	TestEqual(TEXT("a fresh generator uses the shipped default"),
		Generator->GetWindStrength(), AWorldGenerator::DefaultWindStrength, 0.001f);

	TestEqual(TEXT("a value above the range is clamped to the maximum"),
		Generator->SetWindStrength(5.0f), AWorldGenerator::MaxWindStrength, 0.001f);
	TestEqual(TEXT("the clamped value is the one kept"),
		Generator->GetWindStrength(), AWorldGenerator::MaxWindStrength, 0.001f);

	TestEqual(TEXT("a negative value is clamped to the minimum"),
		Generator->SetWindStrength(-3.0f), AWorldGenerator::MinWindStrength, 0.001f);
	TestEqual(TEXT("zero is the minimum, not an error"),
		Generator->SetWindStrength(0.0f), AWorldGenerator::MinWindStrength, 0.001f);

	TestEqual(TEXT("an in-range value is kept as asked"),
		Generator->SetWindStrength(1.5f), 1.5f, 0.001f);

	float PersistedValue = -1.0f;
	const bool bRead = GConfig
		&& GConfig->GetFloat(WindSettingTest::SettingSection, WindSettingTest::SettingKey, PersistedValue, GGameUserSettingsIni);
	TestTrue(TEXT("the applied value reached GameUserSettings.ini"), bRead);
	TestEqual(TEXT("the persisted value matches the applied one"), PersistedValue, 1.5f, 0.001f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
