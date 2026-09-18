#include "XPOrbPickup.h"
#include "PlayerVitalSystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

AXPOrbPickup::AXPOrbPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(CollectRadius);
	SphereComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
	SphereComponent->SetGenerateOverlapEvents(true);
	RootComponent = SphereComponent;

	OrbMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OrbMesh"));
	OrbMesh->SetupAttachment(RootComponent);
	OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OrbMesh->bUseAsyncCooking = true;
}

void AXPOrbPickup::BeginPlay()
{
	Super::BeginPlay();

	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AXPOrbPickup::OnOverlapBegin);

	BaseZ = GetActorLocation().Z;
	BobTimer = FMath::FRandRange(0.0f, 6.28f);

	// Initial pop impulse
	CurrentVelocity = FVector(
		FMath::FRandRange(-100.0f, 100.0f),
		FMath::FRandRange(-100.0f, 100.0f),
		FMath::FRandRange(150.0f, 300.0f)
	);

	GenerateOrbMesh();
}

void AXPOrbPickup::InitializeXP(int32 InAmount)
{
	XPAmount = FMath::Max(1, InAmount);
}

void AXPOrbPickup::GenerateOrbMesh()
{
	if (!OrbMesh)
	{
		return;
	}

	// Generate a 3D octahedron / diamond orb (Minecraft XP orb shape)
	const float S = 12.0f; // Half-size in cm

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	// 6 vertices of an octahedron
	const FVector Top(0.0f, 0.0f, S * 1.3f);
	const FVector Bottom(0.0f, 0.0f, -S * 1.3f);
	const FVector North(S, 0.0f, 0.0f);
	const FVector East(0.0f, S, 0.0f);
	const FVector South(-S, 0.0f, 0.0f);
	const FVector West(0.0f, -S, 0.0f);

	auto AddTri = [&](const FVector& V0, const FVector& V1, const FVector& V2)
	{
		const int32 StartIdx = Vertices.Num();
		Vertices.Add(V0);
		Vertices.Add(V1);
		Vertices.Add(V2);

		Triangles.Add(StartIdx + 0);
		Triangles.Add(StartIdx + 1);
		Triangles.Add(StartIdx + 2);

		const FVector Normal = FVector::CrossProduct(V1 - V0, V2 - V0).GetSafeNormal();
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);

		UVs.Add(FVector2D(0.5f, 0.0f));
		UVs.Add(FVector2D(1.0f, 1.0f));
		UVs.Add(FVector2D(0.0f, 1.0f));

		// XP green-yellow glow color
		const FLinearColor XPColor(0.4f, 1.0f, 0.1f, 1.0f);
		Colors.Add(XPColor);
		Colors.Add(XPColor);
		Colors.Add(XPColor);

		const FProcMeshTangent Tan(FVector(1, 0, 0), false);
		Tangents.Add(Tan);
		Tangents.Add(Tan);
		Tangents.Add(Tan);
	};

	// Upper pyramid
	AddTri(Top, North, East);
	AddTri(Top, East, South);
	AddTri(Top, South, West);
	AddTri(Top, West, North);

	// Lower pyramid
	AddTri(Bottom, East, North);
	AddTri(Bottom, South, East);
	AddTri(Bottom, West, South);
	AddTri(Bottom, North, West);

	OrbMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

void AXPOrbPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Idle rotation
	AddActorLocalRotation(FRotator(0.0f, 150.0f * DeltaTime, 75.0f * DeltaTime));

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;

	if (PlayerPawn)
	{
		const FVector ToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
		const float Dist = ToPlayer.Size();

		if (Dist <= AttractionRadius)
		{
			// Magnet pull towards player
			const FVector Dir = ToPlayer.GetSafeNormal();
			const float Speed = FMath::Lerp(MaxAttractionSpeed, 300.0f, Dist / AttractionRadius);
			CurrentVelocity = Dir * Speed;
			SetActorLocation(GetActorLocation() + CurrentVelocity * DeltaTime);

			if (Dist <= CollectRadius)
			{
				if (UPlayerVitalSystem* Vitals = PlayerPawn->FindComponentByClass<UPlayerVitalSystem>())
				{
					Vitals->AddXP(XPAmount);
				}
				Destroy();
				return;
			}
			return;
		}
	}

	// Physics fall or idle bob
	if (!bIsGrounded)
	{
		CurrentVelocity.Z -= 980.0f * DeltaTime; // Gravity
		FVector NewLoc = GetActorLocation() + CurrentVelocity * DeltaTime;

		FHitResult GroundHit;
		FCollisionQueryParams QParams;
		QParams.AddIgnoredActor(this);
		if (PlayerPawn)
		{
			QParams.AddIgnoredActor(PlayerPawn);
		}

		if (GetWorld()->LineTraceSingleByChannel(GroundHit, GetActorLocation(), NewLoc, ECC_WorldStatic, QParams))
		{
			bIsGrounded = true;
			BaseZ = GroundHit.ImpactPoint.Z + 20.0f;
			SetActorLocation(FVector(NewLoc.X, NewLoc.Y, BaseZ));
			CurrentVelocity = FVector::ZeroVector;
		}
		else
		{
			SetActorLocation(NewLoc);
		}
	}
	else
	{
		// Gentle sinusoidal bobbing
		BobTimer += DeltaTime * 3.0f;
		const float BobOffset = FMath::Sin(BobTimer) * 8.0f;
		FVector Loc = GetActorLocation();
		Loc.Z = BaseZ + BobOffset;
		SetActorLocation(Loc);
	}
}

void AXPOrbPickup::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!OtherActor)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn && Pawn->IsPlayerControlled())
	{
		if (UPlayerVitalSystem* Vitals = Pawn->FindComponentByClass<UPlayerVitalSystem>())
		{
			Vitals->AddXP(XPAmount);
			UE_LOG(LogTemp, Log, TEXT("Player collected XP orb: +%d XP"), XPAmount);
		}
		Destroy();
	}
}
