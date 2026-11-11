#include "VoxelCaveRules.h"

namespace CaveRuleConstants
{
	/** Hash salt of the entrance chunk candidate roll (§4.14). */
	constexpr uint32 SaltEntranceChunk = 0x456E7472u;

	/** Hash salt of the entrance column selection (§4.14). */
	constexpr uint32 SaltEntranceColumn = 0x456E7473u;

	/** Hash salt of the mineshaft region decision (§4.14). */
	constexpr uint32 SaltMineshaftRegion = 0x4D696E65u;

	/** Hash salt of the dungeon region decision (§4.14). */
	constexpr uint32 SaltDungeonRegion = 0x44756E67u;

	/** Below this density a multiplier is treated as "disabled" instead of dividing by zero. */
	constexpr float MinDensity = 1.0e-4f;
}

uint32 FCaveRules::Hash(int32 X, int32 Y, int32 Z, int32 Seed, uint32 Salt)
{
	// §4.14: splitmix-style mix of the position, the seed and a feature salt. The salt must never
	// be reused across features, otherwise two independent decisions correlate.
	uint32 H = static_cast<uint32>(X) * 73856093u;
	H ^= static_cast<uint32>(Y) * 19349663u;
	H ^= static_cast<uint32>(Z) * 83492791u;
	H ^= static_cast<uint32>(Seed);
	H ^= Salt;

	// splitmix32 finalizer: avalanche every input bit into every output bit.
	H ^= H >> 16;
	H *= 0x7FEB352Du;
	H ^= H >> 15;
	H *= 0x846CA68Bu;
	H ^= H >> 16;
	return H;
}

float FCaveRules::Roll01(int32 X, int32 Y, int32 Z, int32 Seed, uint32 Salt)
{
	return static_cast<float>(Hash(X, Y, Z, Seed, Salt) & 0xFFFFu) / 65535.0f;
}

int32 FCaveRules::FloorDiv(int32 A, int32 B)
{
	if (B <= 0)
	{
		return 0;
	}
	const int32 Quotient = A / B;
	const int32 Remainder = A % B;
	// C++ integer division truncates toward zero; the world needs a floor.
	return (Remainder != 0 && ((Remainder < 0) != (B < 0))) ? Quotient - 1 : Quotient;
}

int32 FCaveRules::FloorMod(int32 A, int32 B)
{
	if (B <= 0)
	{
		return 0;
	}
	const int32 Remainder = A % B;
	return (Remainder < 0) ? Remainder + B : Remainder;
}

float FCaveRules::TunnelThreshold(const FVoxelCaveParams& Params, float Density)
{
	const float Derived = Params.TunnelRadius * Params.NoiseGradient * Params.TunnelFrequency;
	return Derived * Derived * FMath::Max(Density, 0.0f);
}

float FCaveRules::NoodleSlabWidth(const FVoxelCaveParams& Params, float Density)
{
	return Params.NoodleRadius * Params.NoiseGradient * Params.NoodleFrequency * FMath::Max(Density, 0.0f);
}

float FCaveRules::CavernCarveThreshold(const FVoxelCaveParams& Params, float Density)
{
	// Inverted on purpose: the cavern test is `>`, so a higher density must lower the bar.
	return Params.CavernThreshold / FMath::Max(Density, CaveRuleConstants::MinDensity);
}

float FCaveRules::RavineRegionCutoff(const FVoxelCaveParams& Params, float Bias)
{
	return Params.RavineRegionThreshold / FMath::Max(Bias, CaveRuleConstants::MinDensity);
}

bool FCaveRules::IsTunnel(float N1, float N2, float Threshold)
{
	// §3.3: the intersection of two near-zero fields is a curve; the threshold thickens it.
	return Threshold > 0.0f && (N1 * N1 + N2 * N2) < Threshold;
}

bool FCaveRules::IsNoodle(float N3, float N4, float Width)
{
	// §3.4: two independent slabs. Square cross-section of side 2*Radius.
	return Width > 0.0f && FMath::Abs(N3) < Width && FMath::Abs(N4) < Width;
}

bool FCaveRules::IsCavern(float Nc, float Mc, const FVoxelCaveParams& Params, float Threshold)
{
	return (Nc + Params.CavernMaskBias * Mc) > Threshold;
}

