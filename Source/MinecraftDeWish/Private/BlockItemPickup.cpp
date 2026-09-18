#include "BlockItemPickup.h"
#include "WorldGenerator.h"
#include "QuickSlotsInventorySystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UObject/UObjectIterator.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ABlockItemPickup::ABlockItemPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->InitSphereRadius(90.0f); // 2x attraction/grab radius (was 45.0f)
	SphereComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SphereComponent->SetGenerateOverlapEvents(true);

	MiniBlockMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MiniBlockMesh"));
	MiniBlockMesh->SetupAttachment(RootComponent);
	MiniBlockMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MiniBlockMesh->SetCastShadow(true);

	// Hard asset reference for cooker & standalone builds
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(TEXT("/Game/Materials/M_Global.M_Global"));
	if (MatFinder.Succeeded())
	{
		CachedMaterial = MatFinder.Object;
	}
}

void ABlockItemPickup::BeginPlay()
{
	Super::BeginPlay();

	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ABlockItemPickup::OnOverlapBegin);

	// Initial gentle pop when spawning
	VerticalVelocity = 120.0f;
	bIsGrounded = false;

	if (MiniBlockMesh->GetNumSections() == 0)
	{
		InitializePickup(BlockID, ItemCount, nullptr);
	}
}

void ABlockItemPickup::LaunchPickup(const FVector& InVelocity, float InCooldown)
{
	Velocity = InVelocity;
	VerticalVelocity = InVelocity.Z;
	PickupCooldown = InCooldown;
	bIsGrounded = false;
}

void ABlockItemPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Count down pickup cooldown (preventing immediate re-pickup after dropping)
	if (PickupCooldown > 0.0f)
	{
		PickupCooldown = FMath::Max(0.0f, PickupCooldown - DeltaTime);
	}

	RunningTime += DeltaTime;

	// Rotate continuously around Z axis
	AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));

	// Physics gravity & trajectory simulation
	UpdateGravity(DeltaTime);

	// Magnet attraction toward player (only if cooldown expired)
	if (PickupCooldown <= 0.0f)
	{
		UpdatePlayerAttraction(DeltaTime);
	}

	// Stack merging in the world (up to 64 items)
	CheckNearbyStackMerging();

	// Gentle floating bob when grounded
	if (bIsGrounded)
	{
		const float BobOffsetZ = FMath::Sin(RunningTime * BobFrequency) * BobHeight;
		MiniBlockMesh->SetRelativeLocation(MeshBaseOffset + FVector(0.0f, 0.0f, BobOffsetZ));
	}
}

void ABlockItemPickup::UpdatePlayerAttraction(float DeltaTime)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->GetPawn())
	{
		return;
	}

	const FVector PlayerTarget = PC->GetPawn()->GetActorLocation() + FVector(0.0f, 0.0f, -20.0f);
	const FVector MyLoc = GetActorLocation();
	const float Dist = FVector::Dist(PlayerTarget, MyLoc);

	if (Dist < AttractionRadius && Dist > 15.0f)
	{
		const FVector FlyDir = (PlayerTarget - MyLoc).GetSafeNormal();
		const float PullRatio = 1.0f - (Dist / AttractionRadius);
		const float CurrentSpeed = FMath::Lerp(AttractSpeed * 0.6f, AttractSpeed * 1.5f, PullRatio);

		AddActorWorldOffset(FlyDir * CurrentSpeed * DeltaTime, false);
		bIsGrounded = false;
	}
}

void ABlockItemPickup::UpdateGravity(float DeltaTime)
{
	if (!bIsGrounded)
	{
		// Apply gravity
		Velocity.Z -= GravityStrength * DeltaTime;
		VerticalVelocity = Velocity.Z;

		// Air drag on horizontal motion
		Velocity.X *= FMath::Clamp(1.0f - (1.2f * DeltaTime), 0.0f, 1.0f);
		Velocity.Y *= FMath::Clamp(1.0f - (1.2f * DeltaTime), 0.0f, 1.0f);

		const FVector CurrentLoc = GetActorLocation();
		const FVector Step = Velocity * DeltaTime;
		const FVector TargetLoc = CurrentLoc + Step;

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(Hit, CurrentLoc, TargetLoc, ECC_WorldStatic, Params))
		{
			if (Hit.ImpactNormal.Z > 0.5f) // Floor hit
			{
				bIsGrounded = true;
				Velocity = FVector::ZeroVector;
				VerticalVelocity = 0.0f;
				GroundZ = Hit.ImpactPoint.Z + 14.0f;
				SetActorLocation(FVector(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, GroundZ));
			}
			else // Wall hit: stop horizontal motion and slide down
			{
				Velocity.X = 0.0f;
				Velocity.Y = 0.0f;
				SetActorLocation(Hit.ImpactPoint + (Hit.ImpactNormal * 12.0f));
			}
		}
		else
		{
			AddActorWorldOffset(Step);
		}
	}
	else
	{
		// Grounded: periodically verify ground still exists (in case block below was mined)
		CheckFloorTimer += DeltaTime;
		if (CheckFloorTimer >= 0.2f)
		{
			CheckFloorTimer = 0.0f;
			const FVector CurrentLoc = GetActorLocation();
			const FVector TraceEnd = CurrentLoc - FVector(0.0f, 0.0f, 25.0f);

			FHitResult Hit;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			if (!GetWorld()->LineTraceSingleByChannel(Hit, CurrentLoc, TraceEnd, ECC_WorldStatic, Params))
			{
				bIsGrounded = false;
				Velocity = FVector::ZeroVector;
				VerticalVelocity = 0.0f;
			}
		}
	}
}

