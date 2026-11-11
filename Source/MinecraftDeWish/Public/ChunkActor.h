#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelDataTypes.h"
#include "ProceduralMeshComponent.h"
#include "ChunkActor.generated.h"

class AWorldGenerator;

/** Mesh buffer generated asynchronously or synchronously */
struct FChunkMeshData
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FVector2D> UV1; // UV1.X stores Texture2DArray Slice Index
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	void Reset()
	{
		Vertices.Reset();
		Triangles.Reset();
		Normals.Reset();
		UV0.Reset();
		UV1.Reset();
		VertexColors.Reset();
		Tangents.Reset();
	}

	bool IsEmpty() const
	{
		return Vertices.Num() == 0;
	}
};

UCLASS()
class MINECRAFTDEWISH_API AChunkActor : public AActor
{
	GENERATED_BODY()

public:
	AChunkActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProceduralMeshComponent* ProceduralMesh;

	/** Separate mesh for non-solid blocks (torches, seeds) — QueryOnly collision (no physics blocking) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProceduralMeshComponent* NonSolidMesh;

	/** Chunk coordinates in grid space */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel")
	FChunkCoord ChunkCoord;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel")
	int32 ChunkHeight = DEFAULT_CHUNK_SIZE_Z;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel")
	float BlockScale = DEFAULT_BLOCK_SCALE;

	/** Fallback master terrain material ensuring cook dependency and standalone build validity */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel")
	UMaterialInterface* DefaultTerrainMaterial = nullptr;

	/** True if blocks in this chunk were modified by the player */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel")
	bool bIsModified = false;

	/** Randomly phase-shift isotropic top-face UVs per block so the repeated texture grid is no longer visible */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Rendering")
	bool bStochasticTopUVs = true;

	/** Enables biome cross-fading of top-face textures (blend data comes from the WorldGenerator) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Rendering")
	bool bEnableTopTextureBlending = true;

	/** Flat block array (Size: 16 * 16 * ChunkHeight) */
	TArray<uint8> BlockData;

	/** Height map: highest solid block Z for each (X, Y) column (Size: 16 * 16). */
	TArray<uint8> HeightMap;

	/**
	 * Diagnostic mask recording which `ECaveArchetype` carved each voxel (Size: 16 * 16 * ChunkHeight).
	 *
	 * Used by the spawner weighting (`cave-generation.md` §3.12) and by the probes. It is **never
	 * persisted**: the archetype is a pure function of the world coordinates, so it can always be
	 * recomputed. The array stays empty unless `AWorldGenerator::bEnableCaveTagging` is on, which
	 * keeps the shipping memory cost at zero.
	 */
	TArray<uint8> CaveTag;

	/**
	 * Temporary buffer used during async generation to avoid data races.
	 * The generation worker writes here; the game thread copies to BlockData at apply time.
	 */
	TArray<uint8> PendingBlockData;

	/** True when PendingBlockData holds freshly generated data not yet copied to BlockData. */
	bool bHasPendingData = false;

	/**
	 * Temporary mesh buffers used during async meshing.
	 * The meshing worker writes here; the game thread applies to the PMC at apply time.
	 */
	FChunkMeshData PendingSolidMeshData;
	FChunkMeshData PendingNonSolidMeshData;
	bool bHasPendingMesh = false;

	void InitializeChunk(const FChunkCoord& InCoord, int32 InHeight, float InScale, AWorldGenerator* InWorldGen);

	FORCEINLINE int32 GetBlockIndex(int32 X, int32 Y, int32 Z) const
	{
		return X + (Y * CHUNK_SIZE_X) + (Z * CHUNK_SIZE_X * CHUNK_SIZE_Y);
	}

	FORCEINLINE bool IsValidCoord(int32 X, int32 Y, int32 Z) const
	{
		return X >= 0 && X < CHUNK_SIZE_X &&
		       Y >= 0 && Y < CHUNK_SIZE_Y &&
		       Z >= 0 && Z < ChunkHeight;
	}

	uint8 GetBlock(int32 X, int32 Y, int32 Z) const;

	/**
	 * Attachment / rotation metadata of a voxel, or 0 when it has none.
	 *
	 * A torch stores its mount in `BlockMetadata` (`FPlacementRules::EncodeAttachMeta`): 0 is the
	 * floor mount, 1..4 are the wall mounts. `SetBlockWithMeta(..., 0)` removes the entry, so a
	 * missing entry and a floor mount are the same thing and read back as 0.
	 */
	uint8 GetBlockMeta(int32 X, int32 Y, int32 Z) const;

	/**
	 * Records the archetype that carved a voxel (diagnostic only, see `CaveTag`).
	 *
	 * Allocates the mask on first use so a generator with tagging disabled never pays for it.
	 */
	void SetCaveTag(int32 X, int32 Y, int32 Z, uint8 Archetype);

	/** Archetype that carved a voxel, or 0 (`ECaveArchetype::None`) when untagged or empty. */
	uint8 GetCaveTag(int32 X, int32 Y, int32 Z) const;

	/** Player edit. Writes the voxel and flags the chunk as modified so it is persisted. */
	void SetBlock(int32 X, int32 Y, int32 Z, uint8 BlockID);

	/**
	 * World-generation write. Writes to PendingBlockData to avoid data races with the
	 * game thread. The data is copied to BlockData at apply time.
	 */
	void SetBlockGenerated(int32 X, int32 Y, int32 Z, uint8 BlockID);

	/** Builds mesh data into the provided buffers (thread-safe, const) */
	void GenerateMeshData(FChunkMeshData& OutSolidMeshData, FChunkMeshData& OutNonSolidMeshData) const;

	/** Applies generated mesh data to the procedural mesh component (must run on GameThread) */
	void ApplyMeshData(const FChunkMeshData& InSolidMeshData, const FChunkMeshData& InNonSolidMeshData);