float FCaveRules::RavineDistance(float R1, const FVoxelCaveParams& Params)
{
	const float Scale = Params.NoiseGradient * Params.RavineFrequency;
	if (Scale <= 0.0f)
	{
		return TNumericLimits<float>::Max();
	}
	return FMath::Abs(R1) / Scale;
}

float FCaveRules::RavineHalfWidth(float T, float Wn, const FVoxelCaveParams& Params)
{
	if (T <= 0.0f || T >= 1.0f)
	{
		// The sin(pi*t) taper closes the fissure at both ends: no surface breach, no flat slab.
		return 0.0f;
	}

	const float Sine = FMath::Sin(PI * T);
	if (Sine <= 0.0f)
	{
		// Guards float error just inside the endpoints, where sin() can come out negative.
		return 0.0f;
	}

	const float Wobble = 1.0f + Params.RavineWidthWobble * Wn;
	return FMath::Max(0.0f, Params.RavineMaxHalfWidth * FMath::Pow(Sine, 0.7f) * Wobble);
}

bool FCaveRules::IsInRavineLedgeBand(int32 Z, float Distance, float HalfWidth, const FVoxelCaveParams& Params)
{
	if (Params.RavineLedgeSpacing <= 0 || Params.RavineLedgeThickness <= 0)
	{
		return false;
	}

	const bool bInLayer = FloorMod(Z, Params.RavineLedgeSpacing) < Params.RavineLedgeThickness;
	const bool bNearWall = Distance > Params.RavineLedgeInnerRatio * HalfWidth;
	return bInLayer && bNearWall;
}

bool FCaveRules::IsRavine(
	float Region,
	float Distance,
	int32 Z,
	int32 TopZ,
	float Wn,
	const FVoxelCaveParams& Params,
	float Bias)
{
	if (Region <= RavineRegionCutoff(Params, Bias))
	{
		return false;
	}

	const int32 BottomZ = Params.RavineMinZ;
	if (TopZ <= BottomZ)
	{
		return false;
	}

	const float T = static_cast<float>(Z - BottomZ) / static_cast<float>(TopZ - BottomZ);
	const float HalfWidth = RavineHalfWidth(T, Wn, Params);
	if (HalfWidth <= 0.0f || Distance >= HalfWidth)
	{
		return false;
	}

	// Ledges only *suppress* ravine carving. A tunnel that already carved this voxel still wins,
	// because the pass evaluates archetypes as a union (§3.6, §3.14).
	return !IsInRavineLedgeBand(Z, Distance, HalfWidth, Params);
}

void FCaveRules::SampleMineshaftColumn(
	int32 WorldX,
	int32 WorldY,
	int32 Seed,
	int32 TerrainHeight,
	const FVoxelCaveParams& Params,
	FMineshaftColumn& Out)
{
	Out = FMineshaftColumn();

	const int32 RegionSize = FMath::Max(Params.MineshaftRegionSize, 32);
	const int32 RegionX = FloorDiv(WorldX, RegionSize);
	const int32 RegionY = FloorDiv(WorldY, RegionSize);

	Out.RegionX = RegionX;
	Out.RegionY = RegionY;

	const uint32 H = Hash(RegionX, RegionY, 0, Seed, CaveRuleConstants::SaltMineshaftRegion);

	// §3.8: `(H & 0xFF) / 255.0 > MineshaftChance` means no lattice in this region.
	const float RegionRoll = static_cast<float>(H & 0xFFu) / 255.0f;
	if (RegionRoll > Params.MineshaftChance)
	{
		return;
	}

	Out.bInRegion = true;

	const int32 Spacing = FMath::Max(Params.MineshaftLatticeSpacing, 8);
	Out.OffsetX = static_cast<int32>(H % static_cast<uint32>(Spacing));
	Out.OffsetY = static_cast<int32>((H >> 8) % static_cast<uint32>(Spacing));

	// Corridor centres sit on `Offset + n * Spacing + Spacing / 2`, the integer form of the
	// `frac(...) == 0.5` centreline of §4.8. An even spacing keeps them on whole blocks.
	const int32 CellX = FloorDiv(WorldX - Out.OffsetX, Spacing);
	const int32 CellY = FloorDiv(WorldY - Out.OffsetY, Spacing);
	Out.LineX = Out.OffsetX + CellX * Spacing + Spacing / 2;
	Out.LineY = Out.OffsetY + CellY * Spacing + Spacing / 2;

	// `FloorToInt(HalfWidth)` columns on each side of the centreline: 1.5 -> 3 blocks wide.
	const int32 Reach = FMath::Max(1, FMath::FloorToInt(Params.MineshaftHalfWidth));
	Out.bAlongY = FMath::Abs(WorldX - Out.LineX) <= Reach;
	Out.bAlongX = FMath::Abs(WorldY - Out.LineY) <= Reach;
	Out.bInCorridor = Out.bAlongY || Out.bAlongX;

	if (!Out.bInCorridor)
	{
		return;
	}

	// The corridor floor is a region constant, so a corridor crossing a chunk border stays
	// continuous without any cross-chunk stamping.
	const int32 MinZ = Params.MineshaftMinZ;
	const int32 MaxZ = FMath::Max(Params.MineshaftMaxZ, MinZ);
	const int32 Span = MaxZ - MinZ;
	Out.FloorZ = MinZ + ((Span > 0) ? static_cast<int32>((H >> 16) % static_cast<uint32>(Span + 1)) : 0);

	// §3.8 / edge cases: a corridor that would break the surface cap simply stops existing there.
	const int32 Height = FMath::Max(Params.MineshaftCorridorHeight, 1);
	if (Out.FloorZ + Height - 1 > TerrainHeight - Params.CaveSurfaceCap)
	{
		Out.bInCorridor = false;
	}
}

