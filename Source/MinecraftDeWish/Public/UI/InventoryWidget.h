// LAMPLIGHTER — inventory screen (36 slots + crafting), built entirely in C++.
// See design/ui/ui-direction.md §3.5 and design/gdd/items.md.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

UCLASS()
class MINECRAFTDEWISH_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UInventoryWidget(const FObjectInitializer& ObjectInitializer);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClosed);
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnClosed OnClosed;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();

	UPROPERTY() TObjectPtr<class UBorder> Panel;
};
