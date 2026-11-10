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
class ABlockCrackActor;
class UMaterialParameterCollection;

/** Fired once per attack on a valid block target. Hook for swing animations and per-hit dig sounds/particles. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnToolStrike, FIntVector, VoxelCoord, uint8, BlockID, FVector, WorldLocation);

/** Fired when a crack overlay advances to a new stage (0..9). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrackStageChanged, FIntVector, VoxelCoord, int32, Stage);

/**
 * Streaming work measured during one tick of AWorldGenerator, in milliseconds.
 *
 * Diagnostic only. It exists so the benchmark harness (UVoxelBenchSubsystem) and
 * Unreal Insights can report where voxel frame time goes: generation, meshing,
 * neighbour remeshing, deferred remesh and streaming bookkeeping. Gameplay never
 * reads it, so it can be dropped from a shipping build without changing behaviour.
 */
struct FVoxelFrameStats
{
	/** Time spent in UpdateChunkStreaming (queueing, sorting, unloading). */
	double StreamingMs = 0.0;

	/** Time spent in GenerateChunkData (terrain, caves, ores, structures). */
	double GenerationMs = 0.0;

	/** Time spent building procedural mesh sections, including neighbour remeshes. */
	double MeshMs = 0.0;

	/** Time spent draining the deferred remesh queue after a rendering setting change. */
	double RemeshMs = 0.0;

	/** Chunks spawned and generated during this tick. */
	int32 ChunksGenerated = 0;

