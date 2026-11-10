#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "BlockCrackActor.generated.h"

/**
 * ABlockCrackActor
 *
 * Runtime overlay actor that draws vanilla-style block cracking on top of a voxel block being mined.
 * It is a pure function of mining progress: Progress (0..1) maps onto the destroy_stage_0..9 crack
 * textures via GetStageFromProgress.
 *
 * The actor is event-driven: it has no Tick, and it does not read UMiningQueueSystem. Blueprint owns
 * the mining state (BreakTime / LastTargetedBlockVoxel on BP_HorrorCharacter) and pushes progress
 * through AWorldGenerator::UpdateCrackMining(Voxel, Progress01) while the block is mined, then
 * AWorldGenerator::ClearCrackMining() on cancel. See ADR-0003.
 *
 * Supports every block shape the voxel renderer produces:
 *  - Cube blocks use six per-face quads with a 0..1 UV mapping onto the crack texture, offset
 *    0.75 UU outward along each face normal so the overlay wins the depth test without z-fighting
 *    the chunk surface.
 *  - Torches (crossed diagonal quads, mirroring AChunkActor::AddTorchMesh) get double-sided
 *    crossed-quad crack meshes offset 0.75 UU along each quad normal so the overlay wins the depth
 *    test from both viewing sides without z-fighting the torch itself.
 */
UCLASS()
class MINECRAFTDEWISH_API ABlockCrackActor : public AActor
{
	GENERATED_BODY()

public:
	ABlockCrackActor();

	/** Procedural mesh that draws the crack overlay; cube faces or crossed torch quads. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProceduralMeshComponent* CrackMesh;

	/**
	 * Initializes the overlay for a mining task: positions the actor at the block center,
	 * builds the crack mesh matching the block shape and binds the crack material.
	 * Called by AWorldGenerator right after spawning the actor for a new mining task.
	 */
	void InitializeCrackOverlay(const FIntVector& InVoxelCoord, uint8 InBlockID, float InBlockScale, const FVector& InWorldCenter);

	/**
	 * Display the crack stage matching Progress (0..1). Called once per frame from AWorldGenerator.
	 * @return True when the displayed stage changed this call.
	 */
	UFUNCTION(BlueprintCallable, Category = "Crack")
	bool UpdateCrackStage(float Progress);

	/** Removes the overlay from the world (block broke, or mining was cancelled). */
	UFUNCTION(BlueprintCallable, Category = "Crack")
	void DestroyCrackOverlay();

	/** Voxel this overlay renders over. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crack")
	FIntVector VoxelCoord = FIntVector(-1, -1, -1);

	/** Block ID this overlay is cracking. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crack")
	uint8 BlockID = 0;

	/** Crack stage [0..9] currently displayed; -1 before the first update. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crack")
	int32 CurrentStage = -1;

	/** Maps mining progress (0..1) to a crack stage (0..9). */
	UFUNCTION(BlueprintPure, Category = "Crack")
	static int32 GetStageFromProgress(float Progress);

protected:
	virtual void BeginPlay() override;

private:
	/** Build cube mesh: per-face quads with 0..1 UVs, offset 0.75 UU outward along each face normal. */
	void BuildCubeCrackMesh();

	/** Build torch mesh: crossed diagonal quads mirroring AChunkActor::AddTorchMesh, offset both normal directions. */
	void BuildTorchCrackMesh();

	/** Creates the dynamic material instance from M_BlockCrack (if present) and applies it to mesh section 0. */
	void CreateCrackMaterialInstance();

	float BlockScale = 100.0f;

	/** Material template loaded in the constructor (M_BlockCrack). Null when the asset is missing. */
	UPROPERTY(Transient)
	UMaterialInterface* CrackMaterialTemplate = nullptr;

	/** Per-instance dynamic material that carries the crack stage texture. */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* CrackMaterial = nullptr;

	/** destroy_stage_0..9 loaded in the constructor; index = crack stage. */
	UPROPERTY(Transient)
	TArray<UTexture2D*> StageTextures;
};
