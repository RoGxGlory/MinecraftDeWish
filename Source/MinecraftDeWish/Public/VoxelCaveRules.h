#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "VoxelDataTypes.h"
#include "VoxelCaveRules.generated.h"

/**
 * Archetype that carved a voxel.
 *
 * Diagnostic only: it is used by the spawner weighting (`cave-generation.md` §3.12) and by the
 * probes. It is never persisted, because it is a pure function of the world coordinates.
 */
UENUM(BlueprintType)
enum class ECaveArchetype : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Tunnel UMETA(DisplayName = "Tunnel"),
	Noodle UMETA(DisplayName = "Noodle"),
	Cavern UMETA(DisplayName = "Cavern"),
	Ravine UMETA(DisplayName = "Ravine"),
	Mineshaft UMETA(DisplayName = "Mineshaft"),
	Dungeon UMETA(DisplayName = "Dungeon"),
	EntranceShaft UMETA(DisplayName = "Entrance Shaft")
};

/**
 * Per-biome cave density profile, one row per `EBiomeType` in `DT_CaveProfiles`.
 *
 * Multipliers scale **thresholds**, never radii: a Desert tunnel and a Mountain tunnel must be
 * the same width or the player cannot learn tunnel readability across biomes
 * (`cave-generation.md` §3.13).
 *
 * When no DataTable is assigned, `FCaveRules::GetDefaultProfile()` reproduces the shipped table
 * documented in the GDD. The fallback lives in code on purpose: it is the documented default of
 * a data-driven system, not a hidden balance value.
 */
USTRUCT(BlueprintType)
struct MINECRAFTDEWISH_API FCaveBiomeProfile : public FTableRowBase
{
	GENERATED_BODY()

	/** Biome this row describes. Must be unique across the table. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Biome")
	EBiomeType Biome = EBiomeType::Plains;

	/** Threshold multiplier for spaghetti tunnels. 0 disables the archetype, 1.5 widens it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Biome", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float TunnelDensity = 1.0f;

	/** Threshold multiplier for noodle passages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Biome", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float NoodleDensity = 1.0f;

	/** Threshold multiplier for caverns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Biome", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float CavernDensity = 1.0f;

	/**
	 * Ravine region bias. Divides `RavineRegionThreshold`, so a value above 1 lets ravines exist
	 * in more of the biome (the GDD table lists 1.15 for Desert: deserts crack, they hollow less).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Biome", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float RavineRegionBias = 1.0f;
};

/**
 * Every carving knob of the cave pass (`cave-generation.md` §7).
 *
 * The master switches (`bEnableCaves`, `bEnableCaveStructures`) live on `AWorldGenerator` next to
 * the other feature toggles. Everything else is here so a designer can tune the underground from
 * one place, and so no cave value is a literal in code (`ue-data.md`).
 *
 * Frequencies are authored as **effective** frequency in blocks⁻¹ (a period of 32 blocks is
 * `1/32`). The pass multiplies the world coordinates by the frequency and passes `1.0` to
 * `FVoxelNoise`, which keeps the effective frequency identical to the authored value. The shipped
 * ore code multiplies coordinates *and* passes a small frequency, which silently shrinks the
 * effective frequency — do not copy that pattern here.
 */
USTRUCT(BlueprintType)
struct MINECRAFTDEWISH_API FVoxelCaveParams
{
	GENERATED_BODY()

	// ==================== BAND ====================

