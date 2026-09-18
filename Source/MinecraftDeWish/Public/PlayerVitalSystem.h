#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerVitalSystem.generated.h"

/** Delegate for health changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged, float, NewHealth, float, MaxHealth, float, DamageAmount);

/** Delegate for XP changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnXPChanged, int32, CurrentXP, int32, XPToNextLevel, int32, Level);

/** Delegate for player death */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDied);

/** Delegate for level up */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelUp, int32, NewLevel);

/**
 * UPlayerVitalSystem
 *
 * Manages the player's health, natural regeneration, experience points, and leveling.
 *
 * Health: 20.0 max (10 hearts × 2 half-hearts), natural regen 1 HP per 4 seconds.
 * XP: Level formula XPToNextLevel = 7 + (Level * 3), matching simplified Minecraft curve.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINECRAFTDEWISH_API UPlayerVitalSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerVitalSystem();

	// ==================== HEALTH ====================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Health")
	float MaxHealth = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitals|Health")
	float CurrentHealth = 20.0f;

	/** Whether natural health regeneration is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Health")
	bool bRegenEnabled = true;

	/** Health regeneration rate (HP per second, default: 0.25 = 1 HP every 4s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Health")
	float RegenRate = 0.25f;

	/** Whether the player is currently alive */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitals|Health")
	bool bIsAlive = true;

	// ==================== EXPERIENCE ====================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitals|XP")
	int32 CurrentXP = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitals|XP")
	int32 XPToNextLevel = 7;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitals|XP")
	int32 Level = 0;

	/** Normalized XP progress [0.0 - 1.0] for UI progress bar */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vitals|XP")
	float XPProgress = 0.0f;

	// ==================== EVENTS ====================

	UPROPERTY(BlueprintAssignable, Category = "Vitals|Events")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Vitals|Events")
	FOnXPChanged OnXPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Vitals|Events")
	FOnPlayerDied OnPlayerDied;

	UPROPERTY(BlueprintAssignable, Category = "Vitals|Events")
	FOnLevelUp OnLevelUp;

	// ==================== HEALTH FUNCTIONS ====================

	/** Apply damage to the player. Returns actual damage dealt. */
	UFUNCTION(BlueprintCallable, Category = "Vitals|Health")
	float ApplyDamage(float DamageAmount, AActor* DamageSource = nullptr);

	/** Heal the player by the given amount. Returns actual health restored. */
	UFUNCTION(BlueprintCallable, Category = "Vitals|Health")
	float Heal(float HealAmount);

	/** Instantly kill the player */
	UFUNCTION(BlueprintCallable, Category = "Vitals|Health")
	void Kill();

	/** Respawn the player with full health at the given location */
	UFUNCTION(BlueprintCallable, Category = "Vitals|Health")
	void Respawn(const FVector& SpawnLocation);

	/** Returns health as a normalized value [0.0 - 1.0] for UI */
	UFUNCTION(BlueprintPure, Category = "Vitals|Health")
	float GetHealthPercent() const { return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f; }

	// ==================== XP FUNCTIONS ====================

	/** Add XP to the player. Handles level-up logic. */
	UFUNCTION(BlueprintCallable, Category = "Vitals|XP")
	void AddXP(int32 Amount);

	/** Returns the XP required to reach the next level */
	UFUNCTION(BlueprintPure, Category = "Vitals|XP")
	static int32 CalculateXPForLevel(int32 InLevel);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float RegenAccumulator = 0.0f;

	void ProcessRegen(float DeltaTime);
	void HandleDeath();
	void RecalculateXPProgress();
};
