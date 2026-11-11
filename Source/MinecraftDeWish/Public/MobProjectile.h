#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MobProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

/**
 * AMobProjectile
 *
 * Lightweight projectile fired by ranged mobs (Skeleton arrows). Travels in a
 * straight line, deals damage to the first pawn it hits, then destroys itself.
 */
UCLASS(BlueprintType, Blueprintable)
class MINECRAFTDEWISH_API AMobProjectile : public AActor
{
	GENERATED_BODY()

public:
	AMobProjectile();

	/** Damage dealt on hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Damage = 4.0f;

	/** Travel speed (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Speed = 2500.0f;

public:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
};