#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ToolTypes.h"
#include "ToolData.h"
#include "BaublesSystem.generated.h"

/** Delegate fired when a tool is equipped or unequipped */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnToolSlotChanged, EToolType, SlotType, const FToolInstance&, ToolData);

/** Delegate fired when a tool is equipped or unequipped */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaublesToggled, bool, bIsOpen);

/** Delegate fired when the auto-selected active tool changes (ADR-0006). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActiveToolChanged, EToolType, NewToolType);

/**
 * UBaublesSystem
 *
 * Minecraft Baubles-style equipment component that manages 5 dedicated tool slots.
 * Instead of cluttering the quickbar with tools, each tool type has its own
 * permanent slot (Pickaxe, Shovel, Axe, Hoe, Sword).
 *
 * The system automatically selects the best equipped tool for any mining or combat action.
 * Open the Baubles UI with the B key (IA_Baubles) to equip/unequip tools.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINECRAFTDEWISH_API UBaublesSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UBaublesSystem();

	// ==================== TOOL SLOTS ====================

	/** Pickaxe slot — effective against stone, ores */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Baubles|Slots")
	FToolInstance PickaxeSlot;

	/** Shovel slot — effective against dirt, sand, gravel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Baubles|Slots")
	FToolInstance ShovelSlot;

	/** Axe slot — effective against wood, planks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Baubles|Slots")
	FToolInstance AxeSlot;

	/** Hoe slot — used for farmland tilling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Baubles|Slots")
	FToolInstance HoeSlot;

	/** Sword slot — used for combat damage bonus */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Baubles|Slots")
	FToolInstance SwordSlot;

	/** Whether the baubles UI panel is currently open */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baubles|State")
	bool bIsBaublesOpen = false;

	/** Currently auto-selected active tool (ADR-0006). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baubles|State")
	EToolType ActiveToolType = EToolType::None;

	/** Anti-bounce delay (seconds) before committing an auto-tool change. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Baubles|AutoSelect", meta = (ClampMin = "0.1", ClampMax = "0.5"))
	float AutoSelectAntiBounceSeconds = 0.175f;

	/** Data-driven tool definitions (DT_Tools wrapper). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Baubles|Data")
	TObjectPtr<UToolDefinition> ToolDefinition;

	// ==================== EVENTS ====================

	UPROPERTY(BlueprintAssignable, Category = "Baubles|Events")
	FOnToolSlotChanged OnToolEquipped;

	UPROPERTY(BlueprintAssignable, Category = "Baubles|Events")
	FOnToolSlotChanged OnToolUnequipped;

	UPROPERTY(BlueprintAssignable, Category = "Baubles|Events")
	FOnBaublesToggled OnBaublesToggled;

	/** Fired when the auto-selected active tool changes (anti-bounced). */
	UPROPERTY(BlueprintAssignable, Category = "Baubles|Events")
	FOnActiveToolChanged OnActiveToolChanged;

	// ==================== TOOL MANAGEMENT ====================

	/**
	 * Equips a tool into its corresponding slot.
	 * If a tool of the same type is already equipped, it is replaced.
	 * @return True if successfully equipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Baubles")
	bool EquipTool(const FToolInstance& Tool);

	/**
	 * Equips a tool from a stable ToolId (resolves static data via ToolDefinition).
	 */
	UFUNCTION(BlueprintCallable, Category = "Baubles")
	bool EquipToolFromToolId(FName ToolId);

	/**
	 * Equips a tool from (type, tier) (resolves static data via ToolDefinition).
	 */
	UFUNCTION(BlueprintCallable, Category = "Baubles")
	bool EquipToolFromTypeTier(EToolType ToolType, EToolTier ToolTier);

	/**
	 * Removes the tool from its type-specific slot.
	 * @return The removed tool instance (check IsValid).
	 */
	UFUNCTION(BlueprintCallable, Category = "Baubles")
	FToolInstance UnequipTool(EToolType SlotType);

	/**
	 * Returns the tool currently equipped in the given slot.
	 */
	UFUNCTION(BlueprintPure, Category = "Baubles")
	FToolInstance GetToolInSlot(EToolType SlotType) const;

	/**
	 * Returns whether a tool is equipped in the given slot.
	 */
	UFUNCTION(BlueprintPure, Category = "Baubles")
	bool HasToolEquipped(EToolType SlotType) const;

	// ==================== AUTO-TOOL SELECTION (ADR-0006) ====================

	/**
	 * Requests an auto-tool selection for the given candidate tool type.
	 * The change is committed only after AutoSelectAntiBounceSeconds of stability.
	 */
	UFUNCTION(BlueprintCallable, Category = "Baubles|AutoSelect")
	void RequestAutoToolSelection(EToolType CandidateToolType);

	/** Forces an immediate commit of the pending auto-tool selection (if any). */
	UFUNCTION(BlueprintCallable, Category = "Baubles|AutoSelect")
	void CommitAutoToolSelection();

	/** Cancels any pending auto-tool selection. */
	UFUNCTION(BlueprintCallable, Category = "Baubles|AutoSelect")
	void CancelAutoToolSelection();

	/** Returns the currently active (auto-selected) tool type. */
	UFUNCTION(BlueprintPure, Category = "Baubles|AutoSelect")
	EToolType GetActiveToolType() const { return ActiveToolType; }

	/** Returns the tool instance currently active (from its slot). */
	UFUNCTION(BlueprintPure, Category = "Baubles|AutoSelect")
	FToolInstance GetActiveTool() const;

	// ==================== MINING INTEGRATION ====================

	/**
	 * Returns the best mining force for a given block based on all equipped tools.
	 * Automatically picks the tool whose type matches the block's preferred tool type.
	 * If no matching tool is equipped, returns hand speed (1.0).
	 */
	UFUNCTION(BlueprintPure, Category = "Baubles|Mining")
	float GetBestMiningForce(uint8 BlockID) const;

	/**
	 * Returns the best equipped tool for mining a specific block.
	 * Returns an invalid FToolInstance if no suitable tool is equipped.
	 */
	UFUNCTION(BlueprintPure, Category = "Baubles|Mining")
	FToolInstance GetBestToolForBlock(uint8 BlockID) const;

	/**
	 * Consumes 1 durability from the tool best suited for the given block.
	 * Called automatically when a block is successfully broken.
	 * @return True if the tool broke from this usage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Baubles|Mining")
	bool ConsumeToolDurability(uint8 BlockID);

	// ==================== COMBAT INTEGRATION ====================

	/**
	 * Returns attack damage based on equipped sword.
	 * If no sword is equipped, returns 1.0 (fist damage).
	 */
	UFUNCTION(BlueprintPure, Category = "Baubles|Combat")
	float GetAttackDamage() const;

	// ==================== UI ====================

	/** Toggles the baubles UI open/closed */
	UFUNCTION(BlueprintCallable, Category = "Baubles|UI")
	void ToggleBaublesUI();

private:
	/** Returns a mutable reference to the slot for a given tool type */
	FToolInstance& GetSlotRef(EToolType SlotType);
	const FToolInstance& GetSlotConstRef(EToolType SlotType) const;

	/** Empty default tool for returning const references */
	static const FToolInstance EmptyTool;

	/** Pending auto-tool candidate during anti-bounce window. */
	EToolType PendingAutoToolType = EToolType::None;
	bool bHasPendingAutoTool = false;

	FTimerHandle AutoSelectTimerHandle;

	void CommitPendingAutoTool();
};
