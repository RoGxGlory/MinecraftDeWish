#include "ChunkActor.h"
#include "WorldGenerator.h"

AChunkActor::AChunkActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = ProceduralMesh;

	ProceduralMesh->bUseAsyncCooking = false;
	ProceduralMesh->SetCastShadow(true);
	ProceduralMesh->bAffectDistanceFieldLighting = false;
	ProceduralMesh->SetCollisionObjectType(ECC_WorldStatic);
	ProceduralMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AChunkActor::BeginPlay()
{
	Super::BeginPlay();
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

bool AChunkActor::ShouldRenderFace(int32 NeighborX, int32 NeighborY, int32 NeighborZ) const
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

	// Tangents
	const FVector TangentDir = FMath::Abs(Normal.Z) > 0.9f ? FVector(1.0f, 0.0f, 0.0f) : FVector::CrossProduct(Normal, FVector::UpVector);
	const FProcMeshTangent Tangent(TangentDir.GetSafeNormal(), false);
	MeshData.Tangents.Add(Tangent);
	MeshData.Tangents.Add(Tangent);
	MeshData.Tangents.Add(Tangent);
	MeshData.Tangents.Add(Tangent);

	// UV0 mapping:
	// For vertical side faces: V0/V1 are bottom (Z=0, UV V=1.0), V2/V3 are top (Z=S, UV V=0.0).
	// This ensures upright textures so the green grass fringe is at the TOP of the block, connecting to grass top.
	if (Face == EBlockFace::Top || Face == EBlockFace::Bottom)
	{
		MeshData.UV0.Add(FVector2D(0.0f, 0.0f));
		MeshData.UV0.Add(FVector2D(1.0f, 0.0f));
		MeshData.UV0.Add(FVector2D(1.0f, 1.0f));
		MeshData.UV0.Add(FVector2D(0.0f, 1.0f));
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

void AChunkActor::GenerateMeshData(FChunkMeshData& OutMeshData) const
{
	OutMeshData.Reset();

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

				// Top (+Z)
				if (ShouldRenderFace(X, Y, Z + 1))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::Top) : 0;
					AddFace(OutMeshData, BlockPos, EBlockFace::Top, Tex);
				}

				// Bottom (-Z)
				if (ShouldRenderFace(X, Y, Z - 1))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::Bottom) : 0;
					AddFace(OutMeshData, BlockPos, EBlockFace::Bottom, Tex);
				}

				// North (+X)
				if (ShouldRenderFace(X + 1, Y, Z))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::North) : 0;
					AddFace(OutMeshData, BlockPos, EBlockFace::North, Tex);
				}

				// South (-X)
				if (ShouldRenderFace(X - 1, Y, Z))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::South) : 0;
					AddFace(OutMeshData, BlockPos, EBlockFace::South, Tex);
				}

				// East (+Y)
				if (ShouldRenderFace(X, Y + 1, Z))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::East) : 0;
					AddFace(OutMeshData, BlockPos, EBlockFace::East, Tex);
				}

				// West (-Y)
				if (ShouldRenderFace(X, Y - 1, Z))
				{
					const int32 Tex = WorldGenerator.IsValid() ? WorldGenerator->GetTextureForBlock(BlockID, EBlockFace::West) : 0;
					AddFace(OutMeshData, BlockPos, EBlockFace::West, Tex);
				}
			}
		}
	}
}

void AChunkActor::ApplyMeshData(const FChunkMeshData& InMeshData)
{
	ProceduralMesh->ClearMeshSection(0);

	if (!InMeshData.IsEmpty())
	{
		ProceduralMesh->CreateMeshSection_LinearColor(
			0,
			InMeshData.Vertices,
			InMeshData.Triangles,
			InMeshData.Normals,
			InMeshData.UV0,
			InMeshData.UV1,
			TArray<FVector2D>(),
			TArray<FVector2D>(),
			InMeshData.VertexColors,
			InMeshData.Tangents,
			true // Collision
		);

		if (WorldGenerator.IsValid() && WorldGenerator->TerrainMaterial)
		{
			ProceduralMesh->SetMaterial(0, WorldGenerator->TerrainMaterial);
		}
	}
}

void AChunkActor::UpdateMesh()
{
	FChunkMeshData MeshData;
	GenerateMeshData(MeshData);
	ApplyMeshData(MeshData);
}

void AChunkActor::SetMaterial(UMaterialInterface* Material)
{
	if (ProceduralMesh && Material)
	{
		ProceduralMesh->SetMaterial(0, Material);
	}
}
