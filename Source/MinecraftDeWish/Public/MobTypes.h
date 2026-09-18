#pragma once

#include "CoreMinimal.h"
#include "VoxelDataTypes.h"
#include "MobTypes.generated.h"

/**
 * Hostile mob type identifiers.
 */
UENUM(BlueprintType)
enum class EMobType : uint8
{
	Zombie   = 0 UMETA(DisplayName = "Zombie"),
	Skeleton = 1 UMETA(DisplayName = "Skeleton"),
	Spider   = 2 UMETA(DisplayName = "Spider"),
	Creeper  = 3 UMETA(DisplayName = "Creeper")
};

/**
 * AI behavior state for the mob state machine.
 */
UENUM(BlueprintType)
enum class EMobAIState : uint8
{
	Idle      = 0 UMETA(DisplayName = "Idle"),
	Wander    = 1 UMETA(DisplayName = "Wander"),
	Chase     = 2 UMETA(DisplayName = "Chase"),
	Attack    = 3 UMETA(DisplayName = "Attack"),
	Flee      = 4 UMETA(DisplayName = "Flee"),
	Explode   = 5 UMETA(DisplayName = "Explode (Creeper)")
};

/**
 * Single item drop entry for a mob's loot table.
 */
USTRUCT(BlueprintType)
struct MINECRAFTDEWISH_API FMobDropEntry
{
	GENERATED_BODY()

	/** Block/Item ID to drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Drop")
	uint8 BlockID = 0;

	/** Minimum drop count */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Drop")
	int32 MinCount = 0;

	/** Maximum drop count */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Drop")
	int32 MaxCount = 1;

	/** Drop chance [0.0 - 1.0], 1.0 = always drops */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Drop")
	float DropChance = 1.0f;
};

/**
 * Complete definition of a mob type's stats, behavior flags, and drop table.
 */
USTRUCT(BlueprintType)
struct MINECRAFTDEWISH_API FMobDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob")
	EMobType MobType = EMobType::Zombie;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Stats")
	float MaxHealth = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Stats")
	float AttackDamage = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Stats")
	float MoveSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Stats")
	float AttackRange = 150.0f; // In Unreal units (1.5 blocks)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Stats")
	float AttackCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Stats")
	float DetectionRange = 1600.0f; // 16 blocks

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Stats")
	int32 XPDrop = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Behavior")
	bool bBurnsInSunlight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Behavior")
	bool bCanClimbWalls = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Behavior")
	bool bIsRanged = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Behavior")
	bool bExplodes = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Behavior")
	float ExplosionFuseTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob|Drops")
	TArray<FMobDropEntry> Drops;

	/** Create a default mob definition for a given type */
	static FMobDefinition GetDefaultDefinition(EMobType Type)
	{
		FMobDefinition Def;
		Def.MobType = Type;

		switch (Type)
		{
		case EMobType::Zombie:
			Def.MaxHealth = 20.0f;
			Def.AttackDamage = 3.0f;
			Def.MoveSpeed = 180.0f;
			Def.AttackRange = 150.0f;
			Def.DetectionRange = 1600.0f;
			Def.XPDrop = 5;
			Def.bBurnsInSunlight = true;
			// Drops: Rotten Flesh (using Dirt as placeholder)
			Def.Drops.Add({1, 0, 2, 0.85f});
			break;

		case EMobType::Skeleton:
			Def.MaxHealth = 20.0f;
			Def.AttackDamage = 4.0f;
			Def.MoveSpeed = 200.0f;
			Def.AttackRange = 1500.0f; // Ranged attack
			Def.DetectionRange = 4000.0f; // 40 blocks
			Def.XPDrop = 5;
			Def.bBurnsInSunlight = true;
			Def.bIsRanged = true;
			// Drops: Bones (using Cobblestone as placeholder), Arrows
			Def.Drops.Add({3, 0, 2, 0.85f});
			break;

		case EMobType::Spider:
			Def.MaxHealth = 16.0f;
			Def.AttackDamage = 2.0f;
			Def.MoveSpeed = 260.0f;
			Def.AttackRange = 150.0f;
			Def.DetectionRange = 1600.0f;
			Def.XPDrop = 5;
			Def.bBurnsInSunlight = false; // Spiders don't burn!
			Def.bCanClimbWalls = true;
			// Drops: String (using Leaves as placeholder), Spider Eye
			Def.Drops.Add({17, 0, 2, 0.75f});
			break;

		case EMobType::Creeper:
			Def.MaxHealth = 20.0f;
			Def.AttackDamage = 0.0f; // Damage is from explosion
			Def.MoveSpeed = 200.0f;
			Def.AttackRange = 300.0f; // 3 blocks triggers fuse
			Def.DetectionRange = 1600.0f;
			Def.XPDrop = 5;
			Def.bBurnsInSunlight = false; // Creepers don't burn!
			Def.bExplodes = true;
			Def.ExplosionFuseTime = 1.5f;
			// Drops: Gunpowder (using Sand as placeholder)
			Def.Drops.Add({18, 0, 2, 0.66f});
			break;
		}

		return Def;
	}
};
