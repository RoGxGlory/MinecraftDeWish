#include "PlayerVitalSystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UPlayerVitalSystem::UPlayerVitalSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f; // Tick 4 times/sec for regen
}

void UPlayerVitalSystem::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	bIsAlive = true;
	XPToNextLevel = CalculateXPForLevel(Level);
	RecalculateXPProgress();
}

void UPlayerVitalSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsAlive)
	{
		ProcessRegen(DeltaTime);
	}
}

// ==================== HEALTH ====================

float UPlayerVitalSystem::ApplyDamage(float DamageAmount, AActor* DamageSource)
{
	if (!bIsAlive || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float ActualDamage = FMath::Min(DamageAmount, CurrentHealth);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, ActualDamage);

	UE_LOG(LogTemp, Log, TEXT("PlayerVitals: Took %.1f damage (HP: %.1f/%.1f) Source: %s"),
		ActualDamage, CurrentHealth, MaxHealth,
		DamageSource ? *DamageSource->GetName() : TEXT("None"));

	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
	}

	return ActualDamage;
}

float UPlayerVitalSystem::Heal(float HealAmount)
{
	if (!bIsAlive || HealAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float ActualHeal = FMath::Min(HealAmount, MaxHealth - CurrentHealth);
	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + HealAmount);

	if (ActualHeal > 0.0f)
	{
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, -ActualHeal);
	}

	return ActualHeal;
}

void UPlayerVitalSystem::Kill()
{
	if (bIsAlive)
	{
		CurrentHealth = 0.0f;
		HandleDeath();
	}
}

void UPlayerVitalSystem::Respawn(const FVector& SpawnLocation)
{
	CurrentHealth = MaxHealth;
	bIsAlive = true;

	// Drop some XP on death (Minecraft mechanic: lose levels)
	const int32 XPLost = FMath::Min(Level * 7, CurrentXP);
	CurrentXP = FMath::Max(0, CurrentXP - XPLost);

	// Recalculate level from remaining XP
	Level = 0;
	XPToNextLevel = CalculateXPForLevel(0);
	int32 TempXP = CurrentXP;
	while (TempXP >= XPToNextLevel)
	{
		TempXP -= XPToNextLevel;
		Level++;
		XPToNextLevel = CalculateXPForLevel(Level);
	}
	CurrentXP = TempXP;
	RecalculateXPProgress();

	// Teleport player
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(SpawnLocation);
	}

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, 0.0f);
	OnXPChanged.Broadcast(CurrentXP, XPToNextLevel, Level);

	UE_LOG(LogTemp, Log, TEXT("PlayerVitals: Respawned at %s (Lost %d XP, Level: %d)"),
		*SpawnLocation.ToString(), XPLost, Level);
}

void UPlayerVitalSystem::HandleDeath()
{
	bIsAlive = false;
	UE_LOG(LogTemp, Warning, TEXT("PlayerVitals: Player died!"));
	OnPlayerDied.Broadcast();
}

void UPlayerVitalSystem::ProcessRegen(float DeltaTime)
{
	if (!bRegenEnabled || CurrentHealth >= MaxHealth)
	{
		RegenAccumulator = 0.0f;
		return;
	}

	RegenAccumulator += RegenRate * DeltaTime;
	if (RegenAccumulator >= 1.0f)
	{
		const float HealAmount = FMath::FloorToFloat(RegenAccumulator);
		RegenAccumulator -= HealAmount;
		Heal(HealAmount);
	}
}

// ==================== XP ====================

void UPlayerVitalSystem::AddXP(int32 Amount)
{
	if (Amount <= 0 || !bIsAlive)
	{
		return;
	}

	CurrentXP += Amount;

	// Level-up loop
	while (CurrentXP >= XPToNextLevel)
	{
		CurrentXP -= XPToNextLevel;
		Level++;
		XPToNextLevel = CalculateXPForLevel(Level);

		UE_LOG(LogTemp, Log, TEXT("PlayerVitals: LEVEL UP! Now Level %d (Next: %d XP)"), Level, XPToNextLevel);
		OnLevelUp.Broadcast(Level);
	}

	RecalculateXPProgress();
	OnXPChanged.Broadcast(CurrentXP, XPToNextLevel, Level);
}

int32 UPlayerVitalSystem::CalculateXPForLevel(int32 InLevel)
{
	// Simplified Minecraft XP curve
	if (InLevel < 16)
	{
		return 2 * InLevel + 7;
	}
	else if (InLevel < 31)
	{
		return 5 * InLevel - 38;
	}
	else
	{
		return 9 * InLevel - 158;
	}
}

void UPlayerVitalSystem::RecalculateXPProgress()
{
	XPProgress = (XPToNextLevel > 0) ? static_cast<float>(CurrentXP) / static_cast<float>(XPToNextLevel) : 0.0f;
}