	/**
	 * Copy PendingBlockData to BlockData (game thread only).
	 * Called when async generation completes.
	 */
	void ApplyPendingData();

	/**
	 * Apply pending mesh data to the PMC (game thread only).
	 * Called when async meshing completes.
	 */
	void ApplyPendingMesh();

	/** Rebuilds chunk mesh synchronously */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void UpdateMesh();

	void SetMaterial(UMaterialInterface* Material);

	/** Calculates which face of a voxel was hit (0=Top, 1=Bottom, 2=North, 3=South, 4=East, 5=West) */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	int32 GetHitFaceAtLocation(const FVector& HitLocation) const;

	/** Identifies voxel at HitLocation and breaks it, updating mesh and spawning item pickup */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	bool BreakBlockAtLocation(const FVector& HitLocation, uint8& OutBrokenBlockID);

	/** Retrieves BlockID and world center of the block at HitLocation */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	bool GetBlockDataAtLocation(const FVector& HitLocation, uint8& OutBlockID, FVector& OutBlockCenter) const;

	/** Returns the Row Name in Block_DataTable for the currently targeted block (e.g. "Grass", "Dirt", "Stone") */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	FName GetTargetedBlockRowName() const;

	/** Breaks the currently targeted voxel block, spawning its pickup item and updating meshes */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void BreakTargetedBlock();

	/** Gets currently targeted block ID */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	uint8 GetTargetedBlockID() const;

	/** Gets currently targeted block durability from Block_DataTable for mining calculation */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	float GetTargetedBlockDurability() const;

	/** Calculates the exact mining break time in seconds based on block durability and tool/hand mining force (Durability / Force) */
	UFUNCTION(BlueprintPure, Category = "Voxel|Mining")
	float GetTargetedBlockBreakTime(float MiningForce = 0.0f) const;

	/** Returns the mining force of the best equipped tool for the targeted block, or base hand force (1.0) if none equipped */
	UFUNCTION(BlueprintPure, Category = "Voxel|Mining")
	float GetCurrentBestToolMiningForce() const;

	/** Base hand mining force when no tools are equipped (always 1.0) */
	UFUNCTION(BlueprintPure, Category = "Voxel|Mining")
	static float GetHandMiningForce();

	/** Calculates timeline play rate based on mining force and durability */
	UFUNCTION(BlueprintPure, Category = "Voxel|Mining")
	float GetTargetedBlockPlayRate(float MiningForce = 0.0f, float TimelineLength = 1.0f) const;

	/** Returns the 2D icon texture for a given BlockID to display in WB_BlockInfo or UI slots */
	UFUNCTION(BlueprintPure, Category = "Voxel|UI")
	static UTexture2D* GetBlockIconTexture(int32 InBlockID);

	/** Returns the 2D icon texture for a given Global_Textures_Array slice index */
	UFUNCTION(BlueprintPure, Category = "Voxel|UI")
	static UTexture2D* GetBlockIconFromSlice(int32 TextureSliceIndex);

	UPROPERTY()
	FVector LastTargetedHitLocation = FVector::ZeroVector;

	UPROPERTY()
	FIntVector LastTargetedVoxelCoord = FIntVector(-1, -1, -1);

	/**
	 * True when LastTargetedVoxelCoord holds a real target.
	 * World voxel X and Y are signed coordinates: a negative component is a perfectly valid
	 * position, so it must never be used as the "no target" sentinel (that bug made every
	 * block and leaf at X < 0 unbreakable).
	 */
	UPROPERTY()
	bool bHasTargetedVoxel = false;

	/** Global static helper to get the targeted block ID from any player controller camera without requiring a chunk reference */
	UFUNCTION(BlueprintPure, Category = "Voxel|Mining", meta = (WorldContext = "WorldContextObject"))
	static uint8 GetGlobalTargetedBlockID(UObject* WorldContextObject, APlayerController* PlayerController);

	/** Global static helper to get the targeted voxel world coordinate (X, Y, Z) from any player controller camera */
	UFUNCTION(BlueprintCallable, Category = "Voxel|Mining", meta = (WorldContext = "WorldContextObject"))
	static FIntVector GetGlobalTargetedBlockVoxel(UObject* WorldContextObject, APlayerController* PlayerController);

	/** Sparse map storing rotation/state data only for blocks that need it (Key: BlockIndex, Value: Meta) */
	UPROPERTY()
	TMap<int32, uint8> BlockMetadata;

	/** Player edit with rotation/state data. Flags the chunk as modified. */
	void SetBlockWithMeta(int32 X, int32 Y, int32 Z, uint8 BlockID, uint8 MetaRotation);

	/** World-generation counterpart of SetBlockWithMeta(); leaves bIsModified untouched. */
	void SetBlockWithMetaGenerated(int32 X, int32 Y, int32 Z, uint8 BlockID, uint8 MetaRotation);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	TWeakObjectPtr<AWorldGenerator> WorldGenerator;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class UPointLightComponent>> TorchLights;

	void UpdateTorchLights();
	void ClearTorchLights();

private:
	void AddFace(
		FChunkMeshData& MeshData,
		const FVector& BlockPos,
		EBlockFace Face,
		uint8 BlockID,
		int32 TextureIndex,
		int32 SecondaryTextureIndex = -1,
		const FVector4& CornerBlendWeights = FVector4(0.0, 0.0, 0.0, 0.0)
	) const;

	void AddTorchMesh(
		FChunkMeshData& MeshData,
		const FVector& BlockPos,
		int32 TextureIndex
	) const;

	bool ShouldRenderFace(uint8 CurrentBlockID, int32 NeighborX, int32 NeighborY, int32 NeighborZ) const;
};