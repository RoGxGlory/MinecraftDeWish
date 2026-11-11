#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "VoxelDataTypes.h"
#include "VoxelPlacementRules.generated.h"

/**
 * Outcome of one block-placement attempt.
 *
 * Every rejection carries a distinct reason so that gameplay stays silent (the world simply
 * refuses to change) while telemetry and automation tests stay precise. The order of the
 * enumerators mirrors the gate order of `design/gdd/block-placement.md` §3.3.
 */
UENUM(BlueprintType)
enum class EPlaceBlockResult : uint8
{
	/** The voxel was written and the consequences applied. */
	Success = 0,

	/** No crosshair hit at all (looking at the sky). Not a validation failure. */
	NoTarget UMETA(DisplayName = "No Target"),

	/** G1 — impact point further than PlacementReach + ReachTolerance from the camera. */
	OutOfReach UMETA(DisplayName = "Out Of Reach"),

	/** G2 — the block is not a placeable item (Air, Bedrock, Water, unknown ID). */
	NotPlaceable UMETA(DisplayName = "Not Placeable"),

	/** G3 — the selected quick slot is empty. */
	NoItem UMETA(DisplayName = "No Item"),

	/** G3 — the selected quick slot holds a different block or a non-block item. */
	ItemMismatch UMETA(DisplayName = "Item Mismatch"),

	/** G4 — the cooldown between two successful commits has not elapsed. */
	Cooldown UMETA(DisplayName = "Cooldown"),

	/** G5 — Z outside [0, ChunkHeight). */
	OutOfBounds UMETA(DisplayName = "Out Of Bounds"),

	/** G6 — the owning chunk is not streamed in. */
	ChunkUnloaded UMETA(DisplayName = "Chunk Unloaded"),

	/** G7 — something non-replaceable already occupies the voxel. */
	Occupied UMETA(DisplayName = "Occupied"),

	/** G8 — no face-adjacent support block; the placement would float. */
	NoSupport UMETA(DisplayName = "No Support"),

	/** G9 — an attaching block (torch) cannot attach in that direction. */
	BadAttach UMETA(DisplayName = "Bad Attach"),

	/** G10 — a pawn capsule overlaps the target voxel. */
	BlockedByPawn UMETA(DisplayName = "Blocked By Pawn")
};

/**
 * Pure, world-free rules of the block-placement system.
 *
 * Everything in here is a deterministic function of its arguments: no UObject, no world
 * query, no member state. That is what makes the ten-gate pipeline of
 * `design/gdd/block-placement.md` §3.3 testable without spawning a level, and it keeps the
 * rules identical on the server, on a client prediction and in an automation test.
 */
struct MINECRAFTDEWISH_API FPlacementRules
{
	/** Face-adjacent offsets D6 = {(+/-1,0,0), (0,+/-1,0), (0,0,+/-1)}. */
	static const TArray<FIntVector>& FaceOffsets();

	/**
	 * Integer step along the dominant axis of an impact normal (§4.2).
	 *
	 * Exact for an axis-aligned cube face, which is why the placement voxel is derived by
	 * stepping rather than by sampling outward with an epsilon. A zero component never
	 * produces a zero step: the dominant axis always steps by +/-1.
	 */
	static FIntVector AxisStep(const FVector& Normal);

	/** G2 — true for every block ID that exists in `EBlockType` except Air, Bedrock and Water. */
	static bool IsPlaceableBlock(uint8 BlockID);

	/** G7 — Air, Water and Torch may be overwritten by a placement. */
	static bool IsReplaceableBlock(uint8 BlockID);

	/** G8 helper — a block that can physically carry another block (§3.4). */
	static bool IsSupportBlock(uint8 BlockID);

	/**
	 * G8 — at least one face-adjacent block can carry the placement.
	 *
	 * @param Voxel       Target voxel.
	 * @param BlockLookup Callable `uint8(const FIntVector&)`. Out-of-world voxels should
	 *                    return Air so a placement at the world edge has no phantom support.
	 */
	static bool HasSupportFromNeighbours(const FIntVector& Voxel, const TFunctionRef<uint8(const FIntVector&)>& BlockLookup);

	/**
	 * G9 — may an attaching block sit in this voxel, attached in direction `AttachDir`?
	 *
	 * @param AttachDir      Direction from the new voxel toward its support.
	 * @param bAllowWall     Enable the four horizontal mounts.
	 * @param bAllowCeiling  Enable the (0,0,+1) mount. Off by default: the crossed-plane
	 *                       torch mesh has no anchor under a ceiling.
	 */
	static bool IsAttachDirectionAllowed(const FIntVector& AttachDir, bool bAllowWall, bool bAllowCeiling);

	/** Torch attachment metadata (§4.5): 0 = floor, 1..4 = wall mounts. */
	static uint8 EncodeAttachMeta(const FIntVector& AttachDir);

	/** Inverse of `EncodeAttachMeta`. An unknown meta decodes to the floor mount. */
	static FIntVector DecodeAttachMeta(uint8 Meta);

	/**
	 * Squared distance between a vertical segment and an axis-aligned box (§4.6).
	 *
	 * Both are axis-aligned, so the exact two-way clamp reduces to three independent
	 * interval distances. Zero when the segment touches or enters the box.
	 *
	 * @param SegmentCenter   Segment midpoint (the pawn capsule centre).
	 * @param SegmentHalfLen  Half the segment length (CapsuleHalfHeight - CapsuleRadius).
	 */
	static float VerticalSegmentBoxDistanceSquared(const FVector& SegmentCenter, float SegmentHalfLen, const FBox& Box);

	/** G10 — the pawn capsule overlaps the voxel AABB, with a `Skin` safety margin (§4.6). */
	static bool VoxelOverlapsCapsule(
		const FIntVector& Voxel,
		float BlockScale,
		const FVector& CapsuleCenter,
		float CapsuleRadius,
		float CapsuleHalfHeight,
		float Skin);

	/** Noise loudness of a placement (§4.8): 2 for a torch, `DefaultLoudness` otherwise. */
	static int32 GetPlaceNoiseLoudness(uint8 BlockID, int32 DefaultLoudness, int32 TorchLoudness);

	/** Material word used to build the `SFX_BlockPlace_<Material>` cue name (§3.11). */
	static FName GetPlaceMaterialName(uint8 BlockID);

	/** Human-readable name of a placement result, for logs and test messages. */
	static const TCHAR* ToString(EPlaceBlockResult Result);
};
