#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "BlockHighlightActor.generated.h"

UCLASS()
class MINECRAFTDEWISH_API ABlockHighlightActor : public AActor
{
	GENERATED_BODY()

public:
	ABlockHighlightActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Highlight")
	UProceduralMeshComponent* OutlineMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	float BlockSize = 100.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	float EdgeThickness = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	FLinearColor EdgeColor = FLinearColor(0.02f, 0.02f, 0.02f, 1.0f);

	void SetTargetBlock(const FVector& BlockWorldCenter);
	void HideHighlight();

protected:
	virtual void BeginPlay() override;

private:
	void BuildEdgeMesh();
};