	/** Lowest carveable Z. Leaves this many stone blocks above the bedrock floor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Band", meta = (ClampMin = "1", ClampMax = "16"))
	int32 CaveFloorZ = 4;

	/** Stone roof kept under the surface. No archetype but an entrance may carve above it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Band", meta = (ClampMin = "3", ClampMax = "12"))
	int32 CaveSurfaceCap = 6;

	/** Design target consumed by the calibration probe only; never read at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Band", meta = (ClampMin = "0.03", ClampMax = "0.07"))
	float CaveVolumeTarget = 0.050f;

	/** Noise gradient per unit of effective frequency, used to derive every radius threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Calibration", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float NoiseGradient = 1.0f;

	/** Standard deviation of the noise amplitude. Documentation / probe constant only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Calibration", meta = (ClampMin = "0.10", ClampMax = "0.60"))
	float NoiseSigma = 0.30f;

	// ==================== TUNNELS (A) ====================

	/** Tunnel half-width in blocks. Below 1.2 the passage stops being comfortably walkable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Tunnels", meta = (ClampMin = "1.2", ClampMax = "4.0"))
	float TunnelRadius = 1.9f;

	/** Tunnel period in blocks⁻¹ (default 1/32). Lower = longer, straighter, rarer tunnels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Tunnels", meta = (ClampMin = "0.015625", ClampMax = "0.0833333"))
	float TunnelFrequency = 0.03125f;

	// ==================== NOODLES (B) ====================

	/** Noodle half-width in blocks. Hard floor 0.9: below it the passage is impassable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Noodles", meta = (ClampMin = "0.9", ClampMax = "2.0"))
	float NoodleRadius = 1.1f;

	/** Noodle period in blocks⁻¹ (default 1/24). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Noodles", meta = (ClampMin = "0.0208333", ClampMax = "0.0833333"))
	float NoodleFrequency = 0.0416667f;

	// ==================== CAVERNS (C) ====================

	/** Chamber volume threshold. Primary density knob of the archetype. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Caverns", meta = (ClampMin = "0.55", ClampMax = "0.90"))
	float CavernThreshold = 0.72f;

	/** Chamber scale in blocks⁻¹ (default 1/48). Lower = fewer, larger chambers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Caverns", meta = (ClampMin = "0.0104167", ClampMax = "0.0416667"))
	float CavernFrequency = 0.0208333f;

	/** Width : height ratio of a chamber (multiplies the Z frequency only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Caverns", meta = (ClampMin = "1.0", ClampMax = "3.0"))
	float CavernVerticalSquash = 1.5f;

	/** Swarm clustering bias. 0 = uniform sponge (do not ship). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Caverns", meta = (ClampMin = "0.0", ClampMax = "0.60"))
	float CavernMaskBias = 0.30f;

	/** Swarm size in blocks⁻¹ (default 1/160). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Caverns", meta = (ClampMin = "0.001953", ClampMax = "0.015625"))
	float CavernMaskFrequency = 0.00625f;

	/** Roof thickness kept above a chamber. Larger than `CaveSurfaceCap` on purpose. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Caverns", meta = (ClampMin = "6", ClampMax = "24"))
	int32 CavernSurfaceCap = 10;

	/** Octave count of the cavern field and its mask. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Caverns", meta = (ClampMin = "1", ClampMax = "4"))
	int32 CavernOctaves = 2;

	// ==================== RAVINES (D) ====================

	/** Maximum fissure half-width at mid-height, in blocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Ravines", meta = (ClampMin = "2.0", ClampMax = "12.0"))
	float RavineMaxHalfWidth = 4.5f;

	/** Centreline period in blocks⁻¹ (default 1/220). Lower = fewer, longer fissures. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Ravines", meta = (ClampMin = "0.001953", ClampMax = "0.0104167"))
	float RavineFrequency = 0.00454545f;

	/** Region-gate period in blocks⁻¹ (default 1/400): selects where ravines may exist at all. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Ravines", meta = (ClampMin = "0.0009766", ClampMax = "0.0104167"))
	float RavineRegionFrequency = 0.0025f;

	/** Region-gate threshold. Higher = ravines exist in less of the world. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Ravines", meta = (ClampMin = "0.2", ClampMax = "0.8"))
	float RavineRegionThreshold = 0.40f;

	/** Fissure floor. The `sin(pi*t)` taper closes the fissure at this end. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Ravines", meta = (ClampMin = "2", ClampMax = "24"))
	int32 RavineMinZ = 6;

	/** Stone roof kept above a fissure top (`TerrainHeight - this`). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Ravines", meta = (ClampMin = "4", ClampMax = "16"))
	int32 RavineTopSurfaceCap = 8;

	/** Vertical distance between ledges, in blocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Ravines", meta = (ClampMin = "4", ClampMax = "12"))
	int32 RavineLedgeSpacing = 7;

	/** Ledge height, in blocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Ravines", meta = (ClampMin = "1", ClampMax = "4"))
	int32 RavineLedgeThickness = 2;

	/** How far out from the centreline (as a ratio of the half-width) the ledge starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Ravines", meta = (ClampMin = "0.3", ClampMax = "0.9"))
	float RavineLedgeInnerRatio = 0.55f;

	/** Octave count of the ravine centreline and region fields. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Ravines", meta = (ClampMin = "1", ClampMax = "4"))
	int32 RavineOctaves = 3;

	/** Strength of the half-width wobble (`1 + Wobble * Wn`). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Ravines", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RavineWidthWobble = 0.25f;

	// ==================== ENTRANCES (E) ====================

	/** Candidate rate per chunk. Primary discoverability knob. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Entrances", meta = (ClampMin = "0.0", ClampMax = "0.30"))
	float CaveEntranceChunkChance = 0.08f;

	/** How far a shaft may dig down looking for the first carved voxel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Entrances", meta = (ClampMin = "16", ClampMax = "80"))
	int32 EntranceMaxShaftDepth = 40;

	/** Shaft cross-section in blocks. Never below 3: 2 is impassable once the rim is added. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Entrances", meta = (ClampMin = "3", ClampMax = "5"))
	int32 EntranceShaftWidth = 3;

	/** Dirt rim width around the shaft — the discovery tell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Entrances", meta = (ClampMin = "0", ClampMax = "4"))
	int32 EntranceRimRadius = 2;

	// ==================== STRUCTURES (F/G) ====================

	/** Fraction of mineshaft regions that carry a corridor lattice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float MineshaftChance = 0.10f;

	/** Region size for the mineshaft lattice decision, in blocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "128", ClampMax = "512"))
	int32 MineshaftRegionSize = 256;

	/** Corridor spacing of the lattice, in blocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "12", ClampMax = "48"))
	int32 MineshaftLatticeSpacing = 32;

	/** Corridor half-width in blocks (3 blocks wide at the default 1.5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "1.0", ClampMax = "2.5"))
	float MineshaftHalfWidth = 1.5f;

	/** Corridor height in blocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "3", ClampMax = "5"))
	int32 MineshaftCorridorHeight = 3;

	/** Lower bound of the band a corridor floor may sit in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "4", ClampMax = "120"))
	int32 MineshaftMinZ = 20;

	/** Upper bound of the band a corridor floor may sit in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "4", ClampMax = "120"))
	int32 MineshaftMaxZ = 60;

	/** Timber frame cadence along a corridor, in blocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "2", ClampMax = "8"))
	int32 MineshaftFrameSpacing = 4;

	/** Probability of a torch on a given frame. Above ~0.2 the mineshaft stops being scary. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "0.0", ClampMax = "0.4"))
	float MineshaftTorchChance = 0.06f;

	/** Fraction of dungeon regions that carry a room. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "0.0", ClampMax = "0.4"))
	float DungeonChance = 0.08f;

	/** Region size for the dungeon decision, in blocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "64", ClampMax = "256"))
	int32 DungeonRegionSize = 128;

	/** Lower bound of the band a dungeon floor may sit in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "4", ClampMax = "120"))
	int32 DungeonMinZ = 16;

	/** Upper bound of the band a dungeon floor may sit in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "4", ClampMax = "120"))
	int32 DungeonMaxZ = 48;

	/** Interior half-extent of a dungeon room (4 gives the 9x9 interior of the GDD). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "2", ClampMax = "8"))
	int32 DungeonRoomRadius = 4;

	/** Interior height of a dungeon room (5 gives floor at Z and ceiling at Z+4). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "3", ClampMax = "8"))
	int32 DungeonRoomHeight = 5;

	/** Chance of a crafting table in a dungeon room. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Structures", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DungeonCraftingTableChance = 0.25f;

	// ==================== PERFORMANCE / INTERACTION ====================

	/** Cave share of `MaxGenerationTimeMs`, in milliseconds. Measured, not enforced at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Performance", meta = (ClampMin = "0.2", ClampMax = "4.0"))
	float CavePassBudgetMs = 1.2f;

	/** Interpolation step of the fine 3D fields, in blocks. 1 = exact and slow, 4 = blobby. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Performance", meta = (ClampMin = "1", ClampMax = "4"))
	int32 CaveNoiseStep = 2;

	/** Sampling step of the cavern mask, in blocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Performance", meta = (ClampMin = "2", ClampMax = "16"))
	int32 CavernMaskStep = 8;

	/** Sampling step of the ravine region gate, in blocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Performance", meta = (ClampMin = "4", ClampMax = "32"))
	int32 RavineRegionStep = 16;

	/**
	 * Ore threshold reduction next to cave air, for Coal and Iron only.
	 *
	 * Ships at 0 (off). Enabling it changes ore distribution and requires a `GeneratorVersion`
	 * bump; it must never apply to Diamond or Emerald.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cave|Interaction", meta = (ClampMin = "0.0", ClampMax = "0.06"))
	float CaveAdjacencyOreBonus = 0.0f;
};

/**
 * One field sampled on a world-anchored lattice, interpolated trilinearly.
 *
 * The lattice **must** be anchored on world multiples of `Step`, never on the chunk-local origin,
 * or the interpolated field would be discontinuous at chunk borders and the cave walls would show
 * a seam every 16 blocks (`cave-generation.md` §3.15.3).
 *
 * Fields are interpolated **separately** and combined afterwards: interpolating a squared sum
 * produces blobby, faceted walls.
 */
struct MINECRAFTDEWISH_API FCaveFieldSet
{
	/** Lattice origin in world block coordinates (a multiple of Step). */
	int32 OriginX = 0;
	int32 OriginY = 0;
	int32 OriginZ = 0;

