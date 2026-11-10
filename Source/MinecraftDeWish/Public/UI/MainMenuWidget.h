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

	/** Returns true if at least one saved world directory exists. */
	bool HasSavedWorlds() const;

	UPROPERTY() TArray<TObjectPtr<class UButton>> EntryButtons;
	UPROPERTY() TObjectPtr<class UVerticalBox> Column;
	UPROPERTY() TObjectPtr<class UBorder> NewWorldPanel;
	UPROPERTY() TObjectPtr<class UEditableTextBox> NameInput;
	UPROPERTY() TObjectPtr<class UEditableTextBox> SeedInput;
	UPROPERTY() TObjectPtr<class UButton> ContinueButton;
	UPROPERTY() TObjectPtr<class UButton> SettingsButton;
};
