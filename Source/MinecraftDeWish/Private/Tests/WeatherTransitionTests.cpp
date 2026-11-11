#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DayNightCycleSystem.h"

/**
 * Verifies that weather transitions are smooth: a non-immediate SetWeather keeps
 * the current (old) interpolated values and only updates the target, while an
 * immediate switch snaps every interpolated value to the target state's params.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeatherTransitionSmoothTest,
	"MinecraftDeWish.Weather.TransitionSmooth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeatherTransitionSmoothTest::RunTest(const FString& Parameters)
{
	UDayNightCycleSystem* Weather = NewObject<UDayNightCycleSystem>(GetTransientPackage());
	TestNotNull(TEXT("weather system created"), Weather);
	if (!Weather)
	{
		return false;
	}

	// Immediate switch: state snaps, transition flag cleared, alpha full.
	Weather->SetWeather(EWeatherState::Rain, true);
	TestEqual(TEXT("current weather is Rain after immediate switch"), Weather->CurrentWeather, EWeatherState::Rain);
	TestEqual(TEXT("target weather matches after immediate switch"), Weather->TargetWeather, EWeatherState::Rain);
	TestFalse(TEXT("not transitioning after immediate switch"), Weather->bWeatherTransitioning);
	TestEqual(TEXT("transition alpha is 1.0 after immediate switch"), Weather->WeatherTransitionAlpha, 1.0f);

	const FWeatherStateParams RainParams = Weather->GetWeatherParams(EWeatherState::Rain);
	TestEqual(TEXT("fog density snaps to Rain params"), Weather->CurrentFogDensity, RainParams.FogDensity);
	TestEqual(TEXT("ambient light snaps to Rain params"), Weather->CurrentAmbientLightFactor, RainParams.AmbientLightFactor);
	TestEqual(TEXT("surface wetness snaps to Rain params"), Weather->CurrentSurfaceWetness, RainParams.SurfaceWetness);

	// Non-immediate switch: old state kept, target updated, transition started.
	Weather->SetWeather(EWeatherState::Fog, false);
	TestEqual(TEXT("current weather unchanged during transition"), Weather->CurrentWeather, EWeatherState::Rain);
	TestEqual(TEXT("target weather updated during transition"), Weather->TargetWeather, EWeatherState::Fog);
	TestTrue(TEXT("transitioning flag set"), Weather->bWeatherTransitioning);
	TestEqual(TEXT("fog density unchanged until first tick"), Weather->CurrentFogDensity, RainParams.FogDensity);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS