#include "ChaosBlockDestructionActor.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/KismetMathLibrary.h"

AChaosBlockDestructionActor::AChaosBlockDestructionActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f; // Tick every frame for smooth 60+ FPS physics simulation

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = ProceduralMesh;
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProceduralMesh->SetCastShadow(true);
	ProceduralMesh->SetMobility(EComponentMobility::Movable);

	// Master terrain material reference
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(TEXT("/Game/Materials/M_Global.M_Global"));
	if (MatFinder.Succeeded())
	{
		CachedMaterial = MatFinder.Object;
	}
}

void AChaosBlockDestructionActor::BeginPlay()
{
	Super::BeginPlay();

	// If spawned directly without deferred initialization
	if (!bIsInitialized && BlockID > 0)
	{
		InitializeDestruction(BlockID, FVector::UpVector, GetActorLocation(), BlockScale, CachedMaterial);
	}
}

void AChaosBlockDestructionActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Particles.Num() == 0)
	{
		return;
	}

	ElapsedTime += DeltaTime;

	const FVector ActorLoc = GetActorLocation();
	const float GroundFriction = 0.65f;
	const float Restitution = 0.32f;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (PC && PC->GetPawn())
	{
		QueryParams.AddIgnoredActor(PC->GetPawn());
	}

	// Physics step for all particles
	for (FBlockDestructionParticle& Part : Particles)
	{
		if (Part.bIsGrounded)
		{
			// Grounded slide decay
			if (!Part.Velocity.IsNearlyZero())
			{
				Part.Velocity = Part.Velocity * FMath::Clamp(1.0f - (8.0f * DeltaTime), 0.0f, 1.0f);
				Part.RelativePos += Part.Velocity * DeltaTime;
				if (Part.Velocity.SizeSquared() < 25.0f)
				{
					Part.Velocity = FVector::ZeroVector;
				}
			}
			continue;
		}

		// 1. Gravity acceleration
		Part.Velocity.Z -= GravityStrength * DeltaTime;

		// 2. Air resistance
		Part.Velocity *= FMath::Clamp(1.0f - (0.4f * DeltaTime), 0.0f, 1.0f);

		// 3. Angular tumbling
		Part.Rotation.Pitch += Part.AngularVelocity.X * DeltaTime;
		Part.Rotation.Yaw   += Part.AngularVelocity.Y * DeltaTime;
		Part.Rotation.Roll  += Part.AngularVelocity.Z * DeltaTime;

		// 4. World collision sweep against terrain
		const FVector Step = Part.Velocity * DeltaTime;
		const FVector StartWorld = ActorLoc + Part.RelativePos;
		const FVector EndWorld = StartWorld + Step;

		const float Radius = FMath::Min(Part.HalfExtents.X, Part.HalfExtents.Z);
		const FCollisionShape SphereShape = FCollisionShape::MakeSphere(Radius);

		FHitResult Hit;
		if (GetWorld()->SweepSingleByChannel(Hit, StartWorld, EndWorld, FQuat::Identity, ECC_WorldStatic, SphereShape, QueryParams))
		{
			// Collision with terrain or surface
			Part.RelativePos = (Hit.ImpactPoint + (Hit.ImpactNormal * Radius)) - ActorLoc;

			const float NormalSpeed = FVector::DotProduct(Part.Velocity, Hit.ImpactNormal);
			if (NormalSpeed < 0.0f)
			{
				const FVector TangentVel = Part.Velocity - (NormalSpeed * Hit.ImpactNormal);
				Part.Velocity = (TangentVel * (1.0f - GroundFriction)) - (Hit.ImpactNormal * NormalSpeed * Restitution);
				Part.AngularVelocity *= 0.5f;

				// Settle onto ground if velocity has damped down
				if (Part.Velocity.SizeSquared() < 600.0f)
				{
					Part.Velocity = FVector::ZeroVector;
					Part.AngularVelocity = FVector::ZeroVector;
					Part.bIsGrounded = true;
				}
			}
		}
		else
		{
			Part.RelativePos += Step;
		}
	}

	// Calculate shrink scale during fade duration
	float CurrentScale = 1.0f;
	if (ElapsedTime >= SimulationLifetime)
	{
		const float FadeElapsed = ElapsedTime - SimulationLifetime;
		CurrentScale = FMath::Clamp(1.0f - (FadeElapsed / FadeDuration), 0.0f, 1.0f);
	}

	// Update procedural mesh buffers
	UpdateMeshBuffers(CurrentScale);

	// Lifespan complete: destroy actor
	if (ElapsedTime >= (SimulationLifetime + FadeDuration))
	{
		Destroy();
	}
}

