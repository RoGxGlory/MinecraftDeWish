#include "BlockCrackActor.h"
#include "VoxelDataTypes.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlockCrack, Log, All);

ABlockCrackActor::ABlockCrackActor()
{
	PrimaryActorTick.bCanEverTick = false; // Event-driven; updated from AWorldGenerator.

	CrackMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CrackMesh"));
	RootComponent = CrackMesh;
	CrackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrackMesh->SetCastShadow(false);
	CrackMesh->bUseAsyncCooking = false;

	CrackMaterialTemplate = ConstructorHelpers::FObjectFinder<UMaterialInterface>(TEXT("/Game/Materials/M_BlockCrack.M_BlockCrack")).Object;

	// destroy_stage_0..9, vanilla-equivalent crack progression. Loaded once here so crack indexes
	// resolve at runtime without a hard reference from any other system.
	StageTextures.SetNum(10);
	StageTextures[0] = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("/Game/Patrix_Texture_Pack/textures/block/destroy_stage_0.destroy_stage_0")).Object;
	StageTextures[1] = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("/Game/Patrix_Texture_Pack/textures/block/destroy_stage_1.destroy_stage_1")).Object;
	StageTextures[2] = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("/Game/Patrix_Texture_Pack/textures/block/destroy_stage_2.destroy_stage_2")).Object;
	StageTextures[3] = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("/Game/Patrix_Texture_Pack/textures/block/destroy_stage_3.destroy_stage_3")).Object;
	StageTextures[4] = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("/Game/Patrix_Texture_Pack/textures/block/destroy_stage_4.destroy_stage_4")).Object;
	StageTextures[5] = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("/Game/Patrix_Texture_Pack/textures/block/destroy_stage_5.destroy_stage_5")).Object;
	StageTextures[6] = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("/Game/Patrix_Texture_Pack/textures/block/destroy_stage_6.destroy_stage_6")).Object;
	StageTextures[7] = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("/Game/Patrix_Texture_Pack/textures/block/destroy_stage_7.destroy_stage_7")).Object;
	StageTextures[8] = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("/Game/Patrix_Texture_Pack/textures/block/destroy_stage_8.destroy_stage_8")).Object;
	StageTextures[9] = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("/Game/Patrix_Texture_Pack/textures/block/destroy_stage_9.destroy_stage_9")).Object;
}

void ABlockCrackActor::InitializeCrackOverlay(const FIntVector& InVoxelCoord, uint8 InBlockID, float InBlockScale, const FVector& InWorldCenter)
{
	VoxelCoord = InVoxelCoord;
	BlockID = InBlockID;
	BlockScale = InBlockScale;
	SetActorLocation(InWorldCenter);

	if (BlockID == static_cast<uint8>(EBlockType::Torch))
	{
		BuildTorchCrackMesh();
	}
	else
	{
		BuildCubeCrackMesh();
	}

	CreateCrackMaterialInstance();

	// Stage -1 forces the first UpdateCrackStage call to apply stage 0 (faint cracks, like vanilla hit-1).
	CurrentStage = -1;
	UpdateCrackStage(0.0f);
}

void ABlockCrackActor::BeginPlay()
{
	Super::BeginPlay();
}

void ABlockCrackActor::CreateCrackMaterialInstance()
{
	if (!CrackMaterialTemplate)
	{
		UE_LOG(LogBlockCrack, Warning, TEXT("M_BlockCrack is missing. The crack overlay for voxel (%d, %d, %d) will not render until /Game/Materials/M_BlockCrack is created."),
			VoxelCoord.X, VoxelCoord.Y, VoxelCoord.Z);
		return;
	}

	CrackMaterial = UMaterialInstanceDynamic::Create(CrackMaterialTemplate, this);
	CrackMesh->SetMaterial(0, CrackMaterial);

	// Seed the parameter so the overlay renders stage 0 even before the first mining tick.
	if (StageTextures.IsValidIndex(0) && StageTextures[0])
	{
		CrackMaterial->SetTextureParameterValue(TEXT("CrackTex"), StageTextures[0]);
	}
}

