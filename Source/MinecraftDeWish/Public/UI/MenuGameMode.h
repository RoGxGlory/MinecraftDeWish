// LAMPLIGHTER — game mode for the menu map: no pawn, no world generation.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MenuGameMode.generated.h"

/**
 * Overrides the gameplay game mode on Lvl_MainMenu only (World Settings override).
 * GlobalDefaultGameMode stays BP_FirstPersonGameMode, so PIE from the gameplay map is untouched.
 */
UCLASS()
class MINECRAFTDEWISH_API AMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMenuGameMode();
};
