// LAMPLIGHTER — settings screen, built entirely in C++.
// See design/ui/ui-direction.md §3.4 and §7.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/VerticalBox.h"
#include "UI/GameSettingsSubsystem.h"
#include "SettingsWidget.generated.h"

UCLASS()
class MINECRAFTDEWISH_API USettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USettingsWidget(const FObjectInitializer& ObjectInitializer);

	/** Raised when the player asks to go back. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBackRequested);
	UPROPERTY(BlueprintAssignable, Category = "Settings")
	FOnBackRequested OnBackRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();
	void ShowPage(int32 PageIndex);
	void RefreshValues();

	// --- page builders ---
	void BuildGraphicsPage(UVerticalBox* Container);
	void BuildGameplayPage(UVerticalBox* Container);
	void BuildAccessibilityPage(UVerticalBox* Container);
	void BuildControlsPage(UVerticalBox* Container);

	// --- field helpers ---
	class USlider* MakeSlider(UVerticalBox* Container, const FText& Label, float Min, float Max, float Value);
	class UCheckBox* MakeToggle(UVerticalBox* Container, const FText& Label, bool Value);
	class UComboBoxString* MakeDropdown(UVerticalBox* Container, const FText& Label, const TArray<FString>& Options, int32 Selected);

	// --- slider handlers ---
	UFUNCTION() void OnRenderDistanceChanged(float V);
	UFUNCTION() void OnWindStrengthChanged(float V);
	UFUNCTION() void OnFOVChanged(float V);
	UFUNCTION() void OnMouseSensitivityChanged(float V);
	UFUNCTION() void OnSubtitleSizeChanged(float V);
	UFUNCTION() void OnCameraShakeChanged(float V);
	UFUNCTION() void OnUITextScaleChanged(float V);

	// --- toggle handlers ---
	UFUNCTION() void OnFastLeavesChanged(bool V);
	UFUNCTION() void OnLeafParticlesChanged(bool V);
	UFUNCTION() void OnMotionBlurChanged(bool V);
	UFUNCTION() void OnSubtitlesChanged(bool V);
	UFUNCTION() void OnPauseInventoryChanged(bool V);
	UFUNCTION() void OnBulbFlickerChanged(bool V);
	UFUNCTION() void OnHUDBreathingChanged(bool V);

	// --- dropdown handlers ---
	UFUNCTION() void OnColourblindChanged(FString Item, ESelectInfo::Type Type);

	UButton* MakeTabButton(const FText& Label, int32 PageIndex);

	// --- tab handlers ---
	UFUNCTION() void HandleGraphicsTab();
	UFUNCTION() void HandleGameplayTab();
	UFUNCTION() void HandleAccessibilityTab();
	UFUNCTION() void HandleControlsTab();
	UFUNCTION() void HandleBackClicked();

	UGameSettingsSubsystem* GetSettings() const;

	UPROPERTY() TArray<TObjectPtr<UButton>> TabButtons;
	UPROPERTY() TArray<TObjectPtr<UVerticalBox>> Pages;
	UPROPERTY() TObjectPtr<UBorder> Panel;
	UPROPERTY() int32 CurrentPage = 0;
};
