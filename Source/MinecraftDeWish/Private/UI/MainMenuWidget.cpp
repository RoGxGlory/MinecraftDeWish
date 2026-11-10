#include "UI/MainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "UI/LamplighterUIStyle.h"

#define LOCTEXT_NAMESPACE "LamplighterUI"

namespace
{
	/** A brush that draws nothing, for the flat text buttons. */
	FSlateBrush MakeNoBrush()
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
		return Brush;
	}
}

UMainMenuWidget::UMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UMainMenuWidget::BuildLayout()
{
	using namespace LamplighterUI;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Root;

	// ---------------------------------------------------------------- column
	Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	if (UCanvasPanelSlot* ColumnSlot = Root->AddChildToCanvas(Column))
	{
		ColumnSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		ColumnSlot->SetAlignment(FVector2D(0.f, 0.f));
		ColumnSlot->SetPosition(FVector2D(Layout::ColumnLeft, Layout::ColumnTop));
		ColumnSlot->SetAutoSize(true);
	}

	UTextBlock* Wordmark = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Wordmark"));
	Wordmark->SetText(LOCTEXT("Title", "LAMPLIGHTER"));
	Wordmark->SetFont(DisplayFont(112, 75));
	Wordmark->SetColorAndOpacity(FSlateColor(Hex(TEXT("#D9D2C0"))));
	Column->AddChildToVerticalBox(Wordmark);

	UTextBlock* Tagline = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Tagline"));
	Tagline->SetText(LOCTEXT("Tagline", "KEEP THE LIGHT. KEEP WATCH."));
	Tagline->SetFont(MonoFont(13));
	Tagline->SetColorAndOpacity(FSlateColor(Ash()));
	if (UVerticalBoxSlot* TaglineSlot = Cast<UVerticalBoxSlot>(Column->AddChildToVerticalBox(Tagline)))
	{
		TaglineSlot->SetPadding(FMargin(4.f, 22.f, 0.f, 0.f));
	}

	USpacer* Gap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("Gap"));
	Gap->SetSize(FVector2D(1.f, 60.f));
	Column->AddChildToVerticalBox(Gap);

	// ---------------------------------------------------------------- entries
	// Continue: shown only when at least one saved world exists.
	ContinueButton = MakeTextButton(LOCTEXT("Continue", "Continuer"), TEXT("ContinueButton"), 31);
	AddColumnEntry(Column, ContinueButton);
	ContinueButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleContinueClicked);
	ContinueButton->SetVisibility(HasSavedWorlds() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	UButton* NewWorldButton = MakeTextButton(LOCTEXT("NewWorld", "Nouveau monde"), TEXT("NewWorldButton"), 31);
	AddColumnEntry(Column, NewWorldButton);
	NewWorldButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleNewWorldClicked);

	SettingsButton = MakeTextButton(LOCTEXT("Settings", "Réglages"), TEXT("SettingsButton"), 31);
	AddColumnEntry(Column, SettingsButton);
	SettingsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleSettingsClicked);

	UButton* QuitButton = MakeTextButton(LOCTEXT("Quit", "Quitter"), TEXT("QuitButton"), 31);
	AddColumnEntry(Column, QuitButton);
	QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleQuitClicked);

	// ---------------------------------------------------------------- footer
	UTextBlock* Build = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Build"));
	Build->SetText(FText::FromString(TEXT("v0.3.0-alpha   ·   build 4471")));
	Build->SetFont(MonoFont(12));
	Build->SetColorAndOpacity(FSlateColor(Ash()));
	if (UCanvasPanelSlot* BuildSlot = Root->AddChildToCanvas(Build))
	{
		BuildSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
		BuildSlot->SetAlignment(FVector2D(0.f, 1.f));
		BuildSlot->SetPosition(FVector2D(Layout::ColumnLeft, -Layout::FooterInset));
		BuildSlot->SetAutoSize(true);
	}

	UTextBlock* Status = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Status"));
	Status->SetText(LOCTEXT("FooterStatus", "AUCUN MONDE"));
	Status->SetFont(MonoFont(12));
	Status->SetColorAndOpacity(FSlateColor(Signal() * 0.55f));
	if (UCanvasPanelSlot* StatusSlot = Root->AddChildToCanvas(Status))
	{
		StatusSlot->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
		StatusSlot->SetAlignment(FVector2D(1.f, 1.f));
		StatusSlot->SetPosition(FVector2D(-Layout::ColumnLeft, -Layout::FooterInset));
		StatusSlot->SetAutoSize(true);
	}

	// ---------------------------------------------------------------- new world
	NewWorldPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NewWorldPanel"));
	NewWorldPanel->SetBrushColor(Surface());
	NewWorldPanel->SetPadding(FMargin(56.f, 54.f));
	NewWorldPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(NewWorldPanel))
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
		PanelSlot->SetPosition(FVector2D(Layout::ColumnLeft, 200.f));
		PanelSlot->SetAutoSize(true);
	}

	UVerticalBox* PanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelBox"));
	NewWorldPanel->SetContent(PanelBox);

	UTextBlock* Kicker = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PanelKicker"));
	Kicker->SetText(LOCTEXT("NewWorldKicker", "NOUVEAU MONDE"));
	Kicker->SetFont(MonoFont(11));
	Kicker->SetColorAndOpacity(FSlateColor(Ash()));
	PanelBox->AddChildToVerticalBox(Kicker);

	UTextBlock* PanelTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PanelTitle"));
	PanelTitle->SetText(LOCTEXT("NewWorldTitle", "LE PREMIER SOIR"));
	PanelTitle->SetFont(DisplayFont(26));
	PanelTitle->SetColorAndOpacity(FSlateColor(Bone()));
	if (UVerticalBoxSlot* TitleSlot = Cast<UVerticalBoxSlot>(PanelBox->AddChildToVerticalBox(PanelTitle)))
	{
		TitleSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}

	auto AddFieldLabel = [this, PanelBox](const FText& Text)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(Text);
		Label->SetFont(MonoFont(11));
		Label->SetColorAndOpacity(FSlateColor(Ash()));
		if (UVerticalBoxSlot* LabelSlot = Cast<UVerticalBoxSlot>(PanelBox->AddChildToVerticalBox(Label)))
		{
			LabelSlot->SetPadding(FMargin(0.f, 34.f, 0.f, 8.f));
		}
	};

	AddFieldLabel(LOCTEXT("WorldNameLabel", "NOM DU MONDE"));
	NameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("NameInput"));
	NameInput->SetHintText(LOCTEXT("WorldNameHint", "Maison de verre"));
	NameInput->SetText(LOCTEXT("WorldNameDefault", "Maison de verre"));
	PanelBox->AddChildToVerticalBox(NameInput);

	AddFieldLabel(LOCTEXT("SeedLabel", "SEED"));
	SeedInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("SeedInput"));
	SeedInput->SetHintText(LOCTEXT("SeedHint", "laisser vide pour tirer au hasard"));
	SeedInput->SetText(FText::FromString(TEXT("1337")));
	PanelBox->AddChildToVerticalBox(SeedInput);

	UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Actions"));
	if (UVerticalBoxSlot* ActionsSlot = Cast<UVerticalBoxSlot>(PanelBox->AddChildToVerticalBox(Actions)))
	{
		ActionsSlot->SetPadding(FMargin(0.f, 52.f, 0.f, 0.f));
	}

	UButton* CreateButton = MakeTextButton(LOCTEXT("Create", "Créer"), TEXT("CreateButton"), 17);
	CreateButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleCreateWorldClicked);
	if (UHorizontalBoxSlot* CreateSlot = Actions->AddChildToHorizontalBox(CreateButton))
	{
		CreateSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	}

	UButton* BackButton = MakeTextButton(LOCTEXT("Back", "Retour"), TEXT("BackButton"), 17);
	BackButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleBackClicked);
	Actions->AddChildToHorizontalBox(BackButton);
}