	/** Chunks remeshed from the deferred queue during this tick. */
	int32 ChunksRemeshed = 0;
};

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

	/** Lowest render distance a player may select, in chunks (radius around the player). */
	static constexpr int32 MinRenderDistance = 2;

	/** Highest render distance a player may select, in chunks (radius around the player). */
	static constexpr int32 MaxRenderDistance = 64;

	/** Shipped default render distance, in chunks. */
	static constexpr int32 DefaultRenderDistance = 8;

	/** Procedural generation seed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings")
	int32 WorldSeed = 1337;

	/**
	 * Render distance in chunks (radius around the player). Runtime-adjustable through
	 * SetRenderDistance(); the last chosen value is persisted in GameUserSettings.ini.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings", meta = (ClampMin = "2", ClampMax = "64"))
	int32 RenderDistance = DefaultRenderDistance;

	/** Shipped default for the leaf rendering mode. */
	static constexpr bool DefaultFastLeaves = true;

	/**
	 * Leaf rendering mode, exposed to the player settings.
	 *
	 * NEUTRALISED: this flag no longer changes the mesh. Every leaf face is generated
	 * again (fancy leaves), because culling the contact between two leaves while the
	 * leaf texture stays alpha-tested produced a hollow shell: the canopy interior
	 * disappeared and thin sections let the player see straight through the tree.
	 *
	 * The property, the getter/setter and the persistence are kept so the player
	 * setting exists today and so the mode can be revived later. Reviving it requires
	 * genuinely opaque leaves first: use the last free vertex-colour channel (R) to mark
	 * leaf faces, feed `max(existing, R)` into the M_Global Opacity Mask, and only then
	 * cull leaf-to-leaf faces again (see Docs/architecture/voxel-performance-plan.md,
	 * lot 3). G now carries the foliage wind flag, so it is no longer available. Do not
	 * re-enable culling before that work lands.
	 *
	 * Set it with SetFastLeaves() so the value is persisted in GameUserSettings.ini.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings")
	bool bFastLeaves = DefaultFastLeaves;

	/** Wind animation is off below this multiplier. */
	static constexpr float MinWindStrength = 0.0f;

	/** Highest wind strength a player may select. */
	static constexpr float MaxWindStrength = 2.0f;

	/** Shipped default wind strength. */
	static constexpr float DefaultWindStrength = 1.0f;

	/**
	 * Multiplier on the foliage wind animation; 0 freezes the leaves.
	 *
	 * The animation itself lives in M_Global's World Position Offset, driven by the leaf
	 * flag in vertex-colour channel G, so nothing is remeshed when this changes: the value
	 * goes to the WindStrength scalar of MPC_World and every material reading it follows.
	 *
	 * Set it with SetWindStrength() so the value is persisted in GameUserSettings.ini.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float WindStrength = DefaultWindStrength;

	/** Height of each chunk in blocks (default 64) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings", meta = (ClampMin = "32", ClampMax = "256"))
	int32 ChunkHeight = DEFAULT_CHUNK_SIZE_Z;

	/** Scale of each voxel block in Unreal units (default 100cm = 1m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings")
	float BlockScale = DEFAULT_BLOCK_SCALE;

	/** Master terrain material using Texture2DArray (M_Global) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Rendering")
	UMaterialInterface* TerrainMaterial = nullptr;

	/**
	 * Material Parameter Collection carrying the runtime world knobs (MPC_World).
	 * Loaded from /Game/Materials/MPC_World when left empty.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Rendering")
	TObjectPtr<UMaterialParameterCollection> WorldParameterCollection = nullptr;

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

	/**
	 * True if the top/bottom face of this block uses a rotationally symmetric (isotropic) texture,
	 * i.e. it can be rotated and phase-shifted per block without looking wrong.
	 * Deliberately keyed on the block ID rather than a texture slice index, because the slice
	 * indices come from Block_DataTable and are not stable.
	 */
	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	bool IsTopFaceIsotropic(uint8 BlockID) const;

	/** True for natural terrain blocks that should receive the world-space tiling-breakup mask */
	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	bool IsNaturalTerrainBlock(uint8 BlockID) const;

	// ==================== TOP TEXTURE BLENDING ====================

	/** Enables smooth cross-fading of top-face textures between neighbouring biomes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Rendering|Blending")
	bool bEnableTopTextureBlending = true;

	/**
	 * Half-width of the biome texture cross-fade, in BLOCKS. The fade distance is divided
	 * by the local biome-field gradient, so the transition is the same physical width
	 * everywhere instead of stretching to dozens of blocks where the fields are flat.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Rendering|Blending", meta = (ClampMin = "0.0", ClampMax = "32.0"))
	float BiomeBlendWidth = 4.0f;

	/** Half-width of the terrain-height ramp across a desert border, in biome-field units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Rendering|Blending", meta = (ClampMin = "0.0", ClampMax = "0.30"))
	float BiomeHeightRamp = 0.06f;

	/**
	 * Field-space jitter added to the biome fields. Makes the border contour interlock in
	 * lobes instead of reading as a clean line. Applied to the shared field sample, so the
	 * hard block assignment and the smooth cross-fade always agree.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Rendering|Blending", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	float BiomeBlendNoise = 0.010f;

	/**
	 * World-space warp amplitude (in blocks) applied to the biome field coordinates.
	 * Low-frequency, so borders meander instead of following the raw noise contour.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Rendering|Blending", meta = (ClampMin = "0.0", ClampMax = "32.0"))
	float BiomeBlendWarp = 3.0f;

	/** Overall strength of the biome cross-fade (0 = disabled, 1 = full) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Rendering|Blending", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BiomeBlendStrength = 1.0f;

	/**
	 * Evaluates the normalized surface-material weights of a world column
	 * (Grass / Sand / Stone). Used by chunk meshing to splat top textures
	 * smoothly between biomes. The values always sum to 1.
	 */
	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	void GetSurfaceMaterialWeights(int32 WorldX, int32 WorldY, float& OutGrass, float& OutSand, float& OutStone) const;

	/**
	 * Fills a (SizeX+1) x (SizeY+1) grid of normalized surface-material weights for chunk
	 * meshing. Field samples are shared across the whole grid and a one-cell apron is
	 * included so the local gradient can be taken by central differences.
	 */
	void GetSurfaceMaterialWeightGrid(int32 BaseX, int32 BaseY, int32 SizeX, int32 SizeY,
		TArray<float>& OutGrass, TArray<float>& OutSand, TArray<float>& OutStone) const;

	/**
	 * Smooth 0..1 desert factor for a world column: 0 outside the desert threshold,
	 * 1 well inside it, and exactly 0.5 on the contour. Used to ramp the desert terrain
	 * flattening across the border instead of stepping at it.
	 */
	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	float GetDesertBlendAt(int32 WorldX, int32 WorldY) const;

	/**
	 * Returns true if the block is visually transparent (e.g. Glass, Leaves, Water).
	 *
	 * This is also the face-culling rule: a face is generated when the block on the
	 * other side is transparent. Do not cull a face that sits behind a leaf: the leaf
	 * texture is alpha-tested, so its holes are see-through and culling the block
	 * behind them would punch a hole straight into the world.
	 */
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

	/** Spawns non-deterministic Chaos destruction fragments for the destroyed block */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Chaos")
	void SpawnChaosBlockDestruction(
		const FVector& VoxelCenter,
		uint8 BlockID,
		const FVector& HitNormal = FVector::UpVector,
		const FVector& HitPoint = FVector::ZeroVector
	);

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

	/**
	 * Change the render distance at runtime, Minecraft-style.
	 *
	 * The requested value is clamped to [MinRenderDistance, MaxRenderDistance], stored in
	 * GameUserSettings.ini so it survives a restart, and applied immediately: chunks inside
	 * the new radius are queued for generation, chunks outside it are saved and unloaded.
	 *
	 * Writes to disk, so call it when the player commits a setting (slider release / Apply),
	 * not every frame while dragging.
	 *
	 * @param NewRenderDistance Requested radius in chunks.
	 * @return The effective render distance after clamping.
	 */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Settings")
	int32 SetRenderDistance(int32 NewRenderDistance);

	/** Render distance in chunks currently in effect (clamped to the supported range). */
	UFUNCTION(BlueprintPure, Category = "World Generation|Settings")
	int32 GetRenderDistance() const { return RenderDistance; }

	/**
	 * Stores the leaf rendering preference.
	 *
	 * The value is persisted in GameUserSettings.ini but currently has no visual effect:
	 * the fast-leaf culling is disabled until leaves can be rendered genuinely opaque
	 * (see bFastLeaves). Calling this does not trigger a remesh.
	 *
	 * @param bNewFastLeaves True for fast leaves, false for fancy leaves.
	 * @return The effective value after the change.
	 */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Settings")
	bool SetFastLeaves(bool bNewFastLeaves);

	/** Current leaf rendering mode: true = fast leaves, false = fancy leaves. */
	UFUNCTION(BlueprintPure, Category = "World Generation|Settings")
	bool GetFastLeaves() const { return bFastLeaves; }

	/**
	 * Change the foliage wind strength at runtime.
	 *
	 * The requested value is clamped to [MinWindStrength, MaxWindStrength], stored in
	 * GameUserSettings.ini so it survives a restart, and applied immediately by writing the
	 * WindStrength scalar of MPC_World. No chunk is remeshed, so this is safe to call from a
	 * slider drag.
	 *
	 * @param NewWindStrength Requested multiplier; 0 freezes the leaves.
	 * @return The effective wind strength after clamping.
	 */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Settings")
	float SetWindStrength(float NewWindStrength);

	/** Wind strength multiplier currently in effect (clamped to the supported range). */
	UFUNCTION(BlueprintPure, Category = "World Generation|Settings")
	float GetWindStrength() const { return WindStrength; }

	// ==================== DIAGNOSTICS ====================

	/** Streaming work measured during the previous tick. Diagnostic only; see FVoxelFrameStats. */
	const FVoxelFrameStats& GetLastFrameStats() const { return LastFrameStats; }

	/** Number of chunks currently spawned and streamed in. */
	UFUNCTION(BlueprintPure, Category = "World Generation|Diagnostics")
	int32 GetActiveChunkCount() const { return ActiveChunks.Num(); }

	/** Number of chunks queued for generation that have not been spawned yet. */
	UFUNCTION(BlueprintPure, Category = "World Generation|Diagnostics")
	int32 GetPendingGenerationCount() const { return GenerationQueue.Num(); }

	/** Number of chunks waiting for a full remesh after a rendering setting change. */
	UFUNCTION(BlueprintPure, Category = "World Generation|Diagnostics")
	int32 GetPendingRemeshCount() const { return RemeshQueue.Num(); }

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

	/** Calculates the mining break time in seconds at the given voxel (Durability / Force). Falls back to 1.0 s. */
	UFUNCTION(BlueprintPure, Category = "Mining|Crack")
	float GetBlockBreakTimeAtVoxel(const FIntVector& VoxelCoord, float MiningForce = 0.0f) const;

	// ==================== MINING CRACK OVERLAY (Blueprint bridge) ====================

	/**
	 * Bridge called by the mining Blueprint (BP_HorrorCharacter) during block-breaking.
	 * Spawns (or reuses) a vanilla-style crack overlay at the given voxel and advances it
	 * to the stage matching Progress01 (0..1). The Blueprint owns the progress source of truth.
	 *
	 * Only one voxel is mined at a time, so any overlay left on another voxel is destroyed here.
	 * @param VoxelCoord Global voxel position of the targeted block.
	 * @param Progress01 Accumulated break time / total time needed, 0..1.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mining|Crack")
	void UpdateCrackMining(const FIntVector& VoxelCoord, float Progress01);

	/**
	 * Single-active-overlay rule: the overlay of ActiveVoxel survives, every other one is stale
	 * (the player changed target without releasing the input). Pure so it can be unit-tested.
	 */
	static bool ShouldKeepCrackOverlay(const FIntVector& OverlayVoxel, const FIntVector& ActiveVoxel);

	/** Destroys all current crack mining overlays (block broke, mining was cancelled, target lost). */
	UFUNCTION(BlueprintCallable, Category = "Mining|Crack")
	void ClearCrackMining();

	// ==================== MINING ANIMATION HOOKS ====================

	/** Fired once per attack on a valid block target. Hook for swing animations and per-hit dig sounds/particles. */
	UPROPERTY(BlueprintAssignable, Category = "Mining|Events")
	FOnToolStrike OnToolStrike;

	/** Fired when a voxel block finishes breaking. Mirrors UMiningQueueSystem::OnBlockMiningCompleted at world scope. */
	UPROPERTY(BlueprintAssignable, Category = "Mining|Events")
	FOnBlockMiningCompleted OnBlockMiningCompleted;

	/** Fired when a crack overlay advances to a new stage (0..9). */
	UPROPERTY(BlueprintAssignable, Category = "Mining|Events")
	FOnCrackStageChanged OnCrackStageChanged;

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

	/** Max chunk meshes rebuilt per streaming tick when a rendering setting changes them all. */
	static constexpr int32 MaxChunkRemeshesPerTick = 8;

	/** Chunks waiting for a full remesh after a setting that changes face culling. */
	TArray<FChunkCoord> RemeshQueue;

	TMap<uint8, FBlockTextureData> BlockTextureCache;
	TSet<uint8> TransparentBlockIDs;

	/** Work measured during the previous Tick; reset at the top of every Tick. */
	FVoxelFrameStats LastFrameStats;

	FString WorldSavePath;

	void InitializeBlockCache();

	/** Read the persisted render distance from GameUserSettings.ini, falling back to the current value. */
	void LoadRenderDistanceSetting();

	/** Write the current render distance to GameUserSettings.ini. */
	void SaveRenderDistanceSetting() const;

	/** Returns the durability of the block, read from Block_DataTable. Falls back to 1.0f if not found. */
	float GetBlockDurabilityFromTable(uint8 BlockID) const;

	/** Read the persisted leaf rendering mode from GameUserSettings.ini, falling back to the current value. */
	void LoadFastLeavesSetting();

	/** Write the current leaf rendering mode to GameUserSettings.ini. */
	void SaveFastLeavesSetting() const;

	/** Read the persisted wind strength from GameUserSettings.ini, falling back to the current value. */
	void LoadWindStrengthSetting();

	/** Write the current wind strength to GameUserSettings.ini. */
	void SaveWindStrengthSetting() const;

	/** Push the current wind strength into MPC_World so every material reading it follows. */
	void ApplyWindStrengthSetting() const;

	/** Queue every active chunk for a full remesh (used when face culling rules change). */
	void QueueFullRemesh();

	/** Rebuild a bounded number of queued chunk meshes per tick. */
	void ProcessRemeshQueue();

	void UpdateChunkStreaming();
	void ProcessGenerationQueue();

	AChunkActor* SpawnChunk(const FChunkCoord& Coord);
	void GenerateChunkData(AChunkActor* Chunk);
	void UpdateNeighborChunkMeshes(const FChunkCoord& CenterCoord);

	EBiomeType GetBiomeAt(int32 WorldX, int32 WorldY) const;

	/**
	 * Single source of truth for the biome classification fields. Both the hard biome
	 * lookup and the smooth cross-fade sample through here, so a texture fade can never
	 * drift away from the contour where the surface block actually flips.
	 */
	void SampleBiomeFields(int32 WorldX, int32 WorldY, float& OutContinental, float& OutTemp, float& OutMoisture) const;

	/** Noise-modulated half-width of the cross-fade band, so the fade distance varies organically. */
	float GetBiomeBlendBand(int32 WorldX, int32 WorldY) const;

	/** Hard biome classification from already-sampled fields (mirrors the historical thresholds). */
	EBiomeType ClassifyBiomeFields(float Continental, float Temp, float Moisture) const;

	/** Smooth desert factor from already-sampled fields. */
	float DesertBlendFromFields(int32 WorldX, int32 WorldY, float Temp, float Moisture) const;

	/**
	 * @param DesertBlend  Smooth desert factor from GetDesertBlendAt(); negative means
	 *                     "derive a hard 0/1 value from Biome" (legacy behaviour).
	 */
	int32 GetTerrainHeight(int32 WorldX, int32 WorldY, EBiomeType Biome, float DesertBlend = -1.0f) const;
	void GenerateTree(AChunkActor* Chunk, int32 LocalX, int32 LocalY, int32 SurfaceZ, EBiomeType Biome);
	void GenerateVillageHouse(AChunkActor* Chunk, int32 CenterX, int32 CenterY, int32 SurfaceZ);

	void SpawnBlockItemDrop(const FVector& WorldLocation, uint8 DroppedBlockID);

	/** Tracked strike normal and impact point for directional Chaos destruction debris */
	FVector LastTargetedHitNormal = FVector::UpVector;
	FVector LastTargetedHitPoint = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<ABlockHighlightActor> BlockHighlightActor = nullptr;

	/** Crack overlays mirroring the active mining interaction, keyed by voxel. Maintained by the Blueprint bridge. */
	TMap<FIntVector, TWeakObjectPtr<ABlockCrackActor>> BlockCrackOverlays;

	/** Spawns (or reuses) the crack overlay at a voxel coordinate. Returns null if spawn fails. */
	ABlockCrackActor* SpawnOrGetCrackOverlay(const FIntVector& VoxelCoord, uint8 BlockID);

	/** Forwards UMiningQueueSystem::OnBlockMiningCompleted onto the world-scope OnBlockMiningCompleted delegate. */
	UFUNCTION()
	void HandleBlockMiningCompleted(const FIntVector& VoxelCoord, uint8 BlockID);

	void UpdateTargetBlockHighlight();
	void EnsureDestroySystemConfigured();
	bool bDestroySystemConfigured = false;
	void EnsureDropItemBound();
	void HandleDropItemAction();
	bool bDropItemBound = false;

	// Persistence helpers
	FString GetChunkSaveFilePath(const FChunkCoord& Coord) const;
	void SaveChunkDelta(AChunkActor* Chunk);
	bool LoadChunkDelta(AChunkActor* Chunk);

	// Backward-compatibility mirror for active mining tasks
	UPROPERTY(Transient)
	TArray<FMiningTask> ActiveMiningTasks;
};
