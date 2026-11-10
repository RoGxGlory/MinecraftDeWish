#include "UI/GameSettingsSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	static const TCHAR* Section = TEXT("MinecraftDeWish.GameSettings");
	static const TCHAR* IniFile = *GGameUserSettingsIni;
}

UGameSettingsSubsystem* UGameSettingsSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) { return nullptr; }
	const UWorld* World = WorldContextObject->GetWorld();
	if (!World) { return nullptr; }
	UGameInstance* GI = World->GetGameInstance();
	return GI ? GI->GetSubsystem<UGameSettingsSubsystem>() : nullptr;
}

void UGameSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Load();
}

void UGameSettingsSubsystem::Deinitialize()
{
	Save();
	Super::Deinitialize();
}

void UGameSettingsSubsystem::Load()
{
	if (!GConfig) { return; }

	int32 IntVal = 0;
	bool BoolVal = false;
	float FloatVal = 0.f;

	if (GConfig->GetInt(Section, TEXT("RenderDistance"), IntVal, IniFile)) { Settings.RenderDistance = IntVal; }
	if (GConfig->GetBool(Section, TEXT("FastLeaves"), BoolVal, IniFile)) { Settings.bFastLeaves = BoolVal; }
	if (GConfig->GetFloat(Section, TEXT("WindStrength"), FloatVal, IniFile)) { Settings.WindStrength = FloatVal; }
	if (GConfig->GetBool(Section, TEXT("LeafParticles"), BoolVal, IniFile)) { Settings.bLeafParticles = BoolVal; }
	if (GConfig->GetInt(Section, TEXT("FOV"), IntVal, IniFile)) { Settings.FOV = IntVal; }
	if (GConfig->GetBool(Section, TEXT("MotionBlur"), BoolVal, IniFile)) { Settings.bMotionBlur = BoolVal; }

	if (GConfig->GetFloat(Section, TEXT("MouseSensitivity"), FloatVal, IniFile)) { Settings.MouseSensitivity = FloatVal; }
	if (GConfig->GetBool(Section, TEXT("Subtitles"), BoolVal, IniFile)) { Settings.bSubtitles = BoolVal; }
	if (GConfig->GetFloat(Section, TEXT("SubtitleSize"), FloatVal, IniFile)) { Settings.SubtitleSize = FloatVal; }
	if (GConfig->GetFloat(Section, TEXT("CameraShake"), FloatVal, IniFile)) { Settings.CameraShake = FloatVal; }

	if (GConfig->GetInt(Section, TEXT("ColourblindMode"), IntVal, IniFile)) { Settings.ColourblindMode = IntVal; }
	if (GConfig->GetBool(Section, TEXT("PauseInventory"), BoolVal, IniFile)) { Settings.bPauseInventory = BoolVal; }
	if (GConfig->GetBool(Section, TEXT("BulbFlickerMenus"), BoolVal, IniFile)) { Settings.bBulbFlickerMenus = BoolVal; }
	if (GConfig->GetBool(Section, TEXT("HUDBreathing"), BoolVal, IniFile)) { Settings.bHUDBreathing = BoolVal; }

	if (GConfig->GetFloat(Section, TEXT("UITextScale"), FloatVal, IniFile)) { Settings.UITextScale = FloatVal; }

	ApplyClamps();
}

void UGameSettingsSubsystem::Save()
{
	if (!GConfig) { return; }

	GConfig->SetInt(Section, TEXT("RenderDistance"), Settings.RenderDistance, IniFile);
	GConfig->SetBool(Section, TEXT("FastLeaves"), Settings.bFastLeaves, IniFile);
	GConfig->SetFloat(Section, TEXT("WindStrength"), Settings.WindStrength, IniFile);
	GConfig->SetBool(Section, TEXT("LeafParticles"), Settings.bLeafParticles, IniFile);
	GConfig->SetInt(Section, TEXT("FOV"), Settings.FOV, IniFile);
	GConfig->SetBool(Section, TEXT("MotionBlur"), Settings.bMotionBlur, IniFile);

	GConfig->SetFloat(Section, TEXT("MouseSensitivity"), Settings.MouseSensitivity, IniFile);
	GConfig->SetBool(Section, TEXT("Subtitles"), Settings.bSubtitles, IniFile);
	GConfig->SetFloat(Section, TEXT("SubtitleSize"), Settings.SubtitleSize, IniFile);
	GConfig->SetFloat(Section, TEXT("CameraShake"), Settings.CameraShake, IniFile);

	GConfig->SetInt(Section, TEXT("ColourblindMode"), Settings.ColourblindMode, IniFile);
	GConfig->SetBool(Section, TEXT("PauseInventory"), Settings.bPauseInventory, IniFile);
	GConfig->SetBool(Section, TEXT("BulbFlickerMenus"), Settings.bBulbFlickerMenus, IniFile);
	GConfig->SetBool(Section, TEXT("HUDBreathing"), Settings.bHUDBreathing, IniFile);

	GConfig->SetFloat(Section, TEXT("UITextScale"), Settings.UITextScale, IniFile);

	GConfig->Flush(false, IniFile);

	OnSettingsChanged.Broadcast();
}

