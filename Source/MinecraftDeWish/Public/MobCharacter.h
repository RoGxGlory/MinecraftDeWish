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

	// ==================== HEALTH ====================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|Health")
	float CurrentHealth = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|Health")
	bool bIsAlive = true;

	// ==================== AI STATE ====================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mob|AI")
	EMobAIState CurrentAIState = EMobAIState::Idle;

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

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	/** Handle sunlight burning */
	void ProcessSunlightBurning(float DeltaTime);

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
	void ProcessChase(float DeltaTime, APawn* Target);

	/** Attack behavior */
	void ProcessAttack(float DeltaTime, APawn* Target);

	/** Creeper explosion behavior */
	void ProcessExplosion(float DeltaTime, APawn* Target);

	float WanderTimer = 0.0f;
	FVector WanderDirection = FVector::ZeroVector;
	float SunBurnAccumulator = 0.0f;
};
