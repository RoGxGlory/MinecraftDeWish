// LAMPLIGHTER — title screen widget, built entirely in C++ (no Widget Blueprint).
// See design/ui/ui-direction.md §3.3 and §3.4.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/WorldDescriptorSubsystem.h"
#include "MainMenuWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNewWorldRequested, FWorldDescriptor, Descriptor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuitRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsRequested);

/**
 * The painted menu: wordmark, entry column, footer, and the New World panel.
 *
 * The widget owns no gameplay state. It broadcasts intent (OnNewWorldRequested /
 * OnQuitRequested) and the player controller performs the travel.
 */
UCLASS()
class MINECRAFTDEWISH_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMainMenuWidget(const FObjectInitializer& ObjectInitializer);

	/** Raised when the player validates the New World panel. */
	UPROPERTY(BlueprintAssignable, Category = "Menu")
	FOnNewWorldRequested OnNewWorldRequested;

	/** Raised when the player asks to quit. */
	UPROPERTY(BlueprintAssignable, Category = "Menu")
	FOnQuitRequested OnQuitRequested;

	/** Raised when the player opens settings. */
	UPROPERTY(BlueprintAssignable, Category = "Menu")
	FOnSettingsRequested OnSettingsRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Builds the widget tree once. RebuildWidget() calls it on first construction. */
	void BuildLayout();

	/** Creates a flat text button in the house style. Registers it for hover refresh. */
	class UButton* MakeTextButton(const FText& Label, const TCHAR* WidgetName, int32 FontSize);

	/** Places a button in the entry column with the house spacing. */
	void AddColumnEntry(class UVerticalBox* Column, class UButton* Entry);

	/** Applies the rest/active text colour to every registered button. */
	void RefreshEntryVisuals();

	/** Swaps between the entry column and the New World panel. */
	void ShowNewWorldPanel(bool bShow);

	UFUNCTION() void HandleNewWorldClicked();
	UFUNCTION() void HandleContinueClicked();
	UFUNCTION() void HandleSettingsClicked();
	UFUNCTION() void HandleQuitClicked();
	UFUNCTION() void HandleCreateWorldClicked();
	UFUNCTION() void HandleBackClicked();
	UFUNCTION() void HandleEntryHovered();

	/** Tuning: pitch (degrees) the angel arm should point at for each menu entry,
	 *  in the same order as EntryButtons. Editable in the widget class defaults. */
	UPROPERTY(EditAnywhere, Category = "Menu|Angel")
	TArray<float> AngelEntryPitches;

	/** Tuning: how fast the arm lerps toward its target pitch (units/sec). */
	UPROPERTY(EditAnywhere, Category = "Menu|Angel")
	float AngelInterpSpeed = 8.f;

	/** Cached pointing actor (the weeping angel's right arm BP). */
	UPROPERTY(Transient)
	TObjectPtr<class AActor> CachedAngelArm = nullptr;

	/** Current (lerped) pitch of the angel arm. */
	float CurrentAngelPitch = 0.f;

	/** Whether we have tried to resolve the angel arm actor. */
	bool bAngelArmResolved = false;

	/** Returns true if at least one saved world directory exists. */
	bool HasSavedWorlds() const;

	/** Index (in EntryButtons) of the currently hovered or focused entry, or the
	 *  first visible one when none is active. Exposed for the menu's pointing actor. */
	UFUNCTION(BlueprintPure, Category = "Menu")
	int32 GetActiveEntryIndex() const;

	UPROPERTY() TArray<TObjectPtr<class UButton>> EntryButtons;
	UPROPERTY() TObjectPtr<class UVerticalBox> Column;
	UPROPERTY() TObjectPtr<class UBorder> NewWorldPanel;
	UPROPERTY() TObjectPtr<class UEditableTextBox> NameInput;
	UPROPERTY() TObjectPtr<class UEditableTextBox> SeedInput;
	UPROPERTY() TObjectPtr<class UButton> ContinueButton;
	UPROPERTY() TObjectPtr<class UButton> SettingsButton;
};
