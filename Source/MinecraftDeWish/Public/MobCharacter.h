#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MobTypes.h"
#include "MobCharacter.generated.h"

class AWorldGenerator;
class UDayNightCycleSystem;

/** Delegate fired when this mob dies */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMobDied, AMobCharacter*, Mob, AActor*, Killer);

/**
 * AMobCharacter
 *
 * Base character class for all hostile mobs (Zombie, Skeleton, Spider, Creeper).
 * Handles health, damage, death drops, XP orb spawning, sunlight burning,
 * and spider wall climbing.
 */
UCLASS(BlueprintType, Blueprintable)
class MINECRAFTDEWISH_API AMobCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMobCharacter();

	// ==================== MOB IDENTITY ====================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob")
	EMobType MobType = EMobType::Zombie;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob")
	FMobDefinition MobDefinition;

	/** Returns the Patrix PBR texture paths (BaseColor, Normal, Specular) for this mob type */
	UFUNCTION(BlueprintPure, Category = "Mob|Visual")
	static void GetMobTexturePaths(EMobType InType, FString& OutBaseColor, FString& OutNormal, FString& OutSpecular);

	// ==================== STATE TREE / BEHAVIOR HELPERS (ADR-0009) ====================

	/** Perceived target resolved by AMobAIController (player). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|AI")
	TWeakObjectPtr<AActor> CurrentTarget;

	/** Enter patrol/wander state. */
	UFUNCTION(BlueprintCallable, Category = "Mob|AI")
	void StartPatrol();

	/** Move toward the given target. */
	UFUNCTION(BlueprintCallable, Category = "Mob|AI")
	void ChaseTarget(AActor* Target);

	/** Perform an attack on the target if cooldown allows. */
	UFUNCTION(BlueprintCallable, Category = "Mob|AI")
	void PerformAttack(AActor* Target);

	/** Flee away from a threat. */
	UFUNCTION(BlueprintCallable, Category = "Mob|AI")
	void FleeFrom(AActor* Threat);

	/** Begin creeper fuse toward target. */
	UFUNCTION(BlueprintCallable, Category = "Mob|AI")
	void StartFuse(AActor* Target);

	/** Cancel an in-progress fuse. */
	UFUNCTION(BlueprintCallable, Category = "Mob|AI")
	void CancelFuse();

	/** Tick an active fuse (radial damage + crater on completion). */
	UFUNCTION(BlueprintCallable, Category = "Mob|AI")
	void UpdateFuse(float DeltaTime, AActor* Target);

	/** Returns the perception target from the AIController. */
	UFUNCTION(BlueprintPure, Category = "Mob|AI")
	AActor* GetPerceptionTarget() const;

	// ==================== HEALTH ====================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|Health")
	float CurrentHealth = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|Health")
	bool bIsAlive = true;

	// ==================== AI STATE ====================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|AI")
	EMobAIState CurrentAIState = EMobAIState::Idle;

	// ==================== ANIMATION (BlueprintReadOnly for ABP_Mob_Base) ====================

	/** Current horizontal movement speed (cm/s). Drives the ABP locomotion state machine. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|Animation")
	float Speed = 0.0f;

	/** True while the mob is falling. Drives jump/fall transitions in the ABP. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|Animation")
	bool bIsFalling = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|AI")
	float AttackCooldownTimer = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|AI")
	float ExplosionFuseTimer = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|AI")
	bool bFuseActive = false;

	// ==================== REFERENCES ====================

	UPROPERTY()
	TWeakObjectPtr<AWorldGenerator> WorldGenerator;

	// ==================== EVENTS ====================

	UPROPERTY(BlueprintAssignable, Category = "Mob|Events")
	FOnMobDied OnMobDied;

	// ==================== FUNCTIONS ====================

	/** Initialize this mob with its type definition */
	UFUNCTION(BlueprintCallable, Category = "Mob")
	void InitializeMob(EMobType InType, AWorldGenerator* InWorldGen);

	/** Apply damage to this mob */
	UFUNCTION(BlueprintCallable, Category = "Mob|Health")
	float ApplyMobDamage(float DamageAmount, AActor* DamageSource = nullptr);

	/** Check if mob is exposed to sky (no block above) */
	UFUNCTION(BlueprintPure, Category = "Mob")
	bool IsExposedToSky() const;

	/** Get the day/night cycle system from the world generator */
	UDayNightCycleSystem* GetDayNightCycle() const;

	/** Weather-driven aggression multiplier applied to movement speed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|AI")
	float WeatherAggressionMultiplier = 1.0f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	/** Handle sunlight burning (legacy per-frame; retained for compatibility) */
	void ProcessSunlightBurning(float DeltaTime);

	/** Timer-driven sunlight burn (1.0s cadence) */
	void ProcessSunlightBurningTimer();

	/** Timer-driven mob update (10 Hz): spider climb, AI, anim state */
	void MobUpdateTimer();

	/** Handle spider wall climbing */
	void ProcessSpiderClimbing(float DeltaTime);

	/** AI state machine tick */
	void TickAIStateMachine(float DeltaTime);

	/** Find the closest player pawn */
	APawn* FindClosestPlayer() const;

	/** Handle mob death — spawn drops and XP orbs */
	void HandleDeath(AActor* Killer);

	/** Spawn item drops from mob's loot table */
	void SpawnDrops();

	/** Spawn XP orbs */
	void SpawnXPOrbs();

	/** Wander behavior */
	void ProcessWander(float DeltaTime);

	/** Chase behavior */
	void ProcessChase(float DeltaTime, AActor* Target);

	/** Attack behavior */
	void ProcessAttack(float DeltaTime, AActor* Target);

	/** Creeper explosion behavior */
	void ProcessExplosion(float DeltaTime, AActor* Target);

	float WanderTimer = 0.0f;
	FVector WanderDirection = FVector::ZeroVector;
	float SunBurnAccumulator = 0.0f;

	/** Timer handles for tick-replacement (bCanEverTick = false per ADR-0009). */
	FTimerHandle SunlightBurnTimerHandle;
	FTimerHandle MobUpdateTimerHandle;
};
