// LAMPLIGHTER — menu player controller: owns the title widget and the travel to gameplay.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/WorldDescriptorSubsystem.h"
#include "MenuPlayerController.generated.h"

class UMainMenuWidget;
class USettingsWidget;

/**
 * Controller used only by Lvl_MainMenu. No pawn, cursor visible, UI-only input.
 * It performs the level travel so the menu widget never touches gameplay objects.
 */
UCLASS()
class MINECRAFTDEWISH_API AMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMenuPlayerController();

protected:
	virtual void BeginPlay() override;

	/** Builds the widget, binds its intent delegates and switches to UI input. */
	void ShowMainMenu();

	UFUNCTION()
	void HandleNewWorldRequested(FWorldDescriptor Descriptor);

	UFUNCTION()
	void HandleQuitRequested();

	UFUNCTION()
	void HandleSettingsRequested();

	UFUNCTION()
	void HandleSettingsBack();

private:
	UPROPERTY()
	TObjectPtr<UMainMenuWidget> MainMenuWidget;

	UPROPERTY()
	TObjectPtr<USettingsWidget> SettingsWidget;
};
