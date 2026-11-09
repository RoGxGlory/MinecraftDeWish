#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ChunkActor.h"
#include "VoxelDataTypes.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

/**
 * ADR-0002 (Docs/architecture/adr-0002-generated-vs-player-voxel-writes.md) splits voxel
 * writes in two: world generation must not mark a chunk as modified (otherwise every visited
 * chunk is saved), player edits must. These are the guard rails for that split.
 */
namespace VoxelWritePolicyTest
{
	/** A throwaway game world, destroyed on scope exit, so tests never leak actors into each other. */
	struct FScopedTestWorld
	{
		FScopedTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/false, TEXT("VoxelWritePolicyTest"));
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

		/** Spawn an unmeshed chunk with a zeroed block array. */
		AChunkActor* SpawnChunk(const FChunkCoord& Coord = FChunkCoord(0, 0))
		{
			AChunkActor* Chunk = World->SpawnActor<AChunkActor>();
			if (Chunk)
			{
				Chunk->InitializeChunk(Coord, DEFAULT_CHUNK_SIZE_Z, DEFAULT_BLOCK_SCALE, nullptr);
			}
			return Chunk;
		}

		UWorld* World = nullptr;
		FWorldContext* WorldContext = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVoxelGeneratedWriteDoesNotDirtyChunkTest,
	"MinecraftDeWish.Voxel.GeneratedWriteDoesNotDirtyChunk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVoxelGeneratedWriteDoesNotDirtyChunkTest::RunTest(const FString& Parameters)
{
	VoxelWritePolicyTest::FScopedTestWorld Scoped;
	AChunkActor* Chunk = Scoped.SpawnChunk();
	TestNotNull(TEXT("chunk actor was spawned"), Chunk);
	if (!Chunk)
	{
		return false;
	}

	TestFalse(TEXT("a freshly initialized chunk is not flagged as modified"), Chunk->bIsModified);

	Chunk->SetBlockGenerated(3, 4, 5, /*Stone*/ 4);
	TestEqual(TEXT("generated write stores the block"), static_cast<int32>(Chunk->GetBlock(3, 4, 5)), 4);
	TestFalse(TEXT("generated write leaves the chunk unmodified"), Chunk->bIsModified);

	Chunk->SetBlockWithMetaGenerated(6, 7, 8, /*Planks*/ 6, /*Rotation*/ 2);
	TestEqual(TEXT("generated meta write stores the block"), static_cast<int32>(Chunk->GetBlock(6, 7, 8)), 6);
	TestFalse(TEXT("generated meta write leaves the chunk unmodified"), Chunk->bIsModified);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVoxelPlayerEditDirtiesChunkTest,
	"MinecraftDeWish.Voxel.PlayerEditDirtiesChunk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVoxelPlayerEditDirtiesChunkTest::RunTest(const FString& Parameters)
{
	VoxelWritePolicyTest::FScopedTestWorld Scoped;

	AChunkActor* Chunk = Scoped.SpawnChunk(FChunkCoord(0, 0));
	TestNotNull(TEXT("chunk actor was spawned"), Chunk);
	if (!Chunk)
	{
		return false;
	}

	TestFalse(TEXT("a freshly initialized chunk is not flagged as modified"), Chunk->bIsModified);

	Chunk->SetBlock(0, 0, 0, /*Stone*/ 4);
	TestTrue(TEXT("a player edit flags the chunk as modified"), Chunk->bIsModified);

	AChunkActor* MetaChunk = Scoped.SpawnChunk(FChunkCoord(1, 0));
	TestNotNull(TEXT("second chunk actor was spawned"), MetaChunk);
	if (!MetaChunk)
	{
		return false;
	}

	MetaChunk->SetBlockWithMeta(1, 2, 3, /*Planks*/ 6, /*Rotation*/ 1);
	TestTrue(TEXT("a player edit with metadata flags the chunk as modified"), MetaChunk->bIsModified);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVoxelWritePolicyEdgeCasesTest,
	"MinecraftDeWish.Voxel.WritePolicyEdgeCases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVoxelWritePolicyEdgeCasesTest::RunTest(const FString& Parameters)
{
	VoxelWritePolicyTest::FScopedTestWorld Scoped;
	AChunkActor* Chunk = Scoped.SpawnChunk();
	TestNotNull(TEXT("chunk actor was spawned"), Chunk);
	if (!Chunk)
	{
		return false;
	}

	// Out-of-range writes must be dropped without side effects, in both policies.
	Chunk->SetBlock(-1, 0, 0, 4);
	Chunk->SetBlock(0, CHUNK_SIZE_Y, 0, 4);
	Chunk->SetBlockGenerated(0, -1, 0, 4);
	Chunk->SetBlockGenerated(0, 0, Chunk->ChunkHeight, 4);
	TestFalse(TEXT("out-of-range writes never flag the chunk as modified"), Chunk->bIsModified);

	// Removing a player block is still a player edit: the chunk must stay saved.
	Chunk->SetBlock(0, 0, 0, 4);
	TestTrue(TEXT("placing a block flags the chunk"), Chunk->bIsModified);

	Chunk->SetBlock(0, 0, 0, /*Air*/ 0);
	TestTrue(TEXT("removing a block still leaves the chunk flagged"), Chunk->bIsModified);

	// A generated write must never clear a dirty flag set by the player.
	Chunk->SetBlockGenerated(0, 0, 0, /*Dirt*/ 1);
	TestTrue(TEXT("a generated write does not clear an existing modification"), Chunk->bIsModified);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
