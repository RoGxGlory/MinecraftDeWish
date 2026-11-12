// LAMPLIGHTER — Blueprint-facing surface for the block-placement rules.
//
// The ten-gate pipeline of `design/gdd/block-placement.md` §3.3 lives in two places:
//   - AWorldGenerator::TryPlaceBlock / CanPlaceAtVoxel  (world-aware: reach, chunk, support,
//     pawn overlap, commit). Both are UFUNCTIONs and therefore BlueprintCallable.
//   - FPlacementRules                                    (pure, world-free math and predicates).
//
// FPlacementRules is a plain C++ struct, so Blueprint cannot call it directly. This library
// re-exposes every rule as a BlueprintPure/BlueprintCallable node, which lets a Blueprint own
// the orchestration (input -> trace -> resolve -> validate -> commit) while the rules stay
// single-sourced in C++.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoxelPlacementRules.h"
#include "BlockPlacementLibrary.generated.h"

class AWorldGenerator;

UCLASS()
class MINECRAFTDEWISH_API UBlockPlacementLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ---------------------------------------------------------------- world context

	/**
	 * Find the active world generator in a world.
	 * Returns the first AWorldGenerator, or null before the world is generated.
	 */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Context", meta = (WorldContext = "WorldContextObject"))
	static AWorldGenerator* GetWorldGenerator(const UObject* WorldContextObject);

	// ---------------------------------------------------------------- target resolution

	/**
	 * Integer step along the dominant axis of a hit normal (§4.2).
	 * Exact for an axis-aligned cube face; a zero component never produces a zero step.
	 */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Target")
	static FIntVector AxisStep(const FVector& ImpactNormal);

	/**
	 * Placement voxel from a crosshair hit (§4.2): recover the hit voxel by stepping
	 * 0.25 blocks inward, then step one full voxel along the normal.
	 *
	 * @param BlockScale Block edge in Unreal units (100 by default, ADR-0001).
	 */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Target")
	static FIntVector ResolvePlacementVoxel(const FVector& ImpactPoint, const FVector& ImpactNormal, float BlockScale);

	/** Hit voxel from a crosshair hit: the block the ray actually struck. */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Target")
	static FIntVector ResolveBreakVoxel(const FVector& ImpactPoint, const FVector& ImpactNormal, float BlockScale);

	/** World location of a voxel's centre, for spawning effects and sounds. */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Target")
	static FVector VoxelCenterToWorld(const FIntVector& Voxel, float BlockScale);

	// ---------------------------------------------------------------- G2 / G7 predicates

	/** G2 — is this block ID a placeable item? Air, Bedrock, Water and unknown IDs are not. */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Rules")
	static bool IsPlaceableBlock(uint8 BlockID);

	/** G7 — may a placement overwrite this block? Air, Water and Torch only. */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Rules")
	static bool IsReplaceableBlock(uint8 BlockID);

	/** G8 helper — can this block physically carry another block? */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Rules")
	static bool IsSupportBlock(uint8 BlockID);

	/** G8 — does the target voxel have at least one face-adjacent support block? */
	UFUNCTION(BlueprintCallable, Category = "Block Placement|Rules")
	static bool HasSupportAtVoxel(AWorldGenerator* WorldGenerator, const FIntVector& Voxel);

	// ---------------------------------------------------------------- G9 attachment

	/** G9 — may an attaching block sit here, attached toward `AttachDir`? */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Attachment")
	static bool IsAttachDirectionAllowed(const FIntVector& AttachDir, bool bAllowWallMounts, bool bAllowCeilingMounts);

	/** Torch metadata from an attachment direction: 0 floor, 1..4 walls (§4.5). */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Attachment")
	static uint8 EncodeAttachMeta(const FIntVector& AttachDir);

	/** Inverse of `EncodeAttachMeta`; unknown metadata decodes to the floor mount. */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Attachment")
	static FIntVector DecodeAttachMeta(uint8 Meta);

	/**
	 * Attachment direction from a hit normal: the direction from the new voxel toward its
	 * support (negated axis step, §4.5).
	 */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Attachment")
	static FIntVector AttachDirectionFromNormal(const FVector& ImpactNormal);

	// ---------------------------------------------------------------- G10 overlap

	/**
	 * G10 — does a vertical capsule overlap a voxel's AABB, plus a safety skin (§4.6)?
	 * Works on raw values so a Blueprint can test a hypothetical body.
	 */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Overlap")
	static bool VoxelOverlapsCapsule(
		const FIntVector& Voxel,
		float BlockScale,
		const FVector& CapsuleCenter,
		float CapsuleRadius,
		float CapsuleHalfHeight,
		float Skin);

	// ---------------------------------------------------------------- feedback helpers

	/** Noise loudness of a placement (§4.8): the torch loudness for a torch, default otherwise. */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Feedback")
	static int32 GetPlaceNoiseLoudness(uint8 BlockID, int32 DefaultLoudness, int32 TorchLoudness);

	/** Material word used to build the `SFX_BlockPlace_<Material>` cue name (§3.11). */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Feedback")
	static FName GetPlaceMaterialName(uint8 BlockID);

	// ---------------------------------------------------------------- result helpers

	/** True only for EPlaceBlockResult::Success. */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Result")
	static bool IsPlacementSuccess(EPlaceBlockResult Result);

	/** Human-readable name of a placement result, for logs and debug text. */
	UFUNCTION(BlueprintPure, Category = "Block Placement|Result")
	static FString PlacementResultToString(EPlaceBlockResult Result);
};
