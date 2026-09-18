#include "ChunkActor.h"
#include "WorldGenerator.h"
#include "MiningQueueSystem.h"
#include "BaublesSystem.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"

AChunkActor::AChunkActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Main mesh for solid blocks — full physics collision
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = ProceduralMesh;
	ProceduralMesh->bUseAsyncCooking = false;
	ProceduralMesh->bUseComplexAsSimpleCollision = true;
	ProceduralMesh->SetCastShadow(true);
	ProceduralMesh->bAffectDistanceFieldLighting = false;
	ProceduralMesh->SetCollisionObjectType(ECC_WorldStatic);
	ProceduralMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// Separate mesh for non-solid blocks (torches, seeds) — trace-only collision
	NonSolidMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("NonSolidMesh"));
	NonSolidMesh->SetupAttachment(RootComponent);
	NonSolidMesh->bUseAsyncCooking = false;
	NonSolidMesh->bUseComplexAsSimpleCollision = true;
	NonSolidMesh->SetCastShadow(true);
	NonSolidMesh->bAffectDistanceFieldLighting = false;
	NonSolidMesh->SetCollisionObjectType(ECC_WorldStatic);
	NonSolidMesh->SetCollisionResponseToAllChannels(ECR_Block);    // Line traces (ECC_WorldStatic) hit this
	NonSolidMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);        // Players walk through
	NonSolidMesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap); // Physics objects pass through
	NonSolidMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // No physics simulation, trace-only

	// Hard asset reference for cooker & standalone builds
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(TEXT("/Game/Materials/M_Global.M_Global"));
	if (MatFinder.Succeeded())
	{
		DefaultTerrainMaterial = MatFinder.Object;
	}
}

void AChunkActor::BeginPlay()
{
	Super::BeginPlay();
}

void AChunkActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTorchLights();
	Super::EndPlay(EndPlayReason);
}

void AChunkActor::InitializeChunk(const FChunkCoord& InCoord, int32 InHeight, float InScale, AWorldGenerator* InWorldGen)
{
	ChunkCoord = InCoord;
	ChunkHeight = InHeight;
	BlockScale = InScale;
	WorldGenerator = InWorldGen;

	BlockData.SetNumZeroed(CHUNK_SIZE_X * CHUNK_SIZE_Y * ChunkHeight);

	SetActorLocation(FVector(
		static_cast<float>(ChunkCoord.X * CHUNK_SIZE_X) * BlockScale,
		static_cast<float>(ChunkCoord.Y * CHUNK_SIZE_Y) * BlockScale,
		0.0f
	));
}

uint8 AChunkActor::GetBlock(int32 X, int32 Y, int32 Z) const
{
	if (!IsValidCoord(X, Y, Z))
	{
		return 0;
	}
	return BlockData[GetBlockIndex(X, Y, Z)];
}

void AChunkActor::SetBlock(int32 X, int32 Y, int32 Z, uint8 BlockID)
{
	if (IsValidCoord(X, Y, Z))
	{
		BlockData[GetBlockIndex(X, Y, Z)] = BlockID;
		bIsModified = true;
	}
}

bool AChunkActor::ShouldRenderFace(uint8 CurrentBlockID, int32 NeighborX, int32 NeighborY, int32 NeighborZ) const
{
	// Vertical boundary checks
	if (NeighborZ < 0)
	{
		return true; // Bedrock bottom floor face seals the world from below
	}
	if (NeighborZ >= ChunkHeight)
	{
		return true; // Sky is air, render top face
	}

	// Inside this chunk
	if (NeighborX >= 0 && NeighborX < CHUNK_SIZE_X &&
	    NeighborY >= 0 && NeighborY < CHUNK_SIZE_Y)
	{
		uint8 NeighborBlock = GetBlock(NeighborX, NeighborY, NeighborZ);
		if (NeighborBlock == 0)
		{
			return true; // Air
		}

		// Cull interior face between touching glass blocks (Minecraft-style)
		if (CurrentBlockID == static_cast<uint8>(EBlockType::Glass) && NeighborBlock == static_cast<uint8>(EBlockType::Glass))
		{
			return false;
		}

		if (WorldGenerator.IsValid())
		{
			return WorldGenerator->IsBlockTransparent(NeighborBlock);
		}
		return false;
	}

	// Cross-chunk boundary query
	if (WorldGenerator.IsValid())
	{
		int32 WorldVoxelX = (ChunkCoord.X * CHUNK_SIZE_X) + NeighborX;
		int32 WorldVoxelY = (ChunkCoord.Y * CHUNK_SIZE_Y) + NeighborY;
		uint8 NeighborBlock = 0;
		if (WorldGenerator->GetVoxelAt(WorldVoxelX, WorldVoxelY, NeighborZ, NeighborBlock))
		{
			if (NeighborBlock == 0)
			{
				return true;
			}

			// Cull interior face between touching glass blocks across chunks
			if (CurrentBlockID == static_cast<uint8>(EBlockType::Glass) && NeighborBlock == static_cast<uint8>(EBlockType::Glass))
			{
				return false;
			}

			return WorldGenerator->IsBlockTransparent(NeighborBlock);
		}

		// Neighbor chunk not yet loaded: render edge faces so cross-section profile is solid!
		return true;
	}

	return true;
}

