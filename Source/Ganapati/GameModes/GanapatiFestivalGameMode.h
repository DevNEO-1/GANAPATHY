// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/GanapatiMainGameMode.h"
#include "GanapatiFestivalGameMode.generated.h"

class ACameraActor;
class AGanapatiNPC;
class AGanapatiInteractable;
class AGanapatiTrainingDummy;
class AGanapatiAsuraMinion;
class UCameraShakeBase;

/**
 * Progression states for the Courtyard Skirmish combat encounter.
 */
UENUM(BlueprintType)
enum class ECourtyardEncounterState : uint8
{
	NotStarted UMETA(DisplayName="Not Started"),
	Active UMETA(DisplayName="Active"),
	Completed UMETA(DisplayName="Completed")
};

/**
 * Progression steps for the Sacred Darshan festival quest.
 */
UENUM(BlueprintType)
enum class ESacredDarshanStep : uint8
{
	Step1_SpeakWithAnand UMETA(DisplayName="Step 1: Speak with Halwai Anand"),
	Step2_ReceiveModakPrasadam UMETA(DisplayName="Step 2: Receive Sacred Modak Prasadam"),
	Step3_GrandPandalPrayer UMETA(DisplayName="Step 3: Offer Prayers at Grand Pandal"),
	Step4_DamageTrainingDummy UMETA(DisplayName="Step 4: Damage Courtyard Training Dummy"),
	Completed UMETA(DisplayName="Quest Completed")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSacredDarshanStepAdvancedSignature, ESacredDarshanStep, CompletedStep, ESacredDarshanStep, NewStep);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSacredDarshanCompletedSignature);

/**
 * AGanapatiFestivalGameMode
 *
 * GameMode for the Ganesh Chaturthi festival street slice.
 * - Spawns and manages the festival environment and props
 * - Authoritatively manages the Sacred Darshan festival questline
 * - Configures AGanapatiGameHUD for dynamic quest tracking, toasts, and banners
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

	/** Returns the current quest step */
	UFUNCTION(BlueprintPure, Category="Ganapati|Quest")
	ESacredDarshanStep GetCurrentQuestStep() const { return CurrentQuestStep; }

	/** Returns true if the Sacred Darshan quest has been completed */
	UFUNCTION(BlueprintPure, Category="Ganapati|Quest")
	bool IsQuestCompleted() const { return CurrentQuestStep == ESacredDarshanStep::Completed; }

	/** Gets current active step title (e.g. "[1/4] Seek the blessings of Halwai Anand") */
	UFUNCTION(BlueprintPure, Category="Ganapati|Quest")
	FString GetCurrentObjectiveTitle() const;

	/** Gets current active step description / prompt hint */
	UFUNCTION(BlueprintPure, Category="Ganapati|Quest")
	FString GetCurrentObjectiveDescription() const;

	/** Returns the 1-based step index (1-4) or 4 if completed */
	UFUNCTION(BlueprintPure, Category="Ganapati|Quest")
	int32 GetCurrentStepNumber() const;

	/** Returns total steps (4) */
	UFUNCTION(BlueprintPure, Category="Ganapati|Quest")
	int32 GetTotalSteps() const { return 4; }

	/** Advances the quest to the specified next step with validation and notifications */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Quest")
	void AdvanceQuestStep(ESacredDarshanStep ExpectedCurrentStep, ESacredDarshanStep NextStep, const FText& CompletionToastText);

	/** Returns the current state of the courtyard combat encounter */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat|Encounter")
	ECourtyardEncounterState GetEncounterState() const { return EncounterState; }

	/** Returns true if the courtyard encounter is currently active */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat|Encounter")
	bool IsEncounterActive() const { return EncounterState == ECourtyardEncounterState::Active; }

	/** Returns true if the courtyard encounter has been cleared */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat|Encounter")
	bool IsEncounterCompleted() const { return EncounterState == ECourtyardEncounterState::Completed; }

	/** Returns the count of defeated official encounter targets */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat|Encounter")
	int32 GetEncounterDefeatedCount() const { return DefeatedAsurasCount; }

	/** Returns total official encounter targets (2) */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat|Encounter")
	int32 GetEncounterTotalCount() const { return TotalAsurasSpawned; }

	/** Starts the courtyard skirmish encounter once per session */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat|Encounter")
	void StartCourtyardEncounter();

public:
	/** Broadcast when a quest step advances */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Quest|Events")
	FOnSacredDarshanStepAdvancedSignature OnQuestStepAdvanced;

	/** Broadcast when the entire Sacred Darshan quest is completed */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Quest|Events")
	FOnSacredDarshanCompletedSignature OnQuestCompleted;

protected:
	virtual void BeginPlay() override;

	/** Ensures the festival street geometry and actors are present in the level */
	void EnsureFestivalEnvironment();

	/** Binds listeners to existing interactive actors for the Sacred Darshan quest */
	void BindQuestListeners();

	/** Initiates the panoramic opening cinematic camera sequence */
	void PlayOpeningCinematic();

	/** Blends camera from panoramic intro back to player pawn */
	void TransitionToPlayerControl();

	/** Event handler when any NPC speaks dialogue */
	UFUNCTION()
	void HandleNPCDialogueSpoken(AGanapatiNPC* NPC, AActor* Interactor, const FText& SpokenLine);

	/** Event handler when any interactable is interacted with */
	UFUNCTION()
	void HandleInteractableInteracted(AActor* Interactor, const FText& Message);

	/** Event handler when Grand Pandal prayer completes */
	UFUNCTION()
	void HandlePrayerCompleted(AActor* Interactor);

	/** Event handler when Training Dummy receives confirmed damage */
	UFUNCTION()
	void HandleDummyDamageConfirmed(AGanapatiTrainingDummy* Dummy, float DamageTaken, AActor* DamageCauser, const FVector& DamageLocation);

	/** Event handler when an Asura Minion is defeated (Phase 5B Subsystem 1) */
	UFUNCTION()
	void HandleAsuraDied(AGanapatiAsuraMinion* Asura);

	/** Blueprint hook when quest step advances */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Quest")
	void BP_OnQuestStepAdvanced(ESacredDarshanStep CompletedStep, ESacredDarshanStep NewStep);

	/** Blueprint hook when quest is completed */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Quest")
	void BP_OnQuestCompleted();

protected:
	/** Duration in seconds to hold opening cinematic panoramic view */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Cinematic", meta=(ClampMin=1.0f, Units="s"))
	float CinematicHoldDuration = 3.0f;

	/** Blend duration to swoop from cinematic view into player view */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Cinematic", meta=(ClampMin=0.5f, Units="s"))
	float CinematicBlendDuration = 2.0f;

	/** Current active step of the Sacred Darshan quest */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Quest")
	ESacredDarshanStep CurrentQuestStep = ESacredDarshanStep::Step1_SpeakWithAnand;

	/** Current progression state of the Courtyard Skirmish encounter */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Combat|Encounter")
	ECourtyardEncounterState EncounterState = ECourtyardEncounterState::NotStarted;

	/** Camera shake triggered when courtyard combat begins */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Encounter")
	TSubclassOf<UCameraShakeBase> EncounterStartCameraShakeClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> CinematicCamera;

	FTimerHandle CinematicTimerHandle;
	FTimerHandle QuestBindTimerHandle;
	FTimerHandle CourtyardAlertTimerHandle;

	int32 TotalAsurasSpawned = 0;
	int32 DefeatedAsurasCount = 0;
	bool bCourtyardAlertTriggered = false;

	/** Checks if player has entered the courtyard and triggers skirmish alert toast */
	void CheckCourtyardProximity();
};
