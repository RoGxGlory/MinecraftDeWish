#include "BlockItemPickup.h"
#include "WorldGenerator.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UObject/UObjectIterator.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"

ABlockItemPickup::ABlockItemPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->InitSphereRadius(45.0f);
	SphereComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SphereComponent->SetGenerateOverlapEvents(true);

	MiniBlockMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MiniBlockMesh"));
	MiniBlockMesh->SetupAttachment(RootComponent);
	MiniBlockMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MiniBlockMesh->SetCastShadow(true);
}

void ABlockItemPickup::BeginPlay()
{
	Super::BeginPlay();

	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ABlockItemPickup::OnOverlapBegin);

	if (MiniBlockMesh->GetNumSections() == 0)
	{
		InitializePickup(BlockID, ItemCount, nullptr);
	}
}

void ABlockItemPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RunningTime += DeltaTime;

	// Rotate around Z axis
	AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));

	// Gentle floating bob
	const float BobOffsetZ = FMath::Sin(RunningTime * BobFrequency) * BobHeight;
	MiniBlockMesh->SetRelativeLocation(MeshBaseOffset + FVector(0.0f, 0.0f, BobOffsetZ));
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

	const float H = 12.0f; // 24cm miniature cube

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

	// Top (+Z)
	AddQuad(FVector(-H, -H, H), FVector(H, -H, H), FVector(H, H, H), FVector(-H, H, H), FVector(0.0f, 0.0f, 1.0f), TopTex);
	// Bottom (-Z)
	AddQuad(FVector(-H, H, -H), FVector(H, H, -H), FVector(H, -H, -H), FVector(-H, -H, -H), FVector(0.0f, 0.0f, -1.0f), BottomTex);
	// North (+X)
	AddQuad(FVector(H, -H, -H), FVector(H, H, -H), FVector(H, H, H), FVector(H, -H, H), FVector(1.0f, 0.0f, 0.0f), FrontTex);
	// South (-X)
	AddQuad(FVector(-H, H, -H), FVector(-H, -H, -H), FVector(-H, -H, H), FVector(-H, H, H), FVector(-1.0f, 0.0f, 0.0f), SideTex);
	// East (+Y)
	AddQuad(FVector(H, H, -H), FVector(-H, H, -H), FVector(-H, H, H), FVector(H, H, H), FVector(0.0f, 1.0f, 0.0f), SideTex);
	// West (-Y)
	AddQuad(FVector(-H, -H, -H), FVector(H, -H, -H), FVector(H, -H, H), FVector(-H, -H, H), FVector(0.0f, -1.0f, 0.0f), SideTex);

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
	if (!OtherActor || OtherActor == this)
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
	UDataTable* BlockDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/Data/Block_DataTable.Block_DataTable"));
	if (!BlockDataTable)
	{
		return false;
	}

	const UScriptStruct* RowStruct = BlockDataTable->GetRowStruct();
	if (!RowStruct)
	{
		return false;
	}

	// Find matching row for BlockID
	const uint8* FoundRow = nullptr;
	for (auto It = BlockDataTable->GetRowMap().CreateConstIterator(); It; ++It)
	{
		const uint8* RowData = It.Value();
		if (!RowData) continue;

		int32 RowBlockID = 0;
		for (TFieldIterator<FIntProperty> PropIt(RowStruct); PropIt; ++PropIt)
		{
			if (PropIt->GetName().Contains(TEXT("BlockID"), ESearchCase::IgnoreCase))
			{
				RowBlockID = PropIt->GetPropertyValue_InContainer(RowData);
				break;
			}
		}

		if (RowBlockID == static_cast<int32>(BlockID))
		{
			FoundRow = RowData;
			break;
		}
	}

	if (!FoundRow)
	{
		return false;
	}

	// Search for WB_QuickSlots widget in the world
	UUserWidget* QuickSlotsWidget = nullptr;
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		if (It->GetWorld() == GetWorld() && It->GetClass()->GetName().Contains(TEXT("WB_QuickSlots")))
		{
			QuickSlotsWidget = *It;
			break;
		}
	}

	if (QuickSlotsWidget)
	{
		UFunction* ProcessFunc = QuickSlotsWidget->FindFunction(FName(TEXT("ProcessBlock")));
		if (ProcessFunc)
		{
			// Allocate buffer matching function parameter size
			uint8* ParamsBuffer = (uint8*)FMemory_Alloca(ProcessFunc->ParmsSize);
			FMemory::Memzero(ParamsBuffer, ProcessFunc->ParmsSize);

			// Copy the struct parameter into the parameter buffer
			for (TFieldIterator<FProperty> PropIt(ProcessFunc); PropIt; ++PropIt)
			{
				if (FStructProperty* StructProp = CastField<FStructProperty>(*PropIt))
				{
					StructProp->CopyCompleteValue_InContainer(ParamsBuffer, FoundRow);
					break;
				}
			}

			QuickSlotsWidget->ProcessEvent(ProcessFunc, ParamsBuffer);
			return true;
		}
	}

	return false;
}
