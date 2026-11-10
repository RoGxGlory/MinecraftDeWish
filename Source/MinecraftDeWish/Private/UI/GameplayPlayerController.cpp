#include "UI/GameplayPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

AGameplayPlayerController::AGameplayPlayerController()
{
	bShowMouseCursor = false;
}

void AGameplayPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("GameplayPlayerController: BeginPlay called"));

	// Ensure input mode is GameOnly (the menu leaves it in UI-only).
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;

	// Add IMC_UI to the enhanced input subsystem.
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (UInputMappingContext* IMC = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_UI.IMC_UI")))
			{
				Subsystem->AddMappingContext(IMC, 100);
				UE_LOG(LogTemp, Log, TEXT("GameplayPlayerController: IMC_UI added successfully"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("GameplayPlayerController: failed to load IMC_UI"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("GameplayPlayerController: no EnhancedInput subsystem"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GameplayPlayerController: no local player"));
	}

	// Pre-create the pause widget (hidden).
	PauseWidget = CreateWidget<UPauseWidget>(this, UPauseWidget::StaticClass());
	if (PauseWidget)
	{
		PauseWidget->OnResumeRequested.AddDynamic(this, &AGameplayPlayerController::HandlePauseWidgetResume);
		PauseWidget->OnSettingsRequested.AddDynamic(this, &AGameplayPlayerController::HandlePauseWidgetSettings);
		PauseWidget->OnQuitRequested.AddDynamic(this, &AGameplayPlayerController::HandlePauseWidgetQuit);
		PauseWidget->AddToViewport(50);
		PauseWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	SettingsWidget = CreateWidget<USettingsWidget>(this, USettingsWidget::StaticClass());
	if (SettingsWidget)
	{
		SettingsWidget->OnBackRequested.AddDynamic(this, &AGameplayPlayerController::HandlePauseWidgetSettings);
		SettingsWidget->AddToViewport(60);
		SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	InventoryWidget = CreateWidget<UInventoryWidget>(this, UInventoryWidget::StaticClass());
	if (InventoryWidget)
	{
		InventoryWidget->AddToViewport(40);
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AGameplayPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UE_LOG(LogTemp, Log, TEXT("GameplayPlayerController: SetupInputComponent called"));

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		UE_LOG(LogTemp, Log, TEXT("GameplayPlayerController: EnhancedInputComponent found"));

		// Add gameplay input contexts here (in case Blueprint's BeginPlay doesn't fire during travel).
		if (ULocalPlayer* LP = GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				const TCHAR* Contexts[] = {
					TEXT("/Game/Input/IMC_Default.IMC_Default"),
					TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"),
					TEXT("/Game/Variant_Horror/Input/IMC_Horror.IMC_Horror"),
					TEXT("/Game/Input/IMC_UI.IMC_UI")
				};
				for (const TCHAR* Path : Contexts)
				{
					if (UInputMappingContext* IMC = LoadObject<UInputMappingContext>(nullptr, Path))
					{
						Subsystem->AddMappingContext(IMC, 0);
						UE_LOG(LogTemp, Log, TEXT("GameplayPlayerController: added mapping context %s"), Path);
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("GameplayPlayerController: failed to load %s"), Path);
					}
				}
			}
		}

		if (UInputAction* PauseAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_PauseMenu.IA_PauseMenu")))
		{
			EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &AGameplayPlayerController::HandlePausePressed);
		}

		if (UInputAction* InventoryAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_Inventory.IA_Inventory")))
		{
			EIC->BindAction(InventoryAction, ETriggerEvent::Started, this, &AGameplayPlayerController::HandleInventoryPressed);
		}
	}
}

void AGameplayPlayerController::HandlePausePressed()
{
	if (bIsPaused)
	{
		HidePauseMenu();
	}
	else
	{
		ShowPauseMenu();
	}
}

void AGameplayPlayerController::ShowPauseMenu()
{
	bIsPaused = true;
	bShowMouseCursor = true;

	if (PauseWidget)
	{
		PauseWidget->SetVisibility(ESlateVisibility::Visible);
	}

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	if (PauseWidget)
	{
		PauseWidget->SetFocus();
	}

	// Defer SetPause to avoid re-entrancy during input handling.
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		SetPause(true);
	}));
}

void AGameplayPlayerController::HidePauseMenu()
{
	bIsPaused = false;
	bShowMouseCursor = false;

	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		SetPause(false);
	}));

	if (PauseWidget)
	{
		PauseWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsWidget)
	{
		SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void AGameplayPlayerController::HandlePauseWidgetResume()
{
	HidePauseMenu();
}

void AGameplayPlayerController::HandlePauseWidgetSettings()
{
	// Toggle settings from pause menu.
	if (SettingsWidget)
	{
		if (SettingsWidget->GetVisibility() == ESlateVisibility::Visible)
		{
			SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			SettingsWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void AGameplayPlayerController::HandlePauseWidgetQuit()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AGameplayPlayerController::HandleInventoryPressed()
{
	if (bInventoryOpen)
	{
		HideInventory();
	}
	else
	{
		ShowInventory();
	}
}

void AGameplayPlayerController::ShowInventory()
{
	bInventoryOpen = true;
	bShowMouseCursor = true;

	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Visible);
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (InventoryWidget)
	{
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
	}
	SetInputMode(InputMode);
}

void AGameplayPlayerController::HideInventory()
{
	bInventoryOpen = false;
	bShowMouseCursor = false;

	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}
