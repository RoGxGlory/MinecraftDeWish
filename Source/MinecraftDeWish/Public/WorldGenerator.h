#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelDataTypes.h"
#include "VoxelNoise.h"
#include "WorldGenerator.generated.h"

class AChunkActor;

UCLASS()
class MINECRAFTDEWISH_API AWorldGenerator : public AActor
{
	GENERATED_BODY()

public:
	AWorldGenerator();

	/** Procedural generation seed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings")
	int32 WorldSeed = 1337;

	/** Render distance in chunks (radius) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings", meta = (ClampMin = "2", ClampMax = "16"))
	int32 RenderDistance = 6;

	/** Height of each chunk in blocks (default 64) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings", meta = (ClampMin = "32", ClampMax = "256"))
	int32 ChunkHeight = DEFAULT_CHUNK_SIZE_Z;

	/** Scale of each voxel block in Unreal units (default 100cm = 1m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Settings")
	float BlockScale = DEFAULT_BLOCK_SCALE;

	/** Master material with Texture2DArray (M_Global) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Rendering")
	UMaterialInterface* TerrainMaterial = nullptr;

	/** Data table containing block definitions (Block_DataTable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Data")
	UDataTable* BlockDataTable = nullptr;

	/** Enable 3D cavern generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Features")
	bool bEnableCaves = true;

	/** Enable ore vein generation (Coal, Iron, Gold, Diamond, Emerald) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Features")
	bool bEnableOres = true;

	/** Enable biome-specific trees and foliage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Features")
	bool bEnableTrees = true;

	/** Frequency for base terrain noise */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Noise")
	float TerrainFrequency = 0.0035f;

	/** Maximum chunk meshes to build per frame to avoid hitches */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Performance")
	int32 MaxChunkGenerationsPerFrame = 2;

	// --- Block & Voxel Queries ---

	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	int32 GetTextureForBlock(uint8 BlockID, EBlockFace Face) const;

	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	bool IsBlockTransparent(uint8 BlockID) const;

	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	int32 GetPredictedTerrainHeight(int32 WorldX, int32 WorldY) const;

	/** Query voxel at global block coordinates */
	bool GetVoxelAt(int32 WorldBlockX, int32 WorldBlockY, int32 WorldBlockZ, uint8& OutBlockID) const;

	UFUNCTION(BlueprintCallable, Category = "World Generation|Interaction")
	bool GetBlockAtWorldLocation(const FVector& WorldLocation, uint8& OutBlockID) const;

	/** Break block at world location, returning dropped block ID */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Interaction")
	bool BreakBlock(const FVector& WorldLocation, uint8& OutDroppedBlockID);

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

	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	FChunkCoord WorldLocationToChunkCoord(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "World Generation|Voxel")
	void WorldLocationToVoxelCoord(const FVector& WorldLocation, int32& OutX, int32& OutY, int32& OutZ) const;

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

	// Persistence helpers
	FString GetChunkSaveFilePath(const FChunkCoord& Coord) const;
	void SaveChunkDelta(AChunkActor* Chunk);
	bool LoadChunkDelta(AChunkActor* Chunk);
};
