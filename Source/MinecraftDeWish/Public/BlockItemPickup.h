#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelDataTypes.h"
#include "ProceduralMeshComponent.h"
#include "Components/SphereComponent.h"
#include "BlockItemPickup.generated.h"

UCLASS(BlueprintType)
class MINECRAFTDEWISH_API ABlockItemPickup : public AActor
{
	GENERATED_BODY()

public:
	ABlockItemPickup();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* SphereComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProceduralMeshComponent* MiniBlockMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta = (ExposeOnSpawn = true))
	uint8 BlockID = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta = (ExposeOnSpawn = true))
	int32 ItemCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	bool bIsStackable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	int32 MaxStackSize = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float RotationSpeed = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float BobHeight = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float BobFrequency = 3.0f;

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void InitializePickup(uint8 InBlockID, int32 InCount = 1, UMaterialInterface* InMaterial = nullptr);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

private:
	float RunningTime = 0.0f;
	FVector MeshBaseOffset = FVector::ZeroVector;

	void BuildMiniBlockMesh(UMaterialInterface* Material);
	bool TryAddToPlayerInventory(AActor* PlayerActor);
};
