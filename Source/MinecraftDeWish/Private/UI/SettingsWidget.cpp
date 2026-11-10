#include "UI/SettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "UI/LamplighterUIStyle.h"

#define LOCTEXT_NAMESPACE "LamplighterUI"

namespace
{
	FSlateBrush MakeNoBrushS()
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::NoDrawType;
		return B;
	}

	FButtonStyle MakeFlatButton()
	{
		FButtonStyle S;
		const FSlateBrush No = MakeNoBrushS();
		S.SetNormal(No).SetHovered(No).SetPressed(No).SetDisabled(No);
		S.NormalPadding = FMargin(0.f, 6.f);
		S.PressedPadding = FMargin(0.f, 6.f);
		return S;
	}
}

USettingsWidget::USettingsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> USettingsWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void USettingsWidget::BuildLayout()
{
	using namespace LamplighterUI;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Root;

	// --- scrim ---
	UBorder* Scrim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Scrim"));
	Scrim->SetBrushColor(FLinearColor(0.043f, 0.055f, 0.067f, 0.72f));
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

	// --- sodium rail (3px leading edge) ---
	// Drawn as a thin border on the left; implemented as a separate widget for clarity.
	UBorder* Rail = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SodiumRail"));
	Rail->SetBrushColor(Sodium());
	if (UCanvasPanelSlot* RailSlot = Root->AddChildToCanvas(Rail))
	{
		RailSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		RailSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		RailSlot->SetOffsets(FMargin(-420.f, -200.f, 3.f, 400.f));
	}

	// --- kicker ---
	UTextBlock* Kicker = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Kicker"));
	Kicker->SetText(LOCTEXT("SettingsKicker", "RÉGLAGES"));
	Kicker->SetFont(MonoFont(11));
	Kicker->SetColorAndOpacity(FSlateColor(Ash()));
	PanelBox->AddChildToVerticalBox(Kicker);

	// --- title ---
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(LOCTEXT("SettingsTitle", "L'ÉQUIPEMENT"));
	Title->SetFont(DisplayFont(26));
	Title->SetColorAndOpacity(FSlateColor(Bone()));
	if (UVerticalBoxSlot* TitleSlot = Cast<UVerticalBoxSlot>(PanelBox->AddChildToVerticalBox(Title)))
	{
		TitleSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}

	// --- tabs ---
	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Tabs"));
	if (UVerticalBoxSlot* TabsSlot = Cast<UVerticalBoxSlot>(PanelBox->AddChildToVerticalBox(Tabs)))
	{
		TabsSlot->SetPadding(FMargin(0.f, 30.f, 0.f, 0.f));
	}

	const TArray<FText> TabLabels = {
		LOCTEXT("TabGraphics", "GRAPHISMES"),
		LOCTEXT("TabGameplay", "JEU"),
		LOCTEXT("TabAccessibility", "ACCESSIBILITÉ"),
		LOCTEXT("TabControls", "CONTRÔLES")
	};
	for (int32 i = 0; i < TabLabels.Num(); i++)
	{
		UButton* Tab = MakeTabButton(TabLabels[i], i);
		if (UHorizontalBoxSlot* TabSlot = Tabs->AddChildToHorizontalBox(Tab))
		{
			TabSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));
		}
	}

	// --- pages ---
	for (int32 i = 0; i < 4; i++)
	{
		UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("Page%d"), i)));
		Page->SetVisibility(i == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (UVerticalBoxSlot* PageSlot = Cast<UVerticalBoxSlot>(PanelBox->AddChildToVerticalBox(Page)))
		{
			PageSlot->SetPadding(FMargin(0.f, 24.f, 0.f, 0.f));
		}
		Pages.Add(Page);
	}

	BuildGraphicsPage(Pages[0]);
	BuildGameplayPage(Pages[1]);
	BuildAccessibilityPage(Pages[2]);
	BuildControlsPage(Pages[3]);

	// --- actions ---
	UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Actions"));
	if (UVerticalBoxSlot* ActionsSlot = Cast<UVerticalBoxSlot>(PanelBox->AddChildToVerticalBox(Actions)))
	{
		ActionsSlot->SetPadding(FMargin(0.f, 40.f, 0.f, 0.f));
	}

	UButton* BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackButton"));
	BackButton->SetStyle(MakeFlatButton());
	UTextBlock* BackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BackText->SetText(LOCTEXT("Back", "RETOUR"));
	BackText->SetFont(UIFont(17));
	BackText->SetColorAndOpacity(FSlateColor(TextRest()));
	BackButton->SetContent(BackText);
	BackButton->OnClicked.AddDynamic(this, &USettingsWidget::HandleBackClicked);
	if (UHorizontalBoxSlot* BackSlot = Actions->AddChildToHorizontalBox(BackButton))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Left);
	}

	RefreshValues();
}

