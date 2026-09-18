#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "QuickSlotsInventorySystem.generated.h"

/** Represents an item entry in a quick slot */
USTRUCT(BlueprintType)
struct MINECRAFTDEWISH_API FQuickSlotData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	uint8 BlockID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 ItemCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 MaxStackSize = 64;

	bool IsEmpty() const { return BlockID == 0 || ItemCount <= 0; }
};

/**
 * UQuickSlotsInventorySystem
 * 
 * Modular inventory management subsystem for the player's 9 quickslots.
 * Directly integrates with the player's AC_Inventory actor component and WB_QuickSlots widget.
 * 
 * Responsibilities:
 * 1. 9-slot inventory data model (64 items max per stack).
 * 2. Ingesting dropped item pickups into the player's quickslots.
 * 3. Interfacing with AC_Inventory and WB_HUD to locate the active on-screen WB_QuickSlots widget.
 * 4. Updating slot Image brushes (ImageItem1..9) with block textures from ChunkActor::GetBlockIconTexture.
 * 5. Updating ItemName display for the selected slot and dispatching ProcessBlock with S_Block_Data.
 */
UCLASS(BlueprintType, Blueprintable)
class MINECRAFTDEWISH_API UQuickSlotsInventorySystem : public UObject
{
	GENERATED_BODY()

public:
	UQuickSlotsInventorySystem();

	/** Total quick slots available (9 slots, matching WB_QuickSlots) */
	static constexpr int32 NUM_QUICK_SLOTS = 9;

	/** Maximum items per slot for stackable blocks */
	static constexpr int32 MAX_STACK_SIZE = 64;

	/**
	 * Ingests an item pickup into the player's quickslots inventory.
	 * Resolves the player's AC_Inventory component, stores the item, and updates WB_QuickSlots.
	 * 
	 * @param PlayerActor The player pawn/character actor picking up the item.
	 * @param BlockID The ID of the picked up block.
	 * @param Count Number of items to add.
	 * @param OutRemainingCount Number of items that could not fit (if inventory is full).
	 * @return True if at least one item was successfully added to quickslots.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlots")
	static bool TryAddItemToPlayerInventory(
		AActor* PlayerActor,
		uint8 BlockID,
		int32 Count,
		int32& OutRemainingCount
	);

	/**
	 * Resolves the active, on-screen WB_QuickSlots widget for a player.
	 * Traverses PlayerActor -> AC_Inventory -> WB_HUD -> WB_QuickSlots,
	 * with viewport fallback (strictly excluding Class Default Objects).
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlots")
	static UUserWidget* ResolveQuickSlotsWidget(AActor* PlayerActor);

	/**
	 * Updates the visual display of a specific slot on WB_QuickSlots.
	 * Sets the ImageItem brush to the block's icon texture and makes it visible.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlots")
	static void RefreshQuickSlotVisual(
		UUserWidget* QuickSlotsWidget,
		int32 SlotIndex, // 1 to 9
		uint8 BlockID,
		int32 ItemCount,
		const FName& BlockName
	);

	/** Returns slot data for a specific slot index (0 to 8) */
	UFUNCTION(BlueprintPure, Category = "Inventory|QuickSlots")
	FQuickSlotData GetSlotData(int32 SlotIndex) const;

	/** Sets slot data directly */
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlots")
	void SetSlotData(int32 SlotIndex, const FQuickSlotData& InData);

private:
	/** Internal slot array */
	UPROPERTY()
	TArray<FQuickSlotData> QuickSlots;
};