void FCaveRules::SampleDungeonColumn(
	int32 WorldX,
	int32 WorldY,
	int32 Seed,
	int32 TerrainHeight,
	const FVoxelCaveParams& Params,
	FDungeonColumn& Out)
{
	Out = FDungeonColumn();

	const int32 RegionSize = FMath::Max(Params.DungeonRegionSize, 32);
	const int32 RegionX = FloorDiv(WorldX, RegionSize);
	const int32 RegionY = FloorDiv(WorldY, RegionSize);

	Out.RegionX = RegionX;
	Out.RegionY = RegionY;

	const uint32 H = Hash(RegionX, RegionY, 0, Seed, CaveRuleConstants::SaltDungeonRegion);
	const float RegionRoll = static_cast<float>(H & 0xFFu) / 255.0f;
	if (RegionRoll > Params.DungeonChance)
	{
		return;
	}

	Out.bActive = true;

	// Keep the whole room inside its region, with a one-block margin around the shell.
	const int32 Radius = FMath::Max(Params.DungeonRoomRadius, 1);
	const int32 Margin = Radius + 2;
	const int32 Span = FMath::Max(RegionSize - 2 * Margin, 1);
	Out.CenterX = RegionX * RegionSize + Margin + static_cast<int32>((H >> 8) % static_cast<uint32>(Span));
	Out.CenterY = RegionY * RegionSize + Margin + static_cast<int32>((H >> 16) % static_cast<uint32>(Span));

	const int32 MinZ = Params.DungeonMinZ;
	const int32 MaxZ = FMath::Max(Params.DungeonMaxZ, MinZ);
	const int32 ZSpan = MaxZ - MinZ;
	Out.FloorZ = MinZ + ((ZSpan > 0) ? static_cast<int32>((H >> 24) % static_cast<uint32>(ZSpan + 1)) : 0);

	// §3.9: the room band is capped at `TerrainHeight - CavernSurfaceCap`, a larger cap than the
	// tunnels' because a 5-block room under a 6-block roof would read as a hole in the ground.
	const int32 Height = FMath::Max(Params.DungeonRoomHeight, 1);
	if (Out.FloorZ + Height > TerrainHeight - Params.CavernSurfaceCap)
	{
		Out.bActive = false;
		return;
	}

	const int32 Shell = Radius + 1;
	Out.bInFootprint = FMath::Abs(WorldX - Out.CenterX) <= Shell && FMath::Abs(WorldY - Out.CenterY) <= Shell;
}