UButton* USettingsWidget::MakeTabButton(const FText& Label, int32 PageIndex)
{
	using namespace LamplighterUI;

	UButton* Tab = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*FString::Printf(TEXT("Tab%d"), PageIndex)));
	Tab->SetStyle(MakeFlatButton());

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(Label);
	Text->SetFont(MonoFont(13));
	Text->SetColorAndOpacity(FSlateColor(PageIndex == 0 ? TextActive() : TextRest()));
	Tab->SetContent(Text);

	switch (PageIndex)
	{
	case 0: Tab->OnClicked.AddDynamic(this, &USettingsWidget::HandleGraphicsTab); break;
	case 1: Tab->OnClicked.AddDynamic(this, &USettingsWidget::HandleGameplayTab); break;
	case 2: Tab->OnClicked.AddDynamic(this, &USettingsWidget::HandleAccessibilityTab); break;
	case 3: Tab->OnClicked.AddDynamic(this, &USettingsWidget::HandleControlsTab); break;
	}

	TabButtons.Add(Tab);
	return Tab;
}

void USettingsWidget::ShowPage(int32 PageIndex)
{
	using namespace LamplighterUI;

	for (int32 i = 0; i < Pages.Num(); i++)
	{
		if (Pages[i])
		{
			Pages[i]->SetVisibility(i == PageIndex ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}

	for (int32 i = 0; i < TabButtons.Num(); i++)
	{
		if (TabButtons[i])
		{
			if (UTextBlock* Text = Cast<UTextBlock>(TabButtons[i]->GetContent()))
			{
				Text->SetColorAndOpacity(FSlateColor(i == PageIndex ? TextActive() : TextRest()));
			}
		}
	}

	CurrentPage = PageIndex;
}

void USettingsWidget::HandleGraphicsTab() { ShowPage(0); }
void USettingsWidget::HandleGameplayTab() { ShowPage(1); }
void USettingsWidget::HandleAccessibilityTab() { ShowPage(2); }
void USettingsWidget::HandleControlsTab() { ShowPage(3); }
void USettingsWidget::HandleBackClicked() { OnBackRequested.Broadcast(); }

UGameSettingsSubsystem* USettingsWidget::GetSettings() const
{
	return UGameSettingsSubsystem::Get(this);
}

// --- Field helpers ---

USlider* USettingsWidget::MakeSlider(UVerticalBox* Container, const FText& Label, float Min, float Max, float Value)
{
	using namespace LamplighterUI;

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(Label);
	LabelText->SetFont(UIFont(14));
	LabelText->SetColorAndOpacity(FSlateColor(TextRest()));
	if (UVerticalBoxSlot* LabelSlot = Cast<UVerticalBoxSlot>(Container->AddChildToVerticalBox(LabelText)))
	{
		LabelSlot->SetPadding(FMargin(0.f, 20.f, 0.f, 4.f));
	}

	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	Slider->SetMinValue(Min);
	Slider->SetMaxValue(Max);
	Slider->SetValue(Value);
	Slider->SetStepSize(0.01f);
	if (UVerticalBoxSlot* SliderSlot = Cast<UVerticalBoxSlot>(Container->AddChildToVerticalBox(Slider)))
	{
		SliderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));
	}

	return Slider;
}