void AChunkActor::AddFace(
	FChunkMeshData& MeshData,
	const FVector& BlockPos,
	EBlockFace Face,
	int32 TextureIndex
) const
{
	const int32 StartIndex = MeshData.Vertices.Num();
	const float S = BlockScale;

	FVector V0, V1, V2, V3;
	FVector Normal;

	switch (Face)
	{
	case EBlockFace::Top: // +Z
		Normal = FVector(0.0f, 0.0f, 1.0f);
		V0 = BlockPos + FVector(0.0f, 0.0f, S);
		V1 = BlockPos + FVector(S, 0.0f, S);
		V2 = BlockPos + FVector(S, S, S);
		V3 = BlockPos + FVector(0.0f, S, S);
		break;

	case EBlockFace::Bottom: // -Z
		Normal = FVector(0.0f, 0.0f, -1.0f);
		V0 = BlockPos + FVector(0.0f, S, 0.0f);
		V1 = BlockPos + FVector(S, S, 0.0f);
		V2 = BlockPos + FVector(S, 0.0f, 0.0f);
		V3 = BlockPos + FVector(0.0f, 0.0f, 0.0f);
		break;

	case EBlockFace::North: // +X
		Normal = FVector(1.0f, 0.0f, 0.0f);
		V0 = BlockPos + FVector(S, 0.0f, 0.0f);
		V1 = BlockPos + FVector(S, S, 0.0f);
		V2 = BlockPos + FVector(S, S, S);
		V3 = BlockPos + FVector(S, 0.0f, S);
		break;

	case EBlockFace::South: // -X
		Normal = FVector(-1.0f, 0.0f, 0.0f);
		V0 = BlockPos + FVector(0.0f, S, 0.0f);
		V1 = BlockPos + FVector(0.0f, 0.0f, 0.0f);
		V2 = BlockPos + FVector(0.0f, 0.0f, S);
		V3 = BlockPos + FVector(0.0f, S, S);
		break;

	case EBlockFace::East: // +Y
		Normal = FVector(0.0f, 1.0f, 0.0f);
		V0 = BlockPos + FVector(S, S, 0.0f);
		V1 = BlockPos + FVector(0.0f, S, 0.0f);
		V2 = BlockPos + FVector(0.0f, S, S);
		V3 = BlockPos + FVector(S, S, S);
		break;

	case EBlockFace::West: // -Y
		Normal = FVector(0.0f, -1.0f, 0.0f);
		V0 = BlockPos + FVector(0.0f, 0.0f, 0.0f);
		V1 = BlockPos + FVector(S, 0.0f, 0.0f);
		V2 = BlockPos + FVector(S, 0.0f, S);
		V3 = BlockPos + FVector(0.0f, 0.0f, S);
		break;
	}

	MeshData.Vertices.Add(V0);
	MeshData.Vertices.Add(V1);
	MeshData.Vertices.Add(V2);
	MeshData.Vertices.Add(V3);

	// Triangles (two CCW triangles forming a quad facing outwards)
	// UE5 ProceduralMeshComponent uses CCW winding for front-facing triangles
	MeshData.Triangles.Add(StartIndex + 0);
	MeshData.Triangles.Add(StartIndex + 2);
	MeshData.Triangles.Add(StartIndex + 1);

	MeshData.Triangles.Add(StartIndex + 0);
	MeshData.Triangles.Add(StartIndex + 3);
	MeshData.Triangles.Add(StartIndex + 2);

	// Normals
	MeshData.Normals.Add(Normal);
	MeshData.Normals.Add(Normal);
	MeshData.Normals.Add(Normal);
	MeshData.Normals.Add(Normal);

	// Determine if this texture is isotropic (seamlessly rotatable by 90-degree steps without directional misalignment)
	const bool bIsIsotropic = (
		TextureIndex == 27 || // grass_block_top
		TextureIndex == 14 || // dirt
		TextureIndex == 6  || // cobblestone
		TextureIndex == 35 || // stone
		TextureIndex == 34 || // sand
		TextureIndex == 4  || // coal_block
		TextureIndex == 5  || // coal_ore
		TextureIndex == 28 || // iron_block
		TextureIndex == 29 || // iron_ore
		TextureIndex == 24 || // gold_block
		TextureIndex == 25 || // gold_ore
		TextureIndex == 12 || // diamond_block
		TextureIndex == 13 || // diamond_ore
		TextureIndex == 15 || // emerald_block
		TextureIndex == 16 || // emerald_ore
		TextureIndex == 17 || // farmland
		TextureIndex == 18 || // farmland_moist
		TextureIndex == 30    // oak_leaves
	);

	int32 Rot = 0;
	if (bIsIsotropic && (Face == EBlockFace::Top || Face == EBlockFace::Bottom))
	{
		const int32 LocalX = FMath::RoundToInt(BlockPos.X / S);
		const int32 LocalY = FMath::RoundToInt(BlockPos.Y / S);
		const int32 LocalZ = FMath::RoundToInt(BlockPos.Z / S);
		const int32 WorldX = ChunkCoord.X * CHUNK_SIZE_X + LocalX;
		const int32 WorldY = ChunkCoord.Y * CHUNK_SIZE_Y + LocalY;
		const int32 WorldZ = LocalZ;

		// Deterministic 2-bit rotation from spatial integer coordinates
		const uint32 PosHash = (static_cast<uint32>(WorldX) * 73856093u) ^
		                       (static_cast<uint32>(WorldY) * 19349663u) ^
		                       (static_cast<uint32>(WorldZ) * 83492791u);
		Rot = PosHash & 3;
	}

	// Tangents aligned with rotated U direction for accurate normal mapping
	static const FVector TopTangents[4] = {
		FVector(1.0f, 0.0f, 0.0f),
		FVector(0.0f, 1.0f, 0.0f),
		FVector(-1.0f, 0.0f, 0.0f),
		FVector(0.0f, -1.0f, 0.0f)
	};

	static const FVector BottomTangents[4] = {
		FVector(1.0f, 0.0f, 0.0f),
		FVector(0.0f, -1.0f, 0.0f),
		FVector(-1.0f, 0.0f, 0.0f),
		FVector(0.0f, 1.0f, 0.0f)
	};

	FVector TangentDir;
	if (Face == EBlockFace::Top)
	{
		TangentDir = TopTangents[Rot];
	}
	else if (Face == EBlockFace::Bottom)
	{
		TangentDir = BottomTangents[Rot];
	}
	else
	{
		TangentDir = FVector::CrossProduct(Normal, FVector::UpVector);
	}

	const FProcMeshTangent Tangent(TangentDir.GetSafeNormal(), false);
	MeshData.Tangents.Add(Tangent);
	MeshData.Tangents.Add(Tangent);
	MeshData.Tangents.Add(Tangent);
	MeshData.Tangents.Add(Tangent);

	// UV0 mapping:
	// For Top/Bottom faces of isotropic blocks, rotate UVs by 0, 90, 180, or 270 degrees to break tiling patterns.
	// For vertical side faces: V0/V1 are bottom (Z=0, UV V=1.0), V2/V3 are top (Z=S, UV V=0.0).
	// This ensures upright textures so the grass fringe is at the TOP of the block, connecting to grass top.
	static const FVector2D RotatedUVs[4][4] = {
		{ FVector2D(0.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(1.0f, 1.0f), FVector2D(0.0f, 1.0f) }, // Rot 0
		{ FVector2D(0.0f, 1.0f), FVector2D(0.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(1.0f, 1.0f) }, // Rot 1 (90 deg CW)
		{ FVector2D(1.0f, 1.0f), FVector2D(0.0f, 1.0f), FVector2D(0.0f, 0.0f), FVector2D(1.0f, 0.0f) }, // Rot 2 (180 deg CW)
		{ FVector2D(1.0f, 0.0f), FVector2D(1.0f, 1.0f), FVector2D(0.0f, 1.0f), FVector2D(0.0f, 0.0f) }  // Rot 3 (270 deg CW)
	};

	if (Face == EBlockFace::Top || Face == EBlockFace::Bottom)
	{
		MeshData.UV0.Add(RotatedUVs[Rot][0]);
		MeshData.UV0.Add(RotatedUVs[Rot][1]);
		MeshData.UV0.Add(RotatedUVs[Rot][2]);
		MeshData.UV0.Add(RotatedUVs[Rot][3]);
	}
	else
	{
		MeshData.UV0.Add(FVector2D(0.0f, 1.0f));
		MeshData.UV0.Add(FVector2D(1.0f, 1.0f));
		MeshData.UV0.Add(FVector2D(1.0f, 0.0f));
		MeshData.UV0.Add(FVector2D(0.0f, 0.0f));
	}

	// UV1.X stores the Texture2DArray slice index
	const float TexSlice = static_cast<float>(TextureIndex);
	MeshData.UV1.Add(FVector2D(TexSlice, 0.0f));
	MeshData.UV1.Add(FVector2D(TexSlice, 0.0f));
	MeshData.UV1.Add(FVector2D(TexSlice, 0.0f));
	MeshData.UV1.Add(FVector2D(TexSlice, 0.0f));

	// Vertex colors also store the texture index in R for compatibility
	FLinearColor VColor(TexSlice, 0.0f, 0.0f, 1.0f);
	MeshData.VertexColors.Add(VColor);
	MeshData.VertexColors.Add(VColor);
	MeshData.VertexColors.Add(VColor);
	MeshData.VertexColors.Add(VColor);
}

void AChunkActor::GenerateMeshData(FChunkMeshData& OutSolidMeshData, FChunkMeshData& OutNonSolidMeshData) const
{
	OutSolidMeshData.Reset();
	OutNonSolidMeshData.Reset();

	for (int32 Z = 0; Z < ChunkHeight; ++Z)
	{
		for (int32 Y = 0; Y < CHUNK_SIZE_Y; ++Y)
		{
			for (int32 X = 0; X < CHUNK_SIZE_X; ++X)
			{
				const uint8 BlockID = BlockData[GetBlockIndex(X, Y, Z)];
				if (BlockID == 0) // Air
				{
					continue;
				}

				const FVector BlockPos(
					static_cast<float>(X) * BlockScale,
					static_cast<float>(Y) * BlockScale,
					static_cast<float>(Z) * BlockScale
				);

				// Determine which mesh buffer to write to based on block solidity
				const bool bIsNonSolid = FBlockHelpers::IsNonSolidBlock(BlockID);
				FChunkMeshData& TargetMeshData = bIsNonSolid ? OutNonSolidMeshData : OutSolidMeshData;

				// Special mesh for Torches: crossed vertical diagonal planes
				if (BlockID == static_cast<uint8>(EBlockType::Torch))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::North) : 36;
					AddTorchMesh(TargetMeshData, BlockPos, Tex);
					continue;
				}

				// Standard Cube Faces:
				// Top (+Z)
				if (ShouldRenderFace(BlockID, X, Y, Z + 1))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::Top) : 0;
					AddFace(TargetMeshData, BlockPos, EBlockFace::Top, Tex);
				}

				// Bottom (-Z)
				if (ShouldRenderFace(BlockID, X, Y, Z - 1))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::Bottom) : 0;
					AddFace(TargetMeshData, BlockPos, EBlockFace::Bottom, Tex);
				}

				// North (+X)
				if (ShouldRenderFace(BlockID, X + 1, Y, Z))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::North) : 0;
					AddFace(TargetMeshData, BlockPos, EBlockFace::North, Tex);
				}

				// South (-X)
				if (ShouldRenderFace(BlockID, X - 1, Y, Z))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::South) : 0;
					AddFace(TargetMeshData, BlockPos, EBlockFace::South, Tex);
				}

				// East (+Y)
				if (ShouldRenderFace(BlockID, X, Y + 1, Z))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::East) : 0;
					AddFace(TargetMeshData, BlockPos, EBlockFace::East, Tex);
				}

				// West (-Y)
				if (ShouldRenderFace(BlockID, X, Y - 1, Z))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::West) : 0;
					AddFace(TargetMeshData, BlockPos, EBlockFace::West, Tex);
				}
			}
		}
	}
}

