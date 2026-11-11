#include "EquippedToolActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AEquippedToolActor::AEquippedToolActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ToolMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ToolMeshComponent"));
	RootComponent = ToolMeshComponent;

	// Tools are visual only on the viewmodel; no collision.
	ToolMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ToolMeshComponent->SetCastShadow(false);
}

void AEquippedToolActor::BeginPlay()
{
	Super::BeginPlay();
}

void AEquippedToolActor::EquipMesh(UStaticMesh* InMesh, EToolType InToolType, EToolTier InToolTier)
{
	if (!ToolMeshComponent)
	{
		return;
	}

	if (!InMesh)
	{
		Unequip();
		return;
	}

	ToolMeshComponent->SetStaticMesh(InMesh);
	CurrentToolType = InToolType;
	CurrentToolTier = InToolTier;
	bIsEquipped = true;

	OnToolEquippedBP(CurrentToolType, CurrentToolTier);
}

void AEquippedToolActor::EquipMeshFromSoftRef(const TSoftObjectPtr<UStaticMesh>& InMesh, EToolType InToolType, EToolTier InToolTier)
{
	UStaticMesh* Loaded = InMesh.Get();
	if (!Loaded && !InMesh.IsNull())
	{
		Loaded = InMesh.LoadSynchronous();
	}
	EquipMesh(Loaded, InToolType, InToolTier);
}

void AEquippedToolActor::Unequip()
{
	if (ToolMeshComponent)
	{
		ToolMeshComponent->SetStaticMesh(nullptr);
	}
	CurrentToolType = EToolType::None;
	CurrentToolTier = EToolTier::None;
	bIsEquipped = false;

	OnToolUnequippedBP();
}