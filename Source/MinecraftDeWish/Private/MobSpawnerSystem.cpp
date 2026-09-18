#include "MobSpawnerSystem.h"
#include "MobCharacter.h"
#include "WorldGenerator.h"
#include "DayNightCycleSystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

UMobSpawnerSystem::UMobSpawnerSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f; // 2 Hz
}

void UMobSpawnerSystem::BeginPlay()
{
	Super::BeginPlay();

	WorldGenerator = Cast<AWorldGenerator>(GetOwner());
}

void UMobSpawnerSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bSpawningEnabled || !WorldGenerator.IsValid())
	{
		return;
	}

	CleanupMobs();

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->GetPawn())
	{
		return;
	}

	const FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
	ProcessDespawn(PlayerLoc);

	SpawnTimer += DeltaTime;
	if (SpawnTimer >= SpawnInterval)
	{
		SpawnTimer = 0.0f;
		ProcessSpawnCycle();
	}
}

// ==================== SPAWN CYCLE ====================

void UMobSpawnerSystem::ProcessSpawnCycle()
{
	if (ActiveMobs.Num() >= MaxMobCap)
	{
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->GetPawn())
	{
		return;
	}

	const FVector PlayerLoc = PC->GetPawn()->GetActorLocation();

	// Attempt to spawn 1-4 mobs
	const int32 SpawnAttempts = FMath::RandRange(1, 4);
	for (int32 i = 0; i < SpawnAttempts && ActiveMobs.Num() < MaxMobCap; ++i)
	{
		FVector SpawnPos;
		if (FindValidSpawnPosition(PlayerLoc, SpawnPos))
		{
			EMobType Type = PickRandomMobType();
			AMobCharacter* Mob = SpawnMob(Type, SpawnPos);
			if (Mob)
			{
				ActiveMobs.Add(Mob);
			}
		}
	}
}

AMobCharacter* UMobSpawnerSystem::SpawnMob(EMobType MobType, const FVector& Location)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AMobCharacter* Mob = GetWorld()->SpawnActor<AMobCharacter>(
		AMobCharacter::StaticClass(),
		Location,
		FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f),
		SpawnParams
	);

	if (Mob && WorldGenerator.IsValid())
	{
		Mob->InitializeMob(MobType, WorldGenerator.Get());

		UE_LOG(LogTemp, Log, TEXT("MobSpawner: Spawned %s at %s"),
			*StaticEnum<EMobType>()->GetDisplayNameTextByValue(static_cast<int64>(MobType)).ToString(),
			*Location.ToString());
	}

	return Mob;
}

// ==================== CLEANUP & DESPAWN ====================

void UMobSpawnerSystem::CleanupMobs()
{
	ActiveMobs.RemoveAll([](const TWeakObjectPtr<AMobCharacter>& MobPtr)
	{
		return !MobPtr.IsValid() || !MobPtr->bIsAlive;
	});
}

void UMobSpawnerSystem::ProcessDespawn(const FVector& PlayerLocation)
{
	for (int32 i = ActiveMobs.Num() - 1; i >= 0; --i)
	{
		if (!ActiveMobs[i].IsValid())
		{
			ActiveMobs.RemoveAt(i);
			continue;
		}

		const float Dist = FVector::Dist(PlayerLocation, ActiveMobs[i]->GetActorLocation());
		if (Dist > DespawnDistance)
		{
			UE_LOG(LogTemp, Log, TEXT("MobSpawner: Despawning %s (distance: %.0f)"),
				*ActiveMobs[i]->GetName(), Dist);
			ActiveMobs[i]->Destroy();
			ActiveMobs.RemoveAt(i);
		}
	}
}

// ==================== SPAWN POSITION FINDING ====================

