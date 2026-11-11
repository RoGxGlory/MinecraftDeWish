#include "PlayerViewModel.h"

#include "EquippedToolActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "WorldGenerator.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "ToolTypes.h"
#include "ToolData.h"



APlayerViewModel::APlayerViewModel()
{
	PrimaryActorTick.bCanEverTick = false;

	ArmsMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ArmsMeshComponent"));
	RootComponent = ArmsMeshComponent;

	ArmsMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArmsMeshComponent->SetCastShadow(false);
}

void APlayerViewModel::BeginPlay()
{
	Super::BeginPlay();

	EnsureArmsMeshLoaded();

	if (bOnlyOwnerSeeArms)
	{
		ArmsMeshComponent->SetOnlyOwnerSee(true);
	}

	SpawnToolActor();

	if (!WorldGenerator)
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AWorldGenerator> It(World); It; ++It)
			{
				WorldGenerator = *It;
				break;
			}
		}
	}

	if (BaublesSystem)
	{
		BaublesSystem->OnActiveToolChanged.AddDynamic(this, &APlayerViewModel::OnActiveToolChangedHandler);
		SyncEquippedToolFromActive();
	}
}

void APlayerViewModel::EnsureArmsMeshLoaded()
{
	if (!ArmsMeshComponent)
	{
		return;
	}
	USkeletalMesh* Mesh = ArmsMeshAsset.Get();
	if (!Mesh && !ArmsMeshAsset.IsNull())
	{
		Mesh = ArmsMeshAsset.LoadSynchronous();
	}
	if (Mesh)
	{
		ArmsMeshComponent->SetSkeletalMesh(Mesh);
	}
}

void APlayerViewModel::SpawnToolActor()
{
	if (!GetWorld())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	EquippedToolActor = GetWorld()->SpawnActor<AEquippedToolActor>(AEquippedToolActor::StaticClass(), FTransform::Identity, Params);
	if (EquippedToolActor && ArmsMeshComponent)
	{
		EquippedToolActor->AttachToComponent(ArmsMeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ToolSocketName);
	}
}

void APlayerViewModel::EquipToolMesh(UStaticMesh* InMesh, EToolType InToolType, EToolTier InToolTier)
{
	if (EquippedToolActor)
	{
		EquippedToolActor->EquipMesh(InMesh, InToolType, InToolTier);
	}
}

void APlayerViewModel::UnequipTool()
{
	if (EquippedToolActor)
	{
		EquippedToolActor->Unequip();
	}
}

void APlayerViewModel::SyncEquippedToolFromActive()
{
	if (!BaublesSystem || !EquippedToolActor)
	{
		return;
	}
	const FToolInstance ActiveTool = BaublesSystem->GetActiveTool();
	if (!ActiveTool.IsValid())
	{
		UnequipTool();
		return;
	}
	UStaticMesh* Mesh = nullptr;
	TSoftObjectPtr<UStaticMesh> MeshRef = ActiveTool.GetMesh(ToolDefinition);
	if (!MeshRef.IsNull())
	{
		Mesh = MeshRef.Get();
		if (!Mesh)
		{
			Mesh = MeshRef.LoadSynchronous();
		}
	}
	EquipToolMesh(Mesh, ActiveTool.ToolType, ActiveTool.ToolTier);
}

void APlayerViewModel::OnActiveToolChangedHandler(EToolType NewToolType)
{
	SyncEquippedToolFromActive();
}

void APlayerViewModel::PlaySwingMontage(EToolType ToolType, float PlayRate)
{
	if (!ArmsMeshComponent)
	{
		return;
	}
	UAnimInstance* AnimInstance = ArmsMeshComponent->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	const TSoftObjectPtr<UAnimMontage>* Found = SwingMontages.Find(ToolType);
	TSoftObjectPtr<UAnimMontage> MontageRef;
	if (Found && !Found->IsNull())
	{
		MontageRef = *Found;
	}
	else if (ToolDefinition && BaublesSystem)
	{
		const FToolInstance ActiveTool = BaublesSystem->GetActiveTool();
		MontageRef = ActiveTool.GetSwingMontage(ToolDefinition);
	}
	if (MontageRef.IsNull())
	{
		return;
	}
	UAnimMontage* Montage = MontageRef.Get();
	if (!Montage)
	{
		Montage = MontageRef.LoadSynchronous();
	}
	if (Montage)
	{
		AnimInstance->Montage_Play(Montage, PlayRate);
		bIsSwinging = true;
	}
}

void APlayerViewModel::StopSwingMontage(float BlendOut)
{
	if (!ArmsMeshComponent)
	{
		return;
	}
	UAnimInstance* AnimInstance = ArmsMeshComponent->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Stop(BlendOut);
	}
}

// ==================== IToolActionReceiver ====================

void APlayerViewModel::ResolveTargetedVoxel()
{
	if (!WorldGenerator)
	{
		return;
	}
	FVector BreakPos = FVector::ZeroVector;
	FVector PlacePos = FVector::ZeroVector;
	uint8 BlockID = 0;
	if (WorldGenerator->TraceForBlock(nullptr, 500.0f, BreakPos, PlacePos, BlockID))
	{
		int32 VX = 0, VY = 0, VZ = 0;
		WorldGenerator->WorldLocationToVoxelCoord(BreakPos, VX, VY, VZ);
		MiningTargetVoxel = FIntVector(VX, VY, VZ);
	}
}