void AChunkActor::AddTorchMesh(
	FChunkMeshData& MeshData,
	const FVector& BlockPos,
	int32 TextureIndex
) const
{
	const float S = BlockScale;
	// Crossed vertical diagonal quads in the center of the voxel (Minecraft style)
	const FVector Bottom1_A = BlockPos + FVector(S * 0.15f, S * 0.15f, 0.0f);
	const FVector Bottom1_B = BlockPos + FVector(S * 0.85f, S * 0.85f, 0.0f);
	const FVector Top1_A    = BlockPos + FVector(S * 0.15f, S * 0.15f, S * 0.80f);
	const FVector Top1_B    = BlockPos + FVector(S * 0.85f, S * 0.85f, S * 0.80f);

	const FVector Bottom2_A = BlockPos + FVector(S * 0.15f, S * 0.85f, 0.0f);
	const FVector Bottom2_B = BlockPos + FVector(S * 0.85f, S * 0.15f, 0.0f);
	const FVector Top2_A    = BlockPos + FVector(S * 0.15f, S * 0.85f, S * 0.80f);
	const FVector Top2_B    = BlockPos + FVector(S * 0.85f, S * 0.15f, S * 0.80f);

	auto AddDoubleSidedQuad = [&](const FVector& V0, const FVector& V1, const FVector& V2, const FVector& V3, const FVector& Normal)
	{
		const int32 StartIdx = MeshData.Vertices.Num();
		MeshData.Vertices.Add(V0);
		MeshData.Vertices.Add(V1);
		MeshData.Vertices.Add(V2);
		MeshData.Vertices.Add(V3);

		// Front face CCW
		MeshData.Triangles.Add(StartIdx + 0);
		MeshData.Triangles.Add(StartIdx + 2);
		MeshData.Triangles.Add(StartIdx + 1);
		MeshData.Triangles.Add(StartIdx + 0);
		MeshData.Triangles.Add(StartIdx + 3);
		MeshData.Triangles.Add(StartIdx + 2);

		// Back face CW (visible from reverse)
		MeshData.Triangles.Add(StartIdx + 0);
		MeshData.Triangles.Add(StartIdx + 1);
		MeshData.Triangles.Add(StartIdx + 2);
		MeshData.Triangles.Add(StartIdx + 0);
		MeshData.Triangles.Add(StartIdx + 2);
		MeshData.Triangles.Add(StartIdx + 3);

		MeshData.Normals.Add(Normal);
		MeshData.Normals.Add(Normal);
		MeshData.Normals.Add(Normal);
		MeshData.Normals.Add(Normal);

		MeshData.UV0.Add(FVector2D(0.0f, 1.0f));
		MeshData.UV0.Add(FVector2D(1.0f, 1.0f));
		MeshData.UV0.Add(FVector2D(1.0f, 0.0f));
		MeshData.UV0.Add(FVector2D(0.0f, 0.0f));

		const float TexSlice = static_cast<float>(TextureIndex);
		MeshData.UV1.Add(FVector2D(TexSlice, 0.0f));
		MeshData.UV1.Add(FVector2D(TexSlice, 0.0f));
		MeshData.UV1.Add(FVector2D(TexSlice, 0.0f));
		MeshData.UV1.Add(FVector2D(TexSlice, 0.0f));

		const FLinearColor VColor(TexSlice, 0.0f, 0.0f, 1.0f);
		MeshData.VertexColors.Add(VColor);
		MeshData.VertexColors.Add(VColor);
		MeshData.VertexColors.Add(VColor);
		MeshData.VertexColors.Add(VColor);

		const FProcMeshTangent Tangent(FVector(1.0f, 0.0f, 0.0f), false);
		MeshData.Tangents.Add(Tangent);
		MeshData.Tangents.Add(Tangent);
		MeshData.Tangents.Add(Tangent);
		MeshData.Tangents.Add(Tangent);
	};

	const FVector Normal1 = FVector(-1.0f, 1.0f, 0.0f).GetSafeNormal();
	const FVector Normal2 = FVector(1.0f, 1.0f, 0.0f).GetSafeNormal();

	AddDoubleSidedQuad(Bottom1_A, Bottom1_B, Top1_B, Top1_A, Normal1);
	AddDoubleSidedQuad(Bottom2_A, Bottom2_B, Top2_B, Top2_A, Normal2);
}

