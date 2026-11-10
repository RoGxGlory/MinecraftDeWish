#include "UI/LoadingScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/LamplighterUIStyle.h"

#define LOCTEXT_NAMESPACE "LamplighterUI"

ULoadingScreenWidget::ULoadingScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> ULoadingScreenWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void ULoadingScreenWidget::BuildLayout()
{
	using namespace LamplighterUI;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Root;

	// --- full-screen scrim ---
	UBorder* Scrim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Scrim"));
	Scrim->SetBrushColor(Void());
	Scrim->SetPadding(FMargin(0.f));
	if (UCanvasPanelSlot* ScrimSlot = Root->AddChildToCanvas(Scrim))
	{
		ScrimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		ScrimSlot->SetOffsets(FMargin(0.f));
	}

	// --- centre column ---
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	if (UCanvasPanelSlot* ColSlot = Root->AddChildToCanvas(Column))
	{
		ColSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		ColSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ColSlot->SetAutoSize(true);
	}

	UTextBlock* Wordmark = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Wordmark"));
	Wordmark->SetText(LOCTEXT("Title", "LAMPLIGHTER"));
	Wordmark->SetFont(DisplayFont(48));
	Wordmark->SetColorAndOpacity(FSlateColor(Bone() * 0.4f));
	Column->AddChildToVerticalBox(Wordmark);

	UTextBlock* Status = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Status"));
	Status->SetText(LOCTEXT("Status", "Restauration de l'alimentation…"));
	Status->SetFont(MonoFont(13));
	Status->SetColorAndOpacity(FSlateColor(Ash()));
	if (UVerticalBoxSlot* StatusSlot = Cast<UVerticalBoxSlot>(Column->AddChildToVerticalBox(Status)))
	{
		StatusSlot->SetPadding(FMargin(0.f, 24.f, 0.f, 0.f));
	}
}

#undef LOCTEXT_NAMESPACE
