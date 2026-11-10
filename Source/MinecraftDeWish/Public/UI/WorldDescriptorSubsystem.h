// LAMPLIGHTER — world descriptor handed from the main menu to the gameplay map.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldDescriptorSubsystem.generated.h"

/** Everything the main menu must pass to the gameplay map. */
USTRUCT(BlueprintType)
struct FWorldDescriptor
{
	GENERATED_BODY()

	/** Player-chosen world name. Shown on the title screen and in the save list. */
	UPROPERTY(BlueprintReadWrite, Category = "World")
	FString WorldName = TEXT("Untitled");

	/** Procedural generation seed. Never travels through level URL options. */
	UPROPERTY(BlueprintReadWrite, Category = "World")
	int32 Seed = 1337;

	/** Dread preset id: Ambient / Standard / Merciless. Stored with the save, not in ini. */
	UPROPERTY(BlueprintReadWrite, Category = "World")
	FString DreadPreset = TEXT("Standard");
};

/**
 * Carries the pending world across the level travel from Lvl_MainMenu to the gameplay map.
 *
 * A GameInstanceSubsystem survives OpenLevel, which AGameMode, AGameState and UWorldSubsystem
 * do not. The gameplay map consumes the descriptor exactly once in AWorldGenerator::BeginPlay,
 * so launching the same map again from the editor (PIE) keeps the map's own defaults.
 */
UCLASS()
class MINECRAFTDEWISH_API UWorldDescriptorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Convenience accessor; returns nullptr outside a game instance (e.g. in the editor). */
	static UWorldDescriptorSubsystem* Get(const UObject* WorldContextObject);

	/** Store the world the next gameplay map must build. */
	UFUNCTION(BlueprintCallable, Category = "World")
	void SetPendingWorld(const FWorldDescriptor& Descriptor);

	UFUNCTION(BlueprintPure, Category = "World")
	bool HasPendingWorld() const { return bHasPendingWorld; }

	/** Returns the pending descriptor and clears it: a world is launched exactly once. */
	UFUNCTION(BlueprintCallable, Category = "World")
	FWorldDescriptor ConsumePendingWorld();

	/** Name of the most recently launched world, for the title screen's "Continue" line. */
	UFUNCTION(BlueprintPure, Category = "World")
	const FString& GetLastWorldName() const { return LastWorldName; }

private:
	UPROPERTY()
	FWorldDescriptor PendingWorld;

	UPROPERTY()
	FString LastWorldName;

	bool bHasPendingWorld = false;
};
