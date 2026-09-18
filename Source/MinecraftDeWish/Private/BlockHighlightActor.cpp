#include "BlockHighlightActor.h"

ABlockHighlightActor::ABlockHighlightActor()
{
	PrimaryActorTick.bCanEverTick = false;

	OutlineMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OutlineMesh"));
	RootComponent = OutlineMesh;
	OutlineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OutlineMesh->SetCastShadow(false);
	OutlineMesh->bUseAsyncCooking = false;
}

void ABlockHighlightActor::BeginPlay()
{
	Super::BeginPlay();
	BuildEdgeMesh();
	SetActorHiddenInGame(true);
}

void ABlockHighlightActor::SetTargetBlock(const FVector& BlockWorldCenter)
{
	SetActorLocation(BlockWorldCenter);
	SetActorHiddenInGame(false);
}

void ABlockHighlightActor::HideHighlight()
{
	SetActorHiddenInGame(true);
}

void ABlockHighlightActor::BuildEdgeMesh()
{
	OutlineMesh->ClearAllMeshSections();

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;

	const float H = BlockSize * 0.5f;
	const float T = EdgeThickness * 0.5f;

	auto AddBox = [&](const FVector& Min, const FVector& Max)
	{
		const int32 StartIdx = Vertices.Num();

		// 8 vertices of cuboid
		const FVector P0(Min.X, Min.Y, Min.Z);
		const FVector P1(Max.X, Min.Y, Min.Z);
		const FVector P2(Max.X, Max.Y, Min.Z);
		const FVector P3(Min.X, Max.Y, Min.Z);
		const FVector P4(Min.X, Min.Y, Max.Z);
		const FVector P5(Max.X, Min.Y, Max.Z);
		const FVector P6(Max.X, Max.Y, Max.Z);
		const FVector P7(Min.X, Max.Y, Max.Z);

		// Top (+Z)
		auto AddQuad = [&](const FVector& V0, const FVector& V1, const FVector& V2, const FVector& V3, const FVector& Norm)
		{
			const int32 Base = Vertices.Num();
			Vertices.Add(V0); Vertices.Add(V1); Vertices.Add(V2); Vertices.Add(V3);
			Triangles.Add(Base + 0); Triangles.Add(Base + 2); Triangles.Add(Base + 1);
			Triangles.Add(Base + 0); Triangles.Add(Base + 3); Triangles.Add(Base + 2);
			Normals.Add(Norm); Normals.Add(Norm); Normals.Add(Norm); Normals.Add(Norm);
			UV0.Add(FVector2D(0,0)); UV0.Add(FVector2D(1,0)); UV0.Add(FVector2D(1,1)); UV0.Add(FVector2D(0,1));
			VertexColors.Add(EdgeColor); VertexColors.Add(EdgeColor); VertexColors.Add(EdgeColor); VertexColors.Add(EdgeColor);
		};

		AddQuad(P4, P5, P6, P7, FVector(0,0,1));
		AddQuad(P3, P2, P1, P0, FVector(0,0,-1));
		AddQuad(P1, P2, P6, P5, FVector(1,0,0));
		AddQuad(P3, P0, P4, P7, FVector(-1,0,0));
		AddQuad(P2, P3, P7, P6, FVector(0,1,0));
		AddQuad(P0, P1, P5, P4, FVector(0,-1,0));
	};

	// 4 Horizontal edges on top (+Z)
	AddBox(FVector(-H - T, -H - T, H - T), FVector(H + T, -H + T, H + T)); // -Y
	AddBox(FVector(-H - T, H - T, H - T),  FVector(H + T, H + T, H + T));  // +Y
	AddBox(FVector(-H - T, -H - T, H - T), FVector(-H + T, H + T, H + T)); // -X
	AddBox(FVector(H - T, -H - T, H - T),  FVector(H + T, H + T, H + T));  // +X

	// 4 Horizontal edges on bottom (-Z)
	AddBox(FVector(-H - T, -H - T, -H - T), FVector(H + T, -H + T, -H + T)); // -Y
	AddBox(FVector(-H - T, H - T, -H - T),  FVector(H + T, H + T, -H + T));  // +Y
	AddBox(FVector(-H - T, -H - T, -H - T), FVector(-H + T, H + T, -H + T)); // -X
	AddBox(FVector(H - T, -H - T, -H - T),  FVector(H + T, H + T, -H + T));  // +X

	// 4 Vertical corner pillars
	AddBox(FVector(-H - T, -H - T, -H - T), FVector(-H + T, -H + T, H + T)); // -X, -Y
	AddBox(FVector(H - T, -H - T, -H - T),  FVector(H + T, -H + T, H + T));  // +X, -Y
	AddBox(FVector(-H - T, H - T, -H - T),  FVector(-H + T, H + T, H + T));  // -X, +Y
	AddBox(FVector(H - T, H - T, -H - T),   FVector(H + T, H + T, H + T));   // +X, +Y

	OutlineMesh->CreateMeshSection_LinearColor(
		0,
		Vertices,
		Triangles,
		Normals,
		UV0,
		TArray<FVector2D>(),
		TArray<FVector2D>(),
		TArray<FVector2D>(),
		VertexColors,
		TArray<FProcMeshTangent>(),
		false
	);
}
