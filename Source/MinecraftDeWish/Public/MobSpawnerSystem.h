#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MobTypes.h"
#include "MobSpawnerSystem.generated.h"

class AWorldGenerator;
class AMobCharacter;
class UDayNightCycleSystem;
class UDataTable;

/**
 * UMobSpawnerSystem
 *
 * Manages hostile mob spawning and despawning around the player.
 *
 * Spawn Rules (matching Minecraft):
 * - Only spawn in dark areas (light level < 7, checked via torch proximity + sky exposure)
 * - Never spawn in direct light or within 5 blocks of a torch
 * - Spawn distance: 24-128 blocks from player
 * - Max mob cap: 20 hostile mobs within 128-block radius
 * - Despawn: mobs > 128 blocks from player are removed
 * - Spawn tick: every 2 seconds, attempt to spawn 1-4 mobs
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINECRAFTDEWISH_API UMobSpawnerSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UMobSpawnerSystem();

	/** Maximum hostile mobs allowed within range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MobSpawner|Settings")
	int32 MaxMobCap = 20;

	/** Minimum spawn distance from player (in Unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MobSpawner|Settings")
	float MinSpawnDistance = 2400.0f; // 24 blocks

	/** Maximum spawn distance from player (in Unreal units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MobSpawner|Settings")
	float MaxSpawnDistance = 12800.0f; // 128 blocks

	/** Distance beyond which mobs despawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MobSpawner|Settings")
	float DespawnDistance = 12800.0f; // 128 blocks

	/** Seconds between spawn attempts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MobSpawner|Settings")
	float SpawnInterval = 2.0f;

	/** Torch light radius in blocks (mobs won't spawn within this range of a torch) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MobSpawner|Settings")
	int32 TorchLightRadius = 5;

	/** Whether spawning is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MobSpawner|Settings")
	bool bSpawningEnabled = true;

	/** Reference to world generator */
	UPROPERTY()
	TWeakObjectPtr<AWorldGenerator> WorldGenerator;

	/** Cached day/night cycle (for weather multipliers) */
	UPROPERTY()
	TWeakObjectPtr<UDayNightCycleSystem> DayNightCycle;

	// ==================== QUERIES ====================

	/** Returns the count of currently alive hostile mobs */
	UFUNCTION(BlueprintPure, Category = "MobSpawner")
	int32 GetActiveMobCount() const { return ActiveMobs.Num(); }

	/** Calculates light level at a voxel position (0=pitch dark, 15=full sunlight) */
	UFUNCTION(BlueprintPure, Category = "MobSpawner")
	int32 CalculateLightLevel(const FIntVector& VoxelPos) const;

	/** Exposed for testing. */
	bool IsValidSpawnSurface(int32 VoxelX, int32 VoxelY, int32 VoxelZ) const;
	bool HasNearbyTorch(int32 VoxelX, int32 VoxelY, int32 VoxelZ) const;

	/** Class map: EMobType -> Blueprint mob class (BP_Mob_*). Populated by designer or DT_Mobs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MobSpawner|Classes")
	TMap<EMobType, TSubclassOf<AMobCharacter>> MobClasses;

	/** Optional DataTable (DT_Mobs) resolving EMobType -> MobClass/mesh/ABP/stats/loot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MobSpawner|Classes")
	TObjectPtr<UDataTable> MobDataTable;

	/** Force spawn a mob from its Blueprint class at a location */
	UFUNCTION(BlueprintCallable, Category = "MobSpawner")
	AMobCharacter* SpawnMob(TSubclassOf<AMobCharacter> MobClass, const FVector& Location);

	/** Legacy: spawn by EMobType, resolves class from MobClasses (falls back to base). */
	UFUNCTION(BlueprintCallable, Category = "MobSpawner", meta=(DeprecatedFunction, DeprecationMessage="Use SpawnMob(TSubclassOf<AMobCharacter>)"))
	AMobCharacter* SpawnMobByType(EMobType MobType, const FVector& Location);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AMobCharacter>> ActiveMobs;

	float SpawnTimer = 0.0f;

	/** Main spawn loop — find valid positions and spawn mobs */
	void ProcessSpawnCycle();

	/** Remove dead or despawned mobs from tracking */
	void CleanupMobs();

	/** Despawn mobs too far from the player */
	void ProcessDespawn(const FVector& PlayerLocation);

	/** Find a valid spawn position near the player */
	bool FindValidSpawnPosition(const FVector& PlayerLocation, FVector& OutSpawnPosition) const;

	/** Check if a voxel position is a valid spawn surface */

	/** Check if area near a voxel has a torch */

	/** Pick a random mob type weighted by conditions */
	EMobType PickRandomMobType() const;
};