void AChunkActor::UpdateTorchLights()
{
	TArray<FVector> RequiredTorchLocations;

	for (int32 Z = 0; Z < ChunkHeight; ++Z)
	{
		for (int32 Y = 0; Y < CHUNK_SIZE_Y; ++Y)
		{
			for (int32 X = 0; X < CHUNK_SIZE_X; ++X)
			{
				if (BlockData[GetBlockIndex(X, Y, Z)] == static_cast<uint8>(EBlockType::Torch))
				{
					const FVector RelPos(
						(static_cast<float>(X) + 0.5f) * BlockScale,
						(static_cast<float>(Y) + 0.5f) * BlockScale,
						(static_cast<float>(Z) + 0.65f) * BlockScale
					);
					RequiredTorchLocations.Add(RelPos);
				}
			}
		}
	}

	while (TorchLights.Num() < RequiredTorchLocations.Num())
	{
		UPointLightComponent* NewLight = NewObject<UPointLightComponent>(this);
		NewLight->SetMobility(EComponentMobility::Movable);
		NewLight->SetupAttachment(RootComponent);
		NewLight->SetIntensity(2500.0f);
		NewLight->SetAttenuationRadius(900.0f);
		NewLight->SetLightColor(FLinearColor(1.0f, 0.78f, 0.50f));
		NewLight->SetCastShadows(false);
		NewLight->RegisterComponentWithWorld(GetWorld());
		TorchLights.Add(NewLight);
	}

	while (TorchLights.Num() > RequiredTorchLocations.Num())
	{
		UPointLightComponent* ExcessLight = TorchLights.Pop();
		if (ExcessLight)
		{
			ExcessLight->DestroyComponent();
		}
	}

	for (int32 i = 0; i < RequiredTorchLocations.Num(); ++i)
	{
		if (TorchLights[i])
		{
			TorchLights[i]->SetRelativeLocation(RequiredTorchLocations[i]);
			TorchLights[i]->SetVisibility(true);
		}
	}
}

