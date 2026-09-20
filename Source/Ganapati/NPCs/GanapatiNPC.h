// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GanapatiNPC.generated.h"

class AGanapatiInteractable;

UENUM(BlueprintType)
enum class EGanapatiNPCState : uint8
{
	Idle UMETA(DisplayName="Idle"),
	Wandering UMETA(DisplayName="Wandering"),
	Praying UMETA(DisplayName="Praying"),
	Talking UMETA(DisplayName="Talking")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGanapatiNPCSpeechSignature, AGanapatiNPC*, NPC, AActor*, Interactor, const FText&, SpokenLine);

/**
 * AGanapatiNPC
 *
 * Festival devotee/townsperson walking around the festival street.
 * Implements autonomous ambient wandering, idle routines, and interactive dialogue.
 */
UCLASS()
class GANAPATI_API AGanapatiNPC : public ACharacter
{
	GENERATED_BODY()

public:
	AGanapatiNPC();

	virtual void Tick(float DeltaTime) override;

	/** Configures the NPC's role name and dialogue pool */
	UFUNCTION(BlueprintCallable, Category="Ganapati|NPC")
	void SetNPCProfile(const FString& InName, const TArray<FText>& InDialogueLines);

	/** Returns current ambient/talking state */
	UFUNCTION(BlueprintPure, Category="Ganapati|NPC")
	EGanapatiNPCState GetNPCState() const { return CurrentState; }

	/** Returns NPC role name */
	UFUNCTION(BlueprintPure, Category="Ganapati|NPC")
	FString GetNPCName() const { return NPCName; }

	/** Resumes normal idle/wandering behavior */
	UFUNCTION(BlueprintCallable, Category="Ganapati|NPC")
	void EndConversation();

public:
	/** Broadcast when this NPC speaks a dialogue line */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|NPC|Events")
	FOnGanapatiNPCSpeechSignature OnNPCDialogueSpoken;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Picks a new state and target location */
	void DecideNextAction();

	/** Move towards TargetLocation */
	void MoveTowardsTarget(float DeltaTime);

	/** Called when player interacts with this NPC's interaction trigger */
	UFUNCTION()
	void HandleInteraction(AActor* Interactor, const FText& Message);

	/** Blueprint hook triggered when NPC speaks dialogue line */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|NPC")
	void BP_OnNPCDialogueSpoken(AActor* Interactor, const FText& SpokenLine);

protected:
	/** NPC Name / Role */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|NPC")
	FString NPCName = TEXT("Devotee");

	/** Current ambient state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|NPC")
	EGanapatiNPCState CurrentState = EGanapatiNPCState::Idle;

	/** Wander radius around initial spawn position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|NPC", meta=(ClampMin=100.0f, Units="cm"))
	float WanderRadius = 800.0f;

	/** Minimum duration to stay idle before wandering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|NPC", meta=(ClampMin=1.0f, Units="s"))
	float MinIdleDuration = 2.0f;

	/** Maximum duration to stay idle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|NPC", meta=(ClampMin=1.0f, Units="s"))
	float MaxIdleDuration = 5.0f;

	// ── Phase 5A Subsystem 3: Interactive Dialogue ──
	/** Attached interaction trigger enabling player [E] interaction */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|NPC")
	TObjectPtr<AGanapatiInteractable> InteractionTrigger;

	/** Dialogue lines spoken by this NPC when interacted with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|NPC")
	TArray<FText> DialogueLines;

	/** Current dialogue index in the cycling sequence */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|NPC")
	int32 CurrentDialogueIndex = -1;

	/** How long after talking before resuming wandering/idling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|NPC", meta=(ClampMin=1.0f, Units="s"))
	float ConversationTimeout = 5.0f;

	/** Turn interpolation speed to face the interacting player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|NPC", meta=(ClampMin=1.0f, Units="deg/s"))
	float FacePlayerTurnSpeed = 8.0f;

private:
	FVector SpawnOrigin;
	FVector TargetLocation;
	FTimerHandle StateTimerHandle;
	FTimerHandle ConversationTimerHandle;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> InteractingPlayer;
};
