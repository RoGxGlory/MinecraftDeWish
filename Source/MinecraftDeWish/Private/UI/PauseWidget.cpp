#include "UI/PauseWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "UI/LamplighterUIStyle.h"

#define LOCTEXT_NAMESPACE "LamplighterUI"

namespace
{
	FSlateBrush MakeNoBrushP()
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::NoDrawType;
		return B;
	}

	FButtonStyle MakeFlatButtonP()
	{
		FButtonStyle S;
		const FSlateBrush No = MakeNoBrushP();
		S.SetNormal(No).SetHovered(No).SetPressed(No).SetDisabled(No);
		S.NormalPadding = FMargin(0.f, 9.f);
		S.PressedPadding = FMargin(0.f, 9.f);
		return S;
	}
}

UPauseWidget::UPauseWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UPauseWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UPauseWidget::BuildLayout()
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
	Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(Surface());
	Panel->SetPadding(FMargin(56.f, 54.f));
	if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetAutoSize(true);
	}

	UVerticalBox* PanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelBox"));
	Panel->SetContent(PanelBox);

	// --- kicker ---
	UTextBlock* Kicker = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Kicker"));
	Kicker->SetText(LOCTEXT("PauseKicker", "EN PAUSE"));
	Kicker->SetFont(MonoFont(11));
	Kicker->SetColorAndOpacity(FSlateColor(Ash()));
	PanelBox->AddChildToVerticalBox(Kicker);

	// --- title ---
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(LOCTEXT("PauseTitle", "REPRENDRE SOUFFLE"));
	Title->SetFont(DisplayFont(26));
	Title->SetColorAndOpacity(FSlateColor(Bone()));
	if (UVerticalBoxSlot* TitleSlot = Cast<UVerticalBoxSlot>(PanelBox->AddChildToVerticalBox(Title)))
	{
		TitleSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}

	// --- entries ---
	UButton* ResumeBtn = MakeTextButton(LOCTEXT("Resume", "Reprendre"), TEXT("ResumeButton"));
	ResumeBtn->OnClicked.AddDynamic(this, &UPauseWidget::HandleResumeClicked);
	if (UVerticalBoxSlot* S = Cast<UVerticalBoxSlot>(PanelBox->AddChildToVerticalBox(ResumeBtn)))
	{
		S->SetPadding(FMargin(0.f, 34.f, 0.f, 0.f));
	}

	UButton* SettingsBtn = MakeTextButton(LOCTEXT("Settings", "Réglages"), TEXT("SettingsButton"));
	SettingsBtn->OnClicked.AddDynamic(this, &UPauseWidget::HandleSettingsClicked);
	PanelBox->AddChildToVerticalBox(SettingsBtn);

	UButton* QuitBtn = MakeTextButton(LOCTEXT("Quit", "Quitter"), TEXT("QuitButton"));
	QuitBtn->OnClicked.AddDynamic(this, &UPauseWidget::HandleQuitClicked);
	PanelBox->AddChildToVerticalBox(QuitBtn);
}

UButton* UPauseWidget::MakeTextButton(const FText& Label, const TCHAR* WidgetName)
{
	using namespace LamplighterUI;

	UButton* Entry = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(WidgetName));
	Entry->SetStyle(MakeFlatButtonP());

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("%s_Label"), WidgetName)));
	Text->SetText(Label);
	Text->SetFont(UIFont(17));
	Text->SetColorAndOpacity(FSlateColor(TextRest()));
	Entry->SetContent(Text);

	return Entry;
}

void UPauseWidget::HandleResumeClicked()
{
	OnResumeRequested.Broadcast();
}

void UPauseWidget::HandleSettingsClicked()
{
	OnSettingsRequested.Broadcast();
}

void UPauseWidget::HandleQuitClicked()
{
	OnQuitRequested.Broadcast();
}

#undef LOCTEXT_NAMESPACE