void AChunkActor::ClearTorchLights()
{
	for (UPointLightComponent* Light : TorchLights)
	{
		if (Light)
		{
			Light->DestroyComponent();
		}
	}
	TorchLights.Empty();
}

void AChunkActor::ApplyMeshData(const FChunkMeshData& InSolidMeshData, const FChunkMeshData& InNonSolidMeshData)
{
	// Clear mesh sections on both components
	ProceduralMesh->ClearMeshSection(0);
	NonSolidMesh->ClearMeshSection(0);

	// Section 0 on ProceduralMesh: Solid blocks — full physics collision (QueryAndPhysics)
	if (!InSolidMeshData.IsEmpty())
	{
		ProceduralMesh->CreateMeshSection_LinearColor(
			0,
			InSolidMeshData.Vertices,
			InSolidMeshData.Triangles,
			InSolidMeshData.Normals,
			InSolidMeshData.UV0,
			InSolidMeshData.UV1,
			TArray<FVector2D>(),
			TArray<FVector2D>(),
			InSolidMeshData.VertexColors,
			InSolidMeshData.Tangents,
			true // Generate collision for solid blocks
		);

		UMaterialInterface* SolidMat = (WorldGenerator.IsValid() && WorldGenerator->TerrainMaterial)
			? WorldGenerator->TerrainMaterial
			: DefaultTerrainMaterial;

		if (SolidMat)
		{
			ProceduralMesh->SetMaterial(0, SolidMat);
		}
	}

	// Section 0 on NonSolidMesh: Non-solid blocks (torches, seeds) — QueryOnly collision
	// Line traces still hit these for WB_BlockInfo and destroy system, but players walk through them.
	if (!InNonSolidMeshData.IsEmpty())
	{
		NonSolidMesh->CreateMeshSection_LinearColor(
			0,
			InNonSolidMeshData.Vertices,
			InNonSolidMeshData.Triangles,
			InNonSolidMeshData.Normals,
			InNonSolidMeshData.UV0,
			InNonSolidMeshData.UV1,
			TArray<FVector2D>(),
			TArray<FVector2D>(),
			InNonSolidMeshData.VertexColors,
			InNonSolidMeshData.Tangents,
			true // Generate collision for trace queries
		);

		UMaterialInterface* NonSolidMat = (WorldGenerator.IsValid() && WorldGenerator->TerrainMaterial)
			? WorldGenerator->TerrainMaterial
			: DefaultTerrainMaterial;

		if (NonSolidMat)
		{
			NonSolidMesh->SetMaterial(0, NonSolidMat);
		}
	}

	UpdateTorchLights();
}

void AChunkActor::UpdateMesh()
{
	FChunkMeshData SolidMeshData;
	FChunkMeshData NonSolidMeshData;
	GenerateMeshData(SolidMeshData, NonSolidMeshData);
	ApplyMeshData(SolidMeshData, NonSolidMeshData);
}

void AChunkActor::SetMaterial(UMaterialInterface* Material)
{
	if (ProceduralMesh && Material)
	{
		ProceduralMesh->SetMaterial(0, Material);
	}
}

