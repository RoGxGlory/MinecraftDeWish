#include "UI/MenuPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/MainMenuWidget.h"
#include "UI/SettingsWidget.h"

namespace
{
	/** Gameplay map reached from the title screen. Kept here so the widget stays UI-only. */
	const FName GameplayLevelName(TEXT("/Game/FirstPerson/Lvl_FirstPersonTest"));
}

AMenuPlayerController::AMenuPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("MenuPlayerController: BeginPlay"));

	// The menu map has no pawn: point the view at the hand-placed gatehouse camera.
	if (AActor* MenuCamera = UGameplayStatics::GetActorOfClass(this, ACameraActor::StaticClass()))
	{
		SetViewTarget(MenuCamera);
		UE_LOG(LogTemp, Log, TEXT("MenuPlayerController: view target set to %s"), *MenuCamera->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MenuPlayerController: no MenuCamera found"));
	}

	ShowMainMenu();
}

void AMenuPlayerController::ShowMainMenu()
{
	if (!MainMenuWidget)
	{
		MainMenuWidget = CreateWidget<UMainMenuWidget>(this, UMainMenuWidget::StaticClass());
		if (MainMenuWidget)
		{
			MainMenuWidget->OnNewWorldRequested.AddDynamic(this, &AMenuPlayerController::HandleNewWorldRequested);
			MainMenuWidget->OnQuitRequested.AddDynamic(this, &AMenuPlayerController::HandleQuitRequested);
			MainMenuWidget->OnSettingsRequested.AddDynamic(this, &AMenuPlayerController::HandleSettingsRequested);
			MainMenuWidget->AddToViewport(10);
			UE_LOG(LogTemp, Log, TEXT("MenuPlayerController: main menu widget added to viewport"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MenuPlayerController: failed to create main menu widget"));
		}
	}

	SetShowMouseCursor(true);

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	// Give focus to the widget after it is on screen (the canvas panel itself is
	// not focusable, so let the widget handle focus internally).
	if (MainMenuWidget)
	{
		MainMenuWidget->SetFocus();
	}
}

void AMenuPlayerController::HandleNewWorldRequested(FWorldDescriptor Descriptor)
{
	if (UWorldDescriptorSubsystem* Descriptors = UWorldDescriptorSubsystem::Get(this))
	{
		Descriptors->SetPendingWorld(Descriptor);
	}

	// No seed in the URL: it would be visible and tamperable. The descriptor carries it.
	UGameplayStatics::OpenLevel(this, GameplayLevelName, true);
}

void AMenuPlayerController::HandleQuitRequested()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AMenuPlayerController::HandleSettingsRequested()
{
	if (!SettingsWidget)
	{
		SettingsWidget = CreateWidget<USettingsWidget>(this, USettingsWidget::StaticClass());
		if (SettingsWidget)
		{
			SettingsWidget->OnBackRequested.AddDynamic(this, &AMenuPlayerController::HandleSettingsBack);
			SettingsWidget->AddToViewport(20);
		}
	}
	else
	{
		SettingsWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void AMenuPlayerController::HandleSettingsBack()
{
	if (SettingsWidget)
	{
		SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}
