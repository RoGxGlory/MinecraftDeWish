#pragma once

#include "CoreMinimal.h"
#include "VoxelDataTypes.h"

/**
 * GC-free, cache-friendly chunk representation.
 *
 * This is the canonical data store for the voxel world (see ADR-0011). It holds
 * only plain data (no UObject), so it can be generated and meshed on worker
 * threads. AChunkActor becomes a thin render/collision view over an FChunkData
 * during the migration and is removed in lot 3.
 */
struct FChunkData
{
	FChunkCoord Coord;
	int32 Height = DEFAULT_CHUNK_SIZE_Z;
	float BlockScale = DEFAULT_BLOCK_SCALE;
	TArray<uint8> Blocks;          // flat, Size = CHUNK_SIZE_X * CHUNK_SIZE_Y * Height
	TArray<uint8> HeightMap;       // highest solid block Z per (X,Y) column, Size = CHUNK_SIZE_X * CHUNK_SIZE_Y
	TMap<int32, uint8> Metadata;   // sparse rotation/state
	bool bIsModified = false;

	FChunkData() = default;
	FChunkData(const FChunkCoord& InCoord, int32 InHeight, float InScale)
		: Coord(InCoord), Height(InHeight), BlockScale(InScale)
	{
		Blocks.SetNumZeroed(CHUNK_SIZE_X * CHUNK_SIZE_Y * Height);
		HeightMap.SetNumZeroed(CHUNK_SIZE_X * CHUNK_SIZE_Y);
	}

	FORCEINLINE int32 GetBlockIndex(int32 X, int32 Y, int32 Z) const
	{
		return X + (Y * CHUNK_SIZE_X) + (Z * CHUNK_SIZE_X * CHUNK_SIZE_Y);
	}

	FORCEINLINE bool IsValidCoord(int32 X, int32 Y, int32 Z) const
	{
		return X >= 0 && X < CHUNK_SIZE_X &&
		       Y >= 0 && Y < CHUNK_SIZE_Y &&
		       Z >= 0 && Z < Height;
	}

	uint8 GetBlock(int32 X, int32 Y, int32 Z) const;

	/** Player edit: writes the voxel and flags the chunk as modified. */
	void SetBlock(int32 X, int32 Y, int32 Z, uint8 BlockID);

	/** World-generation write: writes the voxel, leaves bIsModified untouched. */
	void SetBlockGenerated(int32 X, int32 Y, int32 Z, uint8 BlockID);

	/** World-generation write with metadata: writes voxel + meta, leaves bIsModified untouched. */
	void SetBlockWithMetaGenerated(int32 X, int32 Y, int32 Z, uint8 BlockID, uint8 MetaRotation);

	/** Rebuild height map from block data. */
	void RebuildHeightMap();

	void Reset();
};