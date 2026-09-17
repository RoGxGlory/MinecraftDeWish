#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "VoxelDataTypes.generated.h"

// Chunk dimensions
constexpr int32 CHUNK_SIZE_X = 16;
constexpr int32 CHUNK_SIZE_Y = 16;
constexpr int32 DEFAULT_CHUNK_SIZE_Z = 64;
constexpr float DEFAULT_BLOCK_SCALE = 100.0f; // 100cm = 1m in Unreal Engine

/** 2D Chunk Coordinate */
USTRUCT(BlueprintType)
struct MINECRAFTDEWISH_API FChunkCoord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	int32 Y = 0;

	FChunkCoord() : X(0), Y(0) {}
	FChunkCoord(int32 InX, int32 InY) : X(InX), Y(InY) {}

	bool operator==(const FChunkCoord& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}

	bool operator!=(const FChunkCoord& Other) const
	{
		return !(*this == Other);
	}

	friend uint32 GetTypeHash(const FChunkCoord& Coord)
	{
		return HashCombine(GetTypeHash(Coord.X), GetTypeHash(Coord.Y));
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("(%d, %d)"), X, Y);
	}
};

/** Block identifiers matching Block_DataTable */
UENUM(BlueprintType)
enum class EBlockType : uint8
{
	Air             = 0 UMETA(DisplayName = "Air"),
	Dirt            = 1 UMETA(DisplayName = "Dirt"),
	Grass           = 2 UMETA(DisplayName = "Grass"),
	Cobblestone     = 3 UMETA(DisplayName = "Cobblestone"),
	Stone           = 4 UMETA(DisplayName = "Stone"),
	Wood_Log        = 5 UMETA(DisplayName = "Wood Log"),
	Wood_Planks     = 6 UMETA(DisplayName = "Wood Planks"),
	Iron_Ore        = 7 UMETA(DisplayName = "Iron Ore"),
	Iron_Block      = 8 UMETA(DisplayName = "Iron Block"),
	Gold_Ore        = 9 UMETA(DisplayName = "Gold Ore"),
	Gold_Block      = 10 UMETA(DisplayName = "Gold Block"),
	Diamond_Ore     = 11 UMETA(DisplayName = "Diamond Ore"),
	Diamond_Block   = 12 UMETA(DisplayName = "Diamond Block"),
	Emerald_Ore     = 13 UMETA(DisplayName = "Emerald Ore"),
	Emerald_Block   = 14 UMETA(DisplayName = "Emerald Block"),
	Crafting_Table  = 15 UMETA(DisplayName = "Crafting Table"),
	Furnace         = 16 UMETA(DisplayName = "Furnace"),
	Leaves          = 17 UMETA(DisplayName = "Leaves"),
	Sand            = 18 UMETA(DisplayName = "Sand"),
	Torch           = 19 UMETA(DisplayName = "Torch"),
	Barrel          = 20 UMETA(DisplayName = "Barrel"),
	Glass           = 21 UMETA(DisplayName = "Glass"),
	Fence           = 22 UMETA(DisplayName = "Fence"),
	Fence_Door      = 23 UMETA(DisplayName = "Fence Door"),
	Door            = 24 UMETA(DisplayName = "Door"),
	Bedrock         = 25 UMETA(DisplayName = "Bedrock"),
	Water           = 26 UMETA(DisplayName = "Water"),
	Coal_Ore        = 27 UMETA(DisplayName = "Coal Ore")
};

/** Directional face of a voxel cube */
UENUM(BlueprintType)
enum class EBlockFace : uint8
{
	Top    = 0, // +Z
	Bottom = 1, // -Z
	North  = 2, // +X
	South  = 3, // -X
	East   = 4, // +Y
	West   = 5  // -Y
};

/** Biome Types */
UENUM(BlueprintType)
enum class EBiomeType : uint8
{
	Plains       = 0 UMETA(DisplayName = "Plains"),
	Forest       = 1 UMETA(DisplayName = "Forest"),
	Mountains    = 2 UMETA(DisplayName = "Mountains"),
	Desert       = 3 UMETA(DisplayName = "Desert"),
	HorrorForest = 4 UMETA(DisplayName = "Horror Forest")
};

/** Texture indices for each face of a block (indexes into Texture2DArray) */
USTRUCT(BlueprintType)
struct MINECRAFTDEWISH_API FBlockTextureData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	int32 TopTexture = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	int32 BottomTexture = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	int32 SideTexture = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	int32 FrontTexture = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	bool bIsTransparent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	bool bHasCollision = true;

	FBlockTextureData()
		: TopTexture(0), BottomTexture(0), SideTexture(0), FrontTexture(0), bIsTransparent(false), bHasCollision(true) {}

	int32 GetTextureForFace(EBlockFace Face) const
	{
		switch (Face)
		{
		case EBlockFace::Top:    return TopTexture;
		case EBlockFace::Bottom: return BottomTexture;
		case EBlockFace::North:  return FrontTexture;
		default:                 return SideTexture;
		}
	}
};

/** Structure for serializing modified blocks inside a chunk */
USTRUCT()
struct FChunkDeltaSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 ChunkX = 0;

	UPROPERTY()
	int32 ChunkY = 0;

	// Local block index -> BlockID
	UPROPERTY()
	TMap<int32, uint8> ModifiedBlocks;
};
