#include "VoxelNoise.h"

FVoxelNoise::FVoxelNoise()
{
	SetSeed(1337);
}

FVoxelNoise::FVoxelNoise(int32 InSeed)
{
	SetSeed(InSeed);
}

void FVoxelNoise::SetSeed(int32 InSeed)
{
	Seed = InSeed;
	InitializePermutation();
}

void FVoxelNoise::InitializePermutation()
{
	FRandomStream Rng(Seed);
	TArray<uint8> Source;
	Source.SetNumUninitialized(256);
	for (int32 i = 0; i < 256; ++i)
	{
		Source[i] = static_cast<uint8>(i);
	}

	// Fisher-Yates shuffle
	for (int32 i = 255; i > 0; --i)
	{
		int32 j = Rng.RandRange(0, i);
		Source.Swap(i, j);
	}

	for (int32 i = 0; i < 256; ++i)
	{
		Permutation[i] = Source[i];
		Permutation[256 + i] = Source[i];
	}
}

float FVoxelNoise::Grad2D(int32 Hash, float X, float Y)
{
	static const float Gradients2D[8][2] = {
		{ 1.0f,  0.0f},
		{-1.0f,  0.0f},
		{ 0.0f,  1.0f},
		{ 0.0f, -1.0f},
		{ 0.70710678f,  0.70710678f},
		{-0.70710678f,  0.70710678f},
		{ 0.70710678f, -0.70710678f},
		{-0.70710678f, -0.70710678f}
	};
	const int32 H = Hash & 7;
	return Gradients2D[H][0] * X + Gradients2D[H][1] * Y;
}

float FVoxelNoise::Grad3D(int32 Hash, float X, float Y, float Z)
{
	int32 H = Hash & 15;
	float U = H < 8 ? X : Y;
	float V = H < 4 ? Y : (H == 12 || H == 14 ? X : Z);
	return ((H & 1) ? -U : U) + ((H & 2) ? -V : V);
}

float FVoxelNoise::Perlin2D(float X, float Y, float Frequency) const
{
	X *= Frequency;
	Y *= Frequency;

	int32 Xi = FMath::FloorToInt(X) & 255;
	int32 Yi = FMath::FloorToInt(Y) & 255;

	float Xf = X - FMath::FloorToFloat(X);
	float Yf = Y - FMath::FloorToFloat(Y);

	float U = Fade(Xf);
	float V = Fade(Yf);

	int32 A = Permutation[Xi] + Yi;
	int32 B = Permutation[Xi + 1] + Yi;

	float G00 = Grad2D(Permutation[A], Xf, Yf);
	float G10 = Grad2D(Permutation[B], Xf - 1.0f, Yf);
	float G01 = Grad2D(Permutation[A + 1], Xf, Yf - 1.0f);
	float G11 = Grad2D(Permutation[B + 1], Xf - 1.0f, Yf - 1.0f);

	float X1 = FMath::Lerp(G00, G10, U);
	float X2 = FMath::Lerp(G01, G11, U);

	return FMath::Lerp(X1, X2, V);
}

