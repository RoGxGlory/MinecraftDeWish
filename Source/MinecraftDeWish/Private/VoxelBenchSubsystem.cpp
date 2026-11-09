#include "VoxelBenchSubsystem.h"

#include "WorldGenerator.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Stats/Stats.h"

DEFINE_LOG_CATEGORY_STATIC(LogVoxelBench, Log, All);

namespace
{
	/** Frame budget the voxel benchmark judges samples against (60 FPS). */
	constexpr float FrameBudgetMs = 1000.0f / 60.0f;

	/** Start a run automatically on PIE/standalone start. */
	TAutoConsoleVariable<int32> CVarBenchAutoStart(
		TEXT("mdw.Bench.AutoStart"),
		0,
		TEXT("Start the voxel benchmark automatically when the world begins play (0 = off, 1 = on)."),
		ECVF_Default);

	UVoxelBenchSubsystem* FindBenchSubsystem(UWorld* World)
	{
		return World ? World->GetSubsystem<UVoxelBenchSubsystem>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GBenchStartCommand(
		TEXT("mdw.Bench.Start"),
		TEXT("Start the voxel streaming benchmark. Optional argument: a label used in the CSV file name."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (UVoxelBenchSubsystem* Bench = FindBenchSubsystem(World))
				{
					Bench->StartRecording(Args.Num() > 0 ? Args[0] : FString(TEXT("run")));
				}
			}));

	FAutoConsoleCommandWithWorldAndArgs GBenchStopCommand(
		TEXT("mdw.Bench.Stop"),
		TEXT("Stop the voxel streaming benchmark and write the CSV."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (UVoxelBenchSubsystem* Bench = FindBenchSubsystem(World))
				{
					Bench->StopRecording();
				}
			}));

	FAutoConsoleCommandWithWorldAndArgs GBenchMarkerCommand(
		TEXT("mdw.Bench.Marker"),
		TEXT("Insert a labelled sample into the running voxel benchmark, e.g. mdw.Bench.Marker turn-north."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (UVoxelBenchSubsystem* Bench = FindBenchSubsystem(World))
				{
					Bench->AddMarker(Args.Num() > 0 ? Args[0] : FString(TEXT("marker")));
				}
			}));

	/** CSV escapes nothing, so a comma in free text would silently add a column. */
	FString SanitizeCell(const FString& In)
	{
		FString Out = In;
		Out.ReplaceInline(TEXT(","), TEXT(";"));
		Out.ReplaceInline(TEXT("\n"), TEXT(" "));
		Out.ReplaceInline(TEXT("\r"), TEXT(" "));
		return Out;
	}
}

void UVoxelBenchSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Rows.Reserve(16384);
	FrameTimesMs.Reserve(16384);
}

void UVoxelBenchSubsystem::Deinitialize()
{
	if (bRecording)
	{
		UE_LOG(LogVoxelBench, Warning, TEXT("Voxel benchmark still recording at shutdown; flushing the samples collected so far."));
		FlushToDisk();
		bRecording = false;
	}

	Super::Deinitialize();
}

void UVoxelBenchSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const bool bFromCommandLine = FParse::Param(FCommandLine::Get(), TEXT("VoxelBench"));
	if (bFromCommandLine || CVarBenchAutoStart.GetValueOnGameThread() != 0)
	{
		StartRecording(bFromCommandLine ? FString(TEXT("cmdline")) : FString(TEXT("cvar")));
	}
}

bool UVoxelBenchSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// The editor preview world has no gameplay streaming; only sample real runs.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UVoxelBenchSubsystem::IsTickable() const
{
	// Zero cost when idle: an idle subsystem is not ticked at all.
	return bRecording && !IsTemplate();
}

TStatId UVoxelBenchSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVoxelBenchSubsystem, STATGROUP_Tickables);
}

AWorldGenerator* UVoxelBenchSubsystem::ResolveGenerator()
{
	if (AWorldGenerator* Cached = CachedGenerator.Get())
	{
		return Cached;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AWorldGenerator* Found = nullptr;
	for (TActorIterator<AWorldGenerator> It(World); It; ++It)
	{
		Found = *It;
		break;
	}

	CachedGenerator = Found;
	return Found;
}

void UVoxelBenchSubsystem::Tick(float DeltaTime)
{
	if (!bRecording)
	{
		return;
	}

	AWorldGenerator* Generator = ResolveGenerator();
	if (!Generator)
	{
		return;
	}

	AppendRow(*Generator, DeltaTime, nullptr);
}

void UVoxelBenchSubsystem::AppendRow(const AWorldGenerator& Generator, float DeltaTime, const TCHAR* Marker)
{
	const FVoxelFrameStats& Stats = Generator.GetLastFrameStats();

	FVector PlayerLocation = FVector::ZeroVector;
	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			if (const APawn* Pawn = PC->GetPawn())
			{
				PlayerLocation = Pawn->GetActorLocation();
			}
		}
	}

	const float DeltaMs = DeltaTime * 1000.0f;
	FrameTimesMs.Add(DeltaMs);

	Rows.Add(FString::Printf(
		TEXT("%d,%.3f,%.3f,%d,%d,%d,%d,%d,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f,%.1f,%d,%d,%s"),
		Rows.Num() + 1,
		FPlatformTime::Seconds() - StartSeconds,
		DeltaMs,
		Generator.GetActiveChunkCount(),
		Generator.GetPendingGenerationCount(),
		Generator.GetPendingRemeshCount(),
		Stats.ChunksGenerated,
		Stats.ChunksRemeshed,
		Stats.StreamingMs,
		Stats.GenerationMs,
		Stats.MeshMs,
		Stats.RemeshMs,
		PlayerLocation.X,
		PlayerLocation.Y,
		PlayerLocation.Z,
		Generator.GetRenderDistance(),
		Generator.GetFastLeaves() ? 1 : 0,
		Marker ? Marker : TEXT("")));
}

