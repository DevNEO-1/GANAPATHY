// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GanapatiGameMode.generated.h"

/**
 * AGanapatiGameMode
 *
 * Base GameMode for the Ganapati project.
 * Enforces the unified AGanapatiPlayerCharacter and AGanapatiMainPlayerController
 * across all levels, ensuring custom movement, combat, interaction, and HUD
 * are active even if a level references BP_ThirdPersonGameMode.
 */
UCLASS()
class GANAPATI_API AGanapatiGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGanapatiGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual void BeginPlay() override;

protected:
	/** Ensures the festival street environment builder is present in the level */
	void EnsureFestivalEnvironment();
};