float FVoxelNoise::Perlin3D(float X, float Y, float Z, float Frequency) const
{
	X *= Frequency;
	Y *= Frequency;
	Z *= Frequency;

	int32 Xi = FMath::FloorToInt(X) & 255;
	int32 Yi = FMath::FloorToInt(Y) & 255;
	int32 Zi = FMath::FloorToInt(Z) & 255;

	float Xf = X - FMath::FloorToFloat(X);
	float Yf = Y - FMath::FloorToFloat(Y);
	float Zf = Z - FMath::FloorToFloat(Z);

	float U = Fade(Xf);
	float V = Fade(Yf);
	float W = Fade(Zf);

	int32 A = Permutation[Xi] + Yi;
	int32 AA = Permutation[A] + Zi;
	int32 AB = Permutation[A + 1] + Zi;
	int32 B = Permutation[Xi + 1] + Yi;
	int32 BA = Permutation[B] + Zi;
	int32 BB = Permutation[B + 1] + Zi;

	float G000 = Grad3D(Permutation[AA], Xf, Yf, Zf);
	float G100 = Grad3D(Permutation[BA], Xf - 1.0f, Yf, Zf);
	float G010 = Grad3D(Permutation[AB], Xf, Yf - 1.0f, Zf);
	float G110 = Grad3D(Permutation[BB], Xf - 1.0f, Yf - 1.0f, Zf);
	float G001 = Grad3D(Permutation[AA + 1], Xf, Yf, Zf - 1.0f);
	float G101 = Grad3D(Permutation[BA + 1], Xf - 1.0f, Yf, Zf - 1.0f);
	float G011 = Grad3D(Permutation[AB + 1], Xf, Yf - 1.0f, Zf - 1.0f);
	float G111 = Grad3D(Permutation[BB + 1], Xf - 1.0f, Yf - 1.0f, Zf - 1.0f);

	float LerpX0 = FMath::Lerp(G000, G100, U);
	float LerpX1 = FMath::Lerp(G010, G110, U);
	float LerpY0 = FMath::Lerp(LerpX0, LerpX1, V);

	float LerpX2 = FMath::Lerp(G001, G101, U);
	float LerpX3 = FMath::Lerp(G011, G111, U);
	float LerpY1 = FMath::Lerp(LerpX2, LerpX3, V);

	return FMath::Lerp(LerpY0, LerpY1, W);
}

float FVoxelNoise::Fractal2D(float X, float Y, int32 Octaves, float Frequency, float Persistence, float Lacunarity) const
{
	float Total = 0.0f;
	float Amplitude = 1.0f;
	float MaxAmplitude = 0.0f;
	float CurrentFreq = Frequency;

	for (int32 i = 0; i < Octaves; ++i)
	{
		Total += Perlin2D(X, Y, CurrentFreq) * Amplitude;
		MaxAmplitude += Amplitude;
		Amplitude *= Persistence;
		CurrentFreq *= Lacunarity;
	}

	return MaxAmplitude > 0.0f ? (Total / MaxAmplitude) : 0.0f;
}

float FVoxelNoise::Fractal3D(float X, float Y, float Z, int32 Octaves, float Frequency, float Persistence, float Lacunarity) const
{
	float Total = 0.0f;
	float Amplitude = 1.0f;
	float MaxAmplitude = 0.0f;
	float CurrentFreq = Frequency;

	for (int32 i = 0; i < Octaves; ++i)
	{
		Total += Perlin3D(X, Y, Z, CurrentFreq) * Amplitude;
		MaxAmplitude += Amplitude;
		Amplitude *= Persistence;
		CurrentFreq *= Lacunarity;
	}

	return MaxAmplitude > 0.0f ? (Total / MaxAmplitude) : 0.0f;
}

float FVoxelNoise::Cellular2D(float X, float Y, float Frequency) const
{
	X *= Frequency;
	Y *= Frequency;

	int32 Xi = FMath::FloorToInt(X);
	int32 Yi = FMath::FloorToInt(Y);

	float MinDist = 100.0f;

	for (int32 Dy = -1; Dy <= 1; ++Dy)
	{
		for (int32 Dx = -1; Dx <= 1; ++Dx)
		{
			int32 CellX = Xi + Dx;
			int32 CellY = Yi + Dy;

			// Deterministic pseudo-random point within cell
			int32 Hash = Permutation[(Permutation[CellX & 255] + CellY) & 255];
			float PointX = CellX + static_cast<float>(Hash & 15) / 15.0f;
			float PointY = CellY + static_cast<float>((Hash >> 4) & 15) / 15.0f;

			float DistSq = FMath::Square(PointX - X) + FMath::Square(PointY - Y);
			if (DistSq < MinDist)
			{
				MinDist = DistSq;
			}
		}
	}

	return FMath::Clamp(FMath::Sqrt(MinDist), 0.0f, 1.0f);
}
