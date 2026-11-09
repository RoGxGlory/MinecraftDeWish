#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VoxelBenchSubsystem.generated.h"

class AWorldGenerator;

/**
 * Per-frame recorder for the voxel streaming benchmark.
 *
 * It samples one AWorldGenerator every frame (frame time, chunk counts, and the
 * generation / meshing / remesh / streaming milliseconds reported by
 * FVoxelFrameStats) and writes a CSV to `Saved/Profiling/` when recording stops.
 * That CSV is the baseline the voxel performance plan is measured against: without
 * it, every optimisation claim is an anecdote.
 *
 * It costs nothing when idle because IsTickable() returns false unless recording.
 * Start it from the console (`mdw.Bench.Start [label]`), from Blueprint, or
 * automatically with the `-VoxelBench` command line switch or `mdw.Bench.AutoStart 1`.
 */
UCLASS()
class MINECRAFTDEWISH_API UVoxelBenchSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// -- USubsystem / UWorldSubsystem -------------------------------------
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// -- FTickableGameObject ----------------------------------------------
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;

	/**
	 * Begin sampling. Ignored (with a warning) if a run is already in progress.
	 *
	 * @param Label Short tag woven into the CSV file name, e.g. "rd8-fast" or "walk-north".
	 * @return True if a new recording started.
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel|Benchmark")
	bool StartRecording(const FString& Label = TEXT("run"));

	/**
	 * Stop sampling, write the CSV and log the frame-time summary.
	 *
	 * @return True if a recording was in progress and has been flushed to disk.
	 */
	UFUNCTION(BlueprintCallable, Category = "Voxel|Benchmark")
	bool StopRecording();

	/** Add a labelled sample, so a phase change (spawn, turn, teleport) is visible in the CSV. */
	UFUNCTION(BlueprintCallable, Category = "Voxel|Benchmark")
	void AddMarker(const FString& Marker);

	/** True while samples are being collected. */
	UFUNCTION(BlueprintPure, Category = "Voxel|Benchmark")
	bool IsRecording() const { return bRecording; }

	/** Absolute path of the CSV written by the last completed run (empty before the first). */
	UFUNCTION(BlueprintPure, Category = "Voxel|Benchmark")
	FString GetLastOutputPath() const { return LastOutputPath; }

private:
	/** Append one CSV row describing this frame. Marker may be null. */
	void AppendRow(const AWorldGenerator& Generator, float DeltaTime, const TCHAR* Marker);

	/** Resolve (and cache) the generator being sampled. */
	AWorldGenerator* ResolveGenerator();

	/** Write the collected rows and log the headline numbers. */
	void FlushToDisk();

	/** `Saved/Profiling/VoxelBench-<label>-<timestamp>.csv`. */
	static FString MakeDefaultOutputPath(const FString& Label);

	bool bRecording = false;
	TArray<FString> Rows;
	TArray<float> FrameTimesMs;
	double StartSeconds = 0.0;
	FString LastOutputPath;

	TWeakObjectPtr<AWorldGenerator> CachedGenerator;
};