UCheckBox* USettingsWidget::MakeToggle(UVerticalBox* Container, const FText& Label, bool Value)
{
	using namespace LamplighterUI;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(Container->AddChildToVerticalBox(Row)))
	{
		RowSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(Label);
	LabelText->SetFont(UIFont(14));
	LabelText->SetColorAndOpacity(FSlateColor(TextRest()));
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UCheckBox* Toggle = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	Toggle->SetIsChecked(Value);
	if (UHorizontalBoxSlot* ToggleSlot = Row->AddChildToHorizontalBox(Toggle))
	{
		ToggleSlot->SetHorizontalAlignment(HAlign_Right);
		ToggleSlot->SetVerticalAlignment(VAlign_Center);
	}

	return Toggle;
}

UComboBoxString* USettingsWidget::MakeDropdown(UVerticalBox* Container, const FText& Label, const TArray<FString>& Options, int32 Selected)
{
	using namespace LamplighterUI;

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(Label);
	LabelText->SetFont(UIFont(14));
	LabelText->SetColorAndOpacity(FSlateColor(TextRest()));
	if (UVerticalBoxSlot* LabelSlot = Cast<UVerticalBoxSlot>(Container->AddChildToVerticalBox(LabelText)))
	{
		LabelSlot->SetPadding(FMargin(0.f, 20.f, 0.f, 4.f));
	}

	UComboBoxString* Combo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
	for (const FString& Opt : Options)
	{
		Combo->AddOption(Opt);
	}
	if (Selected >= 0 && Selected < Options.Num())
	{
		Combo->SetSelectedIndex(Selected);
	}
	if (UVerticalBoxSlot* ComboSlot = Cast<UVerticalBoxSlot>(Container->AddChildToVerticalBox(Combo)))
	{
		ComboSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));
	}

	return Combo;
}

// --- Page builders ---

void USettingsWidget::BuildGraphicsPage(UVerticalBox* Container)
{
	using namespace LamplighterUI;

	UGameSettingsSubsystem* S = GetSettings();
	if (!S) { return; }

	USlider* RD = MakeSlider(Container, LOCTEXT("RenderDistance", "DISTANCE DE RENDU"), 2.f, 64.f, (float)S->GetRenderDistance());
	RD->OnValueChanged.AddDynamic(this, &USettingsWidget::OnRenderDistanceChanged);

	UCheckBox* FL = MakeToggle(Container, LOCTEXT("FastLeaves", "FEUILLES RAPIDES"), S->GetFastLeaves());
	FL->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnFastLeavesChanged);

	USlider* WS = MakeSlider(Container, LOCTEXT("WindStrength", "FORCE DU VENT"), 0.f, 2.f, S->GetWindStrength());
	WS->OnValueChanged.AddDynamic(this, &USettingsWidget::OnWindStrengthChanged);

	UCheckBox* LP = MakeToggle(Container, LOCTEXT("LeafParticles", "PARTICULES DE FEUILLES"), S->GetLeafParticles());
	LP->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnLeafParticlesChanged);

	USlider* Fov = MakeSlider(Container, LOCTEXT("FOV", "CHAMP DE VISION"), 70.f, 110.f, (float)S->GetFOV());
	Fov->OnValueChanged.AddDynamic(this, &USettingsWidget::OnFOVChanged);

	UCheckBox* MB = MakeToggle(Container, LOCTEXT("MotionBlur", "FLOU DE MOUVEMENT"), S->GetMotionBlur());
	MB->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnMotionBlurChanged);
}

void USettingsWidget::BuildGameplayPage(UVerticalBox* Container)
{
	using namespace LamplighterUI;

	UGameSettingsSubsystem* S = GetSettings();
	if (!S) { return; }

	USlider* MS = MakeSlider(Container, LOCTEXT("MouseSensitivity", "SENSIBILITÉ SOURIS"), 0.2f, 3.0f, S->GetMouseSensitivity());
	MS->OnValueChanged.AddDynamic(this, &USettingsWidget::OnMouseSensitivityChanged);

	UCheckBox* Sub = MakeToggle(Container, LOCTEXT("Subtitles", "SOUS-TITRES"), S->GetSubtitles());
	Sub->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnSubtitlesChanged);

	USlider* SS = MakeSlider(Container, LOCTEXT("SubtitleSize", "TAILLE DES SOUS-TITRES"), 0.85f, 1.30f, S->GetSubtitleSize());
	SS->OnValueChanged.AddDynamic(this, &USettingsWidget::OnSubtitleSizeChanged);

	USlider* CS = MakeSlider(Container, LOCTEXT("CameraShake", "TREMBLEMENT CAMÉRA"), 0.f, 1.0f, S->GetCameraShake());
	CS->OnValueChanged.AddDynamic(this, &USettingsWidget::OnCameraShakeChanged);
}

