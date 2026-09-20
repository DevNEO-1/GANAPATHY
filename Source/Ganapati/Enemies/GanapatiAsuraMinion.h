// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CombatAttacker.h"
#include "CombatDamageable.h"
#include "GanapatiAsuraMinion.generated.h"

class UTextRenderComponent;
class AGanapatiPlayerCharacter;

/**
 * Combat AI States for the Asura Minion.
 */
UENUM(BlueprintType)
enum class EAsuraAIState : uint8
{
	Idle UMETA(DisplayName="Idle"),
	Chasing UMETA(DisplayName="Chasing"),
	Attacking UMETA(DisplayName="Attacking"),
	Staggered UMETA(DisplayName="Staggered"),
	Dead UMETA(DisplayName="Dead")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAsuraHealthChangedSignature, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAsuraDiedSignature, AGanapatiAsuraMinion*, Asura);

/**
 * AGanapatiAsuraMinion
 *
 * Hostile Asura minion patrolling the festival courtyard arena.
 * - Senses and chases the player character in the courtyard.
 * - Stops at melee range and executes timed attacks.
 * - Deals damage to the player character only upon confirmed hit trace connection.
 * - Receives player melee damage and Divine Shockwave radial knockback.
 * - Staggers when hit and awards player Divine Energy via existing combat pipeline.
 * - Disables collisions and AI on death.
 */
UCLASS()
class GANAPATI_API AGanapatiAsuraMinion : public ACharacter, public ICombatAttacker, public ICombatDamageable
{
	GENERATED_BODY()

public:
	AGanapatiAsuraMinion();

	virtual void Tick(float DeltaTime) override;

	// ~begin ICombatAttacker interface
	virtual void DoAttackTrace(FName DamageSourceBone) override;
	virtual void CheckCombo() override {}
	virtual void CheckChargedAttack() override {}
	// ~end ICombatAttacker interface

	// ~begin ICombatDamageable interface
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override {}
	// ~end ICombatDamageable interface

	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	EAsuraAIState GetAIState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	bool IsDead() const { return CurrentState == EAsuraAIState::Dead; }

	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	bool CountsTowardEncounter() const { return bCountsTowardEncounter; }

public:
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Combat|Events")
	FOnAsuraHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Combat|Events")
	FOnAsuraDiedSignature OnAsuraDied;

protected:
	virtual void BeginPlay() override;

	/** Core AI state tick */
	void UpdateAI(float DeltaTime);

	/** Initiates melee attack sequence */
	virtual void StartAttack();

	/** Executes the physical damage sweep at the peak of the swing */
	void PerformAttackHitCheck();

	/** Concludes attack recovery */
	void FinishAttack();

	/** Recovers from stagger */
	void RecoverFromStagger();

	/** Updates 3D floating health and status text above minion */
	virtual void UpdateHealthText();

	/** Resets hit-stop freeze on skeletal mesh */
	void ResetHitStop();

	/** Blueprint hook triggered when Asura initiates attack telegraph */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Combat")
	void BP_OnAsuraAttackTelegraphed();

	/** Blueprint hook triggered when Asura takes damage */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Combat")
	void BP_OnAsuraHitReact(float Damage, const FVector& HitLocation, const FVector& HitDirection);

	/** Blueprint hook triggered when Asura is defeated */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Combat")
	void BP_OnAsuraDeathSequence(const FVector& FinalImpulse);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components")
	TObjectPtr<UTextRenderComponent> FloatingHealthText;

	/** Display title for 3D in-world health and status text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat")
	FString EnemyDisplayName = TEXT("ASURA MINION");

	/** Resistance against physics impulses (0.0 = full knockback, 1.0 = immovable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Feel", meta=(ClampMin=0.0f, ClampMax=1.0f))
	float KnockbackResistance = 0.0f;

	/** Whether defeating this enemy counts toward the courtyard skirmish victory condition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat")
	bool bCountsTowardEncounter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat", meta=(ClampMin=1.0f))
	float MaxHP = 50.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Combat")
	float CurrentHP = 50.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Combat")
	EAsuraAIState CurrentState = EAsuraAIState::Idle;

	/** Detection range to acquire the player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|AI", meta=(ClampMin=100.0f, Units="cm"))
	float DetectionRadius = 1400.0f;

	/** Distance at which the minion halts to strike */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|AI", meta=(ClampMin=50.0f, Units="cm"))
	float MeleeStopDistance = 140.0f;

	/** Damage dealt per melee hit to player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat", meta=(ClampMin=1.0f))
	float AttackDamage = 15.0f;

	/** Cooldown between attacks in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|AI", meta=(ClampMin=0.5f, Units="s"))
	float AttackCooldown = 1.8f;

	/** Windup delay before hit check connects (telegraph window) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|AI", meta=(ClampMin=0.1f, Units="s"))
	float AttackWindupTime = 0.45f;

	/** Duration of stagger state on receiving a heavy hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|AI", meta=(ClampMin=0.1f, Units="s"))
	float StaggerDuration = 0.6f;

	/** Brief hit-stop animation freeze duration on hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Feel", meta=(ClampMin=0.01f, Units="s"))
	float HitStopDuration = 0.06f;

	/** Forward distance for melee attack sweep */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat", meta=(ClampMin=20.0f, Units="cm"))
	float AttackReach = 150.0f;

	/** Radius of melee attack sweep sphere */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat", meta=(ClampMin=10.0f, Units="cm"))
	float AttackRadius = 65.0f;

	/** Normal walking speed when chasing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|AI")
	float ChaseSpeed = 360.0f;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AGanapatiPlayerCharacter> TargetPlayer;

	FVector SpawnLocation;
	float LastAttackTime = -10.0f;

	FTimerHandle AttackWindupTimerHandle;
	FTimerHandle AttackRecoveryTimerHandle;
	FTimerHandle StaggerTimerHandle;
	FTimerHandle HitStopTimerHandle;
};
