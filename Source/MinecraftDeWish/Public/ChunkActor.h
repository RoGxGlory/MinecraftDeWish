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

	/** Chunk coordinates in grid space */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel")
	FChunkCoord ChunkCoord;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel")
	int32 ChunkHeight = DEFAULT_CHUNK_SIZE_Z;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel")
	float BlockScale = DEFAULT_BLOCK_SCALE;

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

	/** Builds mesh data into the provided buffer (thread-safe) */
	void GenerateMeshData(FChunkMeshData& OutMeshData) const;

	/** Applies generated mesh data to the procedural mesh component (must run on GameThread) */
	void ApplyMeshData(const FChunkMeshData& InMeshData);

	/** Rebuilds chunk mesh synchronously */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void UpdateMesh();

	void SetMaterial(UMaterialInterface* Material);

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TWeakObjectPtr<AWorldGenerator> WorldGenerator;

private:
	void AddFace(
		FChunkMeshData& MeshData,
		const FVector& BlockPos,
		EBlockFace Face,
		int32 TextureIndex
	) const;

	bool ShouldRenderFace(int32 NeighborX, int32 NeighborY, int32 NeighborZ) const;
};
