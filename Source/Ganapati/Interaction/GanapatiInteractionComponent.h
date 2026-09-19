// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GanapatiInteractionComponent.generated.h"

class AGanapatiInteractable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFocusedInteractableChangedSignature, AGanapatiInteractable*, NewInteractable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionCompletedSignature, const FText&, Message);

/**
 * UGanapatiInteractionComponent
 *
 * Senses nearby interactables around the player character,
 * updates the focused target, and dispatches interaction input.
 */
UCLASS(ClassGroup=(Ganapati), meta=(BlueprintSpawnableComponent))
class GANAPATI_API UGanapatiInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGanapatiInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Attempts to interact with the currently focused interactable */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Interaction")
	bool TryInteract();

	/** Gets the current focused interactable (nullptr if none) */
	UFUNCTION(BlueprintPure, Category="Ganapati|Interaction")
	AGanapatiInteractable* GetFocusedInteractable() const { return FocusedInteractable; }

	/** Gets prompt string of current interactable or empty if none */
	UFUNCTION(BlueprintPure, Category="Ganapati|Interaction")
	FText GetCurrentPrompt() const;

	/** Gets the last interaction message displayed */
	UFUNCTION(BlueprintPure, Category="Ganapati|Interaction")
	FText GetLastInteractionMessage() const { return LastInteractionMessage; }

	/** Returns true if an interaction message is actively being shown */
	UFUNCTION(BlueprintPure, Category="Ganapati|Interaction")
	bool HasActiveMessage() const { return MessageTimer > 0.0f; }

public:
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Interaction|Events")
	FOnFocusedInteractableChangedSignature OnFocusedInteractableChanged;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Interaction|Events")
	FOnInteractionCompletedSignature OnInteractionCompleted;

protected:
	virtual void BeginPlay() override;

	/** Scans world sphere around owner for closest AGanapatiInteractable */
	void UpdateFocusedInteractable();

protected:
	/** Radius around player to search for interactables */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction", meta=(ClampMin=50.0f, Units="cm"))
	float InteractionRadius = 500.0f;

	/** Duration in seconds to display an interaction confirmation message */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Interaction", meta=(ClampMin=1.0f, Units="s"))
	float MessageDisplayDuration = 5.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<AGanapatiInteractable> FocusedInteractable;

	FText LastInteractionMessage;
	float MessageTimer = 0.0f;
};
