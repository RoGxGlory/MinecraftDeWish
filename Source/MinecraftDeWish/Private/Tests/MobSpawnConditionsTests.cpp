#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "MobSpawnerSystem.h"

/**
 * Verifies mob spawn-condition defaults and the graceful (no-world) behaviour of
 * the spawn-condition queries.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMobSpawnConditionsTest,
	"MinecraftDeWish.Mob.SpawnConditions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMobSpawnConditionsTest::RunTest(const FString& Parameters)
{
	UMobSpawnerSystem* Spawner = NewObject<UMobSpawnerSystem>(GetTransientPackage());
	TestNotNull(TEXT("mob spawner created"), Spawner);
	if (!Spawner)
	{
		return false;
	}

	TestEqual(TEXT("default max mob cap is 20"), Spawner->MaxMobCap, 20);
	TestEqual(TEXT("default torch light radius is 5 blocks"), Spawner->TorchLightRadius, 5);
	TestEqual(TEXT("default min spawn distance is 24 blocks"), Spawner->MinSpawnDistance, 2400.0f);
	TestEqual(TEXT("default max spawn distance is 128 blocks"), Spawner->MaxSpawnDistance, 12800.0f);
	TestEqual(TEXT("default spawn interval is 2 seconds"), Spawner->SpawnInterval, 2.0f);
	TestTrue(TEXT("spawning enabled by default"), Spawner->bSpawningEnabled);

	// Without a world generator, spawn surfaces are invalid and no torch is found.
	TestFalse(TEXT("no valid spawn surface without world"), Spawner->IsValidSpawnSurface(0, 0, 0));
	TestFalse(TEXT("no nearby torch without world"), Spawner->HasNearbyTorch(0, 0, 0));
	// Light level defaults to full daylight when the world is unavailable.
	TestEqual(TEXT("light level defaults to 15 without world"), Spawner->CalculateLightLevel(FIntVector(0, 0, 0)), 15);

	// Mob cap boundary: a freshly created spawner tracks zero active mobs.
	TestEqual(TEXT("no active mobs initially"), Spawner->GetActiveMobCount(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS