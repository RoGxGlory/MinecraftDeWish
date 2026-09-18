#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelDataTypes.h"
#include "VoxelNoise.h"
#include "MiningQueueSystem.h"
#include "CraftingSmeltingSystem.h"
#include "VoxelStructureGenerator.h"
#include "DayNightCycleSystem.h"
#include "MobSpawnerSystem.h"
#include "WorldGenerator.generated.h"

class AChunkActor;
class ABlockHighlightActor;

/** Delegate fired when the crafting table UI should open */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftingTableOpened);

/** Delegate fired when the furnace UI should open */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFurnaceOpened);

/**
 * AWorldGenerator
 * 
 * Core coordinator actor responsible for:
 * 1. Procedural 3D voxel terrain generation (continentalness, mountain peaks, 3D cave carving, ore veins, bedrock).
 * 2. Infinite chunk streaming around the player pawn (dynamic loading, generation queuing, dirty mesh rebuilds).
 * 3. Chunk delta persistence (saving and loading user-modified blocks to disk).
 * 4. Coordinating dedicated modular subsystems:
 *    - MiningQueueSystem: Concurrent mining tasks, tool progression, queue capacity.
 *    - CraftingSmeltingSystem: Tool crafting and furnace smelting recipes & fuel burn efficiencies.
 *    - VoxelStructureGenerator: Complete 3D tree canopies, village structures, and flora.
 * 5. Player block interaction raycasts, targeted voxel highlighting, and item pickup spawning.
 */
UCLASS()
class MINECRAFTDEWISH_API AWorldGenerator : public AActor
{
	GENERATED_BODY()

public:
	AWorldGenerator();

	// ==================== WORLD GENERATION SETTINGS ====================

	/** Procedural generation seed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings")
	int32 WorldSeed = 1337;

	/** Render distance in chunks (radius around player) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings", meta = (ClampMin = "2", ClampMax = "16"))
	int32 RenderDistance = 6;

	/** Height of each chunk in blocks (default 64) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings", meta = (ClampMin = "32", ClampMax = "256"))
	int32 ChunkHeight = DEFAULT_CHUNK_SIZE_Z;

	/** Scale of each voxel block in Unreal units (default 100cm = 1m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings")
	float BlockScale = DEFAULT_BLOCK_SCALE;

	/** Master terrain material using Texture2DArray (M_Global) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Rendering")
	UMaterialInterface* TerrainMaterial = nullptr;

	/** Optional Blueprint subclass of ChunkActor (e.g. BP_ChunkActor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Rendering")
	TSubclassOf<AChunkActor> ChunkActorClass;

	/** Data table containing block definitions (Block_DataTable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Data")
	UDataTable* BlockDataTable = nullptr;

	/** Enable 3D cavern generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Features")
	bool bEnableCaves = true;

	/** Enable ore vein distribution (Coal, Iron, Gold, Diamond, Emerald) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Features")
	bool bEnableOres = true;

	/** Enable biome-specific trees and foliage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Features")
	bool bEnableTrees = true;

	/** Frequency for base continental terrain noise */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Noise")
	float TerrainFrequency = 0.0035f;

	/** Maximum chunk meshes to build per frame to avoid framerate hitches */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Performance")
	int32 MaxChunkGenerationsPerFrame = 2;

	// ==================== DEDICATED MODULAR SUBSYSTEMS ====================

	/** Dedicated subsystem managing concurrent block mining, tool tiers, and queue limits */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mining")
	UMiningQueueSystem* MiningQueueSystem = nullptr;

	/** Dedicated subsystem managing tool crafting upgrades and furnace smelting recipes */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting|Smelting")
	UCraftingSmeltingSystem* CraftingSmeltingSystem = nullptr;

	/** Day/night cycle subsystem (20-minute cycle, sun rotation, light queries for mob spawning) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation|DayNight")
	UDayNightCycleSystem* DayNightCycleSystem = nullptr;

	/** Hostile mob spawning and despawning subsystem */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation|Mobs")
	UMobSpawnerSystem* MobSpawnerSystem = nullptr;

	// ==================== BLOCK & VOXEL QUERIES ====================

	/** Returns the 2D texture array slice index for a specific face of a block */
	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	int32 GetTextureForBlock(uint8 BlockID, EBlockFace Face) const;

	/** Returns true if the block is visually transparent (e.g. Glass, Leaves, Water) */
	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	bool IsBlockTransparent(uint8 BlockID) const;

	/** Computes estimated surface terrain height at global (WorldX, WorldY) block coordinates */
	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	int32 GetPredictedTerrainHeight(int32 WorldX, int32 WorldY) const;

	/** Query voxel at global block coordinates */
	bool GetVoxelAt(int32 WorldBlockX, int32 WorldBlockY, int32 WorldBlockZ, uint8& OutBlockID) const;

	/** Query voxel at Unreal world location */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Interaction")
	bool GetBlockAtWorldLocation(const FVector& WorldLocation, uint8& OutBlockID) const;

	/** Break block at world location, updating meshes and spawning dropped item */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Interaction")
	bool BreakBlock(const FVector& WorldLocation, uint8& OutDroppedBlockID);

	/** Break block directly by global voxel coordinate */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Interaction")
	bool BreakBlockAtVoxel(int32 VoxelX, int32 VoxelY, int32 VoxelZ, uint8& OutDroppedBlockID);

	/** Place block at world location */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Interaction")
	bool PlaceBlock(const FVector& WorldLocation, uint8 BlockID);

	/** Line trace helper to find block to break and block placement location */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Interaction")
	bool TraceForBlock(
		APlayerCameraManager* Camera,
		float MaxDistance,
		FVector& OutBreakBlockWorldPos,
		FVector& OutPlaceBlockWorldPos,
		uint8& OutHitBlockID
	);

	/** Save all modified chunks to disk */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Save")
	void SaveWorld();

	/** Convert world position into 2D chunk coordinate */
	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	FChunkCoord WorldLocationToChunkCoord(const FVector& WorldLocation) const;

	/** Convert world position into global integer voxel coordinates (X, Y, Z) */
	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	void WorldLocationToVoxelCoord(const FVector& WorldLocation, int32& OutX, int32& OutY, int32& OutZ) const;

	// ==================== MINING QUEUE & TOOLS FACADE ====================

	/** Current tool tier (Hand = 1 slot, Wood = 2, Stone = 3, Iron = 4, Diamond = 5, Obsidian = 6) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mining|Tools")
	EToolTier CurrentToolTier = EToolTier::None;

	/** Returns current queue capacity based on equipped tool tier */
	UFUNCTION(BlueprintPure, Category = "Mining|Queue")
	int32 GetMaxQueueSlots() const;

	/** Returns current base mining force based on equipped tool tier */
	UFUNCTION(BlueprintPure, Category = "Mining|Queue")
	float GetToolMiningForce() const;

	/** Updates tool tier, scaling mining force and queue slots */
	UFUNCTION(BlueprintCallable, Category = "Mining|Queue")
	void SetToolTier(EToolTier NewTier);

	/** Queues a block at exact voxel coordinates for destruction */
	UFUNCTION(BlueprintCallable, Category = "Mining|Queue")
	bool QueueBlockBreakAtVoxel(int32 VoxelX, int32 VoxelY, int32 VoxelZ, float MiningForceOverride = -1.0f);

	/** Queues a block at hit location for destruction */
	UFUNCTION(BlueprintCallable, Category = "Mining|Queue")
	bool QueueBlockBreakAtLocation(const FVector& HitLocation, float MiningForceOverride = -1.0f);

	/** Returns number of blocks actively queued/being mined */
	UFUNCTION(BlueprintPure, Category = "Mining|Queue")
	int32 GetMiningQueueCount() const;

	/** Returns active mining tasks array for UI progress bars */
	UFUNCTION(BlueprintPure, Category = "Mining|Queue")
	TArray<FMiningTask> GetActiveMiningTasks() const;

	// ==================== CRAFTING & SMELTING FACADE ====================

	/** Crafts and equips a new tool tier */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool CraftTool(EToolTier DesiredTier, FString& OutMessage);