bool ABlockCrackActor::UpdateCrackStage(float Progress)
{
	const int32 Stage = GetStageFromProgress(Progress);
	if (Stage == CurrentStage)
	{
		return false;
	}

	CurrentStage = Stage;
	if (CrackMaterial && StageTextures.IsValidIndex(Stage) && StageTextures[Stage])
	{
		CrackMaterial->SetTextureParameterValue(TEXT("CrackTex"), StageTextures[Stage]);
	}
	return true;
}

void ABlockCrackActor::DestroyCrackOverlay()
{
	Destroy();
}

void ABlockCrackActor::BuildCubeCrackMesh()
{
	CrackMesh->ClearAllMeshSections();

	const float Half = BlockScale * 0.5f;
	// Outward normal offset so the overlay wins the depth test against the chunk surface without z-fighting.
	const float Epsilon = 0.75f;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FProcMeshTangent> Tangents;

	auto AddFace = [&](const FVector& V0, const FVector& V1, const FVector& V2, const FVector& V3, const FVector& Normal)
	{
		const int32 StartIdx = Vertices.Num();
		Vertices.Add(V0);
		Vertices.Add(V1);
		Vertices.Add(V2);
		Vertices.Add(V3);

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

		const FProcMeshTangent Tangent(FVector(1.0f, 0.0f, 0.0f), false);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
	};

	const float E = Epsilon;
	// Top / Bottom (+Z / -Z)
	AddFace(FVector(-Half, -Half,  Half + E), FVector(Half, -Half,  Half + E), FVector(Half,  Half,  Half + E), FVector(-Half,  Half,  Half + E), FVector(0.0f, 0.0f,  1.0f));
	AddFace(FVector(-Half,  Half, -Half - E), FVector(Half,  Half, -Half - E), FVector(Half, -Half, -Half - E), FVector(-Half, -Half, -Half - E), FVector(0.0f, 0.0f, -1.0f));
	// North / South (+X / -X)
	AddFace(FVector( Half + E, -Half, -Half), FVector( Half + E,  Half, -Half), FVector( Half + E,  Half,  Half), FVector( Half + E, -Half,  Half), FVector(1.0f, 0.0f, 0.0f));
	AddFace(FVector(-Half - E,  Half, -Half), FVector(-Half - E, -Half, -Half), FVector(-Half - E, -Half,  Half), FVector(-Half - E,  Half,  Half), FVector(-1.0f, 0.0f, 0.0f));
	// East / West (+Y / -Y)
	AddFace(FVector( Half,  Half + E, -Half), FVector(-Half,  Half + E, -Half), FVector(-Half,  Half + E,  Half), FVector( Half,  Half + E,  Half), FVector(0.0f, 1.0f, 0.0f));
	AddFace(FVector(-Half, -Half - E, -Half), FVector( Half, -Half - E, -Half), FVector( Half, -Half - E,  Half), FVector(-Half, -Half - E,  Half), FVector(0.0f, -1.0f, 0.0f));

	CrackMesh->CreateMeshSection_LinearColor(
		0, Vertices, Triangles, Normals, UV0, TArray<FVector2D>(),
		TArray<FVector2D>(), TArray<FVector2D>(),
		TArray<FLinearColor>(), Tangents,
		false, // No collision: must never block line traces or physics.
		false  // No async cooking.
	);
}

