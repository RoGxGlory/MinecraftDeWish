#include "BlockPlacementLibrary.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "WorldGenerator.h"

AWorldGenerator* UBlockPlacementLibrary::GetWorldGenerator(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AWorldGenerator> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (AWorldGenerator* Generator = *It)
		{
			return Generator;
		}
	}

	return nullptr;
}

FIntVector UBlockPlacementLibrary::AxisStep(const FVector& ImpactNormal)
{
	return FPlacementRules::AxisStep(ImpactNormal);
}

FIntVector UBlockPlacementLibrary::ResolveBreakVoxel(const FVector& ImpactPoint, const FVector& ImpactNormal, float BlockScale)
{
	const float Scale = BlockScale > 0.0f ? BlockScale : DEFAULT_BLOCK_SCALE;
	const FVector Sample = ImpactPoint - ImpactNormal * (Scale * 0.25f);
	return FIntVector(
		FMath::FloorToInt(Sample.X / Scale),
		FMath::FloorToInt(Sample.Y / Scale),
		FMath::FloorToInt(Sample.Z / Scale));
}

FIntVector UBlockPlacementLibrary::ResolvePlacementVoxel(const FVector& ImpactPoint, const FVector& ImpactNormal, float BlockScale)
{
	return ResolveBreakVoxel(ImpactPoint, ImpactNormal, BlockScale) + FPlacementRules::AxisStep(ImpactNormal);
}

FVector UBlockPlacementLibrary::VoxelCenterToWorld(const FIntVector& Voxel, float BlockScale)
{
	const float Scale = BlockScale > 0.0f ? BlockScale : DEFAULT_BLOCK_SCALE;
	return FVector(
		(static_cast<float>(Voxel.X) + 0.5f) * Scale,
		(static_cast<float>(Voxel.Y) + 0.5f) * Scale,
		(static_cast<float>(Voxel.Z) + 0.5f) * Scale);
}

bool UBlockPlacementLibrary::IsPlaceableBlock(uint8 BlockID)
{
	return FPlacementRules::IsPlaceableBlock(BlockID);
}

bool UBlockPlacementLibrary::IsReplaceableBlock(uint8 BlockID)
{
	return FPlacementRules::IsReplaceableBlock(BlockID);
}

bool UBlockPlacementLibrary::IsSupportBlock(uint8 BlockID)
{
	return FPlacementRules::IsSupportBlock(BlockID);
}

bool UBlockPlacementLibrary::HasSupportAtVoxel(AWorldGenerator* WorldGenerator, const FIntVector& Voxel)
{
	if (!WorldGenerator)
	{
		return false;
	}

	return FPlacementRules::HasSupportFromNeighbours(
		Voxel,
		[WorldGenerator](const FIntVector& V)
		{
			return WorldGenerator->GetBlockAtVoxel(V.X, V.Y, V.Z);
		});
}

bool UBlockPlacementLibrary::IsAttachDirectionAllowed(const FIntVector& AttachDir, bool bAllowWallMounts, bool bAllowCeilingMounts)
{
	return FPlacementRules::IsAttachDirectionAllowed(AttachDir, bAllowWallMounts, bAllowCeilingMounts);
}

uint8 UBlockPlacementLibrary::EncodeAttachMeta(const FIntVector& AttachDir)
{
	return FPlacementRules::EncodeAttachMeta(AttachDir);
}

FIntVector UBlockPlacementLibrary::DecodeAttachMeta(uint8 Meta)
{
	return FPlacementRules::DecodeAttachMeta(Meta);
}

FIntVector UBlockPlacementLibrary::AttachDirectionFromNormal(const FVector& ImpactNormal)
{
	const FIntVector Step = FPlacementRules::AxisStep(ImpactNormal);
	return FIntVector(-Step.X, -Step.Y, -Step.Z);
}

bool UBlockPlacementLibrary::VoxelOverlapsCapsule(
	const FIntVector& Voxel,
	float BlockScale,
	const FVector& CapsuleCenter,
	float CapsuleRadius,
	float CapsuleHalfHeight,
	float Skin)
{
	return FPlacementRules::VoxelOverlapsCapsule(Voxel, BlockScale, CapsuleCenter, CapsuleRadius, CapsuleHalfHeight, Skin);
}

int32 UBlockPlacementLibrary::GetPlaceNoiseLoudness(uint8 BlockID, int32 DefaultLoudness, int32 TorchLoudness)
{
	return FPlacementRules::GetPlaceNoiseLoudness(BlockID, DefaultLoudness, TorchLoudness);
}

FName UBlockPlacementLibrary::GetPlaceMaterialName(uint8 BlockID)
{
	return FPlacementRules::GetPlaceMaterialName(BlockID);
}

bool UBlockPlacementLibrary::IsPlacementSuccess(EPlaceBlockResult Result)
{
	return Result == EPlaceBlockResult::Success;
}

FString UBlockPlacementLibrary::PlacementResultToString(EPlaceBlockResult Result)
{
	return FString(FPlacementRules::ToString(Result));
}
