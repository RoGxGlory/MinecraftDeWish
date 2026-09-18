#include "MobCharacter.h"
#include "WorldGenerator.h"
#include "DayNightCycleSystem.h"
#include "BlockItemPickup.h"
#include "PlayerVitalSystem.h"
#include "XPOrbPickup.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AMobCharacter::AMobCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Configure character movement for voxel world navigation
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = 200.0f;
		MoveComp->MaxStepHeight = 105.0f; // Can step up 1 block (100cm + margin)
		MoveComp->JumpZVelocity = 500.0f; // Jump roughly 1 block high
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
		MoveComp->GravityScale = 1.5f;
	}

	// Set collision capsule for mob
	GetCapsuleComponent()->SetCapsuleHalfHeight(90.0f);
	GetCapsuleComponent()->SetCapsuleRadius(40.0f);
}

void AMobCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AMobCharacter::InitializeMob(EMobType InType, AWorldGenerator* InWorldGen)
{
	MobType = InType;
	WorldGenerator = InWorldGen;
	MobDefinition = FMobDefinition::GetDefaultDefinition(InType);
	CurrentHealth = MobDefinition.MaxHealth;
	bIsAlive = true;
	CurrentAIState = EMobAIState::Idle;

	// Configure movement speed from definition
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = MobDefinition.MoveSpeed;
	}

	// Spiders are wider and shorter
	if (InType == EMobType::Spider)
	{
		GetCapsuleComponent()->SetCapsuleHalfHeight(50.0f);
		GetCapsuleComponent()->SetCapsuleRadius(60.0f);
	}

	UE_LOG(LogTemp, Log, TEXT("MobCharacter: Initialized %s (HP: %.0f, Speed: %.0f)"),
		*StaticEnum<EMobType>()->GetDisplayNameTextByValue(static_cast<int64>(InType)).ToString(),
		CurrentHealth, MobDefinition.MoveSpeed);
}

void AMobCharacter::GetMobTexturePaths(EMobType InType, FString& OutBaseColor, FString& OutNormal, FString& OutSpecular)
{
	FString FolderName;
	FString AssetName;

	switch (InType)
	{
	case EMobType::Zombie:
		FolderName = TEXT("zombie");
		AssetName = TEXT("zombie");
		break;
	case EMobType::Skeleton:
		FolderName = TEXT("skeleton");
		AssetName = TEXT("skeleton");
		break;
	case EMobType::Spider:
		FolderName = TEXT("spider");
		AssetName = TEXT("spider");
		break;
	case EMobType::Creeper:
		FolderName = TEXT("creeper");
		AssetName = TEXT("creeper");
		break;
	default:
		FolderName = TEXT("zombie");
		AssetName = TEXT("zombie");
		break;
	}

	OutBaseColor = FString::Printf(TEXT("/Game/Patrix_Texture_Pack/textures/entity/%s/%s.%s"), *FolderName, *AssetName, *AssetName);
	OutNormal = FString::Printf(TEXT("/Game/Patrix_Texture_Pack/textures/entity/%s/%s_n.%s_n"), *FolderName, *AssetName, *AssetName);
	OutSpecular = FString::Printf(TEXT("/Game/Patrix_Texture_Pack/textures/entity/%s/%s_s.%s_s"), *FolderName, *AssetName, *AssetName);
}

void AMobCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsAlive)
	{
		return;
	}

	ProcessSunlightBurning(DeltaTime);
	ProcessSpiderClimbing(DeltaTime);
	TickAIStateMachine(DeltaTime);
}

// ==================== HEALTH ====================

float AMobCharacter::ApplyMobDamage(float DamageAmount, AActor* DamageSource)
{
	if (!bIsAlive || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float ActualDamage = FMath::Min(DamageAmount, CurrentHealth);
	CurrentHealth -= ActualDamage;

	// Knockback
	if (DamageSource)
	{
		const FVector KnockDir = (GetActorLocation() - DamageSource->GetActorLocation()).GetSafeNormal2D();
		LaunchCharacter(KnockDir * 400.0f + FVector(0, 0, 200.0f), false, false);
	}

	UE_LOG(LogTemp, Log, TEXT("Mob %s took %.1f damage (HP: %.1f/%.1f)"),
		*GetName(), ActualDamage, CurrentHealth, MobDefinition.MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		HandleDeath(DamageSource);
	}

	return ActualDamage;
}

// ==================== SUNLIGHT ====================

bool AMobCharacter::IsExposedToSky() const
{
	if (!WorldGenerator.IsValid())
	{
		return true;
	}

	const FVector Loc = GetActorLocation();
	const float BlockScale = WorldGenerator->BlockScale;
	const int32 VoxelX = FMath::FloorToInt(Loc.X / BlockScale);
	const int32 VoxelY = FMath::FloorToInt(Loc.Y / BlockScale);
	const int32 VoxelZ = FMath::FloorToInt(Loc.Z / BlockScale);

	// Check every block above the mob up to chunk height
	for (int32 Z = VoxelZ + 1; Z < WorldGenerator->ChunkHeight; ++Z)
	{
		uint8 BlockID = 0;
		if (WorldGenerator->GetVoxelAt(VoxelX, VoxelY, Z, BlockID))
		{
			if (BlockID != 0 && !FBlockHelpers::IsNonSolidBlock(BlockID))
			{
				return false; // Covered by a solid block
			}
		}
	}
	return true;
}

UDayNightCycleSystem* AMobCharacter::GetDayNightCycle() const
{
	if (WorldGenerator.IsValid())
	{
		return WorldGenerator->FindComponentByClass<UDayNightCycleSystem>();
	}
	return nullptr;
}

void AMobCharacter::ProcessSunlightBurning(float DeltaTime)
{
	if (!MobDefinition.bBurnsInSunlight)
	{
		return;
	}

	UDayNightCycleSystem* DayNight = GetDayNightCycle();
	if (!DayNight || !DayNight->IsDay())
	{
		SunBurnAccumulator = 0.0f;
		return;
	}

	if (!IsExposedToSky())
	{
		SunBurnAccumulator = 0.0f;
		return;
	}

	// Burn in sunlight: 1 HP per second
	SunBurnAccumulator += DeltaTime;
	if (SunBurnAccumulator >= 1.0f)
	{
		SunBurnAccumulator -= 1.0f;
		ApplyMobDamage(1.0f, nullptr);
	}
}

// ==================== SPIDER CLIMBING ====================

void AMobCharacter::ProcessSpiderClimbing(float DeltaTime)
{
	if (!MobDefinition.bCanClimbWalls)
	{
		return;
	}

	if (!WorldGenerator.IsValid())
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp || !MoveComp->IsFalling())
	{
		return;
	}

	// Check if there's a wall directly in front of the spider
	const FVector ForwardDir = GetActorForwardVector();
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart + ForwardDir * 80.0f;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
	{
		// Wall detected — climb by adding upward velocity
		MoveComp->Velocity.Z = MobDefinition.MoveSpeed * 0.7f;
	}
}

// ==================== AI STATE MACHINE ====================

APawn* AMobCharacter::FindClosestPlayer() const
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->GetPawn())
	{
		const float Dist = FVector::Dist(GetActorLocation(), PC->GetPawn()->GetActorLocation());
		if (Dist <= MobDefinition.DetectionRange)
		{
			return PC->GetPawn();
		}
	}
	return nullptr;
}

