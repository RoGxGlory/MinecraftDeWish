#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "VoxelDataTypes.h"
#include "ChaosBlockDestructionActor.generated.h"

/** Individual dynamic debris particle data */
struct FBlockDestructionParticle
{
	FVector RelativePos = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	FVector AngularVelocity = FVector::ZeroVector;
	FVector HalfExtents = FVector(3.5f, 3.5f, 3.5f);
	int32 TextureSlice = 14;
	bool bIsGrounded = false;
};

/**
 * AChaosBlockDestructionActor
 *
 * Spawns non-deterministic physical 3D block particles when a voxel block is destroyed.
 * Uses real-time ballistic physics and collision sweeps against world geometry with:
 * 1. Non-deterministic tiny particle slices (32-44 small particles on the ground, ~5-9cm each).
 * 2. Exact block texture slices from M_Global (matching outer and interior block textures).
 * 3. Outward explosive burst velocity, upward pop, and tumbling angular velocity.
 * 4. Realistic gravity, restitution surface bounce, ground settling, and smooth shrink/fade despawn.
 */
UCLASS()
class MINECRAFTDEWISH_API AChaosBlockDestructionActor : public AActor
{
	GENERATED_BODY()

public:
	AChaosBlockDestructionActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProceduralMeshComponent* ProceduralMesh;

	/** Block ID that was broken */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos", meta = (ExposeOnSpawn = true))
	uint8 BlockID = 1;

	/** Size of the original voxel cube in Unreal units (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos")
	float BlockScale = DEFAULT_BLOCK_SCALE;

	/** Duration particles actively tumble and scatter before fading (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos")
	float SimulationLifetime = 2.0f;

	/** Duration over which particles shrink and fade out (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos")
	float FadeDuration = 0.8f;

	/** Gravity acceleration applied to debris particles (cm/s^2) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chaos")
	float GravityStrength = 1350.0f;

	/**
	 * Initializes the destruction particle system, spawns small particle pieces,
	 * assigns materials/textures, and launches them outwards with gravity and collision.
	 *
	 * @param InBlockID Block ID to fracture.
	 * @param InHitNormal Normal vector of the impact strike (for directional debris bias).
	 * @param InHitPoint Exact world location of the strike point.
	 * @param InBlockScale Size of the voxel block (default 100cm).
	 * @param InMaterial Master terrain material (defaults to M_Global).
	 */
	UFUNCTION(BlueprintCallable, Category = "Chaos")
	void InitializeDestruction(
		uint8 InBlockID,
		const FVector& InHitNormal = FVector::UpVector,
		const FVector& InHitPoint = FVector::ZeroVector,
		float InBlockScale = 100.0f,
		UMaterialInterface* InMaterial = nullptr
	);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	float ElapsedTime = 0.0f;
	bool bIsInitialized = false;

	UPROPERTY(Transient)
	UMaterialInterface* CachedMaterial = nullptr;

	TArray<FBlockDestructionParticle> Particles;

	// Cached buffers for fast procedural mesh updates
	TArray<FVector> MeshVertices;
	TArray<int32> MeshTriangles;
	TArray<FVector> MeshNormals;
	TArray<FVector2D> MeshUV0;
	TArray<FVector2D> MeshUV1;
	TArray<FLinearColor> MeshVertexColors;
	TArray<FProcMeshTangent> MeshTangents;

	void ResolveBlockTextures(
		uint8 InBlockID,
		int32& OutTopTex,
		int32& OutBottomTex,
		int32& OutSideTex,
		int32& OutFrontTex,
		int32& OutInteriorTex
	);

	void SpawnParticles(
		int32 TopTex,
		int32 BottomTex,
		int32 SideTex,
		int32 FrontTex,
		int32 InteriorTex,
		const FVector& InHitNormal,
		const FVector& InHitPoint
	);

	void BuildInitialMesh();
	void UpdateMeshBuffers(float CurrentScale);
};
