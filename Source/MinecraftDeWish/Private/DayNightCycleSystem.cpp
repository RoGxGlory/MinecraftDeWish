#include "DayNightCycleSystem.h"
#include "Components/LightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Engine/Scene.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Actor.h"
#include "WorldGenerator.h"

DEFINE_LOG_CATEGORY_STATIC(LogMinecraftDeWish, Log, All);

// Static per-state parameter table
static FWeatherStateParams MakeWeatherParams(
	float FogDensity, float Visibility, float Ambient, float Wetness,
	float Dust, float Rain, float Ash, float Aggro, float SpawnRate,
	float DurMin, float DurMax)
{
	FWeatherStateParams P;
	P.FogDensity = FogDensity;
	P.VisibilityBlocks = Visibility;
	P.AmbientLightFactor = Ambient;
	P.SurfaceWetness = Wetness;
	P.ParticleSpawnDust = Dust;
	P.RainParticleRate = Rain;
	P.AshParticleRate = Ash;
	P.MobAggression = Aggro;
	P.MobSpawnRate = SpawnRate;
	P.DurationMinSeconds = DurMin;
	P.DurationMaxSeconds = DurMax;
	return P;
}

UDayNightCycleSystem::UDayNightCycleSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 10 Hz is sufficient
}

void UDayNightCycleSystem::BeginPlay()
{
	Super::BeginPlay();
	bWasDay = IsDay();

	// Initialize interpolated values from the current (default Overcast) state
	FWeatherStateParams P = GetWeatherParams(CurrentWeather);
	CurrentFogDensity = P.FogDensity;
	CurrentAmbientLightFactor = P.AmbientLightFactor;
	CurrentSurfaceWetness = P.SurfaceWetness;
	CurrentMobSpawnRateMultiplier = P.MobSpawnRate;
	CurrentMobAggressionMultiplier = P.MobAggression;

	// Set initial weather timer
	WeatherTimeRemaining = FMath::FRandRange(P.DurationMinSeconds, P.DurationMaxSeconds);

	CreateEffectComponents();
	UpdateEffects();
}

void UDayNightCycleSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bPauseCycle)
	{
		// Still update weather transitions even when paused? Keep transition running.
	}

	// Advance time
	if (!bPauseCycle)
	{
		const float TimeStep = DeltaTime / CycleDurationSeconds;
		const float PrevTime = TimeOfDay;
		TimeOfDay += TimeStep;

		// Handle day rollover
		if (TimeOfDay >= 1.0f)
		{
			TimeOfDay -= 1.0f;
			DayCount++;
			UE_LOG(LogMinecraftDeWish, Log, TEXT("DayNightCycle: Day %d has begun."), DayCount);
		}

		// Detect day/night transitions -> trigger weather re-roll
		const bool bCurrentlyDay = IsDay();
		if (bCurrentlyDay != bWasDay)
		{
			bWasDay = bCurrentlyDay;
			OnTimeChanged.Broadcast(TimeOfDay, bCurrentlyDay);

			UE_LOG(LogMinecraftDeWish, Log, TEXT("DayNightCycle: %s (Day %d, Time: %.3f)"),
				bCurrentlyDay ? TEXT("SUNRISE") : TEXT("SUNSET"), DayCount, TimeOfDay);

			// Weather change at boundary: pick a new weather state
			SelectNewWeather();
		}
	}

	UpdateSunRotation();
	UpdateWeather(DeltaTime);
}

void UDayNightCycleSystem::UpdateSunRotation()
{
	if (!SunLightActor)
	{
		return;
	}

	const float Pitch = GetSunPitchAngle();
	SunLightActor->SetActorRotation(FRotator(Pitch, -90.0f, 0.0f));

	TArray<UActorComponent*> LightComps;
	SunLightActor->GetComponents(ULightComponent::StaticClass(), LightComps);
	for (UActorComponent* Comp : LightComps)
	{
		if (ULightComponent* Light = Cast<ULightComponent>(Comp))
		{
			Light->SetIntensity(GetSkyLightIntensity() * CurrentAmbientLightFactor * 10.0f);
		}
	}
}

