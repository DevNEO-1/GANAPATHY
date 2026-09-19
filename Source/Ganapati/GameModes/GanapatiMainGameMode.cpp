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

void AGanapatiMainGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Force-override pawn and controller even if a Blueprint child has serialized different defaults
	DefaultPawnClass = AGanapatiPlayerCharacter::StaticClass();
	PlayerControllerClass = AGanapatiMainPlayerController::StaticClass();
}

UClass* AGanapatiMainGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	return AGanapatiPlayerCharacter::StaticClass();
}
