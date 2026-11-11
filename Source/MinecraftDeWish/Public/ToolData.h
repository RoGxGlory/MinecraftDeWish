#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "VoxelDataTypes.h"
#include "ToolData.generated.h"

class UStaticMesh;
class UTexture2D;
class UAnimMontage;

/**
 * FToolRow
 *
 * DataTable row schema for DT_Tools (ADR-0007). One row per (ToolType, ToolTier)
 * combination (25 rows). All static balance/asset data for a tool lives here;
 * FToolInstance keeps only runtime state.
 */
USTRUCT(BlueprintType)
struct MINECRAFTDEWISH_API FToolRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Tool category. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool")
	EToolType ToolType = EToolType::None;

	/** Material tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool")
	EToolTier ToolTier = EToolTier::None;

	/** Base mining force when used on a matching block. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool|Balance", meta = (ClampMin = "0.0"))
	float BaseMiningForce = 1.0f;

	/** Maximum durability for this tool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool|Balance", meta = (ClampMin = "0"))
	int32 DurabilityMax = 0;

	/** Combat damage dealt on hit (swords; others use fist damage). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool|Balance", meta = (ClampMin = "0.0"))
	float AttackDamage = 1.0f;

	/** Static mesh for the tool (soft reference, loaded on equip). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool|Assets")
	TSoftObjectPtr<UStaticMesh> MeshPath;

	/** 2D icon texture (soft reference). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool|Assets")
	TSoftObjectPtr<UTexture2D> IconPath;

	/** Per-tool-class swing montage (soft reference). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool|Assets")
	TSoftObjectPtr<UAnimMontage> SwingMontage;

	/** Named VFX/SFX cue fired at the impact frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool|Assets")
	FName ImpactCue = NAME_None;

	/** Per-block-category mining force multiplier (key = preferred tool type id). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool|Balance")
	TMap<uint8, float> MiningForceByBlockCategory;
};

/**
 * UToolDefinition
 *
 * DataAsset wrapper around DT_Tools providing cached, validated lookups
 * (ADR-0007). Assign this asset to UBaublesSystem::ToolDefinition and
 * APlayerViewModel::ToolDefinition.
 */
UCLASS(BlueprintType)
class MINECRAFTDEWISH_API UToolDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The underlying tools DataTable (DT_Tools). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools")
	TObjectPtr<UDataTable> ToolsTable;

	/** Build a stable ToolId from a (type, tier) pair. Convention: "<Type>_<Tier>". */
	UFUNCTION(BlueprintPure, Category = "Tools")
	static FName MakeToolId(EToolType ToolType, EToolTier ToolTier);

	/** Find a row by stable ToolId. Returns nullptr if not found. (C++ only) */
	const FToolRow* FindToolRow(FName ToolId) const;

	/** Find a row by (type, tier). Returns nullptr if not found. (C++ only) */
	const FToolRow* FindToolRow(EToolType ToolType, EToolTier ToolTier) const;

	/** Convenience: base mining force for a (type, tier), with safe default. */
	UFUNCTION(BlueprintPure, Category = "Tools")
	float GetBaseMiningForce(EToolType ToolType, EToolTier ToolTier) const;

	/** Convenience: max durability for a (type, tier), with safe default. */
	UFUNCTION(BlueprintPure, Category = "Tools")
	int32 GetDurabilityMax(EToolType ToolType, EToolTier ToolTier) const;

	/** Convenience: attack damage for a (type, tier), with safe default. */
	UFUNCTION(BlueprintPure, Category = "Tools")
	float GetAttackDamage(EToolType ToolType, EToolTier ToolTier) const;
};