void USettingsWidget::BuildAccessibilityPage(UVerticalBox* Container)
{
	using namespace LamplighterUI;

	UGameSettingsSubsystem* S = GetSettings();
	if (!S) { return; }

	const TArray<FString> ColourblindOptions = { "Aucun", "Deutéranopie", "Protanopie", "Tritanopie" };
	UComboBoxString* CB = MakeDropdown(Container, LOCTEXT("Colourblind", "INDICATEURS CRITIQUES"), ColourblindOptions, S->GetColourblindMode());
	CB->OnSelectionChanged.AddDynamic(this, &USettingsWidget::OnColourblindChanged);

	UCheckBox* PauseInv = MakeToggle(Container, LOCTEXT("PauseInventory", "METTRE EN PAUSE L'INVENTAIRE"), S->GetPauseInventory());
	PauseInv->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnPauseInventoryChanged);

	UCheckBox* BF = MakeToggle(Container, LOCTEXT("BulbFlicker", "SCINTILLEMENT AMPOULE (MENUS)"), S->GetBulbFlickerMenus());
	BF->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnBulbFlickerChanged);

	UCheckBox* HB = MakeToggle(Container, LOCTEXT("HUDBreathing", "RESPIRATION DU HUD"), S->GetHUDBreathing());
	HB->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnHUDBreathingChanged);
}

void USettingsWidget::BuildControlsPage(UVerticalBox* Container)
{
	using namespace LamplighterUI;

	UTextBlock* Placeholder = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Placeholder->SetText(LOCTEXT("ControlsPlaceholder", "Remappage des touches — à venir (Enhanced Input User Settings)."));
	Placeholder->SetFont(UIFont(14));
	Placeholder->SetColorAndOpacity(FSlateColor(Ash()));
	Placeholder->SetAutoWrapText(true);
	Container->AddChildToVerticalBox(Placeholder);
}

void USettingsWidget::RefreshValues()
{
	// Values are bound at construction time; no-op.
}

// --- Handlers ---

void USettingsWidget::OnRenderDistanceChanged(float V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetRenderDistance((int32)V); }
}

void USettingsWidget::OnWindStrengthChanged(float V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetWindStrength(V); }
}

void USettingsWidget::OnFOVChanged(float V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetFOV((int32)V); }
}

void USettingsWidget::OnMouseSensitivityChanged(float V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetMouseSensitivity(V); }
}

void USettingsWidget::OnSubtitleSizeChanged(float V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetSubtitleSize(V); }
}

void USettingsWidget::OnCameraShakeChanged(float V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetCameraShake(V); }
}

void USettingsWidget::OnUITextScaleChanged(float V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetUITextScale(V); }
}

void USettingsWidget::OnFastLeavesChanged(bool V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetFastLeaves(V); }
}

void USettingsWidget::OnLeafParticlesChanged(bool V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetLeafParticles(V); }
}

void USettingsWidget::OnMotionBlurChanged(bool V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetMotionBlur(V); }
}

void USettingsWidget::OnSubtitlesChanged(bool V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetSubtitles(V); }
}

void USettingsWidget::OnPauseInventoryChanged(bool V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetPauseInventory(V); }
}

void USettingsWidget::OnBulbFlickerChanged(bool V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetBulbFlickerMenus(V); }
}

void USettingsWidget::OnHUDBreathingChanged(bool V)
{
	if (UGameSettingsSubsystem* S = GetSettings()) { S->SetHUDBreathing(V); }
}

void USettingsWidget::OnColourblindChanged(FString Item, ESelectInfo::Type Type)
{
	if (UGameSettingsSubsystem* S = GetSettings())
	{
		// Find index
		const TArray<FString> Options = { "Aucun", "Deutéranopie", "Protanopie", "Tritanopie" };
		for (int32 i = 0; i < Options.Num(); i++)
		{
			if (Options[i] == Item)
			{
				S->SetColourblindMode(i);
				break;
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