void AMobCharacter::TickAIStateMachine(float DeltaTime)
{
	APawn* Target = FindClosestPlayer();
	const float DistToTarget = Target ? FVector::Dist(GetActorLocation(), Target->GetActorLocation()) : MAX_FLT;

	// Update cooldown timer
	if (AttackCooldownTimer > 0.0f)
	{
		AttackCooldownTimer -= DeltaTime;
	}

	switch (CurrentAIState)
	{
	case EMobAIState::Idle:
		WanderTimer += DeltaTime;
		if (WanderTimer > 2.0f)
		{
			CurrentAIState = EMobAIState::Wander;
			WanderTimer = 0.0f;
			WanderDirection = FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal();
		}
		if (Target && DistToTarget <= MobDefinition.DetectionRange)
		{
			CurrentAIState = EMobAIState::Chase;
		}
		break;

	case EMobAIState::Wander:
		ProcessWander(DeltaTime);
		if (Target && DistToTarget <= MobDefinition.DetectionRange)
		{
			CurrentAIState = EMobAIState::Chase;
		}
		break;

	case EMobAIState::Chase:
		if (!Target)
		{
			CurrentAIState = EMobAIState::Idle;
			break;
		}
		if (DistToTarget <= MobDefinition.AttackRange)
		{
			CurrentAIState = MobDefinition.bExplodes ? EMobAIState::Explode : EMobAIState::Attack;
			break;
		}
		ProcessChase(DeltaTime, Target);
		break;

	case EMobAIState::Attack:
		if (!Target || DistToTarget > MobDefinition.AttackRange * 1.5f)
		{
			CurrentAIState = Target ? EMobAIState::Chase : EMobAIState::Idle;
			break;
		}
		ProcessAttack(DeltaTime, Target);
		break;

	case EMobAIState::Explode:
		ProcessExplosion(DeltaTime, Target);
		break;

	default:
		CurrentAIState = EMobAIState::Idle;
		break;
	}
}

void AMobCharacter::ProcessWander(float DeltaTime)
{
	WanderTimer += DeltaTime;

	AddMovementInput(WanderDirection, 0.4f);

	if (WanderTimer > 4.0f)
	{
		CurrentAIState = EMobAIState::Idle;
		WanderTimer = 0.0f;
	}
}

void AMobCharacter::ProcessChase(float DeltaTime, APawn* Target)
{
	if (!Target)
	{
		return;
	}

	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	AddMovementInput(ToTarget, 1.0f);

	// Face the target
	const FRotator LookRot = ToTarget.Rotation();
	SetActorRotation(FRotator(0.0f, LookRot.Yaw, 0.0f));

	// Jump if there's a block in front at ground level
	if (GetCharacterMovement() && !GetCharacterMovement()->IsFalling())
	{
		const FVector TraceStart = GetActorLocation() + FVector(0, 0, -40.0f);
		const FVector TraceEnd = TraceStart + ToTarget * 80.0f;

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
		{
			Jump();
		}
	}
}

void AMobCharacter::ProcessAttack(float DeltaTime, APawn* Target)
{
	if (!Target || AttackCooldownTimer > 0.0f)
	{
		return;
	}

	// Deal damage to the player
	UPlayerVitalSystem* PlayerVitals = Target->FindComponentByClass<UPlayerVitalSystem>();
	if (PlayerVitals)
	{
		PlayerVitals->ApplyDamage(MobDefinition.AttackDamage, this);
	}

	AttackCooldownTimer = MobDefinition.AttackCooldown;

	UE_LOG(LogTemp, Log, TEXT("Mob %s attacked player for %.1f damage"), *GetName(), MobDefinition.AttackDamage);
}

