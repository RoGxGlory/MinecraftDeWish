#include "DayNightCycleSystem.h"
#include "Components/LightComponent.h"

UDayNightCycleSystem::UDayNightCycleSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 10 Hz is sufficient for smooth sun movement
}

void UDayNightCycleSystem::BeginPlay()
{
	Super::BeginPlay();
	bWasDay = IsDay();
}

void UDayNightCycleSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bPauseCycle)
	{
		return;
	}

	// Advance time
	const float TimeStep = DeltaTime / CycleDurationSeconds;
	TimeOfDay += TimeStep;

	// Handle day rollover
	if (TimeOfDay >= 1.0f)
	{
		TimeOfDay -= 1.0f;
		DayCount++;
		UE_LOG(LogTemp, Log, TEXT("DayNightCycle: Day %d has begun."), DayCount);
	}

	// Detect day/night transitions
	const bool bCurrentlyDay = IsDay();
	if (bCurrentlyDay != bWasDay)
	{
		bWasDay = bCurrentlyDay;
		OnTimeChanged.Broadcast(TimeOfDay, bCurrentlyDay);

		UE_LOG(LogTemp, Log, TEXT("DayNightCycle: %s (Day %d, Time: %.3f)"),
			bCurrentlyDay ? TEXT("SUNRISE") : TEXT("SUNSET"), DayCount, TimeOfDay);
	}

	UpdateSunRotation();
}

float UDayNightCycleSystem::GetSkyLightIntensity() const
{
	// Smooth intensity curve:
	// Full brightness at noon (0.25), full darkness at midnight (0.75)
	// Smooth transitions at dawn (0.0) and dusk (0.5)
	if (IsDay())
	{
		// Dawn to noon to dusk: ramp up then down
		if (TimeOfDay < 0.05f)
		{
			// Early dawn transition
			return FMath::Lerp(0.1f, 0.5f, TimeOfDay / 0.05f);
		}
		else if (TimeOfDay < 0.45f)
		{
			// Full daylight
			return 1.0f;
		}
		else
		{
			// Dusk transition
			return FMath::Lerp(1.0f, 0.1f, (TimeOfDay - 0.45f) / 0.05f);
		}
	}
	else
	{
		// Nighttime: very low ambient
		if (TimeOfDay < 0.55f)
		{
			return FMath::Lerp(0.1f, 0.02f, (TimeOfDay - 0.5f) / 0.05f);
		}
		else if (TimeOfDay < 0.95f)
		{
			return 0.02f; // Deep night
		}
		else
		{
			// Pre-dawn brightening
			return FMath::Lerp(0.02f, 0.1f, (TimeOfDay - 0.95f) / 0.05f);
		}
	}
}

float UDayNightCycleSystem::GetSunPitchAngle() const
{
	// Map TimeOfDay to a full 360° sun arc
	// 0.0 = sunrise (pitch 0°), 0.25 = noon (pitch -90°), 0.5 = sunset (pitch -180°)
	return -TimeOfDay * 360.0f;
}

FString UDayNightCycleSystem::GetTimeString() const
{
	// Convert TimeOfDay to 24h clock starting at 6:00 AM for sunrise
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

void UDayNightCycleSystem::UpdateSunRotation()
{
	if (!SunLightActor)
	{
		return;
	}

	const float Pitch = GetSunPitchAngle();
	SunLightActor->SetActorRotation(FRotator(Pitch, -90.0f, 0.0f));

	// Modulate light intensity based on time
	TArray<UActorComponent*> LightComps;
	SunLightActor->GetComponents(ULightComponent::StaticClass(), LightComps);
	for (UActorComponent* Comp : LightComps)
	{
		if (ULightComponent* Light = Cast<ULightComponent>(Comp))
		{
			Light->SetIntensity(GetSkyLightIntensity() * 10.0f); // Base intensity × sky factor
		}
	}
}