	/** Lattice step in blocks. */
	int32 Step = 1;

	int32 SizeX = 0;
	int32 SizeY = 0;
	int32 SizeZ = 0;

	/** `SizeX * SizeY * SizeZ` samples, X fastest. */
	TArray<float> Data;

	/** Allocate (or reuse) the lattice. Does not clear the values. */
	void Reset(int32 InOriginX, int32 InOriginY, int32 InOriginZ, int32 InStep, int32 InSizeX, int32 InSizeY, int32 InSizeZ);

	bool IsValid() const { return SizeX > 0 && SizeY > 0 && SizeZ > 0 && Data.Num() == SizeX * SizeY * SizeZ; }

	FORCEINLINE int32 GetIndex(int32 I, int32 J, int32 K) const
	{
		return I + (J * SizeX) + (K * SizeX * SizeY);
	}

	FORCEINLINE float Get(int32 I, int32 J, int32 K) const { return Data[GetIndex(I, J, K)]; }

	FORCEINLINE void Set(int32 I, int32 J, int32 K, float Value) { Data[GetIndex(I, J, K)] = Value; }

	/** Trilinear sample at an integer world voxel coordinate. Returns 0 when the lattice is empty. */
	float Sample(int32 WorldX, int32 WorldY, int32 WorldZ) const;

