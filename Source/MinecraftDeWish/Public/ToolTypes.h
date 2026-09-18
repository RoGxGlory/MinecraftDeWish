#pragma once

#include "CoreMinimal.h"
#include "VoxelDataTypes.h"
#include "ToolTypes.generated.h"

class UTexture2D;

/**
 * Tool type categories matching Minecraft's tool specialization system.
 * Each tool type is most effective against specific block categories.
 */
UENUM(BlueprintType)
enum class EToolType : uint8
{
	None     = 0 UMETA(DisplayName = "No Tool (Hand)"),
	Pickaxe  = 1 UMETA(DisplayName = "Pickaxe"),   // Stone, ores, furnace
	Shovel   = 2 UMETA(DisplayName = "Shovel"),     // Dirt, sand, gravel
	Axe      = 3 UMETA(DisplayName = "Axe"),        // Wood, planks, fences
	Hoe      = 4 UMETA(DisplayName = "Hoe"),        // Farmland tilling
	Sword    = 5 UMETA(DisplayName = "Sword")       // Combat damage, no mining bonus
};

/**
 * FToolInstance
 *
 * Represents a single equipped tool with its type, tier, and durability state.
 * Used by the BaublesSystem to track equipped tools in dedicated slots.
 */
USTRUCT(BlueprintType)
struct MINECRAFTDEWISH_API FToolInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool")
	EToolType ToolType = EToolType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool")
	EToolTier ToolTier = EToolTier::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool")
	int32 CurrentDurability = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool")
	int32 MaxDurability = 0;

	FToolInstance() = default;

	FToolInstance(EToolType InType, EToolTier InTier)
		: ToolType(InType), ToolTier(InTier)
	{
		MaxDurability = CalculateMaxDurability(InTier);
		CurrentDurability = MaxDurability;
	}

	bool IsValid() const { return ToolType != EToolType::None && ToolTier != EToolTier::None; }
	bool IsBroken() const { return CurrentDurability <= 0; }

	/** Returns base mining force for this tool's tier */
	float GetBaseMiningForce() const
	{
		return CalculateMiningForce(ToolTier);
	}

	/** Calculate base mining force based on tool tier */
	static float CalculateMiningForce(EToolTier Tier)
	{
		switch (Tier)
		{
		case EToolTier::Wood:     return 2.0f;
		case EToolTier::Stone:    return 4.0f;
		case EToolTier::Iron:     return 6.0f;
		case EToolTier::Diamond:  return 10.0f;
		case EToolTier::Obsidian: return 15.0f;
		default:                  return 1.0f;
		}
	}

	/**
	 * Returns the effective mining force when used against a specific block.
	 * Tools used on their specialized block type get full force.
	 * Tools used on non-specialized blocks get 1.0 (hand speed).
	 */
	float GetEffectiveMiningForce(uint8 BlockID) const
	{
		if (!IsValid() || IsBroken())
		{
			return 1.0f; // Hand speed
		}

		// Check if this tool type matches the block's preferred tool
		const uint8 PreferredTool = FBlockHelpers::GetPreferredToolType(BlockID);
		if (PreferredTool == 0 || PreferredTool == static_cast<uint8>(ToolType))
		{
			return GetBaseMiningForce();
		}

		// Wrong tool type: reduced effectiveness (hand speed)
		return 1.0f;
	}

	/** Returns combat damage bonus for swords */
	float GetAttackDamage() const
	{
		if (ToolType != EToolType::Sword || IsBroken())
		{
			return 1.0f; // Fist damage
		}

		switch (ToolTier)
		{
		case EToolTier::Wood:     return 4.0f;
		case EToolTier::Stone:    return 5.0f;
		case EToolTier::Iron:     return 6.0f;
		case EToolTier::Diamond:  return 7.0f;
		case EToolTier::Obsidian: return 8.0f;
		default:                  return 1.0f;
		}
	}

	/** Reduces durability by 1 use. Returns true if tool broke. */
	bool ConsumeDurability(int32 Amount = 1)
	{
		CurrentDurability = FMath::Max(0, CurrentDurability - Amount);
		return CurrentDurability <= 0;
	}

	/** Calculate max durability based on tier (Minecraft values) */
	static int32 CalculateMaxDurability(EToolTier Tier)
	{
		switch (Tier)
		{
		case EToolTier::Wood:     return 59;
		case EToolTier::Stone:    return 131;
		case EToolTier::Iron:     return 250;
		case EToolTier::Diamond:  return 1561;
		case EToolTier::Obsidian: return 2031;
		default:                  return 0;
		}
	}

	/** Get a display name for this tool */
	FString GetDisplayName() const
	{
		const UEnum* TypeEnum = StaticEnum<EToolType>();
		const UEnum* TierEnum = StaticEnum<EToolTier>();
		if (TypeEnum && TierEnum)
		{
			FString TierName = TierEnum->GetDisplayNameTextByValue(static_cast<int64>(ToolTier)).ToString();
			FString TypeName = TypeEnum->GetDisplayNameTextByValue(static_cast<int64>(ToolType)).ToString();
			return FString::Printf(TEXT("%s %s"), *TierName, *TypeName);
		}
		return TEXT("Unknown Tool");
	}

	/** Returns the 2D icon texture for this tool from the Patrix texture pack */
	static UTexture2D* GetToolIconTexture(EToolType Type, EToolTier Tier)
	{
		FString TypeStr;
		switch (Type)
		{
		case EToolType::Pickaxe: TypeStr = TEXT("pickaxe"); break;
		case EToolType::Shovel:  TypeStr = TEXT("shovel"); break;
		case EToolType::Axe:     TypeStr = TEXT("axe"); break;
		case EToolType::Hoe:     TypeStr = TEXT("hoe"); break;
		case EToolType::Sword:   TypeStr = TEXT("sword"); break;
		default: return nullptr;
		}

		FString TierStr;
		switch (Tier)
		{
		case EToolTier::Wood:     TierStr = TEXT("wooden"); break;
		case EToolTier::Stone:    TierStr = TEXT("stone"); break;
		case EToolTier::Iron:     TierStr = TEXT("iron"); break;
		case EToolTier::Diamond:  TierStr = TEXT("diamond"); break;
		case EToolTier::Obsidian: TierStr = TEXT("netherite"); break;
		default: return nullptr;
		}

		const FString AssetName = FString::Printf(TEXT("%s_%s"), *TierStr, *TypeStr);
		const FString Path = FString::Printf(TEXT("/Game/Patrix_Texture_Pack/textures/item/%s.%s"), *AssetName, *AssetName);
		return Cast<UTexture2D>(StaticLoadObject(UObject::StaticClass(), nullptr, *Path));
	}

	UTexture2D* GetIconTexture() const
	{
		return GetToolIconTexture(ToolType, ToolTier);
	}
};
