#include "ToolData.h"

#include "ToolTypes.h"
#include "Engine/DataTable.h"

FName UToolDefinition::MakeToolId(EToolType ToolType, EToolTier ToolTier)
{
	const UEnum* TypeEnum = StaticEnum<EToolType>();
	const UEnum* TierEnum = StaticEnum<EToolTier>();

	FString TypeStr = TypeEnum ? TypeEnum->GetNameStringByValue(static_cast<int64>(ToolType)) : TEXT("None");
	FString TierStr = TierEnum ? TierEnum->GetNameStringByValue(static_cast<int64>(ToolTier)) : TEXT("None");
	return FName(*FString::Printf(TEXT("%s_%s"), *TypeStr, *TierStr));
}

const FToolRow* UToolDefinition::FindToolRow(FName ToolId) const
{
	if (!ToolsTable || ToolId == NAME_None)
	{
		return nullptr;
	}
	return ToolsTable->FindRow<FToolRow>(ToolId, TEXT("UToolDefinition::FindToolRow"));
}

const FToolRow* UToolDefinition::FindToolRow(EToolType ToolType, EToolTier ToolTier) const
{
	return FindToolRow(MakeToolId(ToolType, ToolTier));
}

float UToolDefinition::GetBaseMiningForce(EToolType ToolType, EToolTier ToolTier) const
{
	if (const FToolRow* Row = FindToolRow(ToolType, ToolTier))
	{
		return Row->BaseMiningForce;
	}
	// Fallback to legacy hardcoded values for backward compat / missing data.
	return FToolInstance::CalculateMiningForce(ToolTier);
}

int32 UToolDefinition::GetDurabilityMax(EToolType ToolType, EToolTier ToolTier) const
{
	if (const FToolRow* Row = FindToolRow(ToolType, ToolTier))
	{
		return Row->DurabilityMax;
	}
	return FToolInstance::CalculateMaxDurability(ToolTier);
}

float UToolDefinition::GetAttackDamage(EToolType ToolType, EToolTier ToolTier) const
{
	if (const FToolRow* Row = FindToolRow(ToolType, ToolTier))
	{
		return Row->AttackDamage;
	}
	// Fallback legacy values.
	FToolInstance Tmp(ToolType, ToolTier);
	return Tmp.GetAttackDamage();
}