UButton* UMainMenuWidget::MakeTextButton(const FText& Label, const TCHAR* WidgetName, int32 FontSize)
{
	using namespace LamplighterUI;

	UButton* Entry = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(WidgetName));

	FButtonStyle Style;
	const FSlateBrush NoBrush = MakeNoBrush();
	Style.SetNormal(NoBrush);
	Style.SetHovered(NoBrush);
	Style.SetPressed(NoBrush);
	Style.SetDisabled(NoBrush);
	Style.NormalPadding = FMargin(0.f, 9.f);
	Style.PressedPadding = FMargin(0.f, 9.f);
	Entry->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("%s_Label"), WidgetName)));
	Text->SetText(Label);
	Text->SetFont(DisplayFont(FontSize, 175));
	Text->SetColorAndOpacity(FSlateColor(TextRest()));
	Entry->SetContent(Text);

	Entry->OnHovered.AddDynamic(this, &UMainMenuWidget::HandleEntryHovered);
	Entry->OnUnhovered.AddDynamic(this, &UMainMenuWidget::HandleEntryHovered);

	EntryButtons.Add(Entry);
	return Entry;
}

void UMainMenuWidget::AddColumnEntry(UVerticalBox* InColumn, UButton* Entry)
{
	if (!InColumn || !Entry)
	{
		return;
	}

	if (UVerticalBoxSlot* EntrySlot = Cast<UVerticalBoxSlot>(InColumn->AddChildToVerticalBox(Entry)))
	{
		EntrySlot->SetHorizontalAlignment(HAlign_Left);
		EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}
}

