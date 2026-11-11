#include "VoxelPlacementRules.h"

namespace PlacementRulesConstants
{
	/** Highest block ID that currently exists in `EBlockType` (Coal_Ore). */
	constexpr uint8 HighestBlockID = static_cast<uint8>(EBlockType::Coal_Ore);
}

const TArray<FIntVector>& FPlacementRules::FaceOffsets()
{
	// D6, ordered +X, -X, +Y, -Y, +Z, -Z so that index order is stable for tests and logs.
	static const TArray<FIntVector> Offsets = {
		FIntVector(1, 0, 0),
		FIntVector(-1, 0, 0),
		FIntVector(0, 1, 0),
		FIntVector(0, -1, 0),
		FIntVector(0, 0, 1),
		FIntVector(0, 0, -1)
	};
	return Offsets;
}

FIntVector FPlacementRules::AxisStep(const FVector& Normal)
{
	// The dominant axis wins. Ties are broken in X > Y > Z order so the function stays a pure
	// function even for a 45-degree normal (which a cube face never produces).
	const float AX = FMath::Abs(Normal.X);
	const float AY = FMath::Abs(Normal.Y);
	const float AZ = FMath::Abs(Normal.Z);

	if (AX >= AY && AX >= AZ)
	{
		return FIntVector(Normal.X >= 0.0 ? 1 : -1, 0, 0);
	}
	if (AY >= AZ)
	{
		return FIntVector(0, Normal.Y >= 0.0 ? 1 : -1, 0);
	}
	return FIntVector(0, 0, Normal.Z >= 0.0 ? 1 : -1);
}

bool FPlacementRules::IsPlaceableBlock(uint8 BlockID)
{
	if (BlockID == static_cast<uint8>(EBlockType::Air)
		|| BlockID == static_cast<uint8>(EBlockType::Bedrock)
		|| BlockID == static_cast<uint8>(EBlockType::Water))
	{
		return false;
	}

	// Unknown IDs (past the last enum entry, or holes in the table) are never placeable: the
	// registry is the single source of truth for what a player may hold and place.
	return BlockID <= PlacementRulesConstants::HighestBlockID;
}

bool FPlacementRules::IsReplaceableBlock(uint8 BlockID)
{
	// §3.7: only empty space, fluids and the non-solid decorative torch can be overwritten.
	return BlockID == static_cast<uint8>(EBlockType::Air)
		|| BlockID == static_cast<uint8>(EBlockType::Water)
		|| BlockID == static_cast<uint8>(EBlockType::Torch);
}

bool FPlacementRules::IsSupportBlock(uint8 BlockID)
{
	// §3.4 / §4.4: Air carries nothing, Water has QueryOnly collision, Torch is non-solid.
	if (BlockID == static_cast<uint8>(EBlockType::Air)
		|| BlockID == static_cast<uint8>(EBlockType::Water)
		|| FBlockHelpers::IsNonSolidBlock(BlockID))
	{
		return false;
	}

	return BlockID <= PlacementRulesConstants::HighestBlockID;
}

bool FPlacementRules::HasSupportFromNeighbours(const FIntVector& Voxel, const TFunctionRef<uint8(const FIntVector&)>& BlockLookup)
{
	for (const FIntVector& Offset : FaceOffsets())
	{
		if (IsSupportBlock(BlockLookup(Voxel + Offset)))
		{
			return true;
		}
	}
	return false;
}

bool FPlacementRules::IsAttachDirectionAllowed(const FIntVector& AttachDir, bool bAllowWall, bool bAllowCeiling)
{
	if (AttachDir == FIntVector(0, 0, -1))
	{
		// Floor mount: always allowed, this is the v1 torch.
		return true;
	}

	if (AttachDir == FIntVector(0, 0, 1))
	{
		// Ceiling mount renders as a floor torch until `AddTorchMesh` honours Meta.
		return bAllowCeiling;
	}

	if (AttachDir.X != 0 || AttachDir.Y != 0)
	{
		return bAllowWall;
	}

	// Not a unit face direction at all.
	return false;
}

uint8 FPlacementRules::EncodeAttachMeta(const FIntVector& AttachDir)
{
	// §4.5: floor = 0, walls = 1..4 in the order +X, -X, +Y, -Y. Ceiling has no encoding.
	if (AttachDir == FIntVector(0, 0, -1))
	{
		return 0;
	}
	if (AttachDir == FIntVector(1, 0, 0)) { return 1; }
	if (AttachDir == FIntVector(-1, 0, 0)) { return 2; }
	if (AttachDir == FIntVector(0, 1, 0)) { return 3; }
	if (AttachDir == FIntVector(0, -1, 0)) { return 4; }

	// Unknown or ceiling attachment: fall back to the floor mount, which is what the mesh
	// renders anyway. Never return a meta the renderer cannot interpret.
	return 0;
}

