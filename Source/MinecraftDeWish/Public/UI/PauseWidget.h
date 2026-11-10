// LAMPLIGHTER — pause menu overlay, built entirely in C++.
// See design/ui/ui-direction.md §3.5.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "PauseWidget.generated.h"

UCLASS()
class MINECRAFTDEWISH_API UPauseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPauseWidget(const FObjectInitializer& ObjectInitializer);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResumeRequested);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsRequested);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuitRequested);

	UPROPERTY(BlueprintAssignable, Category = "Pause")
	FOnResumeRequested OnResumeRequested;

	UPROPERTY(BlueprintAssignable, Category = "Pause")
	FOnSettingsRequested OnSettingsRequested;

	UPROPERTY(BlueprintAssignable, Category = "Pause")
	FOnQuitRequested OnQuitRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();
	UButton* MakeTextButton(const FText& Label, const TCHAR* WidgetName);

	UFUNCTION() void HandleResumeClicked();
	UFUNCTION() void HandleSettingsClicked();
	UFUNCTION() void HandleQuitClicked();

	UPROPERTY() TObjectPtr<UBorder> Panel;
};
