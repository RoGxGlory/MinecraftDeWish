#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "ToolTypes.h"
#include "ToolData.h"
#include "BaublesSystem.h"
#include "Engine/SkeletalMesh.h"
#include "ToolActionReceiver.h"
#include "WorldGenerator.h"
#include "PlayerViewModel.generated.h"

class USkeletalMeshComponent;
class UStaticMesh;
class UAnimMontage;
class AEquippedToolActor;
class AWorldGenerator;

/**
 * APlayerViewModel
 *
 * First-person viewmodel: skeletal mesh arms (bOnlyOwnerSee) with a tool
 * socket "weapon_r", plus a persistent AEquippedToolActor whose mesh is
 * swapped on equip (ADR-0006). Owns montage playback per tool class.
 */
UCLASS(BlueprintType, Blueprintable)
class MINECRAFTDEWISH_API APlayerViewModel : public AActor, public IToolActionReceiver
{
	GENERATED_BODY()

public:
	APlayerViewModel();

	// ==================== COMPONENTS ====================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ViewModel")
	TObjectPtr<USkeletalMeshComponent> ArmsMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ViewModel")
	TObjectPtr<AEquippedToolActor> EquippedToolActor;

	// ==================== CONFIG ====================

	/** Skeletal mesh for the first-person arms. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewModel|Config")
	TSoftObjectPtr<USkeletalMesh> ArmsMeshAsset;

	/** Socket on the arms skeleton to attach the tool to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewModel|Config")
	FName ToolSocketName = FName("weapon_r");

	/** Whether only the owning player sees the arms. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewModel|Config")
	bool bOnlyOwnerSeeArms = true;

	/** Per-tool-class swing montages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewModel|Montages")
	TMap<EToolType, TSoftObjectPtr<UAnimMontage>> SwingMontages;

	/** Data-driven tool definitions (DT_Tools wrapper). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewModel|Data")
	TObjectPtr<UToolDefinition> ToolDefinition;

	/** Baubles system to sync equipped tool from active selection (ADR-0006). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewModel|Data")
	TObjectPtr<UBaublesSystem> BaublesSystem;

	// ==================== API ====================

	UFUNCTION(BlueprintCallable, Category = "ViewModel")
	void EquipToolMesh(UStaticMesh* InMesh, EToolType InToolType = EToolType::None, EToolTier InToolTier = EToolTier::None);

	UFUNCTION(BlueprintCallable, Category = "ViewModel")
	void UnequipTool();

	UFUNCTION(BlueprintCallable, Category = "ViewModel")
	void PlaySwingMontage(EToolType ToolType, float PlayRate = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "ViewModel")
	void StopSwingMontage(float BlendOut = 0.2f);

	/** Syncs the equipped tool mesh from the Baubles active selection. */
	UFUNCTION(BlueprintCallable, Category = "ViewModel")
	void SyncEquippedToolFromActive();

	/** Handler for UBaublesSystem::OnActiveToolChanged. */
	UFUNCTION()
	void OnActiveToolChangedHandler(EToolType NewToolType);

	UFUNCTION(BlueprintPure, Category = "ViewModel")
	AEquippedToolActor* GetEquippedToolActor() const { return EquippedToolActor; }

	// ==================== STATE (IToolActionReceiver) ====================

	/** True while a swing montage action window is open. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ViewModel|State")
	bool bIsSwinging = false;

	/** True while a sustained mining loop is running. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ViewModel|State")
	bool bIsMining = false;

	/** Accumulated break time for the current mining target (seconds). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ViewModel|State")
	float MiningAccumulatedTime = 0.0f;

	/** Total break time for the current mining target (seconds). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ViewModel|State")
	float MiningTotalBreakTime = 0.0f;

	/** Current mining progress [0..1] for the crack overlay. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ViewModel|State")
	float MiningProgress = 0.0f;

	/** Voxel currently being mined. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewModel|State")
	FIntVector MiningTargetVoxel = FIntVector::ZeroValue;

	/** Resolved world generator used to queue block breaks and drive crack overlays. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewModel|Data")
	TObjectPtr<AWorldGenerator> WorldGenerator;

	// ==================== IToolActionReceiver ====================

	virtual void HandleActionWindow_Implementation() override;
	virtual void HandlePlayCue_Implementation(FName CueName) override;
	virtual void HandleActionComplete_Implementation() override;
	virtual void HandleMiningLoopBegin_Implementation() override;
	virtual void HandleMiningLoopEnd_Implementation() override;
	virtual void HandleMiningLoopTick_Implementation(float FrameDeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	void EnsureArmsMeshLoaded();
	void SpawnToolActor();
	void ResolveTargetedVoxel();
};