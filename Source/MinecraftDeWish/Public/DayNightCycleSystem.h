#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DayNightCycleSystem.generated.h"

/** Delegate for time-of-day changes (fired every in-game minute) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeChanged, float, TimeOfDay, bool, bIsDay);

/**
 * UDayNightCycleSystem
 *
 * Manages a 20-minute day/night cycle (10 min day + 10 min night) matching Minecraft's timing.
 * Provides time queries for mob spawning/burning, sky light modulation, and sun rotation.
 *
 * TimeOfDay range: [0.0 = sunrise, 0.25 = noon, 0.5 = sunset, 0.75 = midnight, 1.0 = next sunrise]
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MINECRAFTDEWISH_API UDayNightCycleSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UDayNightCycleSystem();

	/** Full cycle duration in seconds (default 1200 = 20 minutes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DayNight|Settings", meta = (ClampMin = "60", ClampMax = "7200"))
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

	// ==================== EVENTS ====================

	UPROPERTY(BlueprintAssignable, Category = "DayNight|Events")
	FOnTimeChanged OnTimeChanged;

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

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	int32 DayCount = 1;
	bool bWasDay = true;

	void UpdateSunRotation();
};