float UDayNightCycleSystem::GetSkyLightIntensity() const
{
	float Base = 0.0f;
	if (IsDay())
	{
		if (TimeOfDay < 0.05f)
		{
			Base = FMath::Lerp(0.1f, 0.5f, TimeOfDay / 0.05f);
		}
		else if (TimeOfDay < 0.45f)
		{
			Base = 1.0f;
		}
		else
		{
			Base = FMath::Lerp(1.0f, 0.1f, (TimeOfDay - 0.45f) / 0.05f);
		}
	}
	else
	{
		if (TimeOfDay < 0.55f)
		{
			Base = FMath::Lerp(0.1f, 0.02f, (TimeOfDay - 0.5f) / 0.05f);
		}
		else if (TimeOfDay < 0.95f)
		{
			Base = 0.02f;
		}
		else
		{
			Base = FMath::Lerp(0.02f, 0.1f, (TimeOfDay - 0.95f) / 0.05f);
		}
	}

	// Apply weather ambient light factor
	return FMath::Clamp(Base * CurrentAmbientLightFactor, 0.0f, 1.0f);
}

float UDayNightCycleSystem::GetSunPitchAngle() const
{
	return -TimeOfDay * 360.0f;
}

FString UDayNightCycleSystem::GetTimeString() const
{
	const float Hours24 = FMath::Fmod(TimeOfDay * 24.0f + 6.0f, 24.0f);
	const int32 Hour = FMath::FloorToInt(Hours24);
	const int32 Minute = FMath::FloorToInt(FMath::Fmod(Hours24, 1.0f) * 60.0f);
	return FString::Printf(TEXT("Day %d, %02d:%02d"), DayCount, Hour, Minute);
}

void UDayNightCycleSystem::SetTimeOfDay(float NewTime)
{
	TimeOfDay = FMath::Fmod(FMath::Max(0.0f, NewTime), 1.0f);
	bWasDay = IsDay();
	UpdateSunRotation();
}

// ==================== WEATHER ====================

FWeatherStateParams UDayNightCycleSystem::GetWeatherParams(EWeatherState State) const
{
	switch (State)
	{
	case EWeatherState::Clear:
		return MakeWeatherParams(0.0f, 120.0f, 1.0f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 120.0f, 240.0f);
	case EWeatherState::Overcast:
		return MakeWeatherParams(0.3f, 80.0f, 0.7f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 300.0f, 480.0f);
	case EWeatherState::Rain:
		return MakeWeatherParams(0.2f, 60.0f, 0.6f, 0.8f, 0.3f, 1.0f, 0.0f, 0.8f, 0.7f, 180.0f, 300.0f);
	case EWeatherState::Fog:
		{
			FWeatherStateParams P = MakeWeatherParams(FogDensityMax, 30.0f, 0.4f, 0.2f, 0.1f, 0.0f, 0.0f, MobAggressionFogMultiplier, 1.3f, 240.0f, 360.0f);
			return P;
		}
	case EWeatherState::AshStorm:
		return MakeWeatherParams(0.6f, 40.0f, 0.3f, 0.0f, 2.0f, 0.0f, 1.0f, 1.5f, 1.5f, 120.0f, 180.0f);
	default:
		return MakeWeatherParams(0.3f, 80.0f, 0.7f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 300.0f, 480.0f);
	}
}

void UDayNightCycleSystem::UpdateWeather(float DeltaTime)
{
	// Decrement weather timer
	WeatherTimeRemaining -= DeltaTime;
	if (WeatherTimeRemaining <= 0.0f && !bWeatherTransitioning)
	{
		SelectNewWeather();
	}

	// Advance transition
	if (bWeatherTransitioning)
	{
		TransitionElapsed += DeltaTime;
		WeatherTransitionAlpha = FMath::Clamp(TransitionElapsed / FMath::Max(TransitionDuration, 0.01f), 0.0f, 1.0f);
		ApplyWeatherTransition(WeatherTransitionAlpha);

		if (WeatherTransitionAlpha >= 1.0f)
		{
			const EWeatherState OldWeather = CurrentWeather;
			bWeatherTransitioning = false;
			CurrentWeather = TargetWeather;
			WeatherTransitionAlpha = 1.0f;
			OnWeatherChanged.Broadcast(static_cast<int32>(CurrentWeather), static_cast<int32>(OldWeather));

			// Reset timer for the new state
			FWeatherStateParams P = GetWeatherParams(CurrentWeather);
			WeatherTimeRemaining = FMath::FRandRange(
				FMath::Max(P.DurationMinSeconds, MinWeatherDurationSeconds),
				P.DurationMaxSeconds);
		}
	}

	UpdateEffects();
}

void UDayNightCycleSystem::SelectNewWeather()
{
	if (bWeatherTransitioning)
	{
		return;
	}

	const EWeatherState Previous = CurrentWeather;
	const EWeatherState Next = PickWeightedWeather();
	if (Next == Previous)
	{
		// Avoid same weather twice in a row: try again by forcing a different one
		// Simple fallback: advance to Overcast
		if (Next != EWeatherState::Overcast)
		{
			StartTransition(EWeatherState::Overcast);
			return;
		}
	}
	StartTransition(Next);
}

EWeatherState UDayNightCycleSystem::PickWeightedWeather() const
{
	// Build candidate list with weights
	struct FCandidate { EWeatherState State; float Weight; };
	TArray<FCandidate> Candidates;

	auto Add = [&](EWeatherState S, float W)
	{
		if (W > 0.0f)
		{
			Candidates.Add({ S, W });
		}
	};

	// Base weights (Clear/AshStorm driven by tuning knobs)
	Add(EWeatherState::Clear, ClearSkyProbability);
	Add(EWeatherState::Overcast, 0.40f);
	Add(EWeatherState::Rain, 0.20f);
	Add(EWeatherState::Fog, 0.20f);
	Add(EWeatherState::AshStorm, AshStormProbability);

	// Time of day modifiers
	for (FCandidate& C : Candidates)
	{
		if (IsDay())
		{
			if (C.State == EWeatherState::Clear) C.Weight *= 2.0f;
			if (C.State == EWeatherState::AshStorm) C.Weight = 0.0f;
		}
		else
		{
			if (C.State == EWeatherState::Fog) C.Weight *= 1.5f;
			if (C.State == EWeatherState::AshStorm) C.Weight *= 3.0f;
		}
	}

	// Previous weather modifiers
	for (FCandidate& C : Candidates)
	{
		// No same weather twice in a row
		if (C.State == CurrentWeather)
		{
			C.Weight = 0.0f;
		}
		// Rain -> Fog x2
		if (CurrentWeather == EWeatherState::Rain && C.State == EWeatherState::Fog)
		{
			C.Weight *= 2.0f;
		}
		// Fog -> Clear x2
		if (CurrentWeather == EWeatherState::Fog && C.State == EWeatherState::Clear)
		{
			C.Weight *= 2.0f;
		}
	}

	// Remove zero-weight
	Candidates.RemoveAll([](const FCandidate& C) { return C.Weight <= 0.0f; });

	if (Candidates.Num() == 0)
	{
		return EWeatherState::Overcast;
	}

	// Normalize and pick
	float Total = 0.0f;
	for (const FCandidate& C : Candidates) Total += C.Weight;
	float Roll = FMath::FRandRange(0.0f, Total);
	float Acc = 0.0f;
	for (const FCandidate& C : Candidates)
	{
		Acc += C.Weight;
		if (Roll <= Acc)
		{
			return C.State;
		}
	}
	return Candidates.Last().State;
}

void UDayNightCycleSystem::StartTransition(EWeatherState ToState)
{
	if (ToState == CurrentWeather && !bWeatherTransitioning)
	{
		// No-op if same and not transitioning
		return;
	}

	TargetWeather = ToState;
	bWeatherTransitioning = true;
	TransitionElapsed = 0.0f;
	TransitionDuration = WeatherTransitionDuration;

	OnWeatherTransitionStarted.Broadcast(
		static_cast<int32>(CurrentWeather),
		static_cast<int32>(TargetWeather),
		TransitionDuration);

	UE_LOG(LogMinecraftDeWish, Log, TEXT("Weather: transitioning %d -> %d over %.1fs"),
		static_cast<int32>(CurrentWeather), static_cast<int32>(TargetWeather), TransitionDuration);
}

void UDayNightCycleSystem::ApplyWeatherTransition(float Alpha)
{
	FWeatherStateParams From = GetWeatherParams(CurrentWeather);
	FWeatherStateParams To = GetWeatherParams(TargetWeather);

	CurrentFogDensity = FMath::Lerp(From.FogDensity, To.FogDensity, Alpha);
	CurrentAmbientLightFactor = FMath::Lerp(From.AmbientLightFactor, To.AmbientLightFactor, Alpha);
	CurrentSurfaceWetness = FMath::Lerp(From.SurfaceWetness, To.SurfaceWetness, Alpha);
	CurrentMobSpawnRateMultiplier = FMath::Lerp(From.MobSpawnRate, To.MobSpawnRate, Alpha);
	CurrentMobAggressionMultiplier = FMath::Lerp(From.MobAggression, To.MobAggression, Alpha);

	OnFogDensityChanged.Broadcast(CurrentFogDensity);
}

