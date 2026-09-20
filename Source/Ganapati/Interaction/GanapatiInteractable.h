// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GanapatiInteractable.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class ACameraActor;
class AGanapatiPlayerCharacter;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGanapatiInteractedSignature, AActor*, Interactor, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGanapatiShrineBlessedSignature, AActor*, Interactor, float, DivineEnergyGranted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGanapatiPrayerStartedSignature, AActor*, Interactor, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGanapatiPrayerCompletedSignature, AActor*, Interactor);

/**
 * AGanapatiInteractable
 *
 * Base class for interactive objects in the festival environment
 * (e.g. Ganesh Pandal shrine, festival prasadam stall, bell ringer, lamps).
 */
UCLASS()
class GANAPATI_API AGanapatiInteractable : public AActor
{
	GENERATED_BODY()

public:
	AGanapatiInteractable();

	/** Trigger interaction from player */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Interaction")
	virtual void TriggerInteraction(AActor* Interactor);

	/** Prompt text displayed when player enters trigger radius */
	UFUNCTION(BlueprintPure, Category="Ganapati|Interaction")
	FText GetPromptText() const { return InteractionPrompt; }

	/** Message displayed when interaction completes */
	UFUNCTION(BlueprintPure, Category="Ganapati|Interaction")
	FText GetInteractionMessage() const { return InteractionMessage; }

	/** Set prompt text */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Interaction")
	void SetPromptText(const FText& InPrompt) { InteractionPrompt = InPrompt; }

	/** Set interaction message */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Interaction")
	void SetInteractionMessage(const FText& InMessage) { InteractionMessage = InMessage; }

	/** Sphere component defining interaction range */
	FORCEINLINE USphereComponent* GetTriggerSphere() const { return TriggerSphere; }

	/** Amount of Divine Energy granted by this interactable */
	UFUNCTION(BlueprintPure, Category="Ganapati|Interaction|Blessing")
	float GetDivineEnergyGranted() const { return DivineEnergyGranted; }

	/** Sets the Divine Energy amount granted */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Interaction|Blessing")
	void SetDivineEnergyGranted(float InEnergy) { DivineEnergyGranted = FMath::Max(0.0f, InEnergy); }

	/** Returns true if this interactable fully restores player health */
	UFUNCTION(BlueprintPure, Category="Ganapati|Interaction|Blessing")
	bool DoesRestoreHealth() const { return bRestoresHealth; }

	/** Sets whether health is restored on interaction */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Interaction|Blessing")
	void SetRestoresHealth(bool bInRestores) { bRestoresHealth = bInRestores; }

	/** Static mesh component representing the interactable */
	FORCEINLINE UStaticMeshComponent* GetInteractableMesh() const { return InteractableMesh; }

	/** Sets whether this interactable is single-use */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Interaction")
	void SetSingleUse(bool bInSingleUse) { bSingleUse = bInSingleUse; }

	/** Returns true if this interactable is single-use */
	UFUNCTION(BlueprintPure, Category="Ganapati|Interaction")
	bool IsSingleUse() const { return bSingleUse; }

	/** Returns true if this interactable triggers a cinematic prayer sequence */
	UFUNCTION(BlueprintPure, Category="Ganapati|Interaction|Prayer")
	bool DoesTriggerPrayerSequence() const { return bTriggersPrayerSequence; }

	/** Sets whether this interactable triggers a cinematic prayer sequence */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Interaction|Prayer")
	void SetTriggersPrayerSequence(bool bInTriggers) { bTriggersPrayerSequence = bInTriggers; }

	/** Returns true if a prayer sequence is currently active */
	UFUNCTION(BlueprintPure, Category="Ganapati|Interaction|Prayer")
	bool IsPrayerActive() const { return bIsPrayerActive; }

	/** Broadcast when player interacts */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Interaction|Events")
	FOnGanapatiInteractedSignature OnInteracted;

	/** Broadcast when shrine blessing is bestowed upon an interactor */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Interaction|Events")
	FOnGanapatiShrineBlessedSignature OnShrineBlessed;

