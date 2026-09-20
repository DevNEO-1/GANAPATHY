// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GanapatiInteractable.generated.h"

class USphereComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGanapatiInteractedSignature, AActor*, Interactor, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGanapatiShrineBlessedSignature, AActor*, Interactor, float, DivineEnergyGranted);

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

	/** Broadcast when player interacts */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Interaction|Events")
	FOnGanapatiInteractedSignature OnInteracted;

	/** Broadcast when shrine blessing is bestowed upon an interactor */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Interaction|Events")
	FOnGanapatiShrineBlessedSignature OnShrineBlessed;

protected:
	virtual void BeginPlay() override;

	/** Blueprint hook for custom visual/audio reactions on interaction */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Interaction")
	void BP_OnInteracted(AActor* Interactor);

	/** Blueprint hook for shrine blessing visual/audio reactions */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Interaction|Blessing")
	void BP_OnShrineBlessingGranted(AActor* Interactor, float EnergyGranted);

	/** Blueprint hook when Modak Prasadam is received from a stall */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Interaction|Prasadam")
	void BP_OnModakPrasadamReceived(AActor* Interactor, float EnergyGranted);

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

	/** Track whether single-use has already triggered */
	bool bHasBeenTriggered = false;
};