void ABlockItemPickup::CheckNearbyStackMerging()
{
	CheckMergeTimer += 0.05f;
	if (CheckMergeTimer < 0.2f)
	{
		return;
	}
	CheckMergeTimer = 0.0f;

	TArray<AActor*> OverlappingPickups;
	SphereComponent->GetOverlappingActors(OverlappingPickups, ABlockItemPickup::StaticClass());

	for (AActor* Actor : OverlappingPickups)
	{
		ABlockItemPickup* Other = Cast<ABlockItemPickup>(Actor);
		if (!Other || Other == this || Other->IsActorBeingDestroyed())
		{
			continue;
		}

		if (Other->BlockID != this->BlockID)
		{
			continue;
		}

		// Use deterministic ID pairing so only one pickup drives the merge
		if (this->GetUniqueID() > Other->GetUniqueID())
		{
			continue;
		}

		if (this->ItemCount >= MaxStackSize)
		{
			// Primary stack is already at maximum (64).
			// If a 65th item tries to merge, ensure it stays visible as a secondary stack next to this stack
			const float DistSq = FVector::DistSquared(this->GetActorLocation(), Other->GetActorLocation());
			if (DistSq < 28.0f * 28.0f)
			{
				FVector PushDir = (Other->GetActorLocation() - this->GetActorLocation()).GetSafeNormal2D();
				if (PushDir.IsNearlyZero())
				{
					PushDir = FVector(1.0f, 0.0f, 0.0f);
				}
				Other->SetActorLocation(this->GetActorLocation() + (PushDir * 35.0f));
				Other->bIsGrounded = false;
			}
		}
		else
		{
			const int32 Space = MaxStackSize - this->ItemCount;
			const int32 AmountToTake = FMath::Min(Space, Other->ItemCount);

			this->ItemCount += AmountToTake;
			Other->ItemCount -= AmountToTake;
			this->UpdatePileMesh();

			if (Other->ItemCount <= 0)
			{
				Other->Destroy();
			}
			else
			{
				// Overflow items remaining in Other (e.g. 65th item)!
				// Position Other distinctly beside the primary stack (+35cm)
				FVector PushDir = (Other->GetActorLocation() - this->GetActorLocation()).GetSafeNormal2D();
				if (PushDir.IsNearlyZero())
				{
					PushDir = FVector(1.0f, 0.0f, 0.0f);
				}
				Other->SetActorLocation(this->GetActorLocation() + (PushDir * 35.0f));
				Other->bIsGrounded = false;
				Other->UpdatePileMesh();
			}
		}
	}
}

void ABlockItemPickup::UpdatePileMesh()
{
	if (!CachedMaterial)
	{
		CachedMaterial = Cast<UMaterialInterface>(StaticLoadObject(
			UMaterialInterface::StaticClass(),
			nullptr,
			TEXT("/Game/Materials/M_Global.M_Global")
		));
	}
	BuildMiniBlockMesh(CachedMaterial);
}

void ABlockItemPickup::InitializePickup(uint8 InBlockID, int32 InCount, UMaterialInterface* InMaterial)
{
	BlockID = InBlockID;
	ItemCount = InCount;

	if (!InMaterial)
	{
		InMaterial = Cast<UMaterialInterface>(StaticLoadObject(
			UMaterialInterface::StaticClass(),
			nullptr,
			TEXT("/Game/Materials/M_Global.M_Global")
		));
	}
	CachedMaterial = InMaterial;

	BuildMiniBlockMesh(InMaterial);
}

void ABlockItemPickup::BuildMiniBlockMesh(UMaterialInterface* Material)
{
	MiniBlockMesh->ClearAllMeshSections();

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FVector2D> UV1;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Resolve texture slice index for this block
	int32 TopTex = 14;    // Default dirt
	int32 SideTex = 14;
	int32 BottomTex = 14;
	int32 FrontTex = 14;

	UDataTable* BlockDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/Data/Block_DataTable.Block_DataTable"));
	if (BlockDataTable)
	{
		const UScriptStruct* RowStruct = BlockDataTable->GetRowStruct();
		if (RowStruct)
		{
			for (auto It = BlockDataTable->GetRowMap().CreateConstIterator(); It; ++It)
			{
				const uint8* RowData = It.Value();
				if (!RowData) continue;

				int32 RowBlockID = 0;
				int32 RowSide = 0, RowTop = 0, RowFront = 0, RowBottom = 0;

				for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
				{
					FProperty* Prop = *PropIt;
					const FString PropName = Prop->GetName();

					if (PropName.Contains(TEXT("BlockID"), ESearchCase::IgnoreCase))
					{
						if (FIntProperty* IP = CastField<FIntProperty>(Prop))
							RowBlockID = IP->GetPropertyValue_InContainer(RowData);
					}
					else if (PropName.Contains(TEXT("Side"), ESearchCase::IgnoreCase) && PropName.Contains(TEXT("Texture"), ESearchCase::IgnoreCase))
					{
						if (FIntProperty* IP = CastField<FIntProperty>(Prop))
							RowSide = IP->GetPropertyValue_InContainer(RowData);
					}
					else if (PropName.Contains(TEXT("Top"), ESearchCase::IgnoreCase) && PropName.Contains(TEXT("Texture"), ESearchCase::IgnoreCase))
					{
						if (FIntProperty* IP = CastField<FIntProperty>(Prop))
							RowTop = IP->GetPropertyValue_InContainer(RowData);
					}
					else if (PropName.Contains(TEXT("Front"), ESearchCase::IgnoreCase) && !PropName.Contains(TEXT("Active"), ESearchCase::IgnoreCase))
					{
						if (FIntProperty* IP = CastField<FIntProperty>(Prop))
							RowFront = IP->GetPropertyValue_InContainer(RowData);
					}
					else if (PropName.Contains(TEXT("Bottom"), ESearchCase::IgnoreCase))
					{
						if (FIntProperty* IP = CastField<FIntProperty>(Prop))
							RowBottom = IP->GetPropertyValue_InContainer(RowData);
					}
				}

				if (RowBlockID == static_cast<int32>(BlockID))
				{
					TopTex = RowTop;
					SideTex = RowSide;
					FrontTex = RowFront;
					BottomTex = RowBottom;
					break;
				}
			}
		}
	}

	const float H = 9.0f; // miniature block radius

	auto AddQuad = [&](const FVector& V0, const FVector& V1, const FVector& V2, const FVector& V3, const FVector& Normal, int32 TexSlice)
	{
		const int32 StartIdx = Vertices.Num();
		Vertices.Add(V0);
		Vertices.Add(V1);
		Vertices.Add(V2);
		Vertices.Add(V3);

		// CCW winding for UE5 front-facing
		Triangles.Add(StartIdx + 0);
		Triangles.Add(StartIdx + 2);
		Triangles.Add(StartIdx + 1);
		Triangles.Add(StartIdx + 0);
		Triangles.Add(StartIdx + 3);
		Triangles.Add(StartIdx + 2);

		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);

		UV0.Add(FVector2D(0.0f, 1.0f));
		UV0.Add(FVector2D(1.0f, 1.0f));
		UV0.Add(FVector2D(1.0f, 0.0f));
		UV0.Add(FVector2D(0.0f, 0.0f));

		const float Slice = static_cast<float>(TexSlice);
		UV1.Add(FVector2D(Slice, 0.0f));
		UV1.Add(FVector2D(Slice, 0.0f));
		UV1.Add(FVector2D(Slice, 0.0f));
		UV1.Add(FVector2D(Slice, 0.0f));

		const FLinearColor VC(Slice, 0.0f, 0.0f, 1.0f);
		VertexColors.Add(VC);
		VertexColors.Add(VC);
		VertexColors.Add(VC);
		VertexColors.Add(VC);

		const FProcMeshTangent Tangent(FVector(1.0f, 0.0f, 0.0f), false);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
	};

	auto AddCube = [&](const FVector& CenterOffset, float YawDegrees)
	{
		const FRotator Rot(0.0f, YawDegrees, 0.0f);
		const FMatrix RotMat = FRotationMatrix(Rot);

		auto Xform = [&](const FVector& LocalPos) -> FVector
		{
			return RotMat.TransformPosition(LocalPos) + CenterOffset;
		};

		// Top (+Z)
		AddQuad(Xform(FVector(-H, -H, H)), Xform(FVector(H, -H, H)), Xform(FVector(H, H, H)), Xform(FVector(-H, H, H)), RotMat.TransformVector(FVector(0.0f, 0.0f, 1.0f)), TopTex);
		// Bottom (-Z)
		AddQuad(Xform(FVector(-H, H, -H)), Xform(FVector(H, H, -H)), Xform(FVector(H, -H, -H)), Xform(FVector(-H, -H, -H)), RotMat.TransformVector(FVector(0.0f, 0.0f, -1.0f)), BottomTex);
		// North (+X)
		AddQuad(Xform(FVector(H, -H, -H)), Xform(FVector(H, H, -H)), Xform(FVector(H, H, H)), Xform(FVector(H, -H, H)), RotMat.TransformVector(FVector(1.0f, 0.0f, 0.0f)), FrontTex);
		// South (-X)
		AddQuad(Xform(FVector(-H, H, -H)), Xform(FVector(-H, -H, -H)), Xform(FVector(-H, -H, H)), Xform(FVector(-H, H, H)), RotMat.TransformVector(FVector(-1.0f, 0.0f, 0.0f)), SideTex);
		// East (+Y)
		AddQuad(Xform(FVector(H, H, -H)), Xform(FVector(-H, H, -H)), Xform(FVector(-H, H, H)), Xform(FVector(H, H, H)), RotMat.TransformVector(FVector(0.0f, 1.0f, 0.0f)), SideTex);
		// West (-Y)
		AddQuad(Xform(FVector(-H, -H, -H)), Xform(FVector(H, -H, -H)), Xform(FVector(H, -H, H)), Xform(FVector(-H, -H, H)), RotMat.TransformVector(FVector(0.0f, -1.0f, 0.0f)), SideTex);
	};

	// Visually stack items as a miniature pile based on stack count
	if (ItemCount <= 1)
	{
		AddCube(FVector::ZeroVector, 0.0f);
	}
	else if (ItemCount < 16)
	{
		AddCube(FVector(-3.5f, -3.0f, 0.0f), -15.0f);
		AddCube(FVector(3.5f, 3.0f, 2.0f), 25.0f);
	}
	else if (ItemCount < 32)
	{
		AddCube(FVector(-4.5f, -3.5f, 0.0f), -20.0f);
		AddCube(FVector(4.0f, -2.5f, 1.5f), 30.0f);
		AddCube(FVector(-1.0f, 4.0f, 3.0f), 10.0f);
	}
	else
	{
		AddCube(FVector(-5.0f, -4.0f, 0.0f), -25.0f);
		AddCube(FVector(4.5f, -3.0f, 1.5f), 35.0f);
		AddCube(FVector(-2.0f, 4.5f, 2.5f), 15.0f);
		AddCube(FVector(0.0f, 0.0f, 6.5f), -10.0f);
	}

	MiniBlockMesh->CreateMeshSection_LinearColor(
		0,
		Vertices,
		Triangles,
		Normals,
		UV0,
		UV1,
		TArray<FVector2D>(),
		TArray<FVector2D>(),
		VertexColors,
		Tangents,
		false
	);

	if (Material)
	{
		MiniBlockMesh->SetMaterial(0, Material);
	}
}

void ABlockItemPickup::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!OtherActor || OtherActor == this || PickupCooldown > 0.0f)
	{
		return;
	}

	// Only players can pick up items
	APawn* PlayerPawn = Cast<APawn>(OtherActor);
	if (!PlayerPawn || !PlayerPawn->IsPlayerControlled())
	{
		return;
	}

	if (TryAddToPlayerInventory(OtherActor))
	{
		Destroy();
	}
}

bool ABlockItemPickup::TryAddToPlayerInventory(AActor* PlayerActor)
{
	if (!PlayerActor)
	{
		return false;
	}

	int32 RemainingCount = ItemCount;
	const bool bAdded = UQuickSlotsInventorySystem::TryAddItemToPlayerInventory(PlayerActor, BlockID, ItemCount, RemainingCount);

	if (bAdded)
	{
		if (RemainingCount <= 0)
		{
			// All items were successfully ingested into player quickslots
			return true;
		}
		else
		{
			// Partially ingested (e.g. slots hit 64 limit), retain leftover count in world
			ItemCount = RemainingCount;
			UpdatePileMesh();
			return false;
		}
	}

	return false;
}
