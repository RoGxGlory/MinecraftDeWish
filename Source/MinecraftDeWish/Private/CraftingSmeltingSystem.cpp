#include "CraftingSmeltingSystem.h"

UCraftingSmeltingSystem::UCraftingSmeltingSystem()
{
	FurnaceBurnTimeRemaining = 0.0f;
	FurnaceTotalBurnTime = 0.0f;
	FurnaceSmeltProgress = 0.0f;
}

bool UCraftingSmeltingSystem::CanCraftTool(EToolTier CurrentTier, EToolTier DesiredTier) const
{
	// Cannot downgrade or craft same tier
	if (static_cast<uint8>(DesiredTier) <= static_cast<uint8>(CurrentTier))
	{
		return false;
	}

	// Must advance sequentially or jump up to highest allowed
	return DesiredTier != EToolTier::None;
}

bool UCraftingSmeltingSystem::CraftTool(EToolTier& CurrentTier, EToolTier DesiredTier, FString& OutMessage)
{
	if (!CanCraftTool(CurrentTier, DesiredTier))
	{
		OutMessage = FString::Printf(TEXT("Cannot craft %s: You already have tier %s or higher."),
			*UEnum::GetValueAsString(DesiredTier), *UEnum::GetValueAsString(CurrentTier));
		return false;
	}

	CurrentTier = DesiredTier;

	FString ToolName = TEXT("Tool");
	switch (DesiredTier)
	{
	case EToolTier::Wood:     ToolName = TEXT("Wooden Pickaxe"); break;
	case EToolTier::Stone:    ToolName = TEXT("Stone Pickaxe"); break;
	case EToolTier::Iron:     ToolName = TEXT("Iron Pickaxe"); break;
	case EToolTier::Diamond:  ToolName = TEXT("Diamond Pickaxe"); break;
	case EToolTier::Obsidian: ToolName = TEXT("Obsidian Pickaxe"); break;
	default: break;
	}

	OutMessage = FString::Printf(TEXT("Successfully crafted %s!"), *ToolName);
	UE_LOG(LogTemp, Log, TEXT("CraftingSmeltingSystem: %s"), *OutMessage);
	return true;
}

bool UCraftingSmeltingSystem::SmeltItem(uint8 InputBlockID, uint8 FuelBlockID, uint8& OutResultBlockID, FString& OutMessage)
{
	OutResultBlockID = 0;

	// Verify valid smelt recipe
	const uint8 Result = GetSmeltResult(InputBlockID);
	if (Result == 0)
	{
		OutMessage = TEXT("This item cannot be smelted in a furnace.");
		return false;
	}

	// Verify or consume fuel
	if (FurnaceBurnTimeRemaining <= 0.0f)
	{
		const float FuelDuration = GetFuelBurnDuration(FuelBlockID);
		if (FuelDuration <= 0.0f)
		{
			OutMessage = TEXT("Invalid fuel! Use Coal, Charcoal, Wood Logs, or Planks.");
			return false;
		}

		FurnaceBurnTimeRemaining = FuelDuration;
		FurnaceTotalBurnTime = FuelDuration;
	}

	// Produce smelted item
	OutResultBlockID = Result;
	FurnaceSmeltProgress = 1.0f; // Smelt complete

	OutMessage = FString::Printf(TEXT("Smelted input %d into result %d."), InputBlockID, OutResultBlockID);
	UE_LOG(LogTemp, Log, TEXT("CraftingSmeltingSystem: %s [Remaining fuel: %.1fs]"), *OutMessage, FurnaceBurnTimeRemaining);
	return true;
}

void UCraftingSmeltingSystem::TickFurnace(float DeltaTime)
{
	if (FurnaceBurnTimeRemaining > 0.0f)
	{
		FurnaceBurnTimeRemaining = FMath::Max(0.0f, FurnaceBurnTimeRemaining - DeltaTime);
		if (FurnaceBurnTimeRemaining == 0.0f)
		{
			FurnaceTotalBurnTime = 0.0f;
		}
	}
}

float UCraftingSmeltingSystem::GetFuelBurnDuration(uint8 FuelBlockID)
{
	switch (FuelBlockID)
	{
	case 16: // Coal Block / Charcoal
	case 27: // Coal Ore
		return 80.0f; // 80 seconds burn time (smelts 10 items)

	case 5:  // Wood Log
		return 15.0f; // 15 seconds burn time (smelts ~2 items)

	case 6:  // Wood Planks
		return 10.0f; // 10 seconds burn time (smelts ~1 item)

	default:
		return 0.0f; // Non-fuel items
	}
}

uint8 UCraftingSmeltingSystem::GetSmeltResult(uint8 InputBlockID)
{
	switch (InputBlockID)
	{
	case 7:  // Iron Ore -> Iron Block / Ingot (ID 8)
		return 8;

	case 9:  // Gold Ore -> Gold Block / Ingot (ID 10)
		return 10;

	case 5:  // Wood Log -> Charcoal (ID 16, Coal block representation)
		return 16;

	case 18: // Sand -> Glass (ID 21)
		return 21;

	case 3:  // Cobblestone -> Smooth Stone (ID 4)
		return 4;

	default:
		return 0; // Not smeltable
	}
}

bool UCraftingSmeltingSystem::IsValidFuel(uint8 BlockID)
{
	return GetFuelBurnDuration(BlockID) > 0.0f;
}
