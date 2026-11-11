#include "ToolTypes.h"
#include "ToolData.h"

FToolInstance::FToolInstance(EToolType InType, EToolTier InTier)
	: ToolType(InType), ToolTier(InTier)
{
	ToolId = UToolDefinition::MakeToolId(InType, InTier);
	InstanceId = FGuid::NewGuid();
	MaxDurability = CalculateMaxDurability(InTier);
	CurrentDurability = MaxDurability;
}

float FToolInstance::GetBaseMiningForce(const UToolDefinition* Definition) const
{
	if (Definition)
	{
		return Definition->GetBaseMiningForce(ToolType, ToolTier);
	}
	return CalculateMiningForce(ToolTier);
}

float FToolInstance::GetEffectiveMiningForce(uint8 BlockID, const UToolDefinition* Definition) const
{
	if (!IsValid() || IsBroken())
	{
		return 1.0f; // Hand speed
	}

	const uint8 PreferredTool = FBlockHelpers::GetPreferredToolType(BlockID);
	if (PreferredTool == 0 || PreferredTool == static_cast<uint8>(ToolType))
	{
		if (Definition)
		{
			if (const FToolRow* Row = Definition->FindToolRow(ToolType, ToolTier))
			{
				const float* Mult = Row->MiningForceByBlockCategory.Find(PreferredTool);
				const float Base = Row->BaseMiningForce;
				return Mult ? (Base * (*Mult)) : Base;
			}
		}
		return GetBaseMiningForce(Definition);
	}

	// Wrong tool type: reduced effectiveness (hand speed)
	return 1.0f;
}

float FToolInstance::GetAttackDamage(const UToolDefinition* Definition) const
{
	if (ToolType != EToolType::Sword || IsBroken())
	{
		return 1.0f; // Fist damage
	}

	if (Definition)
	{
		return Definition->GetAttackDamage(ToolType, ToolTier);
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

int32 FToolInstance::ResolveMaxDurability(const UToolDefinition* Definition) const
{
	if (Definition)
	{
		return Definition->GetDurabilityMax(ToolType, ToolTier);
	}
	return CalculateMaxDurability(ToolTier);
}

TSoftObjectPtr<UStaticMesh> FToolInstance::GetMesh(const UToolDefinition* Definition) const
{
	if (Definition)
	{
		if (const FToolRow* Row = Definition->FindToolRow(ToolType, ToolTier))
		{
			return Row->MeshPath;
		}
	}
	return TSoftObjectPtr<UStaticMesh>();
}

TSoftObjectPtr<UTexture2D> FToolInstance::GetIcon(const UToolDefinition* Definition) const
{
	if (Definition)
	{
		if (const FToolRow* Row = Definition->FindToolRow(ToolType, ToolTier))
		{
			return Row->IconPath;
		}
	}
	return TSoftObjectPtr<UTexture2D>();
}

TSoftObjectPtr<UAnimMontage> FToolInstance::GetSwingMontage(const UToolDefinition* Definition) const
{
	if (Definition)
	{
		if (const FToolRow* Row = Definition->FindToolRow(ToolType, ToolTier))
		{
			return Row->SwingMontage;
		}
	}
	return TSoftObjectPtr<UAnimMontage>();
}

FName FToolInstance::GetImpactCue(const UToolDefinition* Definition) const
{
	if (Definition)
	{
		if (const FToolRow* Row = Definition->FindToolRow(ToolType, ToolTier))
		{
			return Row->ImpactCue;
		}
	}
	return NAME_None;
}