	/** Checks if a tool tier can be crafted */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool CanCraftTool(EToolTier DesiredTier) const;

	/** Remaining burn duration in seconds of current active furnace fuel */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smelting")
	float FurnaceBurnTimeRemaining = 0.0f;

	/** Total burn duration in seconds of the current furnace fuel */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smelting")
	float FurnaceTotalBurnTime = 0.0f;

	/** Smelt cooking progress [0.0 - 1.0] */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smelting")
	float FurnaceSmeltProgress = 0.0f;

	/** Smelts an input block using fuel */
	UFUNCTION(BlueprintCallable, Category = "Smelting")
	bool SmeltItem(uint8 InputBlockID, uint8 FuelBlockID, uint8& OutResultBlockID, FString& OutMessage);

	/** Returns fuel burn duration in seconds (Coal/Charcoal: 80s, Log: 15s, Planks: 10s) */
	UFUNCTION(BlueprintPure, Category = "Smelting")
	static float GetFuelBurnDuration(uint8 FuelBlockID);

	// ==================== BLOCK INTERACTION ====================

	/** Delegate fired when the crafting table UI should open */
	UPROPERTY(BlueprintAssignable, Category = "World Generation|Interaction")
	FOnCraftingTableOpened OnCraftingTableOpened;

	/** Delegate fired when the furnace UI should open */
	UPROPERTY(BlueprintAssignable, Category = "World Generation|Interaction")
	FOnFurnaceOpened OnFurnaceOpened;

	/**
	 * Interact with the currently targeted block (right-click).
	 * If the block is a Crafting Table, opens crafting UI.
	 * If the block is a Furnace, opens smelting UI.
	 * Player must be within 4 blocks of the target.
	 * @return True if an interaction occurred.
	 */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Interaction")
	bool InteractWithTargetBlock();

	/** Returns the targeted block type (for UI to decide what to open) */
	UFUNCTION(BlueprintPure, Category = "World Generation|Interaction")
	uint8 GetTargetedBlockType() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FVoxelNoise Noise;
	TMap<FChunkCoord, AChunkActor*> ActiveChunks;
	TSet<FChunkCoord> GeneratingChunks;
	TArray<FChunkCoord> GenerationQueue;

	FChunkCoord CurrentPlayerChunkCoord;
	bool bHasPlayerCoord = false;

	TMap<uint8, FBlockTextureData> BlockTextureCache;
	TSet<uint8> TransparentBlockIDs;

	FString WorldSavePath;

	void InitializeBlockCache();
	void UpdateChunkStreaming();
	void ProcessGenerationQueue();

	AChunkActor* SpawnChunk(const FChunkCoord& Coord);
	void GenerateChunkData(AChunkActor* Chunk);
	void UpdateNeighborChunkMeshes(const FChunkCoord& CenterCoord);

	EBiomeType GetBiomeAt(int32 WorldX, int32 WorldY) const;
	int32 GetTerrainHeight(int32 WorldX, int32 WorldY, EBiomeType Biome) const;
	void GenerateTree(AChunkActor* Chunk, int32 LocalX, int32 LocalY, int32 SurfaceZ, EBiomeType Biome);
	void GenerateVillageHouse(AChunkActor* Chunk, int32 CenterX, int32 CenterY, int32 SurfaceZ);

	void SpawnBlockItemDrop(const FVector& WorldLocation, uint8 DroppedBlockID);

	UPROPERTY(Transient)
	TObjectPtr<ABlockHighlightActor> BlockHighlightActor = nullptr;

	void UpdateTargetBlockHighlight();
	void RegisterChunkActorInterfaces();
	void EnsureDestroySystemConfigured();
	bool bDestroySystemConfigured = false;

	// Persistence helpers
	FString GetChunkSaveFilePath(const FChunkCoord& Coord) const;
	void SaveChunkDelta(AChunkActor* Chunk);
	bool LoadChunkDelta(AChunkActor* Chunk);

	// Backward-compatibility mirror for active mining tasks
	UPROPERTY(Transient)
	TArray<FMiningTask> ActiveMiningTasks;
};