void AChaosBlockDestructionActor::InitializeDestruction(
	uint8 InBlockID,
	const FVector& InHitNormal,
	const FVector& InHitPoint,
	float InBlockScale,
	UMaterialInterface* InMaterial
)
{
	bIsInitialized = true;
	BlockID = InBlockID;
	BlockScale = InBlockScale;

	if (InMaterial)
	{
		CachedMaterial = InMaterial;
	}
	else if (!CachedMaterial)
	{
		CachedMaterial = Cast<UMaterialInterface>(StaticLoadObject(
			UMaterialInterface::StaticClass(),
			nullptr,
			TEXT("/Game/Materials/M_Global.M_Global")
		));
	}

	int32 TopTex = 14;
	int32 BottomTex = 14;
	int32 SideTex = 14;
	int32 FrontTex = 14;
	int32 InteriorTex = 14;

	ResolveBlockTextures(InBlockID, TopTex, BottomTex, SideTex, FrontTex, InteriorTex);

	SpawnParticles(TopTex, BottomTex, SideTex, FrontTex, InteriorTex, InHitNormal, InHitPoint);

	BuildInitialMesh();
}

void AChaosBlockDestructionActor::ResolveBlockTextures(
	uint8 InBlockID,
	int32& OutTopTex,
	int32& OutBottomTex,
	int32& OutSideTex,
	int32& OutFrontTex,
	int32& OutInteriorTex
)
{
	OutTopTex = 14;      // Dirt default
	OutBottomTex = 14;
	OutSideTex = 14;
	OutFrontTex = 14;
	OutInteriorTex = 14;

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

				if (RowBlockID == static_cast<int32>(InBlockID))
				{
					OutTopTex = RowTop;
					OutSideTex = RowSide;
					OutFrontTex = RowFront;
					OutBottomTex = RowBottom;
					break;
				}
			}
		}
	}

	// Interior slice determination
	switch (static_cast<EBlockType>(InBlockID))
	{
	case EBlockType::Grass:
		OutInteriorTex = OutBottomTex; // Dirt (14)
		break;
	case EBlockType::Wood_Log:
		OutInteriorTex = 33; // Wood Planks
		break;
	case EBlockType::Coal_Ore:
	case EBlockType::Iron_Ore:
	case EBlockType::Gold_Ore:
	case EBlockType::Diamond_Ore:
	case EBlockType::Emerald_Ore:
		OutInteriorTex = 35; // Stone
		break;
	case EBlockType::Crafting_Table:
		OutInteriorTex = 33; // Wood Planks
		break;
	case EBlockType::Furnace:
		OutInteriorTex = 6;  // Cobblestone
		break;
	default:
		OutInteriorTex = OutSideTex;
		break;
	}
}

