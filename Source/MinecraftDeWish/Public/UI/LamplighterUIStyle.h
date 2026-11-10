// LAMPLIGHTER — shared UI constants.
// Header-only on purpose: these are compile-time design tokens, not assets.
// Values mirror design/ui/ui-direction.md §10. Change them there first.

#pragma once

#include "CoreMinimal.h"
#include "Engine/FontFace.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectGlobals.h"

namespace LamplighterUI
{
	/** "#RRGGBB" to linear colour. FLinearColor(FColor) performs the sRGB conversion. */
	inline FLinearColor Hex(const TCHAR* InHex)
	{
		return FLinearColor(FColor::FromHex(FString(InHex)));
	}

	// --- palette -------------------------------------------------------------
	inline FLinearColor Void()      { return Hex(TEXT("#0B0E11")); } // background
	inline FLinearColor Surface()   { return Hex(TEXT("#171C21")); } // panels
	inline FLinearColor Raised()    { return Hex(TEXT("#2A3138")); } // borders
	inline FLinearColor Bone()      { return Hex(TEXT("#E3DCCB")); } // primary text
	inline FLinearColor Sodium()    { return Hex(TEXT("#E8A33D")); } // accent / interactive
	inline FLinearColor Rust()      { return Hex(TEXT("#C8462F")); } // danger
	inline FLinearColor Signal()    { return Hex(TEXT("#4FA6A0")); } // power, cameras
	inline FLinearColor Ash()       { return Hex(TEXT("#5C6268")); } // disabled, secondary

	/** Text at rest: bone, dimmed. */
	inline FLinearColor TextRest()   { return Bone() * 0.62f; }
	/** Text hovered or selected. */
	inline FLinearColor TextActive() { return Bone(); }

	// --- typography ----------------------------------------------------------
	// Loaded from imported UFontFace assets. Falls back to the engine default
	// if the asset is missing (e.g. in a fresh checkout without imported fonts).
	inline UObject* LoadFontFace(const TCHAR* Path)
	{
		UObject* Obj = StaticLoadObject(UFontFace::StaticClass(), nullptr, Path);
		if (!Obj)
		{
			UE_LOG(LogTemp, Warning, TEXT("LamplighterUI: failed to load font face '%s'"), Path);
		}
		return Obj;
	}

	inline FSlateFontInfo FontFace(const TCHAR* Path, int32 Size)
	{
		UFontFace* Face = Cast<UFontFace>(LoadFontFace(Path));
		if (!Face)
		{
			return FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
		}

		// UFontFace implements IFontFaceInterface, not IFontProviderInterface,
		// so FSlateFontInfo(const UObject*) can't use it directly. Build a
		// composite font that references the font face asset.
		FFontData FontData(Face);
		FTypefaceEntry Entry;
		Entry.Name = FName("Regular");
		Entry.Font = FontData;

		FTypeface Typeface;
		Typeface.Fonts.Add(Entry);

		TSharedRef<FStandaloneCompositeFont> CompositeFont = MakeShared<FStandaloneCompositeFont>();
		CompositeFont->DefaultTypeface = Typeface;

		FSlateFontInfo Info(CompositeFont, (float)Size);
		return Info;
	}

	inline FSlateFontInfo DisplayFont(int32 Size, int32 LetterSpacing = 0)
	{
		FSlateFontInfo Info = FontFace(TEXT("/Game/UI/Fonts/F_Oswald/Oswald-Variable.Oswald-Variable"), Size);
		Info.LetterSpacing = LetterSpacing;
		return Info;
	}

	inline FSlateFontInfo UIFont(int32 Size)
	{
		return FontFace(TEXT("/Game/UI/Fonts/F_PlexSans/IBMPlexSans-Regular.IBMPlexSans-Regular"), Size);
	}

	inline FSlateFontInfo MonoFont(int32 Size)
	{
		return FontFace(TEXT("/Game/UI/Fonts/F_PlexMono/IBMPlexMono-Regular.IBMPlexMono-Regular"), Size);
	}

	// --- layout --------------------------------------------------------------
	namespace Layout
	{
		/** Left edge of the painted column, at 1920x1080. */
		constexpr float ColumnLeft = 150.f;
		/** Top edge of the wordmark. */
		constexpr float ColumnTop = 180.f;
		/** Width of the painted column. */
		constexpr float ColumnWidth = 780.f;
		/** Distance from the screen edge to the footer baseline. */
		constexpr float FooterInset = 64.f;
	}

	// --- motion --------------------------------------------------------------
	namespace Motion
	{
		constexpr float HoverSeconds = 0.14f;
		constexpr float PanelSeconds = 0.18f;
		constexpr float PageSeconds = 0.40f;
	}
}