bool UMobSpawnerSystem::FindValidSpawnPosition(const FVector& PlayerLocation, FVector& OutSpawnPosition) const
{
	if (!WorldGenerator.IsValid())
	{
		return false;
	}

	const float BlockScale = WorldGenerator->BlockScale;

	// Try up to 10 random positions
	for (int32 Attempt = 0; Attempt < 10; ++Attempt)
	{
		// Random angle and distance from player
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		const float Distance = FMath::FRandRange(MinSpawnDistance, MaxSpawnDistance);

		const float SpawnX = PlayerLocation.X + FMath::Cos(Angle) * Distance;
		const float SpawnY = PlayerLocation.Y + FMath::Sin(Angle) * Distance;

		const int32 VoxelX = FMath::FloorToInt(SpawnX / BlockScale);
		const int32 VoxelY = FMath::FloorToInt(SpawnY / BlockScale);

		// Find the surface height at this position
		const int32 PredictedHeight = WorldGenerator->GetPredictedTerrainHeight(VoxelX, VoxelY);

		// Check if the surface block and space above are valid
		if (IsValidSpawnSurface(VoxelX, VoxelY, PredictedHeight))
		{
			// Check light level
			const FIntVector SpawnVoxel(VoxelX, VoxelY, PredictedHeight + 1);
			const int32 LightLevel = CalculateLightLevel(SpawnVoxel);

			if (LightLevel < 7) // Dark enough to spawn
			{
				OutSpawnPosition = FVector(
					(static_cast<float>(VoxelX) + 0.5f) * BlockScale,
					(static_cast<float>(VoxelY) + 0.5f) * BlockScale,
					(static_cast<float>(PredictedHeight) + 1.5f) * BlockScale
				);
				return true;
			}
		}
	}

	return false;
}

bool UMobSpawnerSystem::IsValidSpawnSurface(int32 VoxelX, int32 VoxelY, int32 VoxelZ) const
{
	if (!WorldGenerator.IsValid())
	{
		return false;
	}

	// Surface block must be solid
	uint8 SurfaceBlock = 0;
	if (!WorldGenerator->GetVoxelAt(VoxelX, VoxelY, VoxelZ, SurfaceBlock) || SurfaceBlock == 0)
	{
		return false;
	}

	// Must not be a non-solid block
	if (FBlockHelpers::IsNonSolidBlock(SurfaceBlock))
	{
		return false;
	}

	// Two blocks of air above for mob to stand in
	uint8 Above1 = 0, Above2 = 0;
	WorldGenerator->GetVoxelAt(VoxelX, VoxelY, VoxelZ + 1, Above1);
	WorldGenerator->GetVoxelAt(VoxelX, VoxelY, VoxelZ + 2, Above2);

	return Above1 == 0 && Above2 == 0;
}

bool UMobSpawnerSystem::HasNearbyTorch(int32 VoxelX, int32 VoxelY, int32 VoxelZ) const
{
	if (!WorldGenerator.IsValid())
	{
		return false;
	}

	const int32 R = TorchLightRadius;
	for (int32 Dx = -R; Dx <= R; ++Dx)
	{
		for (int32 Dy = -R; Dy <= R; ++Dy)
		{
			for (int32 Dz = -R; Dz <= R; ++Dz)
			{
				if (Dx * Dx + Dy * Dy + Dz * Dz > R * R)
				{
					continue;
				}

				uint8 BlockID = 0;
				if (WorldGenerator->GetVoxelAt(VoxelX + Dx, VoxelY + Dy, VoxelZ + Dz, BlockID))
				{
					if (BlockID == static_cast<uint8>(EBlockType::Torch))
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

int32 UMobSpawnerSystem::CalculateLightLevel(const FIntVector& VoxelPos) const
{
	if (!WorldGenerator.IsValid())
	{
		return 15;
	}

	int32 LightLevel = 0;

	// Sky light contribution: check if exposed to sky
	bool bExposedToSky = true;
	for (int32 Z = VoxelPos.Z + 1; Z < WorldGenerator->ChunkHeight; ++Z)
	{
		uint8 BlockID = 0;
		if (WorldGenerator->GetVoxelAt(VoxelPos.X, VoxelPos.Y, Z, BlockID))
		{
			if (BlockID != 0 && !FBlockHelpers::IsNonSolidBlock(BlockID))
			{
				bExposedToSky = false;
				break;
			}
		}
	}

	if (bExposedToSky)
	{
		// Sky light depends on day/night
		UDayNightCycleSystem* DayNight = WorldGenerator->FindComponentByClass<UDayNightCycleSystem>();
		if (DayNight && DayNight->IsDay())
		{
			LightLevel = 15; // Full daylight
		}
		else
		{
			LightLevel = 4; // Moonlight
		}
	}

	// Torch light contribution
	if (HasNearbyTorch(VoxelPos.X, VoxelPos.Y, VoxelPos.Z))
	{
		LightLevel = FMath::Max(LightLevel, 14); // Torch = light level 14
	}

	return LightLevel;
}

EMobType UMobSpawnerSystem::PickRandomMobType() const
{
	// Weighted random selection
	const int32 Roll = FMath::RandRange(0, 99);

	if (Roll < 40)      return EMobType::Zombie;    // 40%
	else if (Roll < 65) return EMobType::Skeleton;   // 25%
	else if (Roll < 85) return EMobType::Spider;     // 20%
	else                return EMobType::Creeper;    // 15%
}
