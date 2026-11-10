#include "UI/InventoryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/LamplighterUIStyle.h"

#define LOCTEXT_NAMESPACE "LamplighterUI"

UInventoryWidget::UInventoryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UInventoryWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UInventoryWidget::BuildLayout()
{
	using namespace LamplighterUI;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Root;

	// --- scrim (dimmed, not blacked out) ---
	UBorder* Scrim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Scrim"));
	Scrim->SetBrushColor(FLinearColor(0.043f, 0.055f, 0.067f, 0.55f));
	Scrim->SetPadding(FMargin(0.f));
	if (UCanvasPanelSlot* ScrimSlot = Root->AddChildToCanvas(Scrim))
	{
		ScrimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		ScrimSlot->SetOffsets(FMargin(0.f));
	}

	// --- panel ---
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	PanelBorder->SetBrushColor(Surface());
	PanelBorder->SetPadding(FMargin(32.f, 28.f));
	if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetAutoSize(true);
	}
	Panel = PanelBorder;

	UVerticalBox* PanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelBox"));
	PanelBorder->SetContent(PanelBox);

	// --- title ---
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(LOCTEXT("InventoryTitle", "INVENTAIRE"));
	Title->SetFont(MonoFont(11));
	Title->SetColorAndOpacity(FSlateColor(Ash()));
	PanelBox->AddChildToVerticalBox(Title);

	// --- 36-slot grid (6 columns x 6 rows) ---
	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("Grid"));
	Grid->SetSlotPadding(FMargin(2.f));
	if (UVerticalBoxSlot* GridSlot = Cast<UVerticalBoxSlot>(PanelBox->AddChildToVerticalBox(Grid)))
	{
		GridSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}

	for (int32 i = 0; i < 36; i++)
	{
		UBorder* SlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*FString::Printf(TEXT("Slot%d"), i)));
		SlotBorder->SetBrushColor(Raised());
		SlotBorder->SetPadding(FMargin(4.f));

		UUniformGridSlot* GridSlot = Grid->AddChildToUniformGrid(SlotBorder, i / 6, i % 6);
		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	// --- hint ---
	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Hint"));
	Hint->SetText(LOCTEXT("InventoryHint", "Tab pour fermer"));
	Hint->SetFont(MonoFont(11));
	Hint->SetColorAndOpacity(FSlateColor(Ash()));
	if (UVerticalBoxSlot* HintSlot = Cast<UVerticalBoxSlot>(PanelBox->AddChildToVerticalBox(Hint)))
	{
		HintSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}
}

#undef LOCTEXT_NAMESPACE