int32 AChunkActor::GetHitFaceAtLocation(const FVector& HitLocation) const
{
	// Project Face ID Convention (from AC_DestroySystem / BPI_BuildSystem):
	// Option 0 = Front (+X), Option 1 = Back (-X), Option 2 = Left (-Y), Option 3 = Right (+Y), Option 4 = Top (+Z), Option 5 = Bottom (-Z)

	// Determine incoming ray direction from player camera
	FVector RayDir = FVector::ZeroVector;
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PC && PC->PlayerCameraManager)
	{
		RayDir = (HitLocation - PC->PlayerCameraManager->GetCameraLocation()).GetSafeNormal();
	}

	// 6 face outward normals in world space
	static const FVector FaceNormals[6] = {
		FVector(1.0f, 0.0f, 0.0f),   // 0 = Front (+X)
		FVector(-1.0f, 0.0f, 0.0f),  // 1 = Back (-X)
		FVector(0.0f, -1.0f, 0.0f),  // 2 = Left (-Y)
		FVector(0.0f, 1.0f, 0.0f),   // 3 = Right (+Y)
		FVector(0.0f, 0.0f, 1.0f),   // 4 = Top (+Z)
		FVector(0.0f, 0.0f, -1.0f)   // 5 = Bottom (-Z)
	};

	if (!RayDir.IsNearlyZero())
	{
		// The face that was hit is the one whose outward normal directly opposes the incoming ray (-RayDir)
		const FVector InvRay = -RayDir;
		int32 BestFace = 4;
		float MaxDot = -2.0f;
		for (int32 i = 0; i < 6; ++i)
		{
			const float Dot = FVector::DotProduct(FaceNormals[i], InvRay);
			if (Dot > MaxDot)
			{
				MaxDot = Dot;
				BestFace = i;
			}
		}
		return BestFace;
	}

	// Geometric fallback if camera is unavailable
	const FVector LocalHit = HitLocation - GetActorLocation();
	const int32 X = FMath::Clamp(FMath::FloorToInt(LocalHit.X / BlockScale), 0, CHUNK_SIZE_X - 1);
	const int32 Y = FMath::Clamp(FMath::FloorToInt(LocalHit.Y / BlockScale), 0, CHUNK_SIZE_Y - 1);
	const int32 Z = FMath::Clamp(FMath::FloorToInt(LocalHit.Z / BlockScale), 0, ChunkHeight - 1);

	const FVector BlockCenter(
		(static_cast<float>(X) + 0.5f) * BlockScale,
		(static_cast<float>(Y) + 0.5f) * BlockScale,
		(static_cast<float>(Z) + 0.5f) * BlockScale
	);

	const FVector Delta = LocalHit - BlockCenter;
	const float AbsX = FMath::Abs(Delta.X);
	const float AbsY = FMath::Abs(Delta.Y);
	const float AbsZ = FMath::Abs(Delta.Z);

	if (AbsZ >= AbsX && AbsZ >= AbsY)
	{
		return (Delta.Z >= 0.0f) ? 4 : 5; // Top (4) : Bottom (5)
	}
	else if (AbsX >= AbsY)
	{
		return (Delta.X >= 0.0f) ? 0 : 1; // Front (+X = 0) : Back (-X = 1)
	}
	else
	{
		return (Delta.Y <= 0.0f) ? 2 : 3; // Left (-Y = 2) : Right (+Y = 3)
	}
}

bool AChunkActor::BreakBlockAtLocation(const FVector& HitLocation, uint8& OutBrokenBlockID)
{
	if (WorldGenerator.IsValid())
	{
		return WorldGenerator->BreakBlock(HitLocation, OutBrokenBlockID);
	}
	return false;
}

bool AChunkActor::GetBlockDataAtLocation(const FVector& HitLocation, uint8& OutBlockID, FVector& OutBlockCenter) const
{
	const FVector LocalHit = HitLocation - GetActorLocation();
	const int32 X = FMath::FloorToInt(LocalHit.X / BlockScale);
	const int32 Y = FMath::FloorToInt(LocalHit.Y / BlockScale);
	const int32 Z = FMath::FloorToInt(LocalHit.Z / BlockScale);

	if (!IsValidCoord(X, Y, Z))
	{
		OutBlockID = 0;
		return false;
	}

	OutBlockID = GetBlock(X, Y, Z);
	OutBlockCenter = GetActorLocation() + FVector(
		(static_cast<float>(X) + 0.5f) * BlockScale,
		(static_cast<float>(Y) + 0.5f) * BlockScale,
		(static_cast<float>(Z) + 0.5f) * BlockScale
	);
	return OutBlockID != 0;
}

uint8 AChunkActor::GetTargetedBlockID() const
{
	FIntVector TargetCoord = LastTargetedVoxelCoord;

	uint8 BlockID = 0;
	if (WorldGenerator.IsValid() && TargetCoord.X >= 0)
	{
		WorldGenerator->GetVoxelAt(TargetCoord.X, TargetCoord.Y, TargetCoord.Z, BlockID);
	}

	// If not cached or pointing to Air, do a live raycast from the player's camera
	if (BlockID == 0 && WorldGenerator.IsValid())
	{
		APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		if (PC && PC->PlayerCameraManager)
		{
			const FVector CamStart = PC->PlayerCameraManager->GetCameraLocation();
			const FVector CamEnd = CamStart + (PC->PlayerCameraManager->GetActorForwardVector() * (BlockScale * 6.0f));
			FHitResult Hit;
			FCollisionQueryParams QParams;
			QParams.AddIgnoredActor(PC->GetPawn());
			if (GetWorld()->LineTraceSingleByChannel(Hit, CamStart, CamEnd, ECC_WorldStatic, QParams))
			{
				const FVector SamplePos = Hit.ImpactPoint - (Hit.ImpactNormal * (BlockScale * 0.25f));
				int32 BX, BY, BZ;
				WorldGenerator->WorldLocationToVoxelCoord(SamplePos, BX, BY, BZ);
				TargetCoord = FIntVector(BX, BY, BZ);
				WorldGenerator->GetVoxelAt(TargetCoord.X, TargetCoord.Y, TargetCoord.Z, BlockID);
			}
		}
	}

	return BlockID;
}

FName AChunkActor::GetTargetedBlockRowName() const
{
	const uint8 BlockID = GetTargetedBlockID();
	if (BlockID == 0)
	{
		return NAME_None;
	}

	UDataTable* Table = WorldGenerator.IsValid() ? WorldGenerator->BlockDataTable : nullptr;
	if (!Table)
	{
		Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/Data/Block_DataTable.Block_DataTable")));
	}

	if (Table)
	{
		const UScriptStruct* RowStruct = Table->GetRowStruct();
		for (auto It = Table->GetRowMap().CreateConstIterator(); It; ++It)
		{
			const uint8* RowData = It.Value();
			if (!RowData) continue;

			for (TFieldIterator<FIntProperty> PropIt(RowStruct); PropIt; ++PropIt)
			{
				if (PropIt->GetName().Contains(TEXT("BlockID"), ESearchCase::IgnoreCase))
				{
					if (PropIt->GetPropertyValue_InContainer(RowData) == static_cast<int32>(BlockID))
					{
						return It.Key(); // Matches Data Table Row Name, e.g. "Grass", "Dirt", "Stone"
					}
				}
			}
		}
	}

	return NAME_None;
}

