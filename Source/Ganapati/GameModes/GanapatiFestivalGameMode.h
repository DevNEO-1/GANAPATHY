// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/GanapatiMainGameMode.h"
#include "World/GanapatiWorldSubsystem.h"
#include "GanapatiFestivalGameMode.generated.h"

class ACameraActor;
class AGanapatiNPC;
class AGanapatiInteractable;
class AGanapatiTrainingDummy;
class AGanapatiAsuraMinion;
class AGanapatiAsuraCaptain;
class AFestivalStreetBuilder;
class UCameraShakeBase;

/**
 * Progression states for the Asura Captain mini-boss encounter (Phase 5C Subsystem 3).
 */
UENUM(BlueprintType)
enum class ECaptainEncounterState : uint8
{
	Dormant UMETA(DisplayName="Dormant"),
	Intro UMETA(DisplayName="Intro"),
	Active UMETA(DisplayName="Active"),
	Defeated UMETA(DisplayName="Defeated")
};

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSacredJourneyUnlockedSignature);

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

	/** Returns true if courtyard has been fully purified (minions cleared + Captain defeated) */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat|Encounter")
	bool IsCourtyardPurified() const { return bCourtyardPurified; }

	/** Returns current state of the Asura Captain mini-boss encounter (Phase 5C Subsystem 3) */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat|Boss")
	ECaptainEncounterState GetCaptainEncounterState() const { return CaptainEncounterState; }

	/** Returns true if Captain encounter is currently active */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat|Boss")
	bool IsCaptainEncounterActive() const { return CaptainEncounterState == ECaptainEncounterState::Active; }

	/** Returns true if Captain encounter is in intro sequence */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat|Boss")
	bool IsCaptainEncounterIntro() const { return CaptainEncounterState == ECaptainEncounterState::Intro; }

	/** Returns true if Captain has been defeated */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat|Boss")
	bool IsCaptainEncounterDefeated() const { return CaptainEncounterState == ECaptainEncounterState::Defeated; }

	/** Returns active Asura Captain reference */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat|Boss")
	AGanapatiAsuraCaptain* GetActiveCaptain() const;

	/** Activates or deactivates the temporary boss ward barrier */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat|Boss")
	void SetBossBarrierActive(bool bActive);

	/** Blueprint implementable event fired when the courtyard is fully purified (Phase 5C Subsystem 4) */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Combat|Encounter", meta=(DisplayName="On Courtyard Purified"))
	void BP_OnCourtyardPurified();

	/** Returns current overarching story progression state (Phase 5D Subsystem 1) */
	UFUNCTION(BlueprintPure, Category="Ganapati|Story")
	EStoryProgressionState GetStoryProgressionState() const { return CurrentStoryState; }

	/** Returns true if Sacred Journey has begun and the Sacred Path is unlocked (Phase 5D Subsystem 1) */
	UFUNCTION(BlueprintPure, Category="Ganapati|Story")
	bool IsSacredJourneyUnlocked() const { return bSacredJourneyUnlocked; }

	/** Unlocks or locks the Sacred Path gate via the cached street builder (Phase 5D Subsystem 1) */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Story")
	void SetSacredPathUnlocked(bool bUnlocked);

	/** Blueprint implementable event fired when the story progression state changes (Phase 5D Subsystem 1) */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Story", meta=(DisplayName="On Story Progression Changed"))
	void BP_OnStoryProgressionChanged(EStoryProgressionState PreviousState, EStoryProgressionState NewState);

	/** Blueprint implementable event fired when Sacred Journey unlocks (Phase 5D Subsystem 1) */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Story", meta=(DisplayName="On Sacred Journey Unlocked"))
	void BP_OnSacredJourneyUnlocked();

	// ── Phase 6C: World Region & Milestone APIs ──
	/** Returns current active world region (Phase 6C) */
	UFUNCTION(BlueprintPure, Category="Ganapati|World")
	EWorldRegion GetActiveRegion() const;

	/** Returns true if player has discovered Divine Ascension traversal (Phase 6C) */
	UFUNCTION(BlueprintPure, Category="Ganapati|World")
	bool IsDivineAscensionDiscovered() const;

	/** Returns true if player has reached the Mountain Threshold dais (Phase 6C) */
	UFUNCTION(BlueprintPure, Category="Ganapati|World")
	bool IsMountainThresholdReached() const;

	/** Returns true if player has discovered the Sacred Mountain region (Phase 6D) */
	UFUNCTION(BlueprintPure, Category="Ganapati|World")
	bool IsSacredMountainDiscovered() const;

	/** Returns true if player has activated the Kailash Summit Shrine (Phase 6D) */
	UFUNCTION(BlueprintPure, Category="Ganapati|World")
	bool IsMountainShrineActivated() const;

	/** Blueprint implementable event fired when Divine Ascension is discovered (Phase 6C) */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|World", meta=(DisplayName="On Divine Ascension Discovered"))
	void BP_OnDivineAscensionDiscovered();

	/** Blueprint implementable event fired when the Mountain Threshold is reached (Phase 6C) */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|World", meta=(DisplayName="On Mountain Threshold Reached"))
	void BP_OnMountainThresholdReached();

	/** Blueprint implementable event fired when the Sacred Mountain is discovered (Phase 6D) */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|World", meta=(DisplayName="On Sacred Mountain Discovered"))
	void BP_OnSacredMountainDiscovered();

	/** Blueprint implementable event fired when the Kailash Summit Shrine is activated (Phase 6D) */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|World", meta=(DisplayName="On Mountain Shrine Activated"))
	void BP_OnMountainShrineActivated();

