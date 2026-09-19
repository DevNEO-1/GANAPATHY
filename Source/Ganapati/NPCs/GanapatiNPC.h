// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GanapatiNPC.generated.h"

UENUM(BlueprintType)
enum class EGanapatiNPCState : uint8
{
	Idle UMETA(DisplayName="Idle"),
	Wandering UMETA(DisplayName="Wandering"),
	Praying UMETA(DisplayName="Praying")
};

/**
 * AGanapatiNPC
 *
 * Festival devotee/townsperson walking around the festival street.
 * Implements autonomous ambient wandering and idle routines.
 */
UCLASS()
class GANAPATI_API AGanapatiNPC : public ACharacter
{
	GENERATED_BODY()

public:
	AGanapatiNPC();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	/** Picks a new state and target location */
	void DecideNextAction();

	/** Move towards TargetLocation */
	void MoveTowardsTarget(float DeltaTime);

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

private:
	FVector SpawnOrigin;
	FVector TargetLocation;
	FTimerHandle StateTimerHandle;
	float StateTimeRemaining = 0.0f;
};
