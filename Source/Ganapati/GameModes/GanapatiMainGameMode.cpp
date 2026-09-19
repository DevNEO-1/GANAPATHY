// Copyright Ganapati Project. All Rights Reserved.

#include "GameModes/GanapatiMainGameMode.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "Controllers/GanapatiMainPlayerController.h"

AGanapatiMainGameMode::AGanapatiMainGameMode()
{
	// Use the Ganapati unified player character directly — it auto-loads mesh + anim
	DefaultPawnClass = AGanapatiPlayerCharacter::StaticClass();

	// Use the Ganapati main player controller with full Enhanced Input setup
	PlayerControllerClass = AGanapatiMainPlayerController::StaticClass();
}