void UDayNightCycleSystem::SetWeather(EWeatherState NewWeather, bool bImmediate)
{
	if (bImmediate)
	{
		const EWeatherState OldWeather = CurrentWeather;
		CurrentWeather = NewWeather;
		TargetWeather = NewWeather;
		bWeatherTransitioning = false;
		WeatherTransitionAlpha = 1.0f;
		FWeatherStateParams P = GetWeatherParams(NewWeather);
		CurrentFogDensity = P.FogDensity;
		CurrentAmbientLightFactor = P.AmbientLightFactor;
		CurrentSurfaceWetness = P.SurfaceWetness;
		CurrentMobSpawnRateMultiplier = P.MobSpawnRate;
		CurrentMobAggressionMultiplier = P.MobAggression;
		OnWeatherChanged.Broadcast(static_cast<int32>(CurrentWeather), static_cast<int32>(OldWeather));
	}
	else
	{
		StartTransition(NewWeather);
	}
}

float UDayNightCycleSystem::GetVisibilityBlocks() const
{
	FWeatherStateParams From = GetWeatherParams(CurrentWeather);
	FWeatherStateParams To = GetWeatherParams(TargetWeather);
	const float Alpha = bWeatherTransitioning ? WeatherTransitionAlpha : 1.0f;
	// When not transitioning, Alpha is 1.0 but From==To? Current==Target so lerp returns same.
	return FMath::Lerp(From.VisibilityBlocks, To.VisibilityBlocks, Alpha);
}

void UDayNightCycleSystem::CreateEffectComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Fog post-process component
	if (!FogPostProcessComponent)
	{
		FogPostProcessComponent = NewObject<UPostProcessComponent>(Owner, TEXT("WeatherFogPostProcess"));
		if (FogPostProcessComponent)
		{
			Owner->AddInstanceComponent(FogPostProcessComponent);
			FogPostProcessComponent->RegisterComponent();
			FogPostProcessComponent->bUnbound = true;
		}
	}

	// Rain Niagara component
	UNiagaraSystem* RainSys = RainNiagaraSystem.LoadSynchronous();
	if (!RainNiagaraComponent && RainSys)
	{
		RainNiagaraComponent = NewObject<UNiagaraComponent>(Owner, TEXT("WeatherRainNiagara"));
		if (RainNiagaraComponent)
		{
			Owner->AddInstanceComponent(RainNiagaraComponent);
			RainNiagaraComponent->RegisterComponent();
			RainNiagaraComponent->SetAsset(RainSys);
			RainNiagaraComponent->Deactivate();
		}
	}

	UNiagaraSystem* AshSys = AshNiagaraSystem.LoadSynchronous();
	if (!AshNiagaraComponent && AshSys)
	{
		AshNiagaraComponent = NewObject<UNiagaraComponent>(Owner, TEXT("WeatherAshNiagara"));
		if (AshNiagaraComponent)
		{
			Owner->AddInstanceComponent(AshNiagaraComponent);
			AshNiagaraComponent->RegisterComponent();
			AshNiagaraComponent->SetAsset(AshSys);
			AshNiagaraComponent->Deactivate();
		}
	}
}

void UDayNightCycleSystem::DestroyEffectComponents()
{
	if (RainNiagaraComponent) { RainNiagaraComponent->DestroyComponent(); RainNiagaraComponent = nullptr; }
	if (AshNiagaraComponent) { AshNiagaraComponent->DestroyComponent(); AshNiagaraComponent = nullptr; }
	if (FogPostProcessComponent) { FogPostProcessComponent->DestroyComponent(); FogPostProcessComponent = nullptr; }
}

