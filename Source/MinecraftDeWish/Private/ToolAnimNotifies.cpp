#include "ToolAnimNotifies.h"

#include "ToolActionReceiver.h"

void UAnimNotify_ActionWindow::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}
	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (Owner->Implements<UToolActionReceiver>())
		{
			IToolActionReceiver::Execute_HandleActionWindow(Owner);
		}
	}
}

void UAnimNotify_PlayCue::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}
	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (Owner->Implements<UToolActionReceiver>())
		{
			IToolActionReceiver::Execute_HandlePlayCue(Owner, CueName);
		}
	}
}

void UAnimNotify_ActionComplete::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}
	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (Owner->Implements<UToolActionReceiver>())
		{
			IToolActionReceiver::Execute_HandleActionComplete(Owner);
		}
	}
}

void UAnimNotifyState_MiningLoop::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}
	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (Owner->Implements<UToolActionReceiver>())
		{
			IToolActionReceiver::Execute_HandleMiningLoopBegin(Owner);
		}
	}
}

void UAnimNotifyState_MiningLoop::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp)
	{
		return;
	}
	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (Owner->Implements<UToolActionReceiver>())
		{
			IToolActionReceiver::Execute_HandleMiningLoopTick(Owner, FrameDeltaTime);
		}
	}
}

void UAnimNotifyState_MiningLoop::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}
	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (Owner->Implements<UToolActionReceiver>())
		{
			IToolActionReceiver::Execute_HandleMiningLoopEnd(Owner);
		}
	}
}