#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VoxelDataTypes.h"
#include "CraftingSmeltingSystem.generated.h"

/**
 * Smelting recipe definition
 */
USTRUCT(BlueprintType)
struct MINECRAFTDEWISH_API FSmeltRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smelting")
	uint8 InputBlockID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smelting")
	uint8 OutputBlockID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smelting")
	float CookTime = 8.0f; // 8 seconds per smelt standard
};

/**
 * UCraftingSmeltingSystem
 * 
 * Modular system handling:
 * 1. Tool crafting progression at Crafting Tables.
 * 2. Furnace smelting recipes (Iron Ore -> Ingot, Log -> Charcoal, Sand -> Glass, etc.).
 * 3. Fuel burn durations and efficiencies:
 *    - Coal / Charcoal: 80.0s (smelts 10 items)
 *    - Wood Log: 15.0s (smelts ~2 items)
 *    - Wood Planks: 10.0s (smelts ~1 item)
 */
UCLASS(BlueprintType, Blueprintable)
class MINECRAFTDEWISH_API UCraftingSmeltingSystem : public UObject
{
	GENERATED_BODY()

public:
	UCraftingSmeltingSystem();

	// ==================== CRAFTING ====================

	/**
	 * Checks whether a given tool tier can be crafted based on player progression.
	 * 
	 * @param CurrentTier The player's current equipped tool tier.
	 * @param DesiredTier The tier the player wants to craft.
	 * @return True if craftable.
	 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool CanCraftTool(EToolTier CurrentTier, EToolTier DesiredTier) const;

	/**
	 * Attempts to craft and upgrade the player's tool tier.
	 * 
	 * @param CurrentTier [In/Out] The player's current tool tier, updated on success.
	 * @param DesiredTier The tier to craft.
	 * @param OutMessage User feedback message describing the crafting result.
	 * @return True if crafted successfully.
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool CraftTool(UPARAM(ref) EToolTier& CurrentTier, EToolTier DesiredTier, FString& OutMessage);

	// ==================== SMELTING ====================

	/** Remaining burn duration in seconds of current active fuel */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smelting|State")
	float FurnaceBurnTimeRemaining = 0.0f;

	/** Total burn duration in seconds of the current fuel piece */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smelting|State")
	float FurnaceTotalBurnTime = 0.0f;

	/** Normalized smelting progress of the current cooking item [0.0 - 1.0] */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smelting|State")
	float FurnaceSmeltProgress = 0.0f;

	/**
	 * Processes a smelting operation.
	 * 
	 * @param InputBlockID The raw material block to smelt.
	 * @param FuelBlockID The fuel block used to burn.
	 * @param OutResultBlockID The resulting smelted item ID.
	 * @param OutMessage Feedback message describing the result.
	 * @return True if smelting successfully produced an item.
	 */
	UFUNCTION(BlueprintCallable, Category = "Smelting")
	bool SmeltItem(uint8 InputBlockID, uint8 FuelBlockID, uint8& OutResultBlockID, FString& OutMessage);

	/**
	 * Advances the active furnace burn and smelt progress timers.
	 * 
	 * @param DeltaTime Time elapsed in seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Smelting")
	void TickFurnace(float DeltaTime);

	/**
	 * Returns the fuel burn duration in seconds for a given block ID.
	 * - Coal (16 or 27) & Charcoal: 80.0s
	 * - Wood Log (5): 15.0s
	 * - Wood Planks (6): 10.0s
	 * - Other: 0.0s (not fuel)
	 */
	UFUNCTION(BlueprintPure, Category = "Smelting")
	static float GetFuelBurnDuration(uint8 FuelBlockID);

	/**
	 * Looks up the output product ID for a given input block.
	 * Returns 0 if input is not smeltable.
	 */
	UFUNCTION(BlueprintPure, Category = "Smelting")
	static uint8 GetSmeltResult(uint8 InputBlockID);

	/**
	 * Returns true if the block ID is a valid furnace fuel.
	 */
	UFUNCTION(BlueprintPure, Category = "Smelting")
	static bool IsValidFuel(uint8 BlockID);
};
