#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Components/SphereComponent.h"
#include "XPOrbPickup.generated.h"

/**
 * AXPOrbPickup
 *
 * Physical experience orb dropped in the world when mobs are defeated.
 *
 * Features:
 * 1. Procedurally generated glowing diamond/polyhedral orb mesh.
 * 2. Floats, bobs, and rotates in place when idle.
 * 3. Magnet attraction: smoothly flies toward player pawn when within range.
 * 4. Ingestion: on contact with player, deposits XP into UPlayerVitalSystem.
 */
UCLASS(BlueprintType)
class MINECRAFTDEWISH_API AXPOrbPickup : public AActor
{
	GENERATED_BODY()

public:
	AXPOrbPickup();

	/** Collision sphere for pickup trigger and attraction detection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* SphereComponent;

	/** Mesh rendering the glowing XP orb */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProceduralMeshComponent* OrbMesh;

	/** Experience value contained in this orb */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XP", meta = (ExposeOnSpawn = true))
	int32 XPAmount = 5;

	/** Magnet attraction radius (300cm = 3 blocks) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XP")
	float AttractionRadius = 350.0f;

	/** Collect radius (60cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XP")
	float CollectRadius = 60.0f;

	/** Max speed when flying toward player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XP")
	float MaxAttractionSpeed = 900.0f;

	/** Initialize the XP orb with an amount */
	UFUNCTION(BlueprintCallable, Category = "XP")
	void InitializeXP(int32 InAmount);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

private:
	float BobTimer = 0.0f;
	float BaseZ = 0.0f;
	bool bIsGrounded = false;
	FVector CurrentVelocity = FVector::ZeroVector;

	void GenerateOrbMesh();
};