void AChunkActor::BreakTargetedBlock()
{
	FIntVector TargetCoord = LastTargetedVoxelCoord;

	uint8 BlockAtCoord = 0;
	if (WorldGenerator.IsValid() && TargetCoord.X >= 0)
	{
		WorldGenerator->GetVoxelAt(TargetCoord.X, TargetCoord.Y, TargetCoord.Z, BlockAtCoord);
	}

	// If not cached or pointing to Air, live raycast from camera
	if (BlockAtCoord == 0 && WorldGenerator.IsValid())
	{
		APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		if (PC && PC->PlayerCameraManager)
		{
			const FVector CamStart = PC->PlayerCameraManager->GetCameraLocation();
			const FVector CamEnd = CamStart + (PC->PlayerCameraManager->GetActorForwardVector() * (BlockScale * 6.0f));
			FHitResult Hit;
			FCollisionQueryParams QParams;
			QParams.AddIgnoredActor(PC->GetPawn());
			if (GetWorld()->LineTraceSingleByChannel(Hit, CamStart, CamEnd, ECC_WorldStatic, QParams))
			{
				const FVector SamplePos = Hit.ImpactPoint - (Hit.ImpactNormal * (BlockScale * 0.25f));
				int32 BX, BY, BZ;
				WorldGenerator->WorldLocationToVoxelCoord(SamplePos, BX, BY, BZ);
				TargetCoord = FIntVector(BX, BY, BZ);
				WorldGenerator->GetVoxelAt(TargetCoord.X, TargetCoord.Y, TargetCoord.Z, BlockAtCoord);
			}
		}
	}

	if (WorldGenerator.IsValid() && TargetCoord.X >= 0 && BlockAtCoord != 0 && BlockAtCoord != static_cast<uint8>(EBlockType::Bedrock))
	{
		uint8 DroppedID = 0;
		WorldGenerator->BreakBlockAtVoxel(TargetCoord.X, TargetCoord.Y, TargetCoord.Z, DroppedID);
		LastTargetedVoxelCoord = FIntVector(-1, -1, -1);
	}
}

float AChunkActor::GetTargetedBlockDurability() const
{
	const uint8 BlockID = GetTargetedBlockID();
	if (BlockID == 0)
	{
		return 0.5f;
	}

	UDataTable* Table = WorldGenerator.IsValid() ? WorldGenerator->BlockDataTable : nullptr;
	if (!Table)
	{
		Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("/Game/Data/Block_DataTable.Block_DataTable")));
	}

	if (Table)
	{
		const UScriptStruct* RowStruct = Table->GetRowStruct();
		for (auto It = Table->GetRowMap().CreateConstIterator(); It; ++It)
		{
			const uint8* RowData = It.Value();
			if (!RowData) continue;

			int32 RowBlockID = -1;
			double RowDurability = 1.0;

			for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
			{
				FProperty* Prop = *PropIt;
				const FString PropName = Prop->GetName();
				if (PropName.Contains(TEXT("BlockID"), ESearchCase::IgnoreCase))
				{
					if (FIntProperty* IP = CastField<FIntProperty>(Prop))
						RowBlockID = IP->GetPropertyValue_InContainer(RowData);
				}
				else if (PropName.Contains(TEXT("Durability"), ESearchCase::IgnoreCase))
				{
					if (FDoubleProperty* DP = CastField<FDoubleProperty>(Prop))
						RowDurability = DP->GetPropertyValue_InContainer(RowData);
					else if (FFloatProperty* FP = CastField<FFloatProperty>(Prop))
						RowDurability = FP->GetPropertyValue_InContainer(RowData);
				}
			}

			if (RowBlockID == static_cast<int32>(BlockID))
			{
				return FMath::Max(0.05f, static_cast<float>(RowDurability));
			}
		}
	}

	return 1.0f;
}

float AChunkActor::GetHandMiningForce()
{
	return 1.0f;
}

float AChunkActor::GetCurrentBestToolMiningForce() const
{
	const uint8 BlockID = GetTargetedBlockID();

	// 1. Query player's BaublesSystem for equipped specialized tool
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PC && PC->GetPawn())
	{
		if (UBaublesSystem* Baubles = PC->GetPawn()->FindComponentByClass<UBaublesSystem>())
		{
			const float ToolForce = Baubles->GetBestMiningForce(BlockID);
			if (ToolForce > 1.0f)
			{
				return ToolForce;
			}
		}
	}

	// 2. Query WorldGenerator tier facade if applicable
	if (WorldGenerator.IsValid())
	{
		const float WorldGenForce = WorldGenerator->GetToolMiningForce();
		if (WorldGenForce > 1.0f)
		{
			return WorldGenForce;
		}
	}

	// 3. Fallback to base hand mining force
	return GetHandMiningForce();
}

float AChunkActor::GetTargetedBlockBreakTime(float MiningForce) const
{
	const float BestToolForce = GetCurrentBestToolMiningForce();

	// If a specific MiningForce > 1.0 was explicitly passed, respect it (or take higher of passed force or tool force).
	// If MiningForce <= 1.0 (e.g. 0.0 uninitialized or 1.0 default hand force from AC_DestroySystem), use the equipped tool force (which falls back to 1.0 Hand force if no tool equipped).
	float EffectiveMiningForce = BestToolForce;
	if (MiningForce > 1.0f)
	{
		EffectiveMiningForce = FMath::Max(MiningForce, BestToolForce);
	}

	const float Durability = GetTargetedBlockDurability();
	return UMiningQueueSystem::CalculateBreakTime(Durability, EffectiveMiningForce);
}

float AChunkActor::GetTargetedBlockPlayRate(float MiningForce, float TimelineLength) const
{
	const float BreakTime = GetTargetedBlockBreakTime(MiningForce);
	const float Len = FMath::Max(0.01f, TimelineLength);
	return Len / FMath::Max(0.01f, BreakTime);
}

UTexture2D* AChunkActor::GetBlockIconTexture(int32 InBlockID)
{
	// Texture path mappings must match EBlockType enum values exactly
	FString TextureName;
	switch (InBlockID)
	{
	case 1:  TextureName = TEXT("dirt"); break;                                   // Dirt
	case 2:  TextureName = TEXT("grass_block_side"); break;                       // Grass
	case 3:  TextureName = TEXT("cobblestone"); break;                             // Cobblestone
	case 4:  TextureName = TEXT("stone"); break;                                   // Stone
	case 5:  TextureName = TEXT("oak_log"); break;                                 // Wood Log
	case 6:  TextureName = TEXT("oak_planks"); break;                              // Wood Planks
	case 7:  TextureName = TEXT("iron_ore"); break;                                // Iron Ore
	case 8:  TextureName = TEXT("iron_block"); break;                              // Iron Block
	case 9:  TextureName = TEXT("gold_ore"); break;                                // Gold Ore
	case 10: TextureName = TEXT("gold_block"); break;                              // Gold Block
	case 11: TextureName = TEXT("diamond_ore"); break;                              // Diamond Ore
	case 12: TextureName = TEXT("diamond_block"); break;                          // Diamond Block
	case 13: TextureName = TEXT("emerald_ore"); break;                              // Emerald Ore
	case 14: TextureName = TEXT("emerald_block"); break;                          // Emerald Block
	case 15: TextureName = TEXT("crafting_table_front"); break;                    // Crafting Table
	case 16: TextureName = TEXT("furnace_front"); break;                           // Furnace
	case 17: TextureName = TEXT("oak_leaves"); break;                              // Leaves (mapped to oak_leaves in Patrix)
	case 18: TextureName = TEXT("sand"); break;                                    // Sand
	case 19: TextureName = TEXT("torch"); break;                                   // Torch
	case 20: TextureName = TEXT("barrel_side"); break;                             // Barrel
	case 21: TextureName = TEXT("glass"); break;                                   // Glass
	case 22: TextureName = TEXT("oak_planks"); break;                              // Fence
	case 23: TextureName = TEXT("oak_planks"); break;                              // Fence Door
	case 24: TextureName = TEXT("dark_oak_door_bottom"); break;                    // Door
	case 25: TextureName = TEXT("stone"); break;                                   // Bedrock
	case 26: TextureName = TEXT("glass"); break;                                   // Water
	case 27: TextureName = TEXT("coal_ore"); break;                                // Coal Ore
	default:
		TextureName = TEXT("dirt");
		break;
	}

	// 1. Try high-res Patrix texture pack first
	const FString PatrixPath = FString::Printf(TEXT("/Game/Patrix_Texture_Pack/textures/block/%s.%s"), *TextureName, *TextureName);
	if (UTexture2D* PatrixTex = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *PatrixPath)))
	{
		return PatrixTex;
	}

	// 2. Fallback to /Game/Textures/Blocks/
	const FString FallbackPath = FString::Printf(TEXT("/Game/Textures/Blocks/%s.%s"), *TextureName, *TextureName);
	return Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *FallbackPath));
}

UTexture2D* AChunkActor::GetBlockIconFromSlice(int32 TextureSliceIndex)
{
	static const TArray<FString> SliceTextureNames = {
		TEXT("barrel_bottom"),
		TEXT("barrel_side"),
		TEXT("barrel_top"),
		TEXT("barrel_top_open"),
		TEXT("coal_block"),
		TEXT("coal_ore"),
		TEXT("cobblestone"),
		TEXT("crafting_table_front"),
		TEXT("crafting_table_side"),
		TEXT("crafting_table_top"),
		TEXT("dark_oak_door_bottom"),
		TEXT("dark_oak_door_top"),
		TEXT("diamond_block"),
		TEXT("diamond_ore"),
		TEXT("dirt"),
		TEXT("emerald_block"),
		TEXT("emerald_ore"),
		TEXT("farmland"),
		TEXT("farmland_moist"),
		TEXT("furnace_front"),
		TEXT("furnace_front_on"),
		TEXT("furnace_side"),
		TEXT("furnace_top"),
		TEXT("glass"),
		TEXT("gold_block"),
		TEXT("gold_ore"),
		TEXT("grass_block_side"),
		TEXT("grass_block_top"),
		TEXT("iron_block"),
		TEXT("iron_ore"),
		TEXT("oak_leaves"), // mapped from leaves for Patrix pack
		TEXT("oak_log"),
		TEXT("oak_log_top"),
		TEXT("oak_planks"),
		TEXT("sand"),
		TEXT("stone"),
		TEXT("torch")
	};

	FString Name = TEXT("dirt");
	if (SliceTextureNames.IsValidIndex(TextureSliceIndex))
	{
		Name = SliceTextureNames[TextureSliceIndex];
	}

	// 1. Try high-res Patrix texture pack first
	const FString PatrixPath = FString::Printf(TEXT("/Game/Patrix_Texture_Pack/textures/block/%s.%s"), *Name, *Name);
	if (UTexture2D* PatrixTex = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *PatrixPath)))
	{
		return PatrixTex;
	}

	// 2. Fallback to /Game/Textures/Blocks/
	const FString FallbackPath = FString::Printf(TEXT("/Game/Textures/Blocks/%s.%s"), *Name, *Name);
	return Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *FallbackPath));
}
