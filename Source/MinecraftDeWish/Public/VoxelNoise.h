#pragma once

#include "CoreMinimal.h"

/**
 * Fast, deterministic Perlin & Simplex noise generator for voxel terrain generation.
 * Supports 2D/3D gradients, Fractal Brownian Motion (fBm), and Cellular noise.
 */
class MINECRAFTDEWISH_API FVoxelNoise
{
public:
	FVoxelNoise();
	explicit FVoxelNoise(int32 InSeed);

	void SetSeed(int32 InSeed);
	int32 GetSeed() const { return Seed; }

	// Standard 2D / 3D Gradient Noise (Range ~ [-1.0, 1.0])
	float Perlin2D(float X, float Y, float Frequency = 0.01f) const;
	float Perlin3D(float X, float Y, float Z, float Frequency = 0.01f) const;

	// Fractal Brownian Motion (Multi-octave noise)
	float Fractal2D(float X, float Y, int32 Octaves = 4, float Frequency = 0.005f, float Persistence = 0.5f, float Lacunarity = 2.0f) const;
	float Fractal3D(float X, float Y, float Z, int32 Octaves = 3, float Frequency = 0.015f, float Persistence = 0.5f, float Lacunarity = 2.0f) const;

	// Cellular / Voronoi noise (distance to nearest feature point, range [0, 1])
	float Cellular2D(float X, float Y, float Frequency = 0.01f) const;

	// Normalized [0.0, 1.0] convenience wrappers
	float Fractal2D_01(float X, float Y, int32 Octaves = 4, float Frequency = 0.005f, float Persistence = 0.5f, float Lacunarity = 2.0f) const
	{
		return FMath::Clamp((Fractal2D(X, Y, Octaves, Frequency, Persistence, Lacunarity) + 1.0f) * 0.5f, 0.0f, 1.0f);
	}

private:
	int32 Seed = 1337;
	uint8 Permutation[512];

	void InitializePermutation();

	static float Fade(float T)
	{
		return T * T * T * (T * (T * 6.0f - 15.0f) + 10.0f);
	}

	static float Grad2D(int32 Hash, float X, float Y);
	static float Grad3D(int32 Hash, float X, float Y, float Z);
};
