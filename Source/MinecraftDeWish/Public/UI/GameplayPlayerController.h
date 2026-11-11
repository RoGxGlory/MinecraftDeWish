// LAMPLIGHTER — gameplay player controller: owns the pause widget and input.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/InventoryWidget.h"
#include "UI/PauseWidget.h"
#include "UI/SettingsWidget.h"
#include "GameplayPlayerController.generated.h"

UCLASS()
class MINECRAFTDEWISH_API AGameplayPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGameplayPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UFUNCTION()
	void HandlePausePressed();

	UFUNCTION()
	void HandleInventoryPressed();

	UFUNCTION()
	void HandlePlacePressed();

	void ShowInventory();
	void HideInventory();

	UFUNCTION()
	void HandlePauseWidgetResume();

	UFUNCTION()
	void HandlePauseWidgetSettings();

	UFUNCTION()
	void HandlePauseWidgetQuit();

	void ShowPauseMenu();
	void HidePauseMenu();

	UPROPERTY()
	TObjectPtr<UPauseWidget> PauseWidget;

	UPROPERTY()
	TObjectPtr<USettingsWidget> SettingsWidget;

	UPROPERTY()
	TObjectPtr<UInventoryWidget> InventoryWidget;

	bool bInventoryOpen = false;

	bool bIsPaused = false;
};
