#include "WorldGenerator.h"
#include "ChunkActor.h"
#include "BlockItemPickup.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"

AWorldGenerator::AWorldGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f; // Tick 20 times per second for streaming efficiency
}

void AWorldGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (!TerrainMaterial)
	{
		TerrainMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Game/Materials/M_Global.M_Global")));
	}
	if (!BlockDataTable)
	{
		BlockDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/Data/Block_DataTable.Block_DataTable")));
	}

	Noise.SetSeed(WorldSeed);
	InitializeBlockCache();

	WorldSavePath = FPaths::ProjectSavedDir() / TEXT("SaveGames") / FString::Printf(TEXT("World_%d"), WorldSeed);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// Purge stale saves from prior terrain generation parameters to avoid corrupt chunks
	if (PlatformFile.DirectoryExists(*WorldSavePath))
	{
		PlatformFile.DeleteDirectoryRecursively(*WorldSavePath);
	}
	PlatformFile.CreateDirectoryTree(*WorldSavePath);

	// Initial player positioning check
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->GetPawn())
	{
		CurrentPlayerChunkCoord = WorldLocationToChunkCoord(PC->GetPawn()->GetActorLocation());
		bHasPlayerCoord = true;
	}
	else
	{
		CurrentPlayerChunkCoord = FChunkCoord(0, 0);
		bHasPlayerCoord = true;
	}

	// Pre-generate immediate 5x5 chunk spawn area so player spawns on solid terrain
	for (int32 Dx = -2; Dx <= 2; ++Dx)
	{
		for (int32 Dy = -2; Dy <= 2; ++Dy)
		{
			FChunkCoord Coord(CurrentPlayerChunkCoord.X + Dx, CurrentPlayerChunkCoord.Y + Dy);
			if (!ActiveChunks.Contains(Coord))
			{
				AChunkActor* Chunk = SpawnChunk(Coord);
				if (Chunk)
				{
					ActiveChunks.Add(Coord, Chunk);
					GenerateChunkData(Chunk);
					Chunk->UpdateMesh();
				}
			}
		}
	}

	// Teleport player on top of generated terrain so they don't spawn underground
	if (PC && PC->GetPawn())
	{
		FVector PawnLoc = PC->GetPawn()->GetActorLocation();
		int32 SpawnVoxelX = FMath::FloorToInt(PawnLoc.X / BlockScale);
		int32 SpawnVoxelY = FMath::FloorToInt(PawnLoc.Y / BlockScale);
		int32 SpawnHeight = GetPredictedTerrainHeight(SpawnVoxelX, SpawnVoxelY);
		float SpawnZ = (static_cast<float>(SpawnHeight) + 2.0f) * BlockScale;
		PC->GetPawn()->SetActorLocation(FVector(PawnLoc.X, PawnLoc.Y, SpawnZ));
		UE_LOG(LogTemp, Log, TEXT("WorldGenerator: Teleported player to terrain surface Z=%d (%.0f UU)"), SpawnHeight, SpawnZ);
	}

	UpdateChunkStreaming();
}

void AWorldGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SaveWorld();
	Super::EndPlay(EndPlayReason);
}

void AWorldGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Track player chunk
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->GetPawn())
	{
		FChunkCoord NewCoord = WorldLocationToChunkCoord(PC->GetPawn()->GetActorLocation());
		if (NewCoord != CurrentPlayerChunkCoord || !bHasPlayerCoord)
		{
			CurrentPlayerChunkCoord = NewCoord;
			bHasPlayerCoord = true;
			UpdateChunkStreaming();
		}
	}

	ProcessGenerationQueue();
}

void AWorldGenerator::InitializeBlockCache()
{
	// Default fallback mappings matching standard Minecraft textures
	// BlockID: Top, Bottom, Side, Front, bTransparent
	auto RegisterDefault = [this](uint8 ID, int32 Top, int32 Bottom, int32 Side, int32 Front, bool bTrans = false)
	{
		FBlockTextureData Data;
		Data.TopTexture = Top;
		Data.BottomTexture = Bottom;
		Data.SideTexture = Side;
		Data.FrontTexture = Front;
		Data.bIsTransparent = bTrans;
		BlockTextureCache.Add(ID, Data);
		if (bTrans)
		{
			TransparentBlockIDs.Add(ID);
		}
	};

	TransparentBlockIDs.Add(0); // Air is transparent

	// Default fallback mappings matching the user's Global_Textures_Array
	RegisterDefault(1,  14, 14, 14, 14);       // Dirt
	RegisterDefault(2,  27, 14, 26, 26);       // Grass (Top=27 grass_block_top, Bottom=14 dirt, Side=26 grass_block_side)
	RegisterDefault(3,  6, 6, 6, 6);           // Cobblestone
	RegisterDefault(4,  35, 35, 35, 35);       // Stone (35 stone)
	RegisterDefault(5,  32, 32, 31, 31);       // Wood Log (Top/Bottom=32 oak_log_top, Side=31 oak_log)
	RegisterDefault(6,  33, 33, 33, 33);       // Wood Planks (33 oak_planks)
	RegisterDefault(7,  29, 29, 29, 29);       // Iron Ore (29 iron_ore)
	RegisterDefault(8,  28, 28, 28, 28);       // Iron Block (28 iron_block)
	RegisterDefault(9,  25, 25, 25, 25);       // Gold Ore (25 gold_ore)
	RegisterDefault(10, 24, 24, 24, 24);       // Gold Block (24 gold_block)
	RegisterDefault(11, 13, 13, 13, 13);       // Diamond Ore (13 diamond_ore)
	RegisterDefault(12, 12, 12, 12, 12);       // Diamond Block (12 diamond_block)
	RegisterDefault(13, 16, 16, 16, 16);       // Emerald Ore (16 emerald_ore)
	RegisterDefault(14, 15, 15, 15, 15);       // Emerald Block (15 emerald_block)
	RegisterDefault(15, 9, 33, 8, 7);          // Crafting Table (Top=9, Bottom=33, Side=8, Front=7)
	RegisterDefault(16, 22, 22, 21, 19);       // Furnace (Top=22, Bottom=22, Side=21, Front=19)
	RegisterDefault(17, 30, 30, 30, 30, true); // Leaves (30 leaves, transparent)
	RegisterDefault(18, 34, 34, 34, 34);       // Sand (34 sand)
	RegisterDefault(19, 36, 36, 36, 36, true); // Torch (36 torch, transparent)
	RegisterDefault(20, 2, 0, 1, 1);           // Barrel (Top=2, Bottom=0, Side=1, Front=1)
	RegisterDefault(21, 23, 23, 23, 23, true); // Glass (23 glass, transparent)
	RegisterDefault(22, 33, 33, 33, 33, true); // Fence (33 planks)
	RegisterDefault(23, 33, 33, 33, 33, true); // Fence Door
	RegisterDefault(24, 10, 10, 10, 10, true); // Door (10 dark_oak_door_bottom)
	RegisterDefault(25, 5, 5, 5, 5);           // Coal Ore (5 coal_ore)
	RegisterDefault(26, 4, 4, 4, 4);           // Coal Block (4 coal_block)
	RegisterDefault(27, 6, 6, 6, 6);           // Bedrock fallback -> 6 Cobblestone (NOT coal!)

	// If a DataTable is assigned, read its actual row properties via reflection
	if (BlockDataTable)
	{
		const UScriptStruct* RowStruct = BlockDataTable->GetRowStruct();
		if (RowStruct)
		{
			for (auto It = BlockDataTable->GetRowMap().CreateConstIterator(); It; ++It)
			{
				const uint8* RowData = It.Value();
				if (!RowData) continue;

				int32 BlockID = 0;
				int32 SideTex = 0;
				int32 TopTex = 0;
				int32 FrontTex = 0;
				int32 BottomTex = 0;

				for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
				{
					FProperty* Prop = *PropIt;
					const FString PropName = Prop->GetName();

					if (PropName.Contains(TEXT("BlockID"), ESearchCase::IgnoreCase))
					{
						if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
						{
							BlockID = IntProp->GetPropertyValue_InContainer(RowData);
						}
					}
					else if (PropName.Contains(TEXT("Side"), ESearchCase::IgnoreCase) && PropName.Contains(TEXT("Texture"), ESearchCase::IgnoreCase))
					{
						if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
						{
							SideTex = IntProp->GetPropertyValue_InContainer(RowData);
						}
					}
					else if (PropName.Contains(TEXT("Top"), ESearchCase::IgnoreCase) && PropName.Contains(TEXT("Texture"), ESearchCase::IgnoreCase))
					{
						if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
						{
							TopTex = IntProp->GetPropertyValue_InContainer(RowData);
						}
					}
					else if (PropName.Contains(TEXT("Front"), ESearchCase::IgnoreCase) && !PropName.Contains(TEXT("Active"), ESearchCase::IgnoreCase))
					{
						if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
						{
							FrontTex = IntProp->GetPropertyValue_InContainer(RowData);
						}
					}
					else if (PropName.Contains(TEXT("Bottom"), ESearchCase::IgnoreCase))
					{
						if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
						{
							BottomTex = IntProp->GetPropertyValue_InContainer(RowData);
						}
					}
				}

				// Only override if the row has been customized (not dummy values: Side=0, Top=1, Front=2, Bottom=4)
				const bool bIsDummyRow = (SideTex == 0 && TopTex == 1 && FrontTex == 2 && BottomTex == 4);
				if (BlockID > 0 && !bIsDummyRow)
				{
					FBlockTextureData Data;
					Data.TopTexture = TopTex;
					Data.BottomTexture = BottomTex;
					Data.SideTexture = SideTex;
					Data.FrontTexture = FrontTex;
					Data.bIsTransparent = (BlockID == 17 || BlockID == 21 || BlockID == 19 || BlockID == 26);
					BlockTextureCache.Add(static_cast<uint8>(BlockID), Data);
				}
			}
		}
	}
}

int32 AWorldGenerator::GetTextureForBlock(uint8 BlockID, EBlockFace Face) const
{
	const FBlockTextureData* Found = BlockTextureCache.Find(BlockID);
	if (Found)
	{
		return Found->GetTextureForFace(Face);
	}
	return 0;
}

bool AWorldGenerator::IsBlockTransparent(uint8 BlockID) const
{
	return TransparentBlockIDs.Contains(BlockID);
}

FChunkCoord AWorldGenerator::WorldLocationToChunkCoord(const FVector& WorldLocation) const
{
	const float ChunkWidth = static_cast<float>(CHUNK_SIZE_X) * BlockScale;
	const float ChunkLength = static_cast<float>(CHUNK_SIZE_Y) * BlockScale;

	return FChunkCoord(
		FMath::FloorToInt(WorldLocation.X / ChunkWidth),
		FMath::FloorToInt(WorldLocation.Y / ChunkLength)
	);
}

void AWorldGenerator::WorldLocationToVoxelCoord(const FVector& WorldLocation, int32& OutX, int32& OutY, int32& OutZ) const
{
	OutX = FMath::FloorToInt(WorldLocation.X / BlockScale);
	OutY = FMath::FloorToInt(WorldLocation.Y / BlockScale);
	OutZ = FMath::FloorToInt(WorldLocation.Z / BlockScale);
}

bool AWorldGenerator::GetVoxelAt(int32 WorldBlockX, int32 WorldBlockY, int32 WorldBlockZ, uint8& OutBlockID) const
{
	if (WorldBlockZ < 0 || WorldBlockZ >= ChunkHeight)
	{
		OutBlockID = 0;
		return false;
	}

	const int32 ChunkX = FMath::FloorToInt(static_cast<float>(WorldBlockX) / static_cast<float>(CHUNK_SIZE_X));
	const int32 ChunkY = FMath::FloorToInt(static_cast<float>(WorldBlockY) / static_cast<float>(CHUNK_SIZE_Y));

	const FChunkCoord TargetCoord(ChunkX, ChunkY);
	if (AChunkActor* const* FoundChunk = ActiveChunks.Find(TargetCoord))
	{
		if (FoundChunk && IsValid(*FoundChunk))
		{
			int32 LocalX = WorldBlockX - (ChunkX * CHUNK_SIZE_X);
			int32 LocalY = WorldBlockY - (ChunkY * CHUNK_SIZE_Y);
			OutBlockID = (*FoundChunk)->GetBlock(LocalX, LocalY, WorldBlockZ);
			return true;
		}
	}

	OutBlockID = 0;
	return false;
}

bool AWorldGenerator::GetBlockAtWorldLocation(const FVector& WorldLocation, uint8& OutBlockID) const
{
	int32 VoxelX, VoxelY, VoxelZ;
	WorldLocationToVoxelCoord(WorldLocation, VoxelX, VoxelY, VoxelZ);
	return GetVoxelAt(VoxelX, VoxelY, VoxelZ, OutBlockID);
}

void AWorldGenerator::UpdateChunkStreaming()
{
	const int32 Radius = RenderDistance;
	const int32 RadiusSq = Radius * Radius;

	// Enqueue all chunks within render distance
	for (int32 Dx = -Radius; Dx <= Radius; ++Dx)
	{
		for (int32 Dy = -Radius; Dy <= Radius; ++Dy)
		{
			if ((Dx * Dx + Dy * Dy) <= RadiusSq)
			{
				FChunkCoord Coord(CurrentPlayerChunkCoord.X + Dx, CurrentPlayerChunkCoord.Y + Dy);
				if (!ActiveChunks.Contains(Coord) && !GeneratingChunks.Contains(Coord))
				{
					GeneratingChunks.Add(Coord);
					GenerationQueue.Add(Coord);
				}
			}
		}
	}

	// Sort queue closest to player first for instant responsiveness
	const FChunkCoord PlayerCoord = CurrentPlayerChunkCoord;
	GenerationQueue.Sort([PlayerCoord](const FChunkCoord& A, const FChunkCoord& B)
	{
		const int32 DistSqA = FMath::Square(A.X - PlayerCoord.X) + FMath::Square(A.Y - PlayerCoord.Y);
		const int32 DistSqB = FMath::Square(B.X - PlayerCoord.X) + FMath::Square(B.Y - PlayerCoord.Y);
		return DistSqA < DistSqB;
	});

	// Unload chunks outside RenderDistance + 1
	const int32 UnloadDist = Radius + 1;
	const int32 UnloadDistSq = UnloadDist * UnloadDist;
	TArray<FChunkCoord> ChunksToUnload;

	for (auto& Pair : ActiveChunks)
	{
		const FChunkCoord& Coord = Pair.Key;
		const int32 DistSq = FMath::Square(Coord.X - PlayerCoord.X) + FMath::Square(Coord.Y - PlayerCoord.Y);
		if (DistSq > UnloadDistSq)
		{
			ChunksToUnload.Add(Coord);
		}
	}

	for (const FChunkCoord& Coord : ChunksToUnload)
	{
		if (AChunkActor* Chunk = ActiveChunks.FindRef(Coord))
		{
			if (IsValid(Chunk))
			{
				if (Chunk->bIsModified)
				{
					SaveChunkDelta(Chunk);
				}
				Chunk->Destroy();
			}
		}
		ActiveChunks.Remove(Coord);
	}
}

void AWorldGenerator::ProcessGenerationQueue()
{
	int32 GeneratedCount = 0;

	while (GenerationQueue.Num() > 0 && GeneratedCount < MaxChunkGenerationsPerFrame)
	{
		const FChunkCoord Coord = GenerationQueue[0];
		GenerationQueue.RemoveAt(0);

		// Skip if already active or queued out of range
		if (ActiveChunks.Contains(Coord))
		{
			GeneratingChunks.Remove(Coord);
			continue;
		}

		const int32 DistSq = FMath::Square(Coord.X - CurrentPlayerChunkCoord.X) + FMath::Square(Coord.Y - CurrentPlayerChunkCoord.Y);
		if (DistSq > FMath::Square(RenderDistance + 1))
		{
			GeneratingChunks.Remove(Coord);
			continue;
		}

		AChunkActor* Chunk = SpawnChunk(Coord);
		if (Chunk)
		{
			ActiveChunks.Add(Coord, Chunk);
			GenerateChunkData(Chunk);
			Chunk->UpdateMesh();
			UpdateNeighborChunkMeshes(Coord);
			++GeneratedCount;
		}

		GeneratingChunks.Remove(Coord);
	}
}

AChunkActor* AWorldGenerator::SpawnChunk(const FChunkCoord& Coord)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLoc(
		static_cast<float>(Coord.X * CHUNK_SIZE_X) * BlockScale,
		static_cast<float>(Coord.Y * CHUNK_SIZE_Y) * BlockScale,
		0.0f
	);

	UClass* ClassToSpawn = ChunkActorClass ? ChunkActorClass.Get() : AChunkActor::StaticClass();
	AChunkActor* Chunk = GetWorld()->SpawnActor<AChunkActor>(ClassToSpawn, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
	if (Chunk)
	{
		Chunk->InitializeChunk(Coord, ChunkHeight, BlockScale, this);
		if (TerrainMaterial)
		{
			Chunk->SetMaterial(TerrainMaterial);
		}
	}
	return Chunk;
}

void AWorldGenerator::UpdateNeighborChunkMeshes(const FChunkCoord& CenterCoord)
{
	const FChunkCoord Neighbors[] = {
		FChunkCoord(CenterCoord.X + 1, CenterCoord.Y),
		FChunkCoord(CenterCoord.X - 1, CenterCoord.Y),
		FChunkCoord(CenterCoord.X, CenterCoord.Y + 1),
		FChunkCoord(CenterCoord.X, CenterCoord.Y - 1)
	};

	for (const FChunkCoord& Neighbor : Neighbors)
	{
		if (AChunkActor* const* Found = ActiveChunks.Find(Neighbor))
		{
			if (*Found)
			{
				(*Found)->UpdateMesh();
			}
		}
	}
}

int32 AWorldGenerator::GetPredictedTerrainHeight(int32 WorldX, int32 WorldY) const
{
	const EBiomeType Biome = GetBiomeAt(WorldX, WorldY);
	return GetTerrainHeight(WorldX, WorldY, Biome);
}

EBiomeType AWorldGenerator::GetBiomeAt(int32 WorldX, int32 WorldY) const
{
	const float WX = static_cast<float>(WorldX);
	const float WY = static_cast<float>(WorldY);

	const float Continental = Noise.Fractal2D(WX, WY, 3, 0.005f, 0.5f, 2.0f);
	if (Continental > 0.35f)
	{
		return EBiomeType::Mountains;
	}

	const float Temp = Noise.Fractal2D_01(WX, WY, 3, 0.006f);
	const float Moisture = Noise.Fractal2D_01(WX + 5000.0f, WY + 5000.0f, 3, 0.006f);

	if (Temp > 0.60f && Moisture < 0.40f)
	{
		return EBiomeType::Desert;
	}
	if (Temp < 0.40f && Moisture > 0.55f)
	{
		return EBiomeType::HorrorForest;
	}
	if (Moisture > 0.50f)
	{
		return EBiomeType::Forest;
	}
	return EBiomeType::Plains;
}

int32 AWorldGenerator::GetTerrainHeight(int32 WorldX, int32 WorldY, EBiomeType Biome) const
{
	const float WX = static_cast<float>(WorldX);
	const float WY = static_cast<float>(WorldY);

	// Multi-octave natural rolling hills (authentic Minecraft terrain)
	// Frequencies tuned for visible variation within a few chunks:
	//   Continental (0.005): 200-block period → broad landmass shape
	//   Hills (0.02): 50-block period → rolling hills every ~50 blocks
	//   Detail (0.06): 17-block period → block-level surface variation
	const float Continental = Noise.Fractal2D(WX, WY, 3, 0.005f, 0.5f, 2.0f);
	const float Hills = Noise.Fractal2D(WX + 1000.0f, WY + 1000.0f, 3, 0.02f, 0.5f, 2.0f);
	const float Detail = Noise.Fractal2D(WX + 2500.0f, WY + 2500.0f, 2, 0.06f, 0.5f, 2.0f);

	const float BaseHeight = static_cast<float>(ChunkHeight) * 0.4f; // ~25 blocks
	float Height = BaseHeight + (Continental * 8.0f) + (Hills * 8.0f) + (Detail * 3.0f);

	// Mountain peaks smoothly elevate in mountain areas
	if (Continental > 0.20f)
	{
		const float MountainRise = FMath::Square((Continental - 0.20f) / 0.80f) * 20.0f;
		Height += MountainRise;
	}

	if (Biome == EBiomeType::Desert)
	{
		// Gently flatten desert dunes
		Height = FMath::Lerp(Height, BaseHeight + (Continental * 4.0f), 0.5f);
	}

	return FMath::Clamp(FMath::RoundToInt(Height), 4, ChunkHeight - 8);
}

void AWorldGenerator::GenerateTree(AChunkActor* Chunk, int32 LocalX, int32 LocalY, int32 SurfaceZ, EBiomeType Biome)
{
	const int32 TrunkHeight = (Biome == EBiomeType::HorrorForest) ? 6 : 5;

	// Trunk
	for (int32 Z = 1; Z <= TrunkHeight; ++Z)
	{
		const int32 BlockZ = SurfaceZ + Z;
		if (BlockZ < ChunkHeight)
		{
			Chunk->SetBlock(LocalX, LocalY, BlockZ, static_cast<uint8>(EBlockType::Wood_Log));
		}
	}

	// Leaves for standard forest / plains
	if (Biome != EBiomeType::HorrorForest)
	{
		const int32 LeafStartZ = SurfaceZ + TrunkHeight - 2;
		const int32 LeafEndZ = SurfaceZ + TrunkHeight + 1;

		for (int32 Z = LeafStartZ; Z <= LeafEndZ; ++Z)
		{
			const int32 Radius = (Z >= SurfaceZ + TrunkHeight) ? 1 : 2;
			for (int32 Dx = -Radius; Dx <= Radius; ++Dx)
			{
				for (int32 Dy = -Radius; Dy <= Radius; ++Dy)
				{
					const int32 Tx = LocalX + Dx;
					const int32 Ty = LocalY + Dy;

					// Classic Minecraft rounded canopy: omit the 4 outer corners on radius 2 layers
					if (Radius > 1 && FMath::Abs(Dx) == Radius && FMath::Abs(Dy) == Radius)
					{
						continue;
					}

					if (Chunk->IsValidCoord(Tx, Ty, Z))
					{
						if (Chunk->GetBlock(Tx, Ty, Z) == static_cast<uint8>(EBlockType::Air))
						{
							Chunk->SetBlock(Tx, Ty, Z, static_cast<uint8>(EBlockType::Leaves));
						}
					}
				}
			}
		}
	}
	else
	{
		// Horror Forest: Spooky dead branch extensions instead of green leaves
		const int32 TopZ = SurfaceZ + TrunkHeight;
		if (Chunk->IsValidCoord(LocalX + 1, LocalY, TopZ))
			Chunk->SetBlock(LocalX + 1, LocalY, TopZ, static_cast<uint8>(EBlockType::Wood_Log));
		if (Chunk->IsValidCoord(LocalX - 1, LocalY, TopZ))
			Chunk->SetBlock(LocalX - 1, LocalY, TopZ, static_cast<uint8>(EBlockType::Wood_Log));
		if (Chunk->IsValidCoord(LocalX, LocalY + 1, TopZ + 1))
			Chunk->SetBlock(LocalX, LocalY + 1, TopZ + 1, static_cast<uint8>(EBlockType::Wood_Log));
	}
}

void AWorldGenerator::GenerateVillageHouse(AChunkActor* Chunk, int32 CenterX, int32 CenterY, int32 SurfaceZ)
{
	const int32 MinX = CenterX - 2;
	const int32 MaxX = CenterX + 2;
	const int32 MinY = CenterY - 2;
	const int32 MaxY = CenterY + 2;
	const int32 WallHeight = 4;

	// 1. Cobblestone Floor at SurfaceZ
	for (int32 X = MinX; X <= MaxX; ++X)
	{
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			if (Chunk->IsValidCoord(X, Y, SurfaceZ))
			{
				Chunk->SetBlock(X, Y, SurfaceZ, static_cast<uint8>(EBlockType::Cobblestone));
			}
		}
	}

	// 2. Walls, Corner Logs, Windows, and Doorway
	for (int32 H = 1; H <= WallHeight; ++H)
	{
		const int32 Z = SurfaceZ + H;
		if (Z >= ChunkHeight) break;

		for (int32 X = MinX; X <= MaxX; ++X)
		{
			for (int32 Y = MinY; Y <= MaxY; ++Y)
			{
				const bool bIsCorner = (X == MinX || X == MaxX) && (Y == MinY || Y == MaxY);
				const bool bIsWall = (X == MinX || X == MaxX || Y == MinY || Y == MaxY);

				if (bIsCorner)
				{
					if (Chunk->IsValidCoord(X, Y, Z))
					{
						Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Wood_Log));
					}
				}
				else if (bIsWall)
				{
					// Doorway opening on South wall (Y == MinY) at X == CenterX for H == 1, 2
					if (Y == MinY && X == CenterX && (H == 1 || H == 2))
					{
						if (Chunk->IsValidCoord(X, Y, Z))
						{
							Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Air));
						}
					}
					// Glass Windows at H == 2 on East and West walls
					else if (H == 2 && (X == MinX || X == MaxX) && Y == CenterY)
					{
						if (Chunk->IsValidCoord(X, Y, Z))
						{
							Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Glass));
						}
					}
					else
					{
						// Wood Planks wall
						if (Chunk->IsValidCoord(X, Y, Z))
						{
							Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Wood_Planks));
						}
					}
				}
				else
				{
					// Hollow Interior Air
					if (Chunk->IsValidCoord(X, Y, Z))
					{
						Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Air));
					}
				}
			}
		}
	}

	// 3. Cobblestone Roof
	const int32 RoofZ = SurfaceZ + WallHeight + 1;
	if (RoofZ < ChunkHeight)
	{
		for (int32 X = MinX - 1; X <= MaxX + 1; ++X)
		{
			for (int32 Y = MinY - 1; Y <= MaxY + 1; ++Y)
			{
				if (Chunk->IsValidCoord(X, Y, RoofZ))
				{
					Chunk->SetBlock(X, Y, RoofZ, static_cast<uint8>(EBlockType::Cobblestone));
				}
			}
		}
	}

	// 4. Interior Furniture: Crafting Table, Furnace, Torch
	const int32 FloorZ = SurfaceZ + 1;
	if (Chunk->IsValidCoord(MinX + 1, MaxY - 1, FloorZ))
	{
		Chunk->SetBlock(MinX + 1, MaxY - 1, FloorZ, static_cast<uint8>(EBlockType::Crafting_Table));
	}
	if (Chunk->IsValidCoord(MaxX - 1, MaxY - 1, FloorZ))
	{
		Chunk->SetBlock(MaxX - 1, MaxY - 1, FloorZ, static_cast<uint8>(EBlockType::Furnace));
	}
	if (Chunk->IsValidCoord(CenterX, CenterY, SurfaceZ + 3))
	{
		Chunk->SetBlock(CenterX, CenterY, SurfaceZ + 3, static_cast<uint8>(EBlockType::Torch));
	}

	// 5. Cobblestone walkway outside the door
	for (int32 Step = 1; Step <= 3; ++Step)
	{
		const int32 PathY = MinY - Step;
		if (Chunk->IsValidCoord(CenterX, PathY, SurfaceZ))
		{
			Chunk->SetBlock(CenterX, PathY, SurfaceZ, static_cast<uint8>(EBlockType::Cobblestone));
		}
	}
}