bool FCaveRules::SelectEntranceColumn(
	const FChunkCoord& Coord,
	int32 Seed,
	const FVoxelCaveParams& Params,
	int32& OutLocalX,
	int32& OutLocalY)
{
	OutLocalX = 0;
	OutLocalY = 0;

	if (Params.CaveEntranceChunkChance <= 0.0f)
	{
		return false;
	}

	// One candidate per chunk: the chunk roll is independent of the column roll, so raising the
	// chance never biases the chosen column.
	const float ChunkRoll = Roll01(Coord.X, Coord.Y, 0, Seed, CaveRuleConstants::SaltEntranceChunk);
	if (ChunkRoll >= Params.CaveEntranceChunkChance)
	{
		return false;
	}

	// The apron keeps the shaft and its dirt rim fully inside the chunk.
	const int32 Half = FMath::Max(Params.EntranceShaftWidth / 2, 1);
	const int32 Margin = FMath::Clamp(Half + Params.EntranceRimRadius, 1, (CHUNK_SIZE_X - 1) / 2);

	int32 BestX = Margin;
	int32 BestY = Margin;
	uint32 BestHash = MAX_uint32;

	for (int32 LocalY = Margin; LocalY <= CHUNK_SIZE_Y - 1 - Margin; ++LocalY)
	{
		for (int32 LocalX = Margin; LocalX <= CHUNK_SIZE_X - 1 - Margin; ++LocalX)
		{
			const int32 WorldX = Coord.X * CHUNK_SIZE_X + LocalX;
			const int32 WorldY = Coord.Y * CHUNK_SIZE_Y + LocalY;
			const uint32 H = Hash(WorldX, WorldY, 0, Seed, CaveRuleConstants::SaltEntranceColumn);
			if (H < BestHash)
			{
				BestHash = H;
				BestX = LocalX;
				BestY = LocalY;
			}
		}
	}

	OutLocalX = BestX;
	OutLocalY = BestY;
	return true;
}

FCaveBiomeProfile FCaveRules::GetDefaultProfile(EBiomeType Biome)
{
	// Shipped table of `cave-generation.md` §3.13. There is no Tundra biome in `EBiomeType`, so
	// the table's Tundra row has no runtime counterpart and is not represented here.
	FCaveBiomeProfile Profile;
	Profile.Biome = Biome;

	switch (Biome)
	{
	case EBiomeType::Plains:
		Profile.TunnelDensity = 1.00f;
		Profile.NoodleDensity = 1.00f;
		Profile.CavernDensity = 1.00f;
		Profile.RavineRegionBias = 0.90f;
		break;

	case EBiomeType::Forest:
		Profile.TunnelDensity = 1.05f;
		Profile.NoodleDensity = 1.00f;
		Profile.CavernDensity = 1.00f;
		Profile.RavineRegionBias = 1.00f;
		break;

	case EBiomeType::Desert:
		// Deserts crack, they do not hollow.
		Profile.TunnelDensity = 0.75f;
		Profile.NoodleDensity = 0.85f;
		Profile.CavernDensity = 0.90f;
		Profile.RavineRegionBias = 1.15f;
		break;

	case EBiomeType::Mountains:
		// Most rock, most cave: the deep-mine biome.
		Profile.TunnelDensity = 1.25f;
		Profile.NoodleDensity = 1.10f;
		Profile.CavernDensity = 1.15f;
		Profile.RavineRegionBias = 1.00f;
		break;

	case EBiomeType::HorrorForest:
		// The biome that is supposed to be riddled.
		Profile.TunnelDensity = 1.15f;
		Profile.NoodleDensity = 1.20f;
		Profile.CavernDensity = 1.20f;
		Profile.RavineRegionBias = 1.10f;
		break;

	default:
		break;
	}

	return Profile;
}

const TCHAR* FCaveRules::ArchetypeToString(ECaveArchetype Archetype)
{
	switch (Archetype)
	{
	case ECaveArchetype::Tunnel:        return TEXT("Tunnel");
	case ECaveArchetype::Noodle:        return TEXT("Noodle");
	case ECaveArchetype::Cavern:        return TEXT("Cavern");
	case ECaveArchetype::Ravine:        return TEXT("Ravine");
	case ECaveArchetype::Mineshaft:     return TEXT("Mineshaft");
	case ECaveArchetype::Dungeon:       return TEXT("Dungeon");
	case ECaveArchetype::EntranceShaft: return TEXT("EntranceShaft");
	default:                            return TEXT("None");
	}
}

// ==================== FCaveFieldSet ====================