FIntVector FPlacementRules::DecodeAttachMeta(uint8 Meta)
{
	switch (Meta)
	{
	case 1: return FIntVector(1, 0, 0);
	case 2: return FIntVector(-1, 0, 0);
	case 3: return FIntVector(0, 1, 0);
	case 4: return FIntVector(0, -1, 0);
	default: return FIntVector(0, 0, -1);
	}
}

float FPlacementRules::VerticalSegmentBoxDistanceSquared(const FVector& SegmentCenter, float SegmentHalfLen, const FBox& Box)
{
	const float HalfLen = FMath::Max(0.0f, SegmentHalfLen);

	// The segment is axis-aligned on Z, so the three axes decouple into interval distances.
	const float DX = FMath::Max(FMath::Max(Box.Min.X - SegmentCenter.X, 0.0f), SegmentCenter.X - Box.Max.X);
	const float DY = FMath::Max(FMath::Max(Box.Min.Y - SegmentCenter.Y, 0.0f), SegmentCenter.Y - Box.Max.Y);

	const float SegmentMinZ = SegmentCenter.Z - HalfLen;
	const float SegmentMaxZ = SegmentCenter.Z + HalfLen;
	const float DZ = FMath::Max(FMath::Max(Box.Min.Z - SegmentMaxZ, 0.0f), SegmentMinZ - Box.Max.Z);

	return DX * DX + DY * DY + DZ * DZ;
}

bool FPlacementRules::VoxelOverlapsCapsule(
	const FIntVector& Voxel,
	float BlockScale,
	const FVector& CapsuleCenter,
	float CapsuleRadius,
	float CapsuleHalfHeight,
	float Skin)
{
	const float Scale = BlockScale > 0.0f ? BlockScale : DEFAULT_BLOCK_SCALE;

	const FVector BoxMin(Voxel.X * Scale, Voxel.Y * Scale, Voxel.Z * Scale);
	const FVector BoxMax(BoxMin.X + Scale, BoxMin.Y + Scale, BoxMin.Z + Scale);
	const FBox Box(BoxMin, BoxMax);

	// §4.6: the capsule is a vertical segment swept by Radius; the sphere at each end means
	// the segment can be shortened by Radius without changing the swept volume.
	const float SegmentHalfLen = FMath::Max(0.0f, CapsuleHalfHeight - CapsuleRadius);
	const float DistanceSq = VerticalSegmentBoxDistanceSquared(CapsuleCenter, SegmentHalfLen, Box);
	const float Limit = FMath::Max(0.0f, CapsuleRadius + Skin);

	return DistanceSq < Limit * Limit;
}

int32 FPlacementRules::GetPlaceNoiseLoudness(uint8 BlockID, int32 DefaultLoudness, int32 TorchLoudness)
{
	// §4.8: a torch is a quiet claim on a space; a full block is a construction noise.
	return BlockID == static_cast<uint8>(EBlockType::Torch) ? TorchLoudness : DefaultLoudness;
}

FName FPlacementRules::GetPlaceMaterialName(uint8 BlockID)
{
	// §3.11: the cue is `SFX_BlockPlace_<Material>` with five material families. Unknown IDs
	// resolve to Stone so a new block never plays silence by accident.
	switch (static_cast<EBlockType>(BlockID))
	{
	case EBlockType::Dirt:
	case EBlockType::Grass:
		return FName(TEXT("Dirt"));

	case EBlockType::Sand:
		return FName(TEXT("Sand"));

	case EBlockType::Glass:
		return FName(TEXT("Glass"));

	case EBlockType::Wood_Log:
	case EBlockType::Wood_Planks:
	case EBlockType::Crafting_Table:
	case EBlockType::Fence:
	case EBlockType::Fence_Door:
	case EBlockType::Door:
	case EBlockType::Barrel:
		return FName(TEXT("Wood"));

	default:
		return FName(TEXT("Stone"));
	}
}

const TCHAR* FPlacementRules::ToString(EPlaceBlockResult Result)
{
	switch (Result)
	{
	case EPlaceBlockResult::Success:       return TEXT("Success");
	case EPlaceBlockResult::NoTarget:      return TEXT("NoTarget");
	case EPlaceBlockResult::OutOfReach:    return TEXT("OutOfReach");
	case EPlaceBlockResult::NotPlaceable:  return TEXT("NotPlaceable");
	case EPlaceBlockResult::NoItem:        return TEXT("NoItem");
	case EPlaceBlockResult::ItemMismatch:  return TEXT("ItemMismatch");
	case EPlaceBlockResult::Cooldown:      return TEXT("Cooldown");
	case EPlaceBlockResult::OutOfBounds:   return TEXT("OutOfBounds");
	case EPlaceBlockResult::ChunkUnloaded: return TEXT("ChunkUnloaded");
	case EPlaceBlockResult::Occupied:      return TEXT("Occupied");
	case EPlaceBlockResult::NoSupport:     return TEXT("NoSupport");
	case EPlaceBlockResult::BadAttach:     return TEXT("BadAttach");
	case EPlaceBlockResult::BlockedByPawn: return TEXT("BlockedByPawn");
	default:                               return TEXT("Unknown");
	}
}