void ABlockCrackActor::BuildTorchCrackMesh()
{
	CrackMesh->ClearAllMeshSections();

	const float S = BlockScale;
	// Crossed vertical diagonal quads, mirroring AChunkActor::AddTorchMesh, in local space
	// (actor sits at the voxel center, so z runs -S/2 .. +0.3S and x/y span +/-0.35S).
	const FVector Bottom1_A(-0.35f * S, -0.35f * S, -0.5f * S);
	const FVector Bottom1_B( 0.35f * S,  0.35f * S, -0.5f * S);
	const FVector Top1_A   (-0.35f * S, -0.35f * S,  0.3f * S);
	const FVector Top1_B   ( 0.35f * S,  0.35f * S,  0.3f * S);

	const FVector Bottom2_A(-0.35f * S,  0.35f * S, -0.5f * S);
	const FVector Bottom2_B( 0.35f * S, -0.35f * S, -0.5f * S);
	const FVector Top2_A   (-0.35f * S,  0.35f * S,  0.3f * S);
	const FVector Top2_B   ( 0.35f * S, -0.35f * S,  0.3f * S);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FProcMeshTangent> Tangents;

	// Each diagonal plane gets two copies offset along +/- its normal so the crack wins the depth
	// test from both viewing sides (a single coincident plane would z-fight the torch itself).
	const float Epsilon = 0.75f;

	auto AddDoubleSidedQuad = [&](const FVector& V0, const FVector& V1, const FVector& V2, const FVector& V3, const FVector& Normal)
	{
		const int32 StartIdx = Vertices.Num();
		Vertices.Add(V0);
		Vertices.Add(V1);
		Vertices.Add(V2);
		Vertices.Add(V3);

		// Front face CCW
		Triangles.Add(StartIdx + 0);
		Triangles.Add(StartIdx + 2);
		Triangles.Add(StartIdx + 1);
		Triangles.Add(StartIdx + 0);
		Triangles.Add(StartIdx + 3);
		Triangles.Add(StartIdx + 2);

		// Back face CW (visible from reverse)
		Triangles.Add(StartIdx + 0);
		Triangles.Add(StartIdx + 1);
		Triangles.Add(StartIdx + 2);
		Triangles.Add(StartIdx + 0);
		Triangles.Add(StartIdx + 2);
		Triangles.Add(StartIdx + 3);

		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);

		UV0.Add(FVector2D(0.0f, 1.0f));
		UV0.Add(FVector2D(1.0f, 1.0f));
		UV0.Add(FVector2D(1.0f, 0.0f));
		UV0.Add(FVector2D(0.0f, 0.0f));

		const FProcMeshTangent Tangent(FVector(1.0f, 0.0f, 0.0f), false);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
	};

	const FVector Normal1 = FVector(-1.0f, 1.0f, 0.0f).GetSafeNormal();
	const FVector Normal2 = FVector(1.0f, 1.0f, 0.0f).GetSafeNormal();
	const FVector Offset1 = Normal1 * Epsilon;
	const FVector Offset2 = Normal2 * Epsilon;

	AddDoubleSidedQuad(Bottom1_A + Offset1, Bottom1_B + Offset1, Top1_B + Offset1, Top1_A + Offset1, Normal1);
	AddDoubleSidedQuad(Bottom1_A - Offset1, Bottom1_B - Offset1, Top1_B - Offset1, Top1_A - Offset1, -Normal1);

	AddDoubleSidedQuad(Bottom2_A + Offset2, Bottom2_B + Offset2, Top2_B + Offset2, Top2_A + Offset2, Normal2);
	AddDoubleSidedQuad(Bottom2_A - Offset2, Bottom2_B - Offset2, Top2_B - Offset2, Top2_A - Offset2, -Normal2);

	CrackMesh->CreateMeshSection_LinearColor(
		0, Vertices, Triangles, Normals, UV0, TArray<FVector2D>(),
		TArray<FVector2D>(), TArray<FVector2D>(),
		TArray<FLinearColor>(), Tangents,
		false, // No collision: must never block line traces or physics.
		false  // No async cooking.
	);
}

int32 ABlockCrackActor::GetStageFromProgress(float Progress)
{
	const float Clamped = FMath::Clamp(Progress, 0.0f, 0.999999f);
	return FMath::Clamp(FMath::FloorToInt(Clamped * 10.0f), 0, 9);
}