	/**
	 * Bilinear sample at an integer world column, for a `SizeZ == 1` lattice.
	 *
	 * The ravine region gate and the ravine width wobble are 2D fields sampled with
	 * `RavineRegionStep`; sampling them raw per voxel would quantise the gate to the step and make
	 * the fissure appear and vanish in 16-block terraces.
	 */
	float Sample2D(int32 WorldX, int32 WorldY) const;
};

/**
 * 2D mineshaft lattice query for one world column.
 *
 * The corridor floor is derived from the region hash, so it is continuous across chunk borders
 * and needs no cross-chunk stamping. `cave-generation.md` §4.8 states the band as an inequality
 * over Z; §3.8 requires a 3-block-tall corridor, so the band is interpreted as the allowed range
 * of the corridor **floor**, not a full-height carve. Documented in ADR-0020.
 */
struct MINECRAFTDEWISH_API FMineshaftColumn
{
	/** The column's region carries a lattice at all. */
	bool bInRegion = false;

	/** The column is inside a corridor. */
	bool bInCorridor = false;

	/** Corridor runs along Y at constant X (`AxisA`). */
	bool bAlongY = false;

	/** Corridor runs along X at constant Y (`AxisB`). */
	bool bAlongX = false;

	/** Nearest lattice line for `AxisA`. */
	int32 LineX = 0;

	/** Nearest lattice line for `AxisB`. */
	int32 LineY = 0;

	int32 RegionX = 0;
	int32 RegionY = 0;
	int32 OffsetX = 0;
	int32 OffsetY = 0;

	/** Bottom Z of the corridor (the walkable floor). */
	int32 FloorZ = 0;
};

/**
 * 2D dungeon query for one world column.
 *
 * A room is a 9x5x9 interior wrapped in a Cobblestone shell, carved into Stone only, so a dungeon
 * near the surface never builds a floating box (`cave-generation.md` §3.9).
 */
struct MINECRAFTDEWISH_API FDungeonColumn
{
	/** The column's region carries a dungeon at all. */
	bool bActive = false;

	/** The column lies inside the room footprint (interior or shell). */
	bool bInFootprint = false;

	int32 CenterX = 0;
	int32 CenterY = 0;
	int32 FloorZ = 0;
	int32 RegionX = 0;
	int32 RegionY = 0;
};

/**
 * Pure, world-free rules of the cave pass.
 *
 * Every function here is a deterministic function of its arguments and the world seed: no
 * UObject, no world query, no member state. That is the determinism contract of
 * `cave-generation.md` §3.14 and it is what lets the automation suite test a cave without
 * generating one.
 */
struct MINECRAFTDEWISH_API FCaveRules
{
	// ==================== HASH ====================

	/** splitmix-style hash, matching the existing flora/village convention (§4.14). */
	static uint32 Hash(int32 X, int32 Y, int32 Z, int32 Seed, uint32 Salt);

