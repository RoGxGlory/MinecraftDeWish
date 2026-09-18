#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelDataTypes.h"
#include "ProceduralMeshComponent.h"
#include "Components/SphereComponent.h"
#include "BlockItemPickup.generated.h"

/**
 * ABlockItemPickup
 * 
 * Physical dropped block item in the world.
 * 
 * Key Features:
 * 1. Mini-Voxel Mesh: Procedurally renders mini cubes (25cm) with exact block textures from M_Global.
 * 2. Visual Item Pile: As items merge (up to 64), additional mini cubes appear in the pile.
 * 3. Gravity & Physics: Falls realistically to the terrain surface, then transitions to grounded bobbing.
 * 4. Grounded Animation: Smooth continuous rotation (90 deg/s) and vertical sinusoidal bobbing.
 * 5. World Merging: Overlapping pickups of the same BlockID merge into stacks of up to 64.
 *    If a 65th item merges, it is cleanly displaced beside the primary stack (+35cm) as a secondary stack.
 * 6. Magnet Attraction: When a player pawn comes within AttractionRadius (240cm), the item accelerates
 *    smoothly and flies towards the player.
 * 7. Grab Radius: 90cm overlap sphere automatically ingests into the player's quickslots via QuickSlotsInventorySystem.
 */
UCLASS(BlueprintType)
class MINECRAFTDEWISH_API ABlockItemPickup : public AActor
{
	GENERATED_BODY()

public:
	ABlockItemPickup();

	/** Collision sphere for pickup trigger and world merging (90cm radius) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* SphereComponent;

	/** Procedural mesh component rendering the miniature voxel block */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProceduralMeshComponent* MiniBlockMesh;

	/** ID of the block represented by this pickup (e.g. 5 = Wood Log, 1 = Dirt) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta = (ExposeOnSpawn = true))
	uint8 BlockID = 1;

	/** Number of items currently stacked in this pickup */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta = (ExposeOnSpawn = true))
	int32 ItemCount = 1;

	/** Whether this pickup can merge with other pickups of the same type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	bool bIsStackable = true;

	/** Maximum items allowed in a single world stack (64) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	int32 MaxStackSize = 64;

	/** Rotation speed in degrees per second for idle spinning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float RotationSpeed = 90.0f;

	/** Maximum vertical offset in cm for idle bobbing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float BobHeight = 6.0f;

	/** Oscillation frequency for idle bobbing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float BobFrequency = 3.0f;

	/**
	 * Configures the pickup parameters and generates the miniature block mesh.
	 * 
	 * @param InBlockID Block ID to display.
	 * @param InCount Stack count (default 1).
	 * @param InMaterial Terrain master material (M_Global).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void InitializePickup(uint8 InBlockID, int32 InCount = 1, UMaterialInterface* InMaterial = nullptr);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Overlap event triggered when player enters grab sphere */
	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	/** Rebuilds procedural mini-mesh sections reflecting current item count pile */
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void UpdatePileMesh();

private:
	/** Accumulated time for bobbing sine calculation */
	float RunningTime = 0.0f;
	FVector MeshBaseOffset = FVector::ZeroVector;

	// Gravity and Physics
	float VerticalVelocity = 0.0f;
	bool bIsGrounded = false;
	float GroundZ = 0.0f;
	float GravityStrength = 1200.0f;
	float CheckFloorTimer = 0.0f;
	float CheckMergeTimer = 0.0f;

	// Player Magnet Attraction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Attraction", meta = (AllowPrivateAccess = "true"))
	float AttractionRadius = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Attraction", meta = (AllowPrivateAccess = "true"))
	float AttractSpeed = 650.0f;

	UPROPERTY()
	UMaterialInterface* CachedMaterial = nullptr;

	void UpdateGravity(float DeltaTime);
	void UpdatePlayerAttraction(float DeltaTime);
	void CheckNearbyStackMerging();
	void BuildMiniBlockMesh(UMaterialInterface* Material);
	bool TryAddToPlayerInventory(AActor* PlayerActor);
};
