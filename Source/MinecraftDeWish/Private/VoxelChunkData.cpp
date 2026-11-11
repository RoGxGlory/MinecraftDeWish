#include "VoxelChunkData.h"

uint8 FChunkData::GetBlock(int32 X, int32 Y, int32 Z) const
{
	if (!IsValidCoord(X, Y, Z))
	{
		return 0;
	}
	const int32 Index = GetBlockIndex(X, Y, Z);
	if (!Blocks.IsValidIndex(Index))
	{
		return 0;
	}
	return Blocks[Index];
}

void FChunkData::SetBlock(int32 X, int32 Y, int32 Z, uint8 BlockID)
{
	if (!IsValidCoord(X, Y, Z))
	{
		return;
	}
	const int32 Index = GetBlockIndex(X, Y, Z);
	if (!Blocks.IsValidIndex(Index))
	{
		return;
	}
	Blocks[Index] = BlockID;
	bIsModified = true;
}

void FChunkData::SetBlockGenerated(int32 X, int32 Y, int32 Z, uint8 BlockID)
{
	if (!IsValidCoord(X, Y, Z))
	{
		return;
	}
	const int32 Index = GetBlockIndex(X, Y, Z);
	if (!Blocks.IsValidIndex(Index))
	{
		return;
	}
	Blocks[Index] = BlockID;
	// bIsModified left untouched on purpose (ADR-0002).
}

void FChunkData::SetBlockWithMetaGenerated(int32 X, int32 Y, int32 Z, uint8 BlockID, uint8 MetaRotation)
{
	if (!IsValidCoord(X, Y, Z))
	{
		return;
	}
	const int32 Index = GetBlockIndex(X, Y, Z);
	if (!Blocks.IsValidIndex(Index))
	{
		return;
	}
	Blocks[Index] = BlockID;
	if (MetaRotation > 0)
	{
		Metadata.Add(Index, MetaRotation);
	}
	else
	{
		Metadata.Remove(Index);
	}
}

void FChunkData::RebuildHeightMap()
{
	if (HeightMap.Num() != CHUNK_SIZE_X * CHUNK_SIZE_Y)
	{
		HeightMap.SetNumZeroed(CHUNK_SIZE_X * CHUNK_SIZE_Y);
	}
	for (int32 Y = 0; Y < CHUNK_SIZE_Y; ++Y)
	{
		for (int32 X = 0; X < CHUNK_SIZE_X; ++X)
		{
			uint8 TopZ = 0;
			for (int32 Z = Height - 1; Z >= 0; --Z)
			{
				if (GetBlock(X, Y, Z) != 0)
				{
					TopZ = static_cast<uint8>(Z);
					break;
				}
			}
			HeightMap[Y * CHUNK_SIZE_X + X] = TopZ;
		}
	}
}

void FChunkData::Reset()
{
	Blocks.Reset();
	HeightMap.Reset();
	Metadata.Reset();
	bIsModified = false;
}