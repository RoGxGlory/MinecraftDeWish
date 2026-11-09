#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "MiningQueueSystem.h"

/**
 * Regression guard for the "blocks are no longer breakable" bug: QueueBlock used to reject
 * any voxel whose X or Y was negative, but the world is a signed coordinate space and the
 * player spawns at roughly (-4800, -3200). Every block around the spawn was therefore
 * silently unminable. These tests pin the rule down: only the vertical axis is bounded
 * below, and air is never a target.
 */
namespace MiningQueueTest
{
	/** A fresh queue at the default tool tier (Hand: 1 slot), so capacity never hides the rule. */
	UMiningQueueSystem* NewQueue()
	{
		return NewObject<UMiningQueueSystem>(GetTransientPackage());
	}

	constexpr uint8 StoneBlockID = 1;
	constexpr uint8 AirBlockID = 0;
	constexpr float TestDurability = 1.0f;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMiningQueueNegativeCoordinatesTest,
	"MinecraftDeWish.Mining.AcceptsNegativeHorizontalCoordinates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMiningQueueNegativeCoordinatesTest::RunTest(const FString& Parameters)
{
	UMiningQueueSystem* Queue = MiningQueueTest::NewQueue();
	TestNotNull(TEXT("mining queue was created"), Queue);
	if (!Queue)
	{
		return false;
	}

	TestEqual(TEXT("hand tier starts with a single queue slot"), Queue->GetMaxQueueSlots(), 1);

	// The exact voxel the PIE diagnosis reported as unminable: a leaf at the spawn area.
	const FIntVector NegativeVoxel(-30, -28, 28);
	TestTrue(TEXT("a voxel with negative X and Y is queued"),
		Queue->QueueBlock(NegativeVoxel, MiningQueueTest::StoneBlockID, MiningQueueTest::TestDurability));
	TestEqual(TEXT("the negative voxel is actually in the queue"), Queue->GetQueueCount(), 1);

	const TArray<FMiningTask> Tasks = Queue->GetActiveMiningTasks();
	TestTrue(TEXT("the queued task is readable"), Tasks.Num() == 1);
	if (Tasks.Num() == 1)
	{
		TestTrue(TEXT("the queued task kept the signed coordinates"), Tasks[0].VoxelCoord == NegativeVoxel);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMiningQueueInvalidTargetsTest,
	"MinecraftDeWish.Mining.RejectsBelowWorldAndAir",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMiningQueueInvalidTargetsTest::RunTest(const FString& Parameters)
{
	UMiningQueueSystem* Queue = MiningQueueTest::NewQueue();
	TestNotNull(TEXT("mining queue was created"), Queue);
	if (!Queue)
	{
		return false;
	}

	TestFalse(TEXT("a voxel below the world floor is rejected"),
		Queue->QueueBlock(FIntVector(4, 4, -1), MiningQueueTest::StoneBlockID, MiningQueueTest::TestDurability));
	TestEqual(TEXT("a rejected floor voxel does not enter the queue"), Queue->GetQueueCount(), 0);

	TestFalse(TEXT("air is rejected"),
		Queue->QueueBlock(FIntVector(4, 4, 10), MiningQueueTest::AirBlockID, MiningQueueTest::TestDurability));
	TestEqual(TEXT("air does not enter the queue"), Queue->GetQueueCount(), 0);

	TestTrue(TEXT("a normal voxel still queues"),
		Queue->QueueBlock(FIntVector(4, 4, 10), MiningQueueTest::StoneBlockID, MiningQueueTest::TestDurability));
	TestFalse(TEXT("the same voxel cannot be queued twice"),
		Queue->QueueBlock(FIntVector(4, 4, 10), MiningQueueTest::StoneBlockID, MiningQueueTest::TestDurability));
	TestEqual(TEXT("the duplicate did not grow the queue"), Queue->GetQueueCount(), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