bool UVoxelBenchSubsystem::StartRecording(const FString& Label)
{
	if (bRecording)
	{
		UE_LOG(LogVoxelBench, Warning,
			TEXT("Voxel benchmark is already recording; call mdw.Bench.Stop before starting a new run."));
		return false;
	}

	Rows.Reset();
	FrameTimesMs.Reset();
	StartSeconds = FPlatformTime::Seconds();
	LastOutputPath = MakeDefaultOutputPath(Label);
	bRecording = true;

	UE_LOG(LogVoxelBench, Display, TEXT("Voxel benchmark started. CSV will be written to %s"), *LastOutputPath);
	return true;
}

bool UVoxelBenchSubsystem::StopRecording()
{
	if (!bRecording)
	{
		UE_LOG(LogVoxelBench, Warning, TEXT("Voxel benchmark is not recording."));
		return false;
	}

	bRecording = false;
	FlushToDisk();
	return true;
}

void UVoxelBenchSubsystem::AddMarker(const FString& Marker)
{
	if (!bRecording)
	{
		return;
	}

	if (AWorldGenerator* Generator = ResolveGenerator())
	{
		AppendRow(*Generator, 0.0f, *SanitizeCell(Marker));
	}
}

void UVoxelBenchSubsystem::FlushToDisk()
{
	if (Rows.Num() == 0 || FrameTimesMs.Num() == 0)
	{
		UE_LOG(LogVoxelBench, Warning, TEXT("Voxel benchmark produced no samples; nothing written."));
		return;
	}

	const FString Header = TEXT(
		"Sample,TimeSec,FrameMs,ActiveChunks,PendingGen,PendingRemesh,"
		"ChunksGenerated,ChunksRemeshed,StreamingMs,GenerationMs,MeshMs,RemeshMs,"
		"PlayerX,PlayerY,PlayerZ,RenderDistance,FastLeaves,Marker\n");

	const FString Contents = Header + FString::Join(Rows, TEXT("\n")) + TEXT("\n");

	IFileManager::Get().MakeDirectory(*FPaths::GetPath(LastOutputPath), /*Tree=*/true);
	if (!FFileHelper::SaveStringToFile(Contents, *LastOutputPath))
	{
		UE_LOG(LogVoxelBench, Error, TEXT("Could not write the voxel benchmark CSV to %s"), *LastOutputPath);
		return;
	}

	TArray<float> Sorted = FrameTimesMs;
	Sorted.Sort();

	double SumMs = 0.0;
	int32 OverBudget = 0;
	int32 OverDoubleBudget = 0;
	for (const float SampleMs : FrameTimesMs)
	{
		SumMs += SampleMs;
		if (SampleMs > FrameBudgetMs)
		{
			++OverBudget;
		}
		if (SampleMs > FrameBudgetMs * 2.0f)
		{
			++OverDoubleBudget;
		}
	}

	const auto PercentileMs = [&Sorted](float P)
	{
		const int32 LastIndex = Sorted.Num() - 1;
		const int32 Index = FMath::Clamp(FMath::FloorToInt(P * LastIndex), 0, LastIndex);
		return Sorted[Index];
	};

	const double DurationSeconds = FPlatformTime::Seconds() - StartSeconds;
	const double AverageMs = SumMs / static_cast<double>(FrameTimesMs.Num());

	UE_LOG(LogVoxelBench, Display,
		TEXT("Voxel benchmark: %d frames in %.1f s | avg %.2f ms | p50 %.2f | p95 %.2f | p99 %.2f | max %.2f | "
			 "over %.1f ms: %d (%.1f%%), over %.1f ms: %d (%.1f%%) | CSV: %s"),
		FrameTimesMs.Num(),
		DurationSeconds,
		AverageMs,
		PercentileMs(0.50f),
		PercentileMs(0.95f),
		PercentileMs(0.99f),
		Sorted.Last(),
		FrameBudgetMs,
		OverBudget,
		100.0 * static_cast<double>(OverBudget) / static_cast<double>(FrameTimesMs.Num()),
		FrameBudgetMs * 2.0f,
		OverDoubleBudget,
		100.0 * static_cast<double>(OverDoubleBudget) / static_cast<double>(FrameTimesMs.Num()),
		*LastOutputPath);

	Rows.Reset();
	FrameTimesMs.Reset();
}

FString UVoxelBenchSubsystem::MakeDefaultOutputPath(const FString& Label)
{
	const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"));

	// Keep the file name portable: no separators, spaces or reserved characters.
	FString CleanLabel;
	for (const TCHAR Char : Label)
	{
		CleanLabel.AppendChar(FChar::IsAlnum(Char) ? Char : TEXT('_'));
	}
	if (CleanLabel.IsEmpty())
	{
		CleanLabel = TEXT("run");
	}

	return FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Profiling"),
		FString::Printf(TEXT("VoxelBench-%s-%s.csv"), *CleanLabel, *Stamp));
}