void UMainMenuWidget::RefreshEntryVisuals()
{
	using namespace LamplighterUI;

	for (UButton* Entry : EntryButtons)
	{
		if (!Entry)
		{
			continue;
		}

		const bool bActive = Entry->IsHovered() || Entry->HasKeyboardFocus();
		if (UTextBlock* Text = Cast<UTextBlock>(Entry->GetContent()))
		{
			Text->SetColorAndOpacity(FSlateColor(bActive ? TextActive() : TextRest()));
		}
	}
}

void UMainMenuWidget::HandleEntryHovered()
{
	RefreshEntryVisuals();
}

void UMainMenuWidget::ShowNewWorldPanel(bool bShow)
{
	if (Column)
	{
		Column->SetVisibility(bShow ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (NewWorldPanel)
	{
		NewWorldPanel->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UMainMenuWidget::HandleNewWorldClicked()
{
	ShowNewWorldPanel(true);
}

void UMainMenuWidget::HandleContinueClicked()
{
	// Find the first saved world and travel to it.
	const FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	TArray<FString> WorldDirs;
	IFileManager::Get().FindFiles(WorldDirs, *(SaveDir / TEXT("World_*")), true, true);

	if (WorldDirs.Num() == 0)
	{
		return;
	}

	// Extract seed from "World_<seed>".
	const FString DirName = WorldDirs[0];
	const FString SeedStr = DirName.Replace(TEXT("World_"), TEXT(""));
	const int32 Seed = FCString::Atoi(*SeedStr);

	FWorldDescriptor Descriptor;
	Descriptor.WorldName = FString::Printf(TEXT("Monde %d"), Seed);
	Descriptor.Seed = Seed != 0 ? Seed : 1337;
	Descriptor.DreadPreset = TEXT("Standard");

	OnNewWorldRequested.Broadcast(Descriptor);
}

bool UMainMenuWidget::HasSavedWorlds() const
{
	const FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	TArray<FString> WorldDirs;
	IFileManager::Get().FindFiles(WorldDirs, *(SaveDir / TEXT("World_*")), true, true);
	return WorldDirs.Num() > 0;
}

void UMainMenuWidget::HandleBackClicked()
{
	ShowNewWorldPanel(false);
}

void UMainMenuWidget::HandleSettingsClicked()
{
	OnSettingsRequested.Broadcast();
}

void UMainMenuWidget::HandleQuitClicked()
{
	OnQuitRequested.Broadcast();
}

void UMainMenuWidget::HandleCreateWorldClicked()
{
	FWorldDescriptor Descriptor;

	Descriptor.WorldName = NameInput ? NameInput->GetText().ToString().TrimStartAndEnd() : FString();
	if (Descriptor.WorldName.IsEmpty())
	{
		Descriptor.WorldName = TEXT("Sans nom");
	}

	const int32 TypedSeed = SeedInput ? FCString::Atoi(*SeedInput->GetText().ToString().TrimStartAndEnd()) : 0;
	Descriptor.Seed = (TypedSeed != 0)
		? TypedSeed
		: ((FMath::Rand() << 16) ^ FMath::Rand());

	OnNewWorldRequested.Broadcast(Descriptor);
}

#undef LOCTEXT_NAMESPACE