void AMobCharacter::ProcessExplosion(float DeltaTime, APawn* Target)
{
	if (!bFuseActive)
	{
		bFuseActive = true;
		ExplosionFuseTimer = MobDefinition.ExplosionFuseTime;
	}

	// If target moves away, cancel fuse
	if (Target)
	{
		const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
		if (Dist > MobDefinition.AttackRange * 2.0f)
		{
			bFuseActive = false;
			CurrentAIState = EMobAIState::Chase;
			return;
		}
	}

	ExplosionFuseTimer -= DeltaTime;
	if (ExplosionFuseTimer <= 0.0f)
	{
		// EXPLODE: damage player and destroy nearby blocks
		if (Target)
		{
			UPlayerVitalSystem* PlayerVitals = Target->FindComponentByClass<UPlayerVitalSystem>();
			if (PlayerVitals)
			{
				const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
				const float DamageScale = FMath::Clamp(1.0f - (Dist / 500.0f), 0.1f, 1.0f);
				PlayerVitals->ApplyDamage(43.0f * DamageScale, this); // Creeper max damage
			}
		}

		// Destroy blocks in radius (3 blocks)
		if (WorldGenerator.IsValid())
		{
			const float BlockScale = WorldGenerator->BlockScale;
			const int32 CenterX = FMath::FloorToInt(GetActorLocation().X / BlockScale);
			const int32 CenterY = FMath::FloorToInt(GetActorLocation().Y / BlockScale);
			const int32 CenterZ = FMath::FloorToInt(GetActorLocation().Z / BlockScale);
			const int32 Radius = 3;

			for (int32 Dx = -Radius; Dx <= Radius; ++Dx)
			{
				for (int32 Dy = -Radius; Dy <= Radius; ++Dy)
				{
					for (int32 Dz = -Radius; Dz <= Radius; ++Dz)
					{
						if (Dx * Dx + Dy * Dy + Dz * Dz <= Radius * Radius)
						{
							uint8 DroppedID = 0;
							WorldGenerator->BreakBlockAtVoxel(CenterX + Dx, CenterY + Dy, CenterZ + Dz, DroppedID);
						}
					}
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("Mob %s EXPLODED!"), *GetName());

		// Creeper dies in explosion (no drops)
		bIsAlive = false;
		SpawnXPOrbs();
		Destroy();
	}
}

// ==================== DEATH ====================

void AMobCharacter::HandleDeath(AActor* Killer)
{
	bIsAlive = false;
	CurrentAIState = EMobAIState::Idle;

	UE_LOG(LogTemp, Log, TEXT("Mob %s died. Killer: %s"), *GetName(), Killer ? *Killer->GetName() : TEXT("None"));

	SpawnDrops();
	SpawnXPOrbs();

	OnMobDied.Broadcast(this, Killer);

	// Destroy after short delay so any effects can play
	SetLifeSpan(0.1f);
}

void AMobCharacter::SpawnDrops()
{
	if (!WorldGenerator.IsValid())
	{
		return;
	}

	for (const FMobDropEntry& Drop : MobDefinition.Drops)
	{
		if (FMath::FRand() <= Drop.DropChance)
		{
			const int32 Count = FMath::RandRange(Drop.MinCount, Drop.MaxCount);
			if (Count > 0 && Drop.BlockID != 0)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				ABlockItemPickup* Pickup = GetWorld()->SpawnActor<ABlockItemPickup>(
					ABlockItemPickup::StaticClass(),
					GetActorLocation() + FVector(0, 0, 30.0f),
					FRotator::ZeroRotator,
					SpawnParams
				);

				if (Pickup)
				{
					Pickup->InitializePickup(Drop.BlockID, Count, WorldGenerator->TerrainMaterial);
				}
			}
		}
	}
}

void AMobCharacter::SpawnXPOrbs()
{
	if (MobDefinition.XPDrop <= 0)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn 1-3 XP orbs dividing the total XP
	const int32 TotalXP = MobDefinition.XPDrop;
	const int32 OrbCount = FMath::Clamp(TotalXP / 2, 1, 4);
	const int32 XPPerOrb = FMath::Max(1, TotalXP / OrbCount);

	for (int32 i = 0; i < OrbCount; ++i)
	{
		const FVector SpawnLoc = GetActorLocation() + FVector(
			FMath::FRandRange(-20.0f, 20.0f),
			FMath::FRandRange(-20.0f, 20.0f),
			FMath::FRandRange(10.0f, 40.0f)
		);

		AXPOrbPickup* Orb = GetWorld()->SpawnActor<AXPOrbPickup>(
			AXPOrbPickup::StaticClass(),
			SpawnLoc,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (Orb)
		{
			Orb->InitializeXP(XPPerOrb);
		}
	}
}
