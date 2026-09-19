// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GanapatiMainGameMode.generated.h"

/**
 * AGanapatiMainGameMode
 *
 * The concrete GameMode for the main Ganapati game.
 * Sets AGanapatiPlayerCharacter as the default pawn class
 * and AGanapatiMainPlayerController as the default controller.
 *
 * Use this GameMode in the Ganapati-specific levels to enable
 * the unified player character with all combat, movement, and
 * anti-gravity abilities.
 *
 * The existing template and variant GameModes remain untouched
 * and continue to function for their respective levels.
 */
UCLASS()
class GANAPATI_API AGanapatiMainGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGanapatiMainGameMode();
};
