#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "BlockCrackActor.h"
#include "WorldGenerator.h"

/**
 * Pins down the vanilla-equivalent cracking progression: mining progress (0..1) maps onto
 * destroy_stage_0..9. A regression here would silently shift every crack stage in-game.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBlockCrackStageProgressionTest,
	"MinecraftDeWish.Mining.CrackStageFromProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBlockCrackStageProgressionTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("0.0 progress shows stage 0 (faint cracks, first hit)"), ABlockCrackActor::GetStageFromProgress(0.0f), 0);
	TestEqual(TEXT("0.099 stays stage 0 until the first full tenth"), ABlockCrackActor::GetStageFromProgress(0.099f), 0);
	TestEqual(TEXT("0.1 advances to stage 1"), ABlockCrackActor::GetStageFromProgress(0.1f), 1);
	TestEqual(TEXT("0.25 advances to stage 2"), ABlockCrackActor::GetStageFromProgress(0.25f), 2);
	TestEqual(TEXT("0.5 is mid-break stage 5"), ABlockCrackActor::GetStageFromProgress(0.5f), 5);
	TestEqual(TEXT("0.9 advances to stage 9"), ABlockCrackActor::GetStageFromProgress(0.9f), 9);
	TestEqual(TEXT("0.99 is still stage 9"), ABlockCrackActor::GetStageFromProgress(0.99f), 9);
	TestEqual(TEXT("exactly 1.0 progress clamps to stage 9, never stage 10"), ABlockCrackActor::GetStageFromProgress(1.0f), 9);
	TestEqual(TEXT("progress above 1.0 clamps to stage 9"), ABlockCrackActor::GetStageFromProgress(1.5f), 9);
	TestEqual(TEXT("negative progress clamps to stage 0"), ABlockCrackActor::GetStageFromProgress(-0.5f), 0);

	return true;
}

/**
 * Regression guard for overlay stacking: overlays are keyed per voxel, so mining voxel A then
 * voxel B without releasing the input used to leave A's overlay alive until ClearCrackMining ran.
 * The rule is now that only the actively mined voxel keeps an overlay.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBlockCrackSingleActiveOverlayTest,
	"MinecraftDeWish.Mining.KeepsOverlayOnlyOnActiveVoxel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBlockCrackSingleActiveOverlayTest::RunTest(const FString& Parameters)
{
	const FIntVector Active(10, -4, 27);

	TestTrue(TEXT("the overlay of the mined voxel survives its own update"),
		AWorldGenerator::ShouldKeepCrackOverlay(Active, Active));

	TestFalse(TEXT("an overlay left on the previously mined voxel is destroyed"),
		AWorldGenerator::ShouldKeepCrackOverlay(FIntVector(10, -4, 26), Active));

	TestFalse(TEXT("a horizontally adjacent overlay is not kept"),
		AWorldGenerator::ShouldKeepCrackOverlay(FIntVector(11, -4, 27), Active));

	TestTrue(TEXT("negative coordinates still compare exactly"),
		AWorldGenerator::ShouldKeepCrackOverlay(FIntVector(-30, -28, 28), FIntVector(-30, -28, 28)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
