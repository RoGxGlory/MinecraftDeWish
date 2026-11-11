#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DayNightCycleSystem.generated.h"

class UNiagaraComponent;
class UPostProcessComponent;
class UMaterialInterface;
class UNiagaraSystem;

/** Delegate for time-of-day changes (fired every in-game minute) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeChanged, float, TimeOfDay, bool, bIsDay);

/** Delegate fired when the weather state changes (after transition completes) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeatherChanged, int32, NewWeather, int32, OldWeather);

/** Delegate fired when a weather transition starts */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeatherTransitionStarted, int32, FromWeather, int32, ToWeather, float, Duration);

/** Delegate fired when fog density changes (for UI / post-process weight) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFogDensityChanged, float, FogDensity);

/**
 * EWeatherState
 *
 * The five atmospheric states supported by the weather system.
 */
UENUM(BlueprintType)
enum class EWeatherState : uint8
{
	Clear      = 0,
	Overcast   = 1,
	Rain       = 2,
	Fog        = 3,
	AshStorm   = 4
};

/**
 * FWeatherStateParams
 *
 * Per-state target values used to lerp fog, light, particles and mob modifiers
 * during a transition.
 */
USTRUCT(BlueprintType)
struct FWeatherStateParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float FogDensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float VisibilityBlocks = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float AmbientLightFactor = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float SurfaceWetness = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float ParticleSpawnDust = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float RainParticleRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float AshParticleRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float MobAggression = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float MobSpawnRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float DurationMinSeconds = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float DurationMaxSeconds = 240.0f;
};

/**
 * UDayNightCycleSystem
 *
 * Manages a 20-minute day/night cycle and the dynamic weather state machine.
 * Weather states transition smoothly (30-60s lerp) and affect fog density,
 * ambient light, surface wetness, particle spawn rates and mob behaviour.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINECRAFTDEWISH_API UDayNightCycleSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UDayNightCycleSystem();

	// ==================== DAY/NIGHT SETTINGS ====================

	/** Full cycle duration in seconds (default 1200 = 20 minutes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight|Settings", meta = (ClampMin = "600", ClampMax = "2400"))
	float CycleDurationSeconds = 1200.0f;

	/** Current time of day [0.0 - 1.0] */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DayNight|State")
	float TimeOfDay = 0.05f; // Start shortly after sunrise

	/** Whether to pause the cycle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight|Settings")
	bool bPauseCycle = false;

	/** Reference to the directional light (sun) to rotate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight|Settings")
	AActor* SunLightActor = nullptr;

	// ==================== WEATHER SETTINGS (TUNING KNOBS) ====================

	/** Weather change lerp time in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Settings", meta = (ClampMin = "15", ClampMax = "120"))
	float WeatherTransitionDuration = 45.0f;

	/** Maximum fog density (used to scale the Fog state) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Settings", meta = (ClampMin = "0.3", ClampMax = "1.0"))
	float FogDensityMax = 0.8f;

	/** Chance of an ash storm at night (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Settings", meta = (ClampMin = "0.0", ClampMax = "0.2"))
	float AshStormProbability = 0.05f;

	/** Chance of a clear sky (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Settings", meta = (ClampMin = "0.0", ClampMax = "0.3"))
	float ClearSkyProbability = 0.10f;

	/** Rain particle spawn rate (particles/sec) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Settings", meta = (ClampMin = "100", ClampMax = "2000"))
	int32 RainParticleRate = 500;

	/** Ash particle spawn rate (particles/sec) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Settings", meta = (ClampMin = "50", ClampMax = "1000"))
	int32 AshParticleRate = 300;

	/** Mob aggression multiplier in fog */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Settings", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	float MobAggressionFogMultiplier = 1.2f;

	/** Minimum seconds before weather can change again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Settings", meta = (ClampMin = "0", ClampMax = "300"))
	float MinWeatherDurationSeconds = 0.0f;

	// ==================== WEATHER STATE (RUNTIME) ====================

	/** Current (settled) weather state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|State")
	EWeatherState CurrentWeather = EWeatherState::Overcast;

	/** Weather state we are transitioning towards */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|State")
	EWeatherState TargetWeather = EWeatherState::Overcast;

	/** True while a transition is in progress */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|State")
	bool bWeatherTransitioning = false;

	/** Transition progress [0.0 - 1.0] */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|State")
	float WeatherTransitionAlpha = 1.0f;

	/** Seconds remaining before the next weather change is allowed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|State")
	float WeatherTimeRemaining = 300.0f;

	// ==================== INTERPOLATED VALUES (RUNTIME) ====================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|State")
	float CurrentFogDensity = 0.3f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|State")
	float CurrentAmbientLightFactor = 0.7f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|State")
	float CurrentSurfaceWetness = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|State")
	float CurrentMobSpawnRateMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weather|State")
	float CurrentMobAggressionMultiplier = 1.0f;

	// ==================== CONTENT REFERENCES ====================

	/** Fog post-process material (M_PP_Fog). Assign in editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Content")
	TSoftObjectPtr<UMaterialInterface> FogPostProcessMaterial;

	/** Rain Niagara system (NS_Rain). Assign in editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Content")
	TSoftObjectPtr<UNiagaraSystem> RainNiagaraSystem;

	/** Ash storm Niagara system (NS_AshStorm). Assign in editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Content")
	TSoftObjectPtr<UNiagaraSystem> AshNiagaraSystem;

	// ==================== EVENTS ====================

	UPROPERTY(BlueprintAssignable, Category = "DayNight|Events")
	FOnTimeChanged OnTimeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weather|Events")
	FOnWeatherChanged OnWeatherChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weather|Events")
	FOnWeatherTransitionStarted OnWeatherTransitionStarted;

	UPROPERTY(BlueprintAssignable, Category = "Weather|Events")
	FOnFogDensityChanged OnFogDensityChanged;

	// ==================== QUERIES ====================

	/** Returns true during daytime [0.0, 0.5) */
	UFUNCTION(BlueprintPure, Category = "DayNight")
	bool IsDay() const { return TimeOfDay >= 0.0f && TimeOfDay < 0.5f; }

	/** Returns true during nighttime [0.5, 1.0) */
	UFUNCTION(BlueprintPure, Category = "DayNight")
	bool IsNight() const { return !IsDay(); }

	/** Returns sky light intensity [0.0 - 1.0] for light level calculations */
	UFUNCTION(BlueprintPure, Category = "DayNight")
	float GetSkyLightIntensity() const;

	/** Returns sun pitch angle in degrees for directional light rotation */
	UFUNCTION(BlueprintPure, Category = "DayNight")
	float GetSunPitchAngle() const;

	/** Returns a human-readable time string (e.g., "Day 1, 12:00") */
	UFUNCTION(BlueprintPure, Category = "DayNight")
	FString GetTimeString() const;

	/** Returns the current day count (starting from 1) */
	UFUNCTION(BlueprintPure, Category = "DayNight")
	int32 GetDayCount() const { return DayCount; }

	/** Manually set time of day [0.0 - 1.0] */
	UFUNCTION(BlueprintCallable, Category = "DayNight")
	void SetTimeOfDay(float NewTime);

	/** Returns the current weather state as int32 (for Blueprint) */
	UFUNCTION(BlueprintPure, Category = "Weather")
	int32 GetWeatherState() const { return static_cast<int32>(CurrentWeather); }

	/** Returns the target weather state as int32 */
	UFUNCTION(BlueprintPure, Category = "Weather")
	int32 GetTargetWeatherState() const { return static_cast<int32>(TargetWeather); }

	/** Returns fog density [0.0 - 1.0] */
	UFUNCTION(BlueprintPure, Category = "Weather")
	float GetFogDensity() const { return CurrentFogDensity; }

	/** Returns ambient light factor (weather) [0.0 - 1.0] */
	UFUNCTION(BlueprintPure, Category = "Weather")
	float GetAmbientLightFactor() const { return CurrentAmbientLightFactor; }

	/** Returns surface wetness [0.0 - 1.0] */
	UFUNCTION(BlueprintPure, Category = "Weather")
	float GetSurfaceWetness() const { return CurrentSurfaceWetness; }

	/** Returns mob spawn rate multiplier for the current weather */
	UFUNCTION(BlueprintPure, Category = "Weather")
	float GetMobSpawnRateMultiplier() const { return CurrentMobSpawnRateMultiplier; }

	/** Returns mob aggression multiplier for the current weather */
	UFUNCTION(BlueprintPure, Category = "Weather")
	float GetMobAggressionMultiplier() const { return CurrentMobAggressionMultiplier; }

	/** Returns visibility in blocks for the current weather */
	UFUNCTION(BlueprintPure, Category = "Weather")
	float GetVisibilityBlocks() const;

	/** Manually force a weather state (starts a transition) */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetWeather(EWeatherState NewWeather, bool bImmediate = false);

	/** Returns params for a given weather state */
	UFUNCTION(BlueprintPure, Category = "Weather")
	FWeatherStateParams GetWeatherParams(EWeatherState State) const;

	/** Save/restore helpers */
	UFUNCTION(BlueprintCallable, Category = "Weather|Save")
	void SaveWeatherState(float& OutTimeOfDay, int32& OutDayCount, int32& OutCurrentWeather,
		int32& OutTargetWeather, float& OutTransitionAlpha, float& OutTimeRemaining) const;

	UFUNCTION(BlueprintCallable, Category = "Weather|Save")
	void LoadWeatherState(float InTimeOfDay, int32 InDayCount, int32 InCurrentWeather,
		int32 InTargetWeather, float InTransitionAlpha, float InTimeRemaining);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	int32 DayCount = 1;
	bool bWasDay = true;

	void UpdateSunRotation();
	void UpdateWeather(float DeltaTime);
	void SelectNewWeather();
	float ComputeWeatherWeight(EWeatherState State) const;
	void ApplyWeatherTransition(float Alpha);
	void StartTransition(EWeatherState ToState);
	void UpdateEffects();
	void CreateEffectComponents();
	void DestroyEffectComponents();

	EWeatherState PickWeightedWeather() const;

	UPROPERTY()
	TObjectPtr<UPostProcessComponent> FogPostProcessComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> RainNiagaraComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> AshNiagaraComponent = nullptr;

	float TransitionElapsed = 0.0f;
	float TransitionDuration = 45.0f;
};