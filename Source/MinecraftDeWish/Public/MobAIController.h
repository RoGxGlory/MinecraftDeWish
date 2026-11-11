#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "MobAIController.generated.h"

class AMobCharacter;
class UStateTreeAIComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

/**
 * AMobAIController
 *
 * AIController for hostile mobs. Owns the UAIPerceptionComponent (sight + hearing)
 * and the UStateTreeAIComponent that drives behavior decisions (patrol, chase,
 * attack, flee, fuse). Implements ADR-0009: behavior decisions live here / in
 * the StateTree, never in AMobCharacter::Tick.
 */
UCLASS(BlueprintType, Blueprintable)
class MINECRAFTDEWISH_API AMobAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMobAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** Returns the controlled mob pawn. */
	UFUNCTION(BlueprintPure, Category = "Mob|AI")
	AMobCharacter* GetMob() const;

	/** Last actor successfully perceived (player). */
	UFUNCTION(BlueprintPure, Category = "Mob|AI")
	AActor* GetPerceivedTarget() const { return CurrentTarget.Get(); }

	/** Override the perceived target (used by StateTree tasks / Blueprint). */
	UFUNCTION(BlueprintCallable, Category = "Mob|AI")
	void SetPerceivedTarget(AActor* NewTarget);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentTarget;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void ConfigurePerceptionForMob(AMobCharacter* Mob);
};