public:
	/** Broadcast when a quest step advances */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Quest|Events")
	FOnSacredDarshanStepAdvancedSignature OnQuestStepAdvanced;

	/** Broadcast when the entire Sacred Darshan quest is completed */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Quest|Events")
	FOnSacredDarshanCompletedSignature OnQuestCompleted;

	/** Broadcast when overarching story progression advances (Phase 5D Subsystem 1) */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Story|Events")
	FOnStoryProgressionChangedSignature OnStoryProgressionChanged;

	/** Broadcast when Sacred Journey begins and Sacred Path is unlocked (Phase 5D Subsystem 1) */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Story|Events")
	FOnSacredJourneyUnlockedSignature OnSacredJourneyUnlocked;

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

	// ── Phase 6A: Streaming-Safe Subsystem Registration Event Handlers ──
	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Events")
	void HandleNPCRegistered(AGanapatiNPC* NPC);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Events")
	void HandleNPCUnregistered(AGanapatiNPC* NPC);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Events")
	void HandleInteractableRegistered(AGanapatiInteractable* Interactable);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Events")
	void HandleInteractableUnregistered(AGanapatiInteractable* Interactable);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Events")
	void HandleTrainingDummyRegistered(AGanapatiTrainingDummy* Dummy);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Events")
	void HandleTrainingDummyUnregistered(AGanapatiTrainingDummy* Dummy);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Events")
	void HandleAsuraRegistered(AGanapatiAsuraMinion* Asura);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Events")
	void HandleAsuraUnregistered(AGanapatiAsuraMinion* Asura);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Events")
	void HandleCaptainRegistered(AGanapatiAsuraCaptain* Captain);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Events")
	void HandleCaptainUnregistered(AGanapatiAsuraCaptain* Captain);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Events")
	void HandleStreetBuilderRegistered(AFestivalStreetBuilder* StreetBuilder);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Events")
	void HandleStreetBuilderUnregistered(AFestivalStreetBuilder* StreetBuilder);

	// ── Phase 6C: Subsystem Region & Milestone Handlers ──
	UFUNCTION()
	void HandleWorldRegionChanged(EWorldRegion PreviousRegion, EWorldRegion NewRegion);

	UFUNCTION()
	void HandleDivineAscensionDiscovered(bool bDiscovered);

	UFUNCTION()
	void HandlePilgrimageMilestoneReached(bool bReached);

	// ── Phase 6D: Subsystem Mountain & Shrine Handlers ──
	UFUNCTION()
	void HandleSacredMountainDiscovered(bool bDiscovered);

	UFUNCTION()
	void HandleMountainShrineActivated(bool bActivated);

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

	/** Current state of Asura Captain mini-boss encounter (Phase 5C Subsystem 3) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Combat|Boss")
	ECaptainEncounterState CaptainEncounterState = ECaptainEncounterState::Dormant;

	/** Camera shake triggered when courtyard combat begins */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Encounter")
	TSubclassOf<UCameraShakeBase> EncounterStartCameraShakeClass;

	/** Current overarching session story progression state (Phase 5D Subsystem 1) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Story")
	EStoryProgressionState CurrentStoryState = EStoryProgressionState::FestivalBeginning;

	/** Whether the Sacred Journey has begun and Sacred Path is unlocked (Phase 5D Subsystem 1) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Story")
	bool bSacredJourneyUnlocked = false;

	/** Event handler called after the 2.5-second divine story presentation completes (Phase 5D Subsystem 1) */
	UFUNCTION()
	void OnDivineStoryMomentCompleted();

	/** Event handler when Asura Captain is defeated (Phase 5C Subsystem 3) */
	UFUNCTION()
	void HandleCaptainDied(AGanapatiAsuraMinion* Asura);

	/** Event handler when player character dies (resets active boss fight) */
	UFUNCTION()
	void HandlePlayerDied();

private:
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> CinematicCamera;

	UPROPERTY(Transient)
	TWeakObjectPtr<AGanapatiAsuraCaptain> CachedCaptain;

	UPROPERTY(Transient)
	TWeakObjectPtr<AFestivalStreetBuilder> CachedStreetBuilder;

	FTimerHandle CinematicTimerHandle;
	FTimerHandle CourtyardAlertTimerHandle;
	FTimerHandle CaptainProximityTimerHandle;
	FTimerHandle CaptainIntroTimerHandle;
	FTimerHandle DivineMomentTimerHandle;

	// ── Phase 6A: Streaming-safe encounter tracking sets ──
	TSet<TWeakObjectPtr<AGanapatiAsuraMinion>> TrackedEncounterAsuras;
	TSet<TWeakObjectPtr<AGanapatiAsuraMinion>> TrackedDefeatedAsuras;

	int32 TotalAsurasSpawned = 0;
	int32 DefeatedAsurasCount = 0;
	bool bCourtyardAlertTriggered = false;
	bool bCourtyardPurified = false;

	/** Checks if player has entered the courtyard and triggers skirmish alert toast */
	void CheckCourtyardProximity();

	/** Checks if player has approached the Asura Captain and triggers boss encounter */
	void CheckCaptainProximity();

	/** Initiates the Captain mini-boss encounter */
	void StartCaptainEncounter();

	/** Transitions Captain encounter from Intro to Active */
	void TransitionCaptainToActive();

	/** Evaluates whether courtyard purification criteria are met (Phase 5C Subsystem 4) */
	void CheckCourtyardPurification();

	/** Executes the grand courtyard purification celebration and player blessing */
	void TriggerCourtyardPurification();
};