void FCaveFieldSet::Reset(int32 InOriginX, int32 InOriginY, int32 InOriginZ, int32 InStep, int32 InSizeX, int32 InSizeY, int32 InSizeZ)
{
	OriginX = InOriginX;
	OriginY = InOriginY;
	OriginZ = InOriginZ;
	Step = FMath::Max(InStep, 1);
	SizeX = FMath::Max(InSizeX, 0);
	SizeY = FMath::Max(InSizeY, 0);
	SizeZ = FMath::Max(InSizeZ, 0);

	const int32 Total = SizeX * SizeY * SizeZ;
	// Reuses the existing allocation when the lattice size did not change, so a streaming pass
	// does not allocate once per chunk.
	if (Data.Num() != Total)
	{
		Data.SetNumUninitialized(Total);
	}
}

float FCaveFieldSet::Sample(int32 WorldX, int32 WorldY, int32 WorldZ) const
{
	if (!IsValid())
	{
		return 0.0f;
	}

	if (SizeX < 2 || SizeY < 2 || SizeZ < 2)
	{
		return Get(0, 0, 0);
	}

	const float FX = static_cast<float>(WorldX - OriginX) / static_cast<float>(Step);
	const float FY = static_cast<float>(WorldY - OriginY) / static_cast<float>(Step);
	const float FZ = static_cast<float>(WorldZ - OriginZ) / static_cast<float>(Step);

	int32 IX = FMath::FloorToInt(FX);
	int32 IY = FMath::FloorToInt(FY);
	int32 IZ = FMath::FloorToInt(FZ);

	IX = FMath::Clamp(IX, 0, SizeX - 2);
	IY = FMath::Clamp(IY, 0, SizeY - 2);
	IZ = FMath::Clamp(IZ, 0, SizeZ - 2);

	// §4.13: w1 = frac(x / step). Exact 0/0.5 weights because the lattice is world-anchored and
	// the chunk origin is a multiple of the step.
	const float TX = FMath::Clamp(FX - static_cast<float>(IX), 0.0f, 1.0f);
	const float TY = FMath::Clamp(FY - static_cast<float>(IY), 0.0f, 1.0f);
	const float TZ = FMath::Clamp(FZ - static_cast<float>(IZ), 0.0f, 1.0f);

	const float C000 = Get(IX, IY, IZ);
	const float C100 = Get(IX + 1, IY, IZ);
	const float C010 = Get(IX, IY + 1, IZ);
	const float C110 = Get(IX + 1, IY + 1, IZ);
	const float C001 = Get(IX, IY, IZ + 1);
	const float C101 = Get(IX + 1, IY, IZ + 1);
	const float C011 = Get(IX, IY + 1, IZ + 1);
	const float C111 = Get(IX + 1, IY + 1, IZ + 1);

	const float X00 = FMath::Lerp(C000, C100, TX);
	const float X10 = FMath::Lerp(C010, C110, TX);
	const float X01 = FMath::Lerp(C001, C101, TX);
	const float X11 = FMath::Lerp(C011, C111, TX);

	const float Y0 = FMath::Lerp(X00, X10, TY);
	const float Y1 = FMath::Lerp(X01, X11, TY);

	return FMath::Lerp(Y0, Y1, TZ);
}

float FCaveFieldSet::Sample2D(int32 WorldX, int32 WorldY) const
{
	if (!IsValid())
	{
		return 0.0f;
	}

	if (SizeX < 2 || SizeY < 2)
	{
		return Get(0, 0, 0);
	}

	const float FX = static_cast<float>(WorldX - OriginX) / static_cast<float>(Step);
	const float FY = static_cast<float>(WorldY - OriginY) / static_cast<float>(Step);

	int32 IX = FMath::Clamp(FMath::FloorToInt(FX), 0, SizeX - 2);
	int32 IY = FMath::Clamp(FMath::FloorToInt(FY), 0, SizeY - 2);

	const float TX = FMath::Clamp(FX - static_cast<float>(IX), 0.0f, 1.0f);
	const float TY = FMath::Clamp(FY - static_cast<float>(IY), 0.0f, 1.0f);

	const float C00 = Get(IX, IY, 0);
	const float C10 = Get(IX + 1, IY, 0);
	const float C01 = Get(IX, IY + 1, 0);
	const float C11 = Get(IX + 1, IY + 1, 0);

	return FMath::Lerp(FMath::Lerp(C00, C10, TX), FMath::Lerp(C01, C11, TX), TY);
}