	/** `(Hash & 0xFFFF) / 65535`, in [0, 1]. */
	static float Roll01(int32 X, int32 Y, int32 Z, int32 Seed, uint32 Salt);

	/** Floor division that rounds toward negative infinity (world coordinates are signed). */
	static int32 FloorDiv(int32 A, int32 B);

	/** Modulo that returns a non-negative remainder for a positive divisor. */
	static int32 FloorMod(int32 A, int32 B);

	// ==================== DERIVED THRESHOLDS (§4.2) ====================

	/** `(Radius * Gradient * Frequency)^2 * Density`. Carve when `N1^2 + N2^2` is below it. */
	static float TunnelThreshold(const FVoxelCaveParams& Params, float Density);

	/** `Radius * Gradient * Frequency * Density`. Carve when both `|N|` are below it. */
	static float NoodleSlabWidth(const FVoxelCaveParams& Params, float Density);

	/**
	 * `CavernThreshold / Density`.
	 *
	 * The GDD (§3.5, §7.4) writes `> Threshold * Density`, which would *reduce* carving as density
	 * rises — the inverse of the documented intent ("0 disables the archetype, 1.5 widens it").
	 * Because the cavern test is a `>` comparison, density must divide. Documented in ADR-0020.
	 */
	static float CavernCarveThreshold(const FVoxelCaveParams& Params, float Density);

	/** `RavineRegionThreshold / Bias`. Same inversion as the caverns: a `>` test divides. */
	static float RavineRegionCutoff(const FVoxelCaveParams& Params, float Bias);

	// ==================== CRITERIA ====================

	static bool IsTunnel(float N1, float N2, float Threshold);

	static bool IsNoodle(float N3, float N4, float Width);

	/** `Nc + Bias * Mc > Threshold`. */
	static bool IsCavern(float Nc, float Mc, const FVoxelCaveParams& Params, float Threshold);

	/** Distance to the ravine centreline, in blocks: `|R1| / (Gradient * Frequency)`. */
	static float RavineDistance(float R1, const FVoxelCaveParams& Params);

	/** `MaxHalfWidth * sin(pi*t)^0.7 * (1 + Wobble * Wn)`, zero outside `t in [0,1]`. */
	static float RavineHalfWidth(float T, float Wn, const FVoxelCaveParams& Params);

	/** `(Z mod Spacing) < Thickness && Distance > InnerRatio * HalfWidth`. */
	static bool IsInRavineLedgeBand(int32 Z, float Distance, float HalfWidth, const FVoxelCaveParams& Params);

	/**
	 * Full ravine criterion for one voxel.
	 *
	 * @param Region   Region-gate field value.
	 * @param Distance Distance to the centreline in blocks.
	 * @param Z        Voxel Z.
	 * @param TopZ     `TerrainHeight - RavineTopSurfaceCap`.
	 * @param Wn       Width-wobble field value.
	 * @param Bias     Biome ravine region bias.
	 */
	static bool IsRavine(
		float Region,
		float Distance,
		int32 Z,
		int32 TopZ,
		float Wn,
		const FVoxelCaveParams& Params,
		float Bias);

	// ==================== STRUCTURES ====================

	/** Resolves the mineshaft lattice state of one world column. */
	static void SampleMineshaftColumn(
		int32 WorldX,
		int32 WorldY,
		int32 Seed,
		int32 TerrainHeight,
		const FVoxelCaveParams& Params,
		FMineshaftColumn& Out);

	/** Resolves the dungeon state of one world column. */
	static void SampleDungeonColumn(
		int32 WorldX,
		int32 WorldY,
		int32 Seed,
		int32 TerrainHeight,
		const FVoxelCaveParams& Params,
		FDungeonColumn& Out);

	/**
	 * Picks the entrance column of a chunk, or returns false when the chunk has no candidate.
	 *
	 * One candidate per chunk, chosen in `[2 .. 13]^2` so the shaft and its rim always fit inside
	 * the chunk (same apron rule as the tree pass: no cross-chunk stamping, no border tearing).
	 */
	static bool SelectEntranceColumn(
		const FChunkCoord& Coord,
		int32 Seed,
		const FVoxelCaveParams& Params,
		int32& OutLocalX,
		int32& OutLocalY);

	// ==================== BIOME PROFILE ====================

	/** Shipped profile of a biome (§3.13), used when no `DT_CaveProfiles` row exists. */
	static FCaveBiomeProfile GetDefaultProfile(EBiomeType Biome);

	/** Human-readable name of an archetype, for logs and test messages. */
	static const TCHAR* ArchetypeToString(ECaveArchetype Archetype);
};
