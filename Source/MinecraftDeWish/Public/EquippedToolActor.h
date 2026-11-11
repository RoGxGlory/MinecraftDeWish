#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "ToolTypes.h"
#include "EquippedToolActor.generated.h"

class UStaticMeshComponent;
class UStaticMesh;

/**
 * AEquippedToolActor
 *
 * A single, persistent first-person tool actor whose StaticMesh is swapped on
 * equip (ADR-0006). Never spawn/destroy per tool: keeps animation state warm
 * and avoids equip hitches. Tools are StaticMesh (no skinning).
 *
 * Pivot convention: grip centre, handle along local +Z (per technical-contract).
 */
UCLASS(BlueprintType, Blueprintable)
class MINECRAFTDEWISH_API AEquippedToolActor : public AActor
{
	GENERATED_BODY()

public:
	AEquippedToolActor();

	// ==================== COMPONENTS ====================

	/** Static mesh representing the currently equipped tool. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EquippedTool")
	TObjectPtr<UStaticMeshComponent> ToolMeshComponent;

	// ==================== STATE ====================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EquippedTool|State")
	EToolType CurrentToolType = EToolType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EquippedTool|State")
	EToolTier CurrentToolTier = EToolTier::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EquippedTool|State")
	bool bIsEquipped = false;

	// ==================== API ====================

	/**
	 * Swaps the displayed mesh to the given static mesh and records the tool identity.
	 * Passing nullptr clears the tool.
	 */
	UFUNCTION(BlueprintCallable, Category = "EquippedTool")
	void EquipMesh(UStaticMesh* InMesh, EToolType InToolType = EToolType::None, EToolTier InToolTier = EToolTier::None);

	/**
	 * Convenience: equip from a soft mesh reference (async-safe load).
	 */
	UFUNCTION(BlueprintCallable, Category = "EquippedTool")
	void EquipMeshFromSoftRef(const TSoftObjectPtr<UStaticMesh>& InMesh, EToolType InToolType = EToolType::None, EToolTier InToolTier = EToolTier::None);

	/** Clears the equipped mesh and resets state. */
	UFUNCTION(BlueprintCallable, Category = "EquippedTool")
	void Unequip();

	/** Returns true if a mesh is currently displayed. */
	UFUNCTION(BlueprintPure, Category = "EquippedTool")
	bool IsEquipped() const { return bIsEquipped; }

protected:
	virtual void BeginPlay() override;

	/** Blueprint hook fired after a tool mesh is equipped. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EquippedTool")
	void OnToolEquippedBP(EToolType ToolType, EToolTier ToolTier);

	/** Blueprint hook fired after a tool is unequipped. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EquippedTool")
	void OnToolUnequippedBP();
};