#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "WorldGenerator.h"

/**
 * Pins the chunk generation time budget defaults and range so a regression in
 * the streaming performance knobs is caught.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVoxelChunkGenerationBudgetTest,
	"MinecraftDeWish.Voxel.ChunkGenerationBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVoxelChunkGenerationBudgetTest::RunTest(const FString& Parameters)
{
	AWorldGenerator* WorldGenerator = NewObject<AWorldGenerator>(GetTransientPackage());
	TestNotNull(TEXT("world generator created"), WorldGenerator);
	if (!WorldGenerator)
	{
		return false;
	}

	TestEqual(TEXT("default generation time budget is 4.0 ms"), WorldGenerator->MaxGenerationTimeMs, 4.0f);
	TestEqual(TEXT("default max chunk generations per frame is 2"), WorldGenerator->MaxChunkGenerationsPerFrame, 2);

	// Budget must stay within the documented [0.5, 16.0] ms clamp range.
	TestTrue(TEXT("default budget within clamp range"),
		WorldGenerator->MaxGenerationTimeMs >= 0.5f && WorldGenerator->MaxGenerationTimeMs <= 16.0f);

	// Mutating the budget keeps the value (no hidden clamping at assignment).
	WorldGenerator->MaxGenerationTimeMs = 8.0f;
	TestEqual(TEXT("budget assignable"), WorldGenerator->MaxGenerationTimeMs, 8.0f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS