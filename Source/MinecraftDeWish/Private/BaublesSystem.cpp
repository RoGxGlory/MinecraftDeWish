#include "BaublesSystem.h"

const FToolInstance UBaublesSystem::EmptyTool = FToolInstance();

UBaublesSystem::UBaublesSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ==================== SLOT ACCESS ====================

FToolInstance& UBaublesSystem::GetSlotRef(EToolType SlotType)
{
	switch (SlotType)
	{
	case EToolType::Pickaxe: return PickaxeSlot;
	case EToolType::Shovel:  return ShovelSlot;
	case EToolType::Axe:     return AxeSlot;
	case EToolType::Hoe:     return HoeSlot;
	case EToolType::Sword:   return SwordSlot;
	default:                 return PickaxeSlot; // Fallback
	}
}

const FToolInstance& UBaublesSystem::GetSlotConstRef(EToolType SlotType) const
{
	switch (SlotType)
	{
	case EToolType::Pickaxe: return PickaxeSlot;
	case EToolType::Shovel:  return ShovelSlot;
	case EToolType::Axe:     return AxeSlot;
	case EToolType::Hoe:     return HoeSlot;
	case EToolType::Sword:   return SwordSlot;
	default:                 return EmptyTool;
	}
}

// ==================== TOOL MANAGEMENT ====================

bool UBaublesSystem::EquipTool(const FToolInstance& Tool)
{
	if (Tool.ToolType == EToolType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("BaublesSystem: Cannot equip tool with type None"));
		return false;
	}

	FToolInstance& Slot = GetSlotRef(Tool.ToolType);
	Slot = Tool;

	OnToolEquipped.Broadcast(Tool.ToolType, Slot);

	UE_LOG(LogTemp, Log, TEXT("BaublesSystem: Equipped %s (Durability: %d/%d)"),
		*Tool.GetDisplayName(), Tool.CurrentDurability, Tool.MaxDurability);

	return true;
}

FToolInstance UBaublesSystem::UnequipTool(EToolType SlotType)
{
	if (SlotType == EToolType::None)
	{
		return FToolInstance();
	}

	FToolInstance& Slot = GetSlotRef(SlotType);
	FToolInstance Removed = Slot;

	// Clear the slot
	Slot = FToolInstance();

	if (Removed.IsValid())
	{
		OnToolUnequipped.Broadcast(SlotType, Removed);
		UE_LOG(LogTemp, Log, TEXT("BaublesSystem: Unequipped %s"), *Removed.GetDisplayName());
	}

	return Removed;
}

FToolInstance UBaublesSystem::GetToolInSlot(EToolType SlotType) const
{
	return GetSlotConstRef(SlotType);
}

bool UBaublesSystem::HasToolEquipped(EToolType SlotType) const
{
	return GetSlotConstRef(SlotType).IsValid();
}

// ==================== MINING INTEGRATION ====================

float UBaublesSystem::GetBestMiningForce(uint8 BlockID) const
{
	const FToolInstance BestTool = GetBestToolForBlock(BlockID);
	if (BestTool.IsValid())
	{
		return BestTool.GetEffectiveMiningForce(BlockID);
	}
	return 1.0f; // Hand speed
}

FToolInstance UBaublesSystem::GetBestToolForBlock(uint8 BlockID) const
{
	const uint8 PreferredToolType = FBlockHelpers::GetPreferredToolType(BlockID);

	if (PreferredToolType == 0)
	{
		// No specific tool preference — any tool works, find the highest tier equipped
		float BestForce = 0.0f;
		FToolInstance BestTool;

		const FToolInstance* AllSlots[] = { &PickaxeSlot, &ShovelSlot, &AxeSlot, &HoeSlot };
		for (const FToolInstance* Slot : AllSlots)
		{
			if (Slot->IsValid() && !Slot->IsBroken())
			{
				const float Force = Slot->GetBaseMiningForce();
				if (Force > BestForce)
				{
					BestForce = Force;
					BestTool = *Slot;
				}
			}
		}
		return BestTool;
	}

	// Map preferred tool type to EToolType
	const EToolType TargetType = static_cast<EToolType>(PreferredToolType);
	const FToolInstance& MatchingTool = GetSlotConstRef(TargetType);

	if (MatchingTool.IsValid() && !MatchingTool.IsBroken())
	{
		return MatchingTool;
	}

	return FToolInstance(); // No suitable tool
}

bool UBaublesSystem::ConsumeToolDurability(uint8 BlockID)
{
	const uint8 PreferredToolType = FBlockHelpers::GetPreferredToolType(BlockID);
	if (PreferredToolType == 0)
	{
		return false; // No tool needed, no durability consumed
	}

	const EToolType TargetType = static_cast<EToolType>(PreferredToolType);
	FToolInstance& Slot = GetSlotRef(TargetType);

	if (Slot.IsValid() && !Slot.IsBroken())
	{
		const bool bBroke = Slot.ConsumeDurability(1);
		if (bBroke)
		{
			UE_LOG(LogTemp, Warning, TEXT("BaublesSystem: %s broke!"), *Slot.GetDisplayName());
			OnToolUnequipped.Broadcast(TargetType, Slot);
			Slot = FToolInstance(); // Clear broken tool
		}
		return bBroke;
	}
	return false;
}

// ==================== COMBAT INTEGRATION ====================

float UBaublesSystem::GetAttackDamage() const
{
	if (SwordSlot.IsValid() && !SwordSlot.IsBroken())
	{
		return SwordSlot.GetAttackDamage();
	}
	return 1.0f; // Fist damage
}

// ==================== UI ====================

void UBaublesSystem::ToggleBaublesUI()
{
	bIsBaublesOpen = !bIsBaublesOpen;
	OnBaublesToggled.Broadcast(bIsBaublesOpen);

	UE_LOG(LogTemp, Log, TEXT("BaublesSystem: UI toggled %s"), bIsBaublesOpen ? TEXT("OPEN") : TEXT("CLOSED"));
}
