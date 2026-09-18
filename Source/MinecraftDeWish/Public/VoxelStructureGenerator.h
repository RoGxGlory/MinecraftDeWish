#pragma once

#include "CoreMinimal.h"
#include "VoxelDataTypes.h"
#include "ChunkActor.h"

/**
 * FVoxelStructureGenerator
 * 
 * Modular procedural structure generator responsible for:
 * 1. Tree generation (Trunk, complete leaves canopy on all sides).
 * 2. Village generation (Roads, cobblestone foundations, wood plank walls, door openings, torches placed on the ground).
 * 3. Flora & surface foliage (tall grass, flowers).
 */
class MINECRAFTDEWISH_API FVoxelStructureGenerator
{
public:
	/**
	 * Generates a tree with an oak wood log trunk and a full, rounded 3D leaf canopy.
	 * 
	 * @param Chunk The target chunk receiving the tree blocks.
	 * @param LocalX X-coordinate inside the chunk [0..15].
	 * @param LocalY Y-coordinate inside the chunk [0..15].
	 * @param SurfaceZ Z-coordinate of the surface grass/dirt block.
	 * @param ChunkHeight Maximum vertical height of the chunk.
	 * @param Biome The biome type determining tree appearance (e.g. standard forest vs horror forest).
	 */
	static void GenerateTree(
		AChunkActor* Chunk,
		int32 LocalX,
		int32 LocalY,
		int32 SurfaceZ,
		int32 ChunkHeight,
		EBiomeType Biome
	);

	/**
	 * Generates a procedural village house complete with:
	 * - Cobblestone foundation & walkway
	 * - Wood log corners and wood plank walls
	 * - Glass windows and doorway
	 * - Cobblestone roof
	 * - Interior Crafting Table, Furnace, and ground-aligned Torch
	 * 
	 * @param Chunk The target chunk receiving the house blocks.
	 * @param CenterX Central X-coordinate inside the chunk.
	 * @param CenterY Central Y-coordinate inside the chunk.
	 * @param SurfaceZ Ground level Z-coordinate.
	 * @param ChunkHeight Maximum vertical height of the chunk.
	 */
	static void GenerateVillageHouse(
		AChunkActor* Chunk,
		int32 CenterX,
		int32 CenterY,
		int32 SurfaceZ,
		int32 ChunkHeight
	);
};