void UDayNightCycleSystem::UpdateEffects()
{
	// Indoor detection: suppress rain/ash particles when the player is not exposed to sky
	bool bPlayerExposedToSky = true;
	if (AWorldGenerator* WG = Cast<AWorldGenerator>(GetOwner()))
	{
		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				if (APawn* Pawn = PC->GetPawn())
				{
					const FVector Loc = Pawn->GetActorLocation();
					const float BS = WG->BlockScale;
					const int32 VX = FMath::FloorToInt(Loc.X / BS);
					const int32 VY = FMath::FloorToInt(Loc.Y / BS);
					const int32 VZ = FMath::FloorToInt(Loc.Z / BS);
					bPlayerExposedToSky = true;
					for (int32 Z = VZ + 1; Z < WG->ChunkHeight; ++Z)
					{
						uint8 BlockID = 0;
						if (WG->GetVoxelAt(VX, VY, Z, BlockID) && BlockID != 0 && !FBlockHelpers::IsNonSolidBlock(BlockID))
						{
							bPlayerExposedToSky = false;
							break;
						}
					}
				}
			}
		}
	}

	// Fog post-process weight
	if (FogPostProcessComponent)
	{
		FogPostProcessComponent->BlendWeight = CurrentFogDensity;
		UMaterialInterface* FogMat = FogPostProcessMaterial.LoadSynchronous();
		if (FogMat && FogPostProcessComponent->Settings.WeightedBlendables.Array.Num() == 0)
		{
			FWeightedBlendable Blendable;
			Blendable.Weight = 1.0f;
			Blendable.Object = FogMat;
			FogPostProcessComponent->Settings.WeightedBlendables.Array.Add(Blendable);
		}
	}

	// Rain particles (intensity lerped across transitions)
	if (RainNiagaraComponent)
	{
		const float FromRain = (CurrentWeather == EWeatherState::Rain) ? 1.0f : 0.0f;
		const float ToRain = (TargetWeather == EWeatherState::Rain) ? 1.0f : 0.0f;
		const float RainFactor = bWeatherTransitioning
			? FMath::Lerp(FromRain, ToRain, WeatherTransitionAlpha)
			: FromRain;
		if (RainFactor > 0.0f && bPlayerExposedToSky)
		{
			if (!RainNiagaraComponent->IsActive())
			{
				RainNiagaraComponent->Activate(true);
			}
			RainNiagaraComponent->SetFloatParameter(FName("SpawnRate"), static_cast<float>(RainParticleRate) * RainFactor);
		}
		else
		{
			if (RainNiagaraComponent->IsActive())
			{
				RainNiagaraComponent->Deactivate();
			}
		}
	}

	// Ash particles (intensity lerped across transitions)
	if (AshNiagaraComponent)
	{
		const float FromAsh = (CurrentWeather == EWeatherState::AshStorm) ? 1.0f : 0.0f;
		const float ToAsh = (TargetWeather == EWeatherState::AshStorm) ? 1.0f : 0.0f;
		const float AshFactor = bWeatherTransitioning
			? FMath::Lerp(FromAsh, ToAsh, WeatherTransitionAlpha)
			: FromAsh;
		if (AshFactor > 0.0f && bPlayerExposedToSky)
		{
			if (!AshNiagaraComponent->IsActive())
			{
				AshNiagaraComponent->Activate(true);
			}
			AshNiagaraComponent->SetFloatParameter(FName("SpawnRate"), static_cast<float>(AshParticleRate) * AshFactor);
		}
		else
		{
			if (AshNiagaraComponent->IsActive())
			{
				AshNiagaraComponent->Deactivate();
			}
		}
	}
}

void UDayNightCycleSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyEffectComponents();
	Super::EndPlay(EndPlayReason);
}

void UDayNightCycleSystem::SaveWeatherState(float& OutTimeOfDay, int32& OutDayCount, int32& OutCurrentWeather,
	int32& OutTargetWeather, float& OutTransitionAlpha, float& OutTimeRemaining) const
{
	OutTimeOfDay = TimeOfDay;
	OutDayCount = DayCount;
	OutCurrentWeather = static_cast<int32>(CurrentWeather);
	OutTargetWeather = static_cast<int32>(TargetWeather);
	OutTransitionAlpha = WeatherTransitionAlpha;
	OutTimeRemaining = WeatherTimeRemaining;
}

void UDayNightCycleSystem::LoadWeatherState(float InTimeOfDay, int32 InDayCount, int32 InCurrentWeather,
	int32 InTargetWeather, float InTransitionAlpha, float InTimeRemaining)
{
	TimeOfDay = InTimeOfDay;
	DayCount = InDayCount;
	CurrentWeather = static_cast<EWeatherState>(InCurrentWeather);
	TargetWeather = static_cast<EWeatherState>(InTargetWeather);
	WeatherTransitionAlpha = InTransitionAlpha;
	WeatherTimeRemaining = InTimeRemaining;
	bWeatherTransitioning = (CurrentWeather != TargetWeather);
	bWasDay = IsDay();
	UpdateSunRotation();
	// Restore interpolated values
	FWeatherStateParams From = GetWeatherParams(CurrentWeather);
	FWeatherStateParams To = GetWeatherParams(TargetWeather);
	CurrentFogDensity = FMath::Lerp(From.FogDensity, To.FogDensity, WeatherTransitionAlpha);
	CurrentAmbientLightFactor = FMath::Lerp(From.AmbientLightFactor, To.AmbientLightFactor, WeatherTransitionAlpha);
	CurrentSurfaceWetness = FMath::Lerp(From.SurfaceWetness, To.SurfaceWetness, WeatherTransitionAlpha);
	CurrentMobSpawnRateMultiplier = FMath::Lerp(From.MobSpawnRate, To.MobSpawnRate, WeatherTransitionAlpha);
	CurrentMobAggressionMultiplier = FMath::Lerp(From.MobAggression, To.MobAggression, WeatherTransitionAlpha);
	UpdateEffects();
}