#include "VoxelStructureGenerator.h"

void FVoxelStructureGenerator::GenerateTree(
	AChunkActor* Chunk,
	int32 LocalX,
	int32 LocalY,
	int32 SurfaceZ,
	int32 ChunkHeight,
	EBiomeType Biome
)
{
	if (!Chunk)
	{
		return;
	}

	const int32 TrunkHeight = (Biome == EBiomeType::HorrorForest) ? 6 : 5;

	// 1. Trunk (Oak Wood Logs)
	for (int32 Z = 1; Z <= TrunkHeight; ++Z)
	{
		const int32 BlockZ = SurfaceZ + Z;
		if (BlockZ < ChunkHeight)
		{
			Chunk->SetBlock(LocalX, LocalY, BlockZ, static_cast<uint8>(EBlockType::Wood_Log));
		}
	}

	// 2. Full 3D Leaves Canopy for standard forest / plains
	if (Biome != EBiomeType::HorrorForest)
	{
		const int32 LeafStartZ = SurfaceZ + TrunkHeight - 2;
		const int32 LeafEndZ = SurfaceZ + TrunkHeight + 1;

		for (int32 Z = LeafStartZ; Z <= LeafEndZ; ++Z)
		{
			if (Z >= ChunkHeight) break;

			// Radius 2 on lower layers, Radius 1 on top layer
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

void FVoxelStructureGenerator::GenerateVillageHouse(
	AChunkActor* Chunk,
	int32 CenterX,
	int32 CenterY,
	int32 SurfaceZ,
	int32 ChunkHeight
)
{
	if (!Chunk)
	{
		return;
	}

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

	// 4. Interior Furniture: Crafting Table, Furnace, Torch (ground placed)
	const int32 FloorZ = SurfaceZ + 1;
	if (Chunk->IsValidCoord(MinX + 1, MaxY - 1, FloorZ))
	{
		Chunk->SetBlock(MinX + 1, MaxY - 1, FloorZ, static_cast<uint8>(EBlockType::Crafting_Table));
	}
	if (Chunk->IsValidCoord(MaxX - 1, MaxY - 1, FloorZ))
	{
		Chunk->SetBlock(MaxX - 1, MaxY - 1, FloorZ, static_cast<uint8>(EBlockType::Furnace));
	}
	// Torch placed directly on the floor
	if (Chunk->IsValidCoord(CenterX, CenterY, FloorZ))
	{
		Chunk->SetBlock(CenterX, CenterY, FloorZ, static_cast<uint8>(EBlockType::Torch));
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