	/** Broadcast when prayer interaction starts */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Interaction|Events")
	FOnGanapatiPrayerStartedSignature OnPrayerStarted;

	/** Broadcast when prayer interaction completes */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Interaction|Events")
	FOnGanapatiPrayerCompletedSignature OnPrayerCompleted;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Blueprint hook for custom visual/audio reactions on interaction */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Interaction")
	void BP_OnInteracted(AActor* Interactor);

	/** Blueprint hook for shrine blessing visual/audio reactions */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Interaction|Blessing")
	void BP_OnShrineBlessingGranted(AActor* Interactor, float EnergyGranted);

	/** Blueprint hook when Modak Prasadam is received from a stall */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Interaction|Prasadam")
	void BP_OnModakPrasadamReceived(AActor* Interactor, float EnergyGranted);

	/** Blueprint hook when prayer sequence begins */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Interaction|Prayer")
	void BP_OnPrayerStarted(AActor* Interactor, float Duration);

	/** Blueprint hook when prayer sequence ends */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Interaction|Prayer")
	void BP_OnPrayerCompleted(AActor* Interactor);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components")
	TObjectPtr<USphereComponent> TriggerSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components")
	TObjectPtr<UStaticMeshComponent> InteractableMesh;

	/** Prompt shown in HUD when in range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction")
	FText InteractionPrompt = FText::FromString(TEXT("Press [E] to Interact"));

	/** Message shown in HUD after interaction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction")
	FText InteractionMessage = FText::FromString(TEXT("You offered your prayers with devotion."));

	/** If true, interacting restores the player's health to maximum */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction|Blessing")
	bool bRestoresHealth = true;

	/** Amount of Divine Energy granted upon interaction (safely clamped by player's MaxDivineEnergy) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction|Blessing", meta=(ClampMin=0.0f))
	float DivineEnergyGranted = 50.0f;

	/** If true, can only be interacted with once */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction")
	bool bSingleUse = false;

	// ── Phase 5A Subsystem 2: Prayer Sequence Settings ──
	/** If true, interacting triggers a cinematic prayer sequence locking movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction|Prayer")
	bool bTriggersPrayerSequence = false;

	/** Total prayer sequence hold duration in seconds before blending back */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction|Prayer", meta=(ClampMin=0.5f, Units="s"))
	float PrayerHoldDuration = 2.2f;

	/** Camera blend-in duration when prayer starts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction|Prayer", meta=(ClampMin=0.0f, Units="s"))
	float PrayerBlendInDuration = 0.8f;

	/** Camera blend-out duration when prayer ends */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction|Prayer", meta=(ClampMin=0.0f, Units="s"))
	float PrayerBlendOutDuration = 0.8f;

	/** Relative offset for prayer camera from shrine root */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction|Prayer")
	FVector PrayerCameraRelativeOffset = FVector(-280.0f, -100.0f, 120.0f);

	/** Relative look-at focal point for prayer camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction|Prayer")
	FVector PrayerCameraLookAtOffset = FVector(0.0f, 0.0f, 150.0f);

private:
	void StartPrayerSequence(AGanapatiPlayerCharacter* PlayerChar);
	void FinishPrayerSequence(TWeakObjectPtr<AGanapatiPlayerCharacter> WeakPlayerChar, TWeakObjectPtr<APlayerController> WeakPC);
	void CleanupPrayerCamera();

	/** Track whether single-use has already triggered */
	bool bHasBeenTriggered = false;

	/** Whether a prayer sequence is currently running on this interactable */
	bool bIsPrayerActive = false;

	/** Temporary camera actor spawned for the prayer sequence */
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> PrayerCamera;

	/** Timer handle for finishing prayer hold */
	FTimerHandle PrayerTimerHandle;

	/** Timer handle for completing blend-out and restoring camera target */
	FTimerHandle PrayerBlendTimerHandle;
};
