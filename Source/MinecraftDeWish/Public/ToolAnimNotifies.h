#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ToolAnimNotifies.generated.h"

class USkeletalMeshComponent;
class UAnimSequenceBase;

/**
 * UAnimNotify_ActionWindow
 *
 * Opens the damage/mining window during a swing montage (ADR-0005).
 * Notifies the owning actor if it implements IToolActionReceiver.
 */
UCLASS(meta = (DisplayName = "Action Window"))
class MINECRAFTDEWISH_API UAnimNotify_ActionWindow : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notify")
	FName ActionTag = NAME_None;
};

/**
 * UAnimNotify_PlayCue
 *
 * Fires a named VFX/SFX cue at the correct frame of a swing montage.
 */
UCLASS(meta = (DisplayName = "Play Cue"))
class MINECRAFTDEWISH_API UAnimNotify_PlayCue : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notify")
	FName CueName = NAME_None;
};

/**
 * UAnimNotify_ActionComplete
 *
 * Signals that the swing action has fully completed (recovery done).
 */
UCLASS(meta = (DisplayName = "Action Complete"))
class MINECRAFTDEWISH_API UAnimNotify_ActionComplete : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};

/**
 * UAnimNotifyState_MiningLoop
 *
 * Sustained mining loop: begin/end/tick callbacks while the montage section plays.
 */
UCLASS(meta = (DisplayName = "Mining Loop"))
class MINECRAFTDEWISH_API UAnimNotifyState_MiningLoop : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};