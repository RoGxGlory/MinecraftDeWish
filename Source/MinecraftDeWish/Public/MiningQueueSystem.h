#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VoxelDataTypes.h"
#include "MiningQueueSystem.generated.h"

/**
 * Delegate broadcast when a queued block mining operation completes.
 * @param VoxelCoord The global voxel coordinates (X, Y, Z) that was broken.
 * @param BlockID The ID of the block that was broken.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBlockMiningCompleted, const FIntVector&, VoxelCoord, uint8, BlockID);

/**
 * UMiningQueueSystem
 * 
 * Modular subsystem responsible for managing concurrent block destruction requests,
 * tool tiers, mining force calculations, and active mining task queues.
 * 
 * Tool Progression & Concurrent Mining Capabilities:
 * - Hand (No Tool): Force = 1.0, Max Queue Slots = 1
 * - Wood Tool:      Force = 2.0, Max Queue Slots = 2
 * - Stone Tool:     Force = 4.0, Max Queue Slots = 3
 * - Iron Tool:      Force = 6.0, Max Queue Slots = 4
 * - Diamond Tool:   Force = 10.0, Max Queue Slots = 5
 * - Obsidian Tool:  Force = 15.0, Max Queue Slots = 6
 */
UCLASS(BlueprintType, Blueprintable)
class MINECRAFTDEWISH_API UMiningQueueSystem : public UObject
{
	GENERATED_BODY()

public:
	UMiningQueueSystem();

	/** Current equipped tool tier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mining|Tools")
	EToolTier CurrentToolTier = EToolTier::None;

	/** Delegate fired whenever a block finishes mining */
	UPROPERTY(BlueprintAssignable, Category = "Mining|Events")
	FOnBlockMiningCompleted OnBlockMiningCompleted;

	/**
	 * Returns the maximum number of blocks that can be queued simultaneously based on tool tier.
	 * Hand = 1, Wood = 2, Stone = 3, Iron = 4, Diamond = 5, Obsidian = 6.
	 */
	UFUNCTION(BlueprintPure, Category = "Mining|Queue")
	int32 GetMaxQueueSlots() const;

	/**
	 * Returns the base mining force multiplier for the current tool tier.
	 * Hand = 1.0, Wood = 2.0, Stone = 4.0, Iron = 6.0, Diamond = 10.0, Obsidian = 15.0.
	 */
	UFUNCTION(BlueprintPure, Category = "Mining|Queue")
	float GetToolMiningForce() const;

	/**
	 * Updates the current tool tier.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mining|Tools")
	void SetToolTier(EToolTier NewTier);

	/**
	 * Queues a voxel block for mining.
	 * If the block is already queued, the request is safely ignored.
	 * If the queue is at capacity, the request is rejected.
	 * 
	 * @param VoxelCoord Global block coordinate (X, Y, Z).
	 * @param BlockID ID of the block being mined.
	 * @param Durability Base durability value of the block from Block_DataTable.
	 * @param MiningForceOverride If >= 0, overrides the tool's mining force.
	 * @return True if successfully queued, false if rejected (e.g. queue full or duplicate).
	 */
	UFUNCTION(BlueprintCallable, Category = "Mining|Queue")
	bool QueueBlock(const FIntVector& VoxelCoord, uint8 BlockID, float Durability, float MiningForceOverride = -1.0f);

	/**
	 * Advances progress on all active mining tasks simultaneously.
	 * Completed tasks are removed from the active queue and returned in OutCompletedVoxels.
	 * 
	 * @param DeltaTime Time elapsed in seconds since last frame.
	 * @param OutCompletedVoxels Array populated with voxel coordinates that finished breaking this frame.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mining|Queue")
	void TickQueue(float DeltaTime, TArray<FIntVector>& OutCompletedVoxels);

	/**
	 * Removes a voxel from the queue without breaking it (e.g. if player cancels).
	 */
	UFUNCTION(BlueprintCallable, Category = "Mining|Queue")
	bool CancelMiningTask(const FIntVector& VoxelCoord);

	/**
	 * Clears all active mining tasks.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mining|Queue")
	void ClearQueue();

	/** Returns current count of blocks actively being mined. */
	UFUNCTION(BlueprintPure, Category = "Mining|Queue")
	int32 GetQueueCount() const { return ActiveMiningTasks.Num(); }

	/** Returns copies of all active mining tasks for UI progress display. */
	UFUNCTION(BlueprintPure, Category = "Mining|Queue")
	TArray<FMiningTask> GetActiveMiningTasks() const { return ActiveMiningTasks; }

	/**
	 * Static helper to compute break time in seconds from block durability and mining force.
	 */
	UFUNCTION(BlueprintPure, Category = "Mining|Math")
	static float CalculateBreakTime(float Durability, float MiningForce);

	/**
	 * Static helper to get base mining force for a specific tool tier.
	 */
	UFUNCTION(BlueprintPure, Category = "Mining|Math")
	static float GetMiningForceForTier(EToolTier Tier);

	/**
	 * Static helper to get queue capacity for a specific tool tier.
	 */
	UFUNCTION(BlueprintPure, Category = "Mining|Math")
	static int32 GetQueueSlotsForTier(EToolTier Tier);

private:
	/** Currently active concurrent mining tasks */
	UPROPERTY()
	TArray<FMiningTask> ActiveMiningTasks;
};
