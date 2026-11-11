#include "MobAIController.h"
#include "MobCharacter.h"

#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

AMobAIController::AMobAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bStartAILogicOnPossess = true;
	bStopAILogicOnUnposses = true;

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));

	// Perception component (sight + hearing) replaces UPawnSensingComponent (ADR-0009).
	UAIPerceptionComponent* PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*PerceptionComp);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1600.0f;
	SightConfig->LoseSightRadius = 2000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 2000.0f;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	PerceptionComp->ConfigureSense(*SightConfig);
	PerceptionComp->ConfigureSense(*HearingConfig);
	PerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());
}

void AMobAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UAIPerceptionComponent* Perc = GetAIPerceptionComponent())
	{
		Perc->OnTargetPerceptionUpdated.AddDynamic(this, &AMobAIController::OnTargetPerceptionUpdated);
	}

	if (AMobCharacter* Mob = GetMob())
	{
		ConfigurePerceptionForMob(Mob);
	}
}

void AMobAIController::OnUnPossess()
{
	if (UAIPerceptionComponent* Perc = GetAIPerceptionComponent())
	{
		Perc->OnTargetPerceptionUpdated.RemoveDynamic(this, &AMobAIController::OnTargetPerceptionUpdated);
	}
	Super::OnUnPossess();
}

AMobCharacter* AMobAIController::GetMob() const
{
	return Cast<AMobCharacter>(GetPawn());
}

void AMobAIController::SetPerceivedTarget(AActor* NewTarget)
{
	CurrentTarget = NewTarget;
}

void AMobAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		CurrentTarget = Actor;
	}
	else if (CurrentTarget.Get() == Actor)
	{
		// Keep last-known position behavior: clear only sight loss; hearing may re-acquire.
		CurrentTarget.Reset();
	}
}

void AMobAIController::ConfigurePerceptionForMob(AMobCharacter* Mob)
{
	if (!Mob)
	{
		return;
	}

	const float Range = Mob->MobDefinition.DetectionRange;
	if (SightConfig)
	{
		SightConfig->SightRadius = Range;
		SightConfig->LoseSightRadius = Range * 1.25f;
	}
	if (HearingConfig)
	{
		HearingConfig->HearingRange = Range * 1.5f;
	}
	if (UAIPerceptionComponent* Perc = GetAIPerceptionComponent())
	{
		Perc->RequestStimuliListenerUpdate();
	}
}