void AWorldGenerator::GenerateChunkData(AChunkActor* Chunk)
{
	const FChunkCoord Coord = Chunk->ChunkCoord;

	for (int32 X = 0; X < CHUNK_SIZE_X; ++X)
	{
		for (int32 Y = 0; Y < CHUNK_SIZE_Y; ++Y)
		{
			const int32 WorldX = (Coord.X * CHUNK_SIZE_X) + X;
			const int32 WorldY = (Coord.Y * CHUNK_SIZE_Y) + Y;

			const EBiomeType Biome = GetBiomeAt(WorldX, WorldY);
			const int32 TerrainHeight = GetTerrainHeight(WorldX, WorldY, Biome);

			for (int32 Z = 0; Z < ChunkHeight; ++Z)
			{
				if (Z == 0)
				{
					// Indestructible bedrock layer at the floor
					Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Bedrock));
				}
				else if (Z == TerrainHeight)
				{
					// Surface block
					if (Biome == EBiomeType::Desert)
					{
						Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Sand));
					}
					else if (Biome == EBiomeType::Mountains && Z > static_cast<int32>(ChunkHeight * 0.65f))
					{
						Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Stone));
					}
					else
					{
						Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Grass));
					}
				}
				else if (Z < TerrainHeight && Z >= TerrainHeight - 3)
				{
					// Sub-surface layer
					if (Biome == EBiomeType::Desert)
					{
						Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Sand));
					}
					else
					{
						Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Dirt));
					}
				}
				else if (Z < TerrainHeight - 3)
				{
					// Deep underground stone
					Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Stone));
				}
				else
				{
					// Air above surface
					Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Air));
				}
			}

			// 3D Worm / Cavern Caves (tubular worm tunnels, strictly underground to preserve surface)
			if (bEnableCaves)
			{
				const int32 MaxCaveZ = TerrainHeight - 6;
				if (MaxCaveZ > 6)
				{
					const float WX = static_cast<float>(WorldX);
					const float WY = static_cast<float>(WorldY);

					for (int32 Z = 4; Z <= MaxCaveZ; ++Z)
					{
						const float WZ = static_cast<float>(Z);

						// 3D Worm Cave: Intersection of two 3D noise fields forming tubular worm tunnels
						const float N1 = Noise.Perlin3D(WX * 0.035f, WY * 0.035f, WZ * 0.035f);
						const float N2 = Noise.Perlin3D((WX + 317.0f) * 0.035f, (WY + 571.0f) * 0.035f, (WZ + 193.0f) * 0.035f);

						if ((N1 * N1 + N2 * N2) < 0.0035f)
						{
							Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Air));
						}
					}
				}
			}

			// Stratified Ore Veins
			if (bEnableOres)
			{
				for (int32 Z = 1; Z < TerrainHeight - 2; ++Z)
				{
					if (Chunk->GetBlock(X, Y, Z) == static_cast<uint8>(EBlockType::Stone))
					{
						const float WX = static_cast<float>(WorldX);
						const float WY = static_cast<float>(WorldY);
						const float WZ = static_cast<float>(Z);

						// Diamond Ore (Deepest, rare)
						if (Z <= 16)
						{
							if (Noise.Fractal3D(WX * 2.5f, WY * 2.5f, WZ * 2.5f, 2, 0.05f) > 0.74f)
							{
								Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Diamond_Ore));
								continue;
							}
						}

						// Gold Ore (Medium-deep)
						if (Z <= 32)
						{
							if (Noise.Fractal3D(WX * 2.0f, WY * 2.0f, WZ * 2.0f + 100.0f, 2, 0.045f) > 0.68f)
							{
								Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Gold_Ore));
								continue;
							}
						}

						// Iron Ore (Common underground)
						if (Z <= 55)
						{
							if (Noise.Fractal3D(WX * 1.5f, WY * 1.5f, WZ * 1.5f + 200.0f, 2, 0.038f) > 0.60f)
							{
								Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Iron_Ore));
								continue;
							}
						}

						// Coal Ore (Abundant across all depths)
						if (Z <= 60)
						{
							if (Noise.Fractal3D(WX * 1.2f, WY * 1.2f, WZ * 1.2f + 300.0f, 2, 0.032f) > 0.54f)
							{
								Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Coal_Ore));
								continue;
							}
						}

						// Emerald Ore (Rare, exclusive to mountains)
						if (Biome == EBiomeType::Mountains && Z >= 20)
						{
							if (Noise.Fractal3D(WX * 3.0f, WY * 3.0f, WZ * 3.0f + 400.0f, 2, 0.06f) > 0.78f)
							{
								Chunk->SetBlock(X, Y, Z, static_cast<uint8>(EBlockType::Emerald_Ore));
								continue;
							}
						}
					}
				}
			}
		}
	}

	// Phase 2: Flora & Trees (Separate pass so air above surface does not overwrite positive Dx/Dy leaves!)
	if (bEnableTrees)
	{
		for (int32 X = 2; X <= CHUNK_SIZE_X - 3; ++X)
		{
			for (int32 Y = 2; Y <= CHUNK_SIZE_Y - 3; ++Y)
			{
				const int32 WorldX = (Coord.X * CHUNK_SIZE_X) + X;
				const int32 WorldY = (Coord.Y * CHUNK_SIZE_Y) + Y;

				const EBiomeType Biome = GetBiomeAt(WorldX, WorldY);
				const int32 TerrainHeight = GetTerrainHeight(WorldX, WorldY, Biome);

				uint32 THash = (static_cast<uint32>(WorldX) * 374761393u) ^ (static_cast<uint32>(WorldY) * 668265263u) ^ static_cast<uint32>(WorldSeed);
				THash = (THash ^ (THash >> 13)) * 1274126177u;
				const float TreeRoll = static_cast<float>(THash & 0xFFFF) / 65535.0f;

				bool bSpawnTree = false;

				if (Biome == EBiomeType::Forest && TreeRoll < 0.040f)
				{
					bSpawnTree = true;
				}
				else if (Biome == EBiomeType::HorrorForest && TreeRoll < 0.030f)
				{
					bSpawnTree = true;
				}
				else if (Biome == EBiomeType::Plains && TreeRoll < 0.008f)
				{
					bSpawnTree = true;
				}

				if (bSpawnTree && TerrainHeight + 8 < ChunkHeight)
				{
					GenerateTree(Chunk, X, Y, TerrainHeight, Biome);
				}
			}
		}
	}

	// Procedural Village Generation in Plains or Desert
	const int32 CenterWorldX = (Coord.X * CHUNK_SIZE_X) + 8;
	const int32 CenterWorldY = (Coord.Y * CHUNK_SIZE_Y) + 8;
	const EBiomeType CenterBiome = GetBiomeAt(CenterWorldX, CenterWorldY);

	if (CenterBiome == EBiomeType::Plains || CenterBiome == EBiomeType::Desert)
	{
		const uint32 VHash = (static_cast<uint32>(Coord.X * 198491317) ^ static_cast<uint32>(Coord.Y * 6542989) ^ static_cast<uint32>(WorldSeed));
		if ((VHash % 7) == 0)
		{
			const int32 CenterSurfaceZ = GetTerrainHeight(CenterWorldX, CenterWorldY, CenterBiome);
			if (CenterSurfaceZ + 8 < ChunkHeight)
			{
				GenerateVillageHouse(Chunk, 8, 8, CenterSurfaceZ);
			}
		}
	}

	// Load persistent player modifications from disk
	LoadChunkDelta(Chunk);
}

bool AWorldGenerator::BreakBlock(const FVector& WorldLocation, uint8& OutDroppedBlockID)
{
	int32 VoxelX, VoxelY, VoxelZ;
	WorldLocationToVoxelCoord(WorldLocation, VoxelX, VoxelY, VoxelZ);

	const int32 ChunkX = FMath::FloorToInt(static_cast<float>(VoxelX) / static_cast<float>(CHUNK_SIZE_X));
	const int32 ChunkY = FMath::FloorToInt(static_cast<float>(VoxelY) / static_cast<float>(CHUNK_SIZE_Y));

	const FChunkCoord Coord(ChunkX, ChunkY);
	if (AChunkActor* const* FoundChunk = ActiveChunks.Find(Coord))
	{
		if (*FoundChunk)
		{
			const int32 LocalX = VoxelX - (ChunkX * CHUNK_SIZE_X);
			const int32 LocalY = VoxelY - (ChunkY * CHUNK_SIZE_Y);

			OutDroppedBlockID = (*FoundChunk)->GetBlock(LocalX, LocalY, VoxelZ);

			// Bedrock (25) is indestructible like Minecraft
			if (OutDroppedBlockID == static_cast<uint8>(EBlockType::Bedrock) || OutDroppedBlockID == static_cast<uint8>(EBlockType::Air))
			{
				return false;
			}

			// Destroy block to Air
			(*FoundChunk)->SetBlock(LocalX, LocalY, VoxelZ, static_cast<uint8>(EBlockType::Air));
			(*FoundChunk)->UpdateMesh();

			// If on boundary, update adjacent neighbor chunk
			if (LocalX == 0)
			{
				if (AChunkActor* const* Neighbor = ActiveChunks.Find(FChunkCoord(ChunkX - 1, ChunkY)))
					if (*Neighbor) (*Neighbor)->UpdateMesh();
			}
			else if (LocalX == CHUNK_SIZE_X - 1)
			{
				if (AChunkActor* const* Neighbor = ActiveChunks.Find(FChunkCoord(ChunkX + 1, ChunkY)))
					if (*Neighbor) (*Neighbor)->UpdateMesh();
			}

			if (LocalY == 0)
			{
				if (AChunkActor* const* Neighbor = ActiveChunks.Find(FChunkCoord(ChunkX, ChunkY - 1)))
					if (*Neighbor) (*Neighbor)->UpdateMesh();
			}
			else if (LocalY == CHUNK_SIZE_Y - 1)
			{
				if (AChunkActor* const* Neighbor = ActiveChunks.Find(FChunkCoord(ChunkX, ChunkY + 1)))
					if (*Neighbor) (*Neighbor)->UpdateMesh();
			}

			SaveChunkDelta(*FoundChunk);
			SpawnBlockItemDrop(WorldLocation, OutDroppedBlockID);
			return true;
		}
	}

	OutDroppedBlockID = 0;
	return false;
}

void AWorldGenerator::SpawnBlockItemDrop(const FVector& WorldLocation, uint8 DroppedBlockID)
{
	if (DroppedBlockID == 0 || DroppedBlockID == static_cast<uint8>(EBlockType::Bedrock))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector DropPos = WorldLocation + FVector(0.0f, 0.0f, BlockScale * 0.2f);

	ABlockItemPickup* Pickup = GetWorld()->SpawnActor<ABlockItemPickup>(
		ABlockItemPickup::StaticClass(),
		DropPos,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (Pickup)
	{
		Pickup->InitializePickup(DroppedBlockID, 1, TerrainMaterial);
	}
}

bool AWorldGenerator::PlaceBlock(const FVector& WorldLocation, uint8 BlockID)
{
	if (BlockID == 0) return false;

	int32 VoxelX, VoxelY, VoxelZ;
	WorldLocationToVoxelCoord(WorldLocation, VoxelX, VoxelY, VoxelZ);

	if (VoxelZ < 0 || VoxelZ >= ChunkHeight) return false;

	const int32 ChunkX = FMath::FloorToInt(static_cast<float>(VoxelX) / static_cast<float>(CHUNK_SIZE_X));
	const int32 ChunkY = FMath::FloorToInt(static_cast<float>(VoxelY) / static_cast<float>(CHUNK_SIZE_Y));

	const FChunkCoord Coord(ChunkX, ChunkY);
	if (AChunkActor* const* FoundChunk = ActiveChunks.Find(Coord))
	{
		if (*FoundChunk)
		{
			const int32 LocalX = VoxelX - (ChunkX * CHUNK_SIZE_X);
			const int32 LocalY = VoxelY - (ChunkY * CHUNK_SIZE_Y);

			(*FoundChunk)->SetBlock(LocalX, LocalY, VoxelZ, BlockID);
			(*FoundChunk)->UpdateMesh();

			if (LocalX == 0)
			{
				if (AChunkActor* const* Neighbor = ActiveChunks.Find(FChunkCoord(ChunkX - 1, ChunkY)))
					if (*Neighbor) (*Neighbor)->UpdateMesh();
			}
			else if (LocalX == CHUNK_SIZE_X - 1)
			{
				if (AChunkActor* const* Neighbor = ActiveChunks.Find(FChunkCoord(ChunkX + 1, ChunkY)))
					if (*Neighbor) (*Neighbor)->UpdateMesh();
			}

			if (LocalY == 0)
			{
				if (AChunkActor* const* Neighbor = ActiveChunks.Find(FChunkCoord(ChunkX, ChunkY - 1)))
					if (*Neighbor) (*Neighbor)->UpdateMesh();
			}
			else if (LocalY == CHUNK_SIZE_Y - 1)
			{
				if (AChunkActor* const* Neighbor = ActiveChunks.Find(FChunkCoord(ChunkX, ChunkY + 1)))
					if (*Neighbor) (*Neighbor)->UpdateMesh();
			}

			SaveChunkDelta(*FoundChunk);
			return true;
		}
	}
	return false;
}

bool AWorldGenerator::TraceForBlock(
	APlayerCameraManager* Camera,
	float MaxDistance,
	FVector& OutBreakBlockWorldPos,
	FVector& OutPlaceBlockWorldPos,
	uint8& OutHitBlockID
)
{
	if (!Camera)
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC)
		{
			Camera = PC->PlayerCameraManager;
		}
	}

	if (!Camera)
	{
		return false;
	}

	const FVector Start = Camera->GetCameraLocation();
	const FVector End = Start + (Camera->GetActorForwardVector() * MaxDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->GetPawn())
	{
		Params.AddIgnoredActor(PC->GetPawn());
	}

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		if (Hit.GetActor() && Hit.GetActor()->IsA<AChunkActor>())
		{
			// Step inward along normal to identify the clicked block
			const FVector BreakSamplePos = Hit.ImpactPoint - (Hit.ImpactNormal * (BlockScale * 0.25f));
			int32 BX, BY, BZ;
			WorldLocationToVoxelCoord(BreakSamplePos, BX, BY, BZ);

			OutBreakBlockWorldPos = FVector(
				(static_cast<float>(BX) + 0.5f) * BlockScale,
				(static_cast<float>(BY) + 0.5f) * BlockScale,
				(static_cast<float>(BZ) + 0.5f) * BlockScale
			);

			// Step outward along normal to identify placement block
			const FVector PlaceSamplePos = Hit.ImpactPoint + (Hit.ImpactNormal * (BlockScale * 0.25f));
			int32 PX, PY, PZ;
			WorldLocationToVoxelCoord(PlaceSamplePos, PX, PY, PZ);

			OutPlaceBlockWorldPos = FVector(
				(static_cast<float>(PX) + 0.5f) * BlockScale,
				(static_cast<float>(PY) + 0.5f) * BlockScale,
				(static_cast<float>(PZ) + 0.5f) * BlockScale
			);

			GetVoxelAt(BX, BY, BZ, OutHitBlockID);
			return true;
		}
	}

	OutHitBlockID = 0;
	return false;
}

FString AWorldGenerator::GetChunkSaveFilePath(const FChunkCoord& Coord) const
{
	return WorldSavePath / FString::Printf(TEXT("Chunk_%d_%d.dat"), Coord.X, Coord.Y);
}

void AWorldGenerator::SaveChunkDelta(AChunkActor* Chunk)
{
	if (!Chunk || !Chunk->bIsModified)
	{
		return;
	}

	const FString FilePath = GetChunkSaveFilePath(Chunk->ChunkCoord);

	FBufferArchive Archive;
	uint32 Magic = 0x564F584C; // 'VOXL'
	Archive << Magic;

	// Count modified blocks by reading file or saving current state
	// For high performance, we write the modified state
	int32 TotalVoxels = Chunk->BlockData.Num();
	Archive << TotalVoxels;
	Archive.Serialize(Chunk->BlockData.GetData(), TotalVoxels);

	FFileHelper::SaveArrayToFile(Archive, *FilePath);
	Chunk->bIsModified = false;
}

bool AWorldGenerator::LoadChunkDelta(AChunkActor* Chunk)
{
	if (!Chunk)
	{
		return false;
	}

	const FString FilePath = GetChunkSaveFilePath(Chunk->ChunkCoord);
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		return false;
	}

	TArray<uint8> FileBytes;
	if (!FFileHelper::LoadFileToArray(FileBytes, *FilePath) || FileBytes.Num() < 8)
	{
		return false;
	}

	FMemoryReader Reader(FileBytes);
	uint32 Magic = 0;
	Reader << Magic;

	if (Magic != 0x564F584C)
	{
		return false;
	}

	int32 TotalVoxels = 0;
	Reader << TotalVoxels;

	if (TotalVoxels == Chunk->BlockData.Num())
	{
		Reader.Serialize(Chunk->BlockData.GetData(), TotalVoxels);
		return true;
	}

	return false;
}

void AWorldGenerator::SaveWorld()
{
	for (auto& Pair : ActiveChunks)
	{
		if (Pair.Value && Pair.Value->bIsModified)
		{
			SaveChunkDelta(Pair.Value);
		}
	}
}