void APlayerViewModel::HandleActionWindow_Implementation()
{
	bIsSwinging = true;

	if (!WorldGenerator)
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AWorldGenerator> It(World); It; ++It)
			{
				WorldGenerator = *It;
				break;
			}
		}
	}
	if (!WorldGenerator)
	{
		return;
	}

	FVector BreakPos = FVector::ZeroVector;
	FVector PlacePos = FVector::ZeroVector;
	uint8 BlockID = 0;
	if (WorldGenerator->TraceForBlock(nullptr, 500.0f, BreakPos, PlacePos, BlockID) && BlockID != 0)
	{
		int32 VX = 0, VY = 0, VZ = 0;
		WorldGenerator->WorldLocationToVoxelCoord(BreakPos, VX, VY, VZ);
		MiningTargetVoxel = FIntVector(VX, VY, VZ);
		// Queue the break at the targeted voxel (mining cadence driven by the notify).
		WorldGenerator->QueueBlockBreakAtVoxel(MiningTargetVoxel.X, MiningTargetVoxel.Y, MiningTargetVoxel.Z);
	}
}

void APlayerViewModel::HandlePlayCue_Implementation(FName CueName)
{
	if (CueName.IsNone())
	{
		return;
	}

	FVector Location = FVector::ZeroVector;
	if (EquippedToolActor)
	{
		Location = EquippedToolActor->GetActorLocation();
	}

	// Attempt to load a Niagara system named NS_<CueName>.
	const FString NiagaraPath = FString::Printf(TEXT("/Game/Effects/NS_%s.NS_%s"), *CueName.ToString(), *CueName.ToString());
	if (UNiagaraSystem* NiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, *NiagaraPath))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NiagaraSystem, Location, FRotator::ZeroRotator);
	}

	// Attempt to load a matching sound asset SFX_<CueName>.
	const FString SoundPath = FString::Printf(TEXT("/Game/Audio/SFX/SFX_%s.SFX_%s"), *CueName.ToString(), *CueName.ToString());
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, *SoundPath))
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, Location);
	}

	UE_LOG(LogMinecraftDeWish, Log, TEXT("PlayerViewModel: PlayCue '%s'"), *CueName.ToString());
}

void APlayerViewModel::HandleActionComplete_Implementation()
{
	bIsSwinging = false;
	bIsMining = false;
	MiningAccumulatedTime = 0.0f;
	MiningTotalBreakTime = 0.0f;
	MiningProgress = 0.0f;

	if (WorldGenerator)
	{
		WorldGenerator->ClearCrackMining();
	}
}

void APlayerViewModel::HandleMiningLoopBegin_Implementation()
{
	bIsMining = true;
	MiningAccumulatedTime = 0.0f;
	MiningProgress = 0.0f;

	if (!WorldGenerator)
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AWorldGenerator> It(World); It; ++It)
			{
				WorldGenerator = *It;
				break;
			}
		}
	}

	ResolveTargetedVoxel();

	if (WorldGenerator)
	{
		float Force = 1.0f;
		if (BaublesSystem)
		{
			uint8 BlockID = 0;
			WorldGenerator->GetVoxelAt(MiningTargetVoxel.X, MiningTargetVoxel.Y, MiningTargetVoxel.Z, BlockID);
			Force = BaublesSystem->GetBestMiningForce(BlockID);
		}
		MiningTotalBreakTime = WorldGenerator->GetBlockBreakTimeAtVoxel(MiningTargetVoxel, Force);
	}
}

void APlayerViewModel::HandleMiningLoopTick_Implementation(float FrameDeltaTime)
{
	if (!bIsMining)
	{
		return;
	}

	if (!WorldGenerator)
	{
		return;
	}

	if (MiningTargetVoxel == FIntVector(0, 0, 0))
	{
		ResolveTargetedVoxel();
	}

	MiningAccumulatedTime += FrameDeltaTime;
	if (MiningTotalBreakTime <= 0.0f)
	{
		float Force = 1.0f;
		if (BaublesSystem)
		{
			uint8 BlockID = 0;
			WorldGenerator->GetVoxelAt(MiningTargetVoxel.X, MiningTargetVoxel.Y, MiningTargetVoxel.Z, BlockID);
			Force = BaublesSystem->GetBestMiningForce(BlockID);
		}
		MiningTotalBreakTime = WorldGenerator->GetBlockBreakTimeAtVoxel(MiningTargetVoxel, Force);
	}

	if (MiningTotalBreakTime > 0.0f)
	{
		MiningProgress = FMath::Clamp(MiningAccumulatedTime / MiningTotalBreakTime, 0.0f, 1.0f);
	}
	else
	{
		MiningProgress = 1.0f;
	}

	WorldGenerator->UpdateCrackMining(MiningTargetVoxel, MiningProgress);
}

void APlayerViewModel::HandleMiningLoopEnd_Implementation()
{
	bIsMining = false;
	MiningAccumulatedTime = 0.0f;
	MiningTotalBreakTime = 0.0f;
	MiningProgress = 0.0f;

	if (WorldGenerator)
	{
		WorldGenerator->ClearCrackMining();
	}
}