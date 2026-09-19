// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GanapatiInteractable.generated.h"

class USphereComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGanapatiInteractedSignature, AActor*, Interactor, const FText&, Message);

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

	/** Sphere component defining interaction range */
	FORCEINLINE USphereComponent* GetTriggerSphere() const { return TriggerSphere; }

	/** Broadcast when player interacts */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Interaction|Events")
	FOnGanapatiInteractedSignature OnInteracted;

protected:
	virtual void BeginPlay() override;

	/** Blueprint hook for custom visual/audio reactions on interaction */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Interaction")
	void BP_OnInteracted(AActor* Interactor);

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

	/** If true, can only be interacted with once */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction")
	bool bSingleUse = false;

	/** Track whether single-use has already triggered */
	bool bHasBeenTriggered = false;
};
