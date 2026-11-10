// LAMPLIGHTER — loading screen widget, built entirely in C++.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingScreenWidget.generated.h"

UCLASS()
class MINECRAFTDEWISH_API ULoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ULoadingScreenWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();
};
