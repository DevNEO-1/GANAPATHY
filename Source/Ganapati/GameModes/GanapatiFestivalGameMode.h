// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/GanapatiMainGameMode.h"
#include "GanapatiFestivalGameMode.generated.h"

class ACameraActor;

/**
 * AGanapatiFestivalGameMode
 *
 * GameMode for the Ganesh Chaturthi festival street slice.
 * - Spawns and manages the festival environment and props
 * - Configures AGanapatiGameHUD for objectives, health, and prompts
 * - Executes an opening cinematic camera sequence showcasing the Pandal
 *   before smoothly blending into third-person player control.
 */
UCLASS()
class GANAPATI_API AGanapatiFestivalGameMode : public AGanapatiMainGameMode
{
	GENERATED_BODY()

public:
	AGanapatiFestivalGameMode();

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
	virtual void BeginPlay() override;

	/** Ensures the festival street geometry and actors are present in the level */
	void EnsureFestivalEnvironment();

	/** Initiates the panoramic opening cinematic camera sequence */
	void PlayOpeningCinematic();

	/** Blends camera from panoramic intro back to player pawn */
	void TransitionToPlayerControl();

protected:
	/** Duration in seconds to hold opening cinematic panoramic view */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Cinematic", meta=(ClampMin=1.0f, Units="s"))
	float CinematicHoldDuration = 3.0f;

	/** Blend duration to swoop from cinematic view into player view */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Cinematic", meta=(ClampMin=0.5f, Units="s"))
	float CinematicBlendDuration = 2.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> CinematicCamera;

	FTimerHandle CinematicTimerHandle;
};