void AChaosBlockDestructionActor::SpawnParticles(
	int32 TopTex,
	int32 BottomTex,
	int32 SideTex,
	int32 FrontTex,
	int32 InteriorTex,
	const FVector& InHitNormal,
	const FVector& InHitPoint
)
{
	Particles.Reset();

	// Spawn between 32 and 44 small particle slices
	const int32 TargetCount = FMath::RandRange(32, 44);
	const float HalfBlock = BlockScale * 0.45f;

	for (int32 i = 0; i < TargetCount; ++i)
	{
		FBlockDestructionParticle Part;

		// Random non-deterministic position distributed throughout the block cube
		Part.RelativePos = FVector(
			FMath::FRandRange(-HalfBlock, HalfBlock),
			FMath::FRandRange(-HalfBlock, HalfBlock),
			FMath::FRandRange(-HalfBlock, HalfBlock)
		);

		// Particle size: way smaller (half-extent 2.5cm to 4.5cm -> 5cm to 9cm across)
		Part.HalfExtents = FVector(
			FMath::FRandRange(2.5f, 4.5f),
			FMath::FRandRange(2.5f, 4.5f),
			FMath::FRandRange(2.5f, 4.5f)
		);

		// Texture slice assignment matching spatial location in the block
		if (Part.RelativePos.Z > HalfBlock * 0.4f)
		{
			Part.TextureSlice = TopTex;
		}
		else if (Part.RelativePos.Z < -HalfBlock * 0.4f)
		{
			Part.TextureSlice = BottomTex;
		}
		else if (Part.RelativePos.X > HalfBlock * 0.4f)
		{
			Part.TextureSlice = FrontTex;
		}
		else if (FMath::Abs(Part.RelativePos.Y) > HalfBlock * 0.4f || Part.RelativePos.X < -HalfBlock * 0.4f)
		{
			Part.TextureSlice = SideTex;
		}
		else
		{
			Part.TextureSlice = InteriorTex;
		}

		// Initial explosion velocity: radial burst outwards from block center
		FVector RadialDir = Part.RelativePos.GetSafeNormal();
		if (RadialDir.IsNearlyZero())
		{
			RadialDir = FMath::VRand();
		}

		Part.Velocity = RadialDir * FMath::FRandRange(220.0f, 440.0f);

		// Directional bias from mining strike normal
		if (!InHitNormal.IsNearlyZero())
		{
			Part.Velocity += InHitNormal * FMath::FRandRange(100.0f, 220.0f);
		}

		// Upward pop
		Part.Velocity.Z += FMath::FRandRange(180.0f, 380.0f);

		// Lateral scatter
		Part.Velocity += FMath::VRand() * FMath::FRandRange(40.0f, 100.0f);

		// Initial random 3D orientation and tumbling angular velocity
		Part.Rotation = FRotator(
			FMath::FRandRange(0.0f, 360.0f),
			FMath::FRandRange(0.0f, 360.0f),
			FMath::FRandRange(0.0f, 360.0f)
		);

		Part.AngularVelocity = FVector(
			FMath::FRandRange(-720.0f, 720.0f),
			FMath::FRandRange(-720.0f, 720.0f),
			FMath::FRandRange(-720.0f, 720.0f)
		);

		Part.bIsGrounded = false;

		Particles.Add(Part);
	}
}

