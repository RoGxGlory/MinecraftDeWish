// LAMPLIGHTER — game settings subsystem: the single writer of [MinecraftDeWish.GameSettings].
//
// WorldGenerator used to call GConfig->Set* + Flush directly against
// GGameUserSettingsIni, racing with UGameUserSettings::SaveSettings. This
// subsystem is now the only code path that touches that file for game settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameSettingsSubsystem.generated.h"

/** All game settings persisted in [MinecraftDeWish.GameSettings]. */
USTRUCT(BlueprintType)
struct FGameSettings
{
	GENERATED_BODY()

	// --- Graphics ---
	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	int32 RenderDistance = 8;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	bool bFastLeaves = true;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	float WindStrength = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	bool bLeafParticles = true;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	int32 FOV = 90;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics")
	bool bMotionBlur = false;

	// --- Gameplay ---
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	float MouseSensitivity = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	bool bSubtitles = true;

	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	float SubtitleSize = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	float CameraShake = 1.0f;

	// --- Accessibility ---
	UPROPERTY(BlueprintReadWrite, Category = "Accessibility")
	int32 ColourblindMode = 0; // 0=Off, 1=Deuteranopia, 2=Protanopia, 3=Tritanopia

	UPROPERTY(BlueprintReadWrite, Category = "Accessibility")
	bool bPauseInventory = false; // assist toggle

	UPROPERTY(BlueprintReadWrite, Category = "Accessibility")
	bool bBulbFlickerMenus = true;

	UPROPERTY(BlueprintReadWrite, Category = "Accessibility")
	bool bHUDBreathing = true;

	// --- UI ---
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	float UITextScale = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameSettingsChanged);

UCLASS()
class MINECRAFTDEWISH_API UGameSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Load settings from ini (called on init). */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void Load();

	/** Persist settings to ini. The ONLY writer of [MinecraftDeWish.GameSettings]. */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void Save();

	/** The live settings struct. Mutate then call Save() (or use the typed setters). */
	UFUNCTION(BlueprintPure, Category = "Settings")
	const FGameSettings& GetSettings() const { return Settings; }

	UFUNCTION(BlueprintPure, Category = "Settings")
	FGameSettings& GetMutableSettings() { return Settings; }

	// --- Typed accessors (used by WorldGenerator and other consumers) ---

	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	int32 GetRenderDistance() const { return Settings.RenderDistance; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetRenderDistance(int32 Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	bool GetFastLeaves() const { return Settings.bFastLeaves; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetFastLeaves(bool Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	float GetWindStrength() const { return Settings.WindStrength; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetWindStrength(float Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	bool GetLeafParticles() const { return Settings.bLeafParticles; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetLeafParticles(bool Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	int32 GetFOV() const { return Settings.FOV; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetFOV(int32 Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	bool GetMotionBlur() const { return Settings.bMotionBlur; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetMotionBlur(bool Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	float GetMouseSensitivity() const { return Settings.MouseSensitivity; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetMouseSensitivity(float Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	bool GetSubtitles() const { return Settings.bSubtitles; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetSubtitles(bool Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	float GetSubtitleSize() const { return Settings.SubtitleSize; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetSubtitleSize(float Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	float GetCameraShake() const { return Settings.CameraShake; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetCameraShake(float Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Accessibility")
	int32 GetColourblindMode() const { return Settings.ColourblindMode; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetColourblindMode(int32 Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Accessibility")
	bool GetPauseInventory() const { return Settings.bPauseInventory; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetPauseInventory(bool Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Accessibility")
	bool GetBulbFlickerMenus() const { return Settings.bBulbFlickerMenus; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetBulbFlickerMenus(bool Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Accessibility")
	bool GetHUDBreathing() const { return Settings.bHUDBreathing; }
	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void SetHUDBreathing(bool Value);

	UFUNCTION(BlueprintPure, Category = "Settings|UI")
	float GetUITextScale() const { return Settings.UITextScale; }
	UFUNCTION(BlueprintCallable, Category = "Settings|UI")
	void SetUITextScale(float Value);

	/** Convenience accessor. */
	static UGameSettingsSubsystem* Get(const UObject* WorldContextObject);

	/** Fired whenever a setting changes (after Save). */
	UPROPERTY(BlueprintAssignable, Category = "Settings")
	FOnGameSettingsChanged OnSettingsChanged;

private:
	UPROPERTY()
	FGameSettings Settings;

	void ApplyClamps();
};
