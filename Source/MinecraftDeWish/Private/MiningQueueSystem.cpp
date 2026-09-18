#include "MiningQueueSystem.h"

UMiningQueueSystem::UMiningQueueSystem()
{
	CurrentToolTier = EToolTier::None;
}

int32 UMiningQueueSystem::GetMaxQueueSlots() const
{
	return UMiningQueueSystem::GetQueueSlotsForTier(CurrentToolTier);
}

float UMiningQueueSystem::GetToolMiningForce() const
{
	return UMiningQueueSystem::GetMiningForceForTier(CurrentToolTier);
}

void UMiningQueueSystem::SetToolTier(EToolTier NewTier)
{
	CurrentToolTier = NewTier;
	UE_LOG(LogTemp, Log, TEXT("MiningQueueSystem: Upgraded tool tier to %d (Force: %.1f, Max Slots: %d)"),
		static_cast<int32>(CurrentToolTier), GetToolMiningForce(), GetMaxQueueSlots());
}

bool UMiningQueueSystem::QueueBlock(const FIntVector& VoxelCoord, uint8 BlockID, float Durability, float MiningForceOverride)
{
	// Do not queue invalid coordinates or air
	if (VoxelCoord.X < 0 || VoxelCoord.Y < 0 || VoxelCoord.Z < 0 || BlockID == 0)
	{
		return false;
	}

	// Prevent duplicate entries for the same voxel
	for (const FMiningTask& Task : ActiveMiningTasks)
	{
		if (Task.VoxelCoord == VoxelCoord)
		{
			return false;
		}
	}

	// Check queue capacity against current tool tier
	const int32 MaxSlots = GetMaxQueueSlots();
	if (ActiveMiningTasks.Num() >= MaxSlots)
	{
		UE_LOG(LogTemp, Warning, TEXT("MiningQueueSystem: Queue is at maximum capacity (%d/%d) for current tool tier!"),
			ActiveMiningTasks.Num(), MaxSlots);
		return false;
	}

	// Determine effective mining force
	const float EffectiveForce = (MiningForceOverride > 0.0f) ? MiningForceOverride : GetToolMiningForce();
	const float BreakTime = CalculateBreakTime(Durability, EffectiveForce);

	FMiningTask NewTask;
	NewTask.VoxelCoord = VoxelCoord;
	NewTask.BlockID = BlockID;
	NewTask.TotalBreakTime = BreakTime;
	NewTask.ElapsedTime = 0.0f;
	NewTask.Progress = 0.0f;

	ActiveMiningTasks.Add(NewTask);

	UE_LOG(LogTemp, Log, TEXT("MiningQueueSystem: Queued voxel (%d, %d, %d) [BlockID %d, Durability %.1f, Time %.2fs]. Active Queue: %d/%d"),
		VoxelCoord.X, VoxelCoord.Y, VoxelCoord.Z, BlockID, Durability, BreakTime, ActiveMiningTasks.Num(), MaxSlots);

	return true;
}

void UMiningQueueSystem::TickQueue(float DeltaTime, TArray<FIntVector>& OutCompletedVoxels)
{
	OutCompletedVoxels.Empty();

	if (ActiveMiningTasks.Num() == 0)
	{
		return;
	}

	// Tick down all active tasks concurrently
	for (int32 i = ActiveMiningTasks.Num() - 1; i >= 0; --i)
	{
		FMiningTask& Task = ActiveMiningTasks[i];
		Task.ElapsedTime += DeltaTime;
		Task.Progress = FMath::Clamp(Task.ElapsedTime / FMath::Max(0.001f, Task.TotalBreakTime), 0.0f, 1.0f);

		if (Task.ElapsedTime >= Task.TotalBreakTime)
		{
			const FIntVector CompletedCoord = Task.VoxelCoord;
			const uint8 CompletedID = Task.BlockID;

			ActiveMiningTasks.RemoveAt(i);
			OutCompletedVoxels.Add(CompletedCoord);

			if (OnBlockMiningCompleted.IsBound())
			{
				OnBlockMiningCompleted.Broadcast(CompletedCoord, CompletedID);
			}
		}
	}
}

bool UMiningQueueSystem::CancelMiningTask(const FIntVector& VoxelCoord)
{
	for (int32 i = 0; i < ActiveMiningTasks.Num(); ++i)
	{
		if (ActiveMiningTasks[i].VoxelCoord == VoxelCoord)
		{
			ActiveMiningTasks.RemoveAt(i);
			return true;
		}
	}
	return false;
}

void UMiningQueueSystem::ClearQueue()
{
	ActiveMiningTasks.Empty();
}

float UMiningQueueSystem::CalculateBreakTime(float Durability, float MiningForce)
{
	// Ensure minimum durability and mining force
	const float SafeDurability = FMath::Max(0.05f, Durability);
	const float SafeForce = FMath::Max(0.1f, MiningForce);

	// Formula: BreakTime = Durability * 1.5 / MiningForce
	// Examples:
	// Wood Log (Durability 2.0) with Hand (Force 1.0): 2.0 * 1.5 / 1.0 = 3.0s
	// Wood Log (Durability 2.0) with Wood Tool (Force 2.0): 2.0 * 1.5 / 2.0 = 1.5s
	// Stone (Durability 1.5) with Iron Tool (Force 6.0): 1.5 * 1.5 / 6.0 = 0.375s
	return FMath::Clamp((SafeDurability * 1.5f) / SafeForce, 0.05f, 30.0f);
}

float UMiningQueueSystem::GetMiningForceForTier(EToolTier Tier)
{
	switch (Tier)
	{
	case EToolTier::None:     return 1.0f;
	case EToolTier::Wood:     return 2.0f;
	case EToolTier::Stone:    return 4.0f;
	case EToolTier::Iron:     return 6.0f;
	case EToolTier::Diamond:  return 10.0f;
	case EToolTier::Obsidian: return 15.0f;
	default:                  return 1.0f;
	}
}

int32 UMiningQueueSystem::GetQueueSlotsForTier(EToolTier Tier)
{
	switch (Tier)
	{
	case EToolTier::None:     return 1;
	case EToolTier::Wood:     return 2;
	case EToolTier::Stone:    return 3;
	case EToolTier::Iron:     return 4;
	case EToolTier::Diamond:  return 5;
	case EToolTier::Obsidian: return 6;
	default:                  return 1;
	}
}
