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

	/** Flat block array (Size: 16 * 16 * ChunkHeight) */
	TArray<uint8> BlockData;

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
	void SetBlock(int32 X, int32 Y, int32 Z, uint8 BlockID);

	/** Builds mesh data into the provided buffers (thread-safe) */
	void GenerateMeshData(FChunkMeshData& OutSolidMeshData, FChunkMeshData& OutNonSolidMeshData) const;

	/** Applies generated mesh data to the procedural mesh component (must run on GameThread) */
	void ApplyMeshData(const FChunkMeshData& InSolidMeshData, const FChunkMeshData& InNonSolidMeshData);

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
		int32 TextureIndex
	) const;

	void AddTorchMesh(
		FChunkMeshData& MeshData,
		const FVector& BlockPos,
		int32 TextureIndex
	) const;

	bool ShouldRenderFace(uint8 CurrentBlockID, int32 NeighborX, int32 NeighborY, int32 NeighborZ) const;
};