void UGameSettingsSubsystem::ApplyClamps()
{
	Settings.RenderDistance = FMath::Clamp(Settings.RenderDistance, 2, 64);
	Settings.WindStrength = FMath::Clamp(Settings.WindStrength, 0.0f, 2.0f);
	Settings.FOV = FMath::Clamp(Settings.FOV, 70, 110);
	Settings.MouseSensitivity = FMath::Clamp(Settings.MouseSensitivity, 0.2f, 3.0f);
	Settings.SubtitleSize = FMath::Clamp(Settings.SubtitleSize, 0.85f, 1.30f);
	Settings.CameraShake = FMath::Clamp(Settings.CameraShake, 0.0f, 1.0f);
	Settings.ColourblindMode = FMath::Clamp(Settings.ColourblindMode, 0, 3);
	Settings.UITextScale = FMath::Clamp(Settings.UITextScale, 0.85f, 1.30f);
}

// --- Typed setters (clamp + save + notify) ---

void UGameSettingsSubsystem::SetRenderDistance(int32 Value)
{
	Settings.RenderDistance = FMath::Clamp(Value, 2, 64);
	Save();
}

void UGameSettingsSubsystem::SetFastLeaves(bool Value)
{
	Settings.bFastLeaves = Value;
	Save();
}

void UGameSettingsSubsystem::SetWindStrength(float Value)
{
	Settings.WindStrength = FMath::Clamp(Value, 0.0f, 2.0f);
	Save();
}

void UGameSettingsSubsystem::SetLeafParticles(bool Value)
{
	Settings.bLeafParticles = Value;
	Save();
}

void UGameSettingsSubsystem::SetFOV(int32 Value)
{
	Settings.FOV = FMath::Clamp(Value, 70, 110);
	Save();
}

void UGameSettingsSubsystem::SetMotionBlur(bool Value)
{
	Settings.bMotionBlur = Value;
	Save();
}

void UGameSettingsSubsystem::SetMouseSensitivity(float Value)
{
	Settings.MouseSensitivity = FMath::Clamp(Value, 0.2f, 3.0f);
	Save();
}

void UGameSettingsSubsystem::SetSubtitles(bool Value)
{
	Settings.bSubtitles = Value;
	Save();
}

void UGameSettingsSubsystem::SetSubtitleSize(float Value)
{
	Settings.SubtitleSize = FMath::Clamp(Value, 0.85f, 1.30f);
	Save();
}

void UGameSettingsSubsystem::SetCameraShake(float Value)
{
	Settings.CameraShake = FMath::Clamp(Value, 0.0f, 1.0f);
	Save();
}

void UGameSettingsSubsystem::SetColourblindMode(int32 Value)
{
	Settings.ColourblindMode = FMath::Clamp(Value, 0, 3);
	Save();
}

void UGameSettingsSubsystem::SetPauseInventory(bool Value)
{
	Settings.bPauseInventory = Value;
	Save();
}

void UGameSettingsSubsystem::SetBulbFlickerMenus(bool Value)
{
	Settings.bBulbFlickerMenus = Value;
	Save();
}

void UGameSettingsSubsystem::SetHUDBreathing(bool Value)
{
	Settings.bHUDBreathing = Value;
	Save();
}

void UGameSettingsSubsystem::SetUITextScale(float Value)
{
	Settings.UITextScale = FMath::Clamp(Value, 0.85f, 1.30f);
	Save();
}