void AChaosBlockDestructionActor::BuildInitialMesh()
{
	MeshVertices.Reset();
	MeshTriangles.Reset();
	MeshNormals.Reset();
	MeshUV0.Reset();
	MeshUV1.Reset();
	MeshVertexColors.Reset();
	MeshTangents.Reset();

	auto AddFace = [&](
		const FVector& V0, const FVector& V1, const FVector& V2, const FVector& V3,
		const FVector& Normal,
		int32 TexSlice
	)
	{
		const int32 StartIdx = MeshVertices.Num();

		MeshVertices.Add(V0);
		MeshVertices.Add(V1);
		MeshVertices.Add(V2);
		MeshVertices.Add(V3);

		MeshTriangles.Add(StartIdx + 0);
		MeshTriangles.Add(StartIdx + 2);
		MeshTriangles.Add(StartIdx + 1);

		MeshTriangles.Add(StartIdx + 0);
		MeshTriangles.Add(StartIdx + 3);
		MeshTriangles.Add(StartIdx + 2);

		MeshNormals.Add(Normal);
		MeshNormals.Add(Normal);
		MeshNormals.Add(Normal);
		MeshNormals.Add(Normal);

		MeshUV0.Add(FVector2D(0.0f, 1.0f));
		MeshUV0.Add(FVector2D(1.0f, 1.0f));
		MeshUV0.Add(FVector2D(1.0f, 0.0f));
		MeshUV0.Add(FVector2D(0.0f, 0.0f));

		const float Slice = static_cast<float>(TexSlice);
		const FVector2D SliceUV(Slice, 0.0f);
		MeshUV1.Add(SliceUV);
		MeshUV1.Add(SliceUV);
		MeshUV1.Add(SliceUV);
		MeshUV1.Add(SliceUV);

		const FLinearColor VC(Slice, 0.0f, 0.0f, 1.0f);
		MeshVertexColors.Add(VC);
		MeshVertexColors.Add(VC);
		MeshVertexColors.Add(VC);
		MeshVertexColors.Add(VC);

		const FVector TangentDir = (V1 - V0).GetSafeNormal();
		const FProcMeshTangent Tangent(TangentDir.IsNearlyZero() ? FVector(1.0f, 0.0f, 0.0f) : TangentDir, false);
		MeshTangents.Add(Tangent);
		MeshTangents.Add(Tangent);
		MeshTangents.Add(Tangent);
		MeshTangents.Add(Tangent);
	};

	for (const FBlockDestructionParticle& Part : Particles)
	{
		const FRotationMatrix RotMatrix(Part.Rotation);
		const FVector H = Part.HalfExtents;

		auto Xform = [&](const FVector& LocalPos) -> FVector
		{
			return Part.RelativePos + RotMatrix.TransformPosition(LocalPos);
		};

		// 1. Top (+Z)
		AddFace(Xform(FVector(-H.X, -H.Y, H.Z)), Xform(FVector(H.X, -H.Y, H.Z)), Xform(FVector(H.X, H.Y, H.Z)), Xform(FVector(-H.X, H.Y, H.Z)),
			RotMatrix.TransformVector(FVector(0.0f, 0.0f, 1.0f)), Part.TextureSlice);

		// 2. Bottom (-Z)
		AddFace(Xform(FVector(-H.X, H.Y, -H.Z)), Xform(FVector(H.X, H.Y, -H.Z)), Xform(FVector(H.X, -H.Y, -H.Z)), Xform(FVector(-H.X, -H.Y, -H.Z)),
			RotMatrix.TransformVector(FVector(0.0f, 0.0f, -1.0f)), Part.TextureSlice);

		// 3. North (+X)
		AddFace(Xform(FVector(H.X, -H.Y, -H.Z)), Xform(FVector(H.X, H.Y, -H.Z)), Xform(FVector(H.X, H.Y, H.Z)), Xform(FVector(H.X, -H.Y, H.Z)),
			RotMatrix.TransformVector(FVector(1.0f, 0.0f, 0.0f)), Part.TextureSlice);

		// 4. South (-X)
		AddFace(Xform(FVector(-H.X, H.Y, -H.Z)), Xform(FVector(-H.X, -H.Y, -H.Z)), Xform(FVector(-H.X, -H.Y, H.Z)), Xform(FVector(-H.X, H.Y, H.Z)),
			RotMatrix.TransformVector(FVector(-1.0f, 0.0f, 0.0f)), Part.TextureSlice);

		// 5. East (+Y)
		AddFace(Xform(FVector(H.X, H.Y, -H.Z)), Xform(FVector(-H.X, H.Y, -H.Z)), Xform(FVector(-H.X, H.Y, H.Z)), Xform(FVector(H.X, H.Y, H.Z)),
			RotMatrix.TransformVector(FVector(0.0f, 1.0f, 0.0f)), Part.TextureSlice);

		// 6. West (-Y)
		AddFace(Xform(FVector(-H.X, -H.Y, -H.Z)), Xform(FVector(H.X, -H.Y, -H.Z)), Xform(FVector(H.X, -H.Y, H.Z)), Xform(FVector(-H.X, -H.Y, H.Z)),
			RotMatrix.TransformVector(FVector(0.0f, -1.0f, 0.0f)), Part.TextureSlice);
	}

	TArray<FVector2D> EmptyUVs;
	ProceduralMesh->CreateMeshSection_LinearColor(
		0,
		MeshVertices,
		MeshTriangles,
		MeshNormals,
		MeshUV0,
		MeshUV1,
		EmptyUVs,
		EmptyUVs,
		MeshVertexColors,
		MeshTangents,
		false
	);

	if (CachedMaterial)
	{
		ProceduralMesh->SetMaterial(0, CachedMaterial);
	}
}

void AChaosBlockDestructionActor::UpdateMeshBuffers(float CurrentScale)
{
	int32 VertIdx = 0;

	for (const FBlockDestructionParticle& Part : Particles)
	{
		const FRotationMatrix RotMatrix(Part.Rotation);
		const FVector H = Part.HalfExtents * CurrentScale;

		auto Xform = [&](const FVector& LocalPos) -> FVector
		{
			return Part.RelativePos + RotMatrix.TransformPosition(LocalPos);
		};

		// Helper to update 4 vertices of a face
		auto UpdateFace = [&](const FVector& V0, const FVector& V1, const FVector& V2, const FVector& V3, const FVector& Normal)
		{
			MeshVertices[VertIdx + 0] = V0;
			MeshVertices[VertIdx + 1] = V1;
			MeshVertices[VertIdx + 2] = V2;
			MeshVertices[VertIdx + 3] = V3;

			MeshNormals[VertIdx + 0] = Normal;
			MeshNormals[VertIdx + 1] = Normal;
			MeshNormals[VertIdx + 2] = Normal;
			MeshNormals[VertIdx + 3] = Normal;

			const FVector TangentDir = (V1 - V0).GetSafeNormal();
			const FProcMeshTangent Tangent(TangentDir.IsNearlyZero() ? FVector(1.0f, 0.0f, 0.0f) : TangentDir, false);
			MeshTangents[VertIdx + 0] = Tangent;
			MeshTangents[VertIdx + 1] = Tangent;
			MeshTangents[VertIdx + 2] = Tangent;
			MeshTangents[VertIdx + 3] = Tangent;

			VertIdx += 4;
		};

		// 1. Top (+Z)
		UpdateFace(Xform(FVector(-H.X, -H.Y, H.Z)), Xform(FVector(H.X, -H.Y, H.Z)), Xform(FVector(H.X, H.Y, H.Z)), Xform(FVector(-H.X, H.Y, H.Z)),
			RotMatrix.TransformVector(FVector(0.0f, 0.0f, 1.0f)));

		// 2. Bottom (-Z)
		UpdateFace(Xform(FVector(-H.X, H.Y, -H.Z)), Xform(FVector(H.X, H.Y, -H.Z)), Xform(FVector(H.X, -H.Y, -H.Z)), Xform(FVector(-H.X, -H.Y, -H.Z)),
			RotMatrix.TransformVector(FVector(0.0f, 0.0f, -1.0f)));

		// 3. North (+X)
		UpdateFace(Xform(FVector(H.X, -H.Y, -H.Z)), Xform(FVector(H.X, H.Y, -H.Z)), Xform(FVector(H.X, H.Y, H.Z)), Xform(FVector(H.X, -H.Y, H.Z)),
			RotMatrix.TransformVector(FVector(1.0f, 0.0f, 0.0f)));

		// 4. South (-X)
		UpdateFace(Xform(FVector(-H.X, H.Y, -H.Z)), Xform(FVector(-H.X, -H.Y, -H.Z)), Xform(FVector(-H.X, -H.Y, H.Z)), Xform(FVector(-H.X, H.Y, H.Z)),
			RotMatrix.TransformVector(FVector(-1.0f, 0.0f, 0.0f)));

		// 5. East (+Y)
		UpdateFace(Xform(FVector(H.X, H.Y, -H.Z)), Xform(FVector(-H.X, H.Y, -H.Z)), Xform(FVector(-H.X, H.Y, H.Z)), Xform(FVector(H.X, H.Y, H.Z)),
			RotMatrix.TransformVector(FVector(0.0f, 1.0f, 0.0f)));

		// 6. West (-Y)
		UpdateFace(Xform(FVector(-H.X, -H.Y, -H.Z)), Xform(FVector(H.X, -H.Y, -H.Z)), Xform(FVector(H.X, -H.Y, H.Z)), Xform(FVector(-H.X, -H.Y, H.Z)),
			RotMatrix.TransformVector(FVector(0.0f, -1.0f, 0.0f)));
	}

	TArray<FVector2D> EmptyUVs;
	ProceduralMesh->UpdateMeshSection_LinearColor(
		0,
		MeshVertices,
		MeshNormals,
		MeshUV0,
		MeshUV1,
		EmptyUVs,
		EmptyUVs,
		MeshVertexColors,
		MeshTangents
	);
}
