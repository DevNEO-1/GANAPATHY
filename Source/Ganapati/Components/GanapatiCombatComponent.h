// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatAttacker.h"
#include "CombatDamageable.h"
#include "Animation/AnimInstance.h"
#include "GanapatiCombatComponent.generated.h"

class ACharacter;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGanapatiDamageDealtSignature, float, Damage, const FVector&, ImpactPoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGanapatiAttackStateChangedSignature, bool, bIsAttacking);

/**
 * UGanapatiCombatComponent
 *
 * Encapsulates the core combat capabilities of the Ganapati player character:
 * - Multi-stage light combo attacks with input buffering
 * - Press-and-hold charged / heavy attacks with charge loops
 * - Dynamic sphere sweep melee collision detection
 * - Knockback and launch impulse calculations
 * - Threat broadcasting to notify nearby AI enemies of incoming attacks
 * - Extensible weapon statistics and socket routing for future mythological weapons
 */
UCLASS(ClassGroup=(Ganapati), meta=(BlueprintSpawnableComponent))
class GANAPATI_API UGanapatiCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGanapatiCombatComponent();

	/** Called to initiate or queue a light combo attack */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat")
	void StartLightAttack();

	/** Called when light attack input is released */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat")
	void StopLightAttack();

	/** Called to initiate or charge a heavy attack */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat")
	void StartChargedAttack();

	/** Called when charged attack input is released to resolve the heavy strike */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat")
	void StopChargedAttack();

	/** Executes the melee attack sphere sweep collision from a weapon or bone socket. Invoked via AnimNotify_DoAttackTrace */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat")
	void DoAttackTrace(FName DamageSourceBone);

	/** Validates if another attack in the combo string is queued. Invoked via AnimNotify_CheckCombo */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat")
	void CheckCombo();

	/** Checks whether to keep looping or resolve the charged attack. Invoked via AnimNotify_CheckChargedAttack */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat")
	void CheckChargedAttack();

	/** Notifies nearby AI enemies in front of the character that a strike is impending so they can react/dodge */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat")
	void NotifyEnemiesOfIncomingAttack();

	/** Returns true if currently performing an attack montage */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	bool IsAttacking() const { return bIsAttacking; }

	/** Returns true if currently charging a heavy attack */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	bool IsChargingAttack() const { return bIsChargingAttack; }

	/** Sets the active weapon damage multiplier */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat|Weapon")
	void SetWeaponDamageMultiplier(float NewMultiplier) { WeaponDamageMultiplier = FMath::Max(0.1f, NewMultiplier); }

	/** Sets the weapon socket name used for attack traces */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat|Weapon")
	void SetWeaponSocketName(FName NewSocketName) { WeaponSocketName = NewSocketName; }

	/** Gets the active weapon socket name */
	UFUNCTION(BlueprintPure, Category="Ganapati|Combat|Weapon")
	FName GetWeaponSocketName() const { return WeaponSocketName; }

public:
	/** Broadcast when the character successfully deals melee damage to an actor */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Combat|Events")
	FOnGanapatiDamageDealtSignature OnDamageDealt;

	/** Broadcast when entering or exiting an attack animation state */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Combat|Events")
	FOnGanapatiAttackStateChangedSignature OnAttackStateChanged;

protected:
	virtual void BeginPlay() override;

	/** Internal handler to play light combo attacks */
	void ExecuteComboAttack();

	/** Internal handler to play charged heavy attacks */
	void ExecuteChargedAttack();

	/** Switches montage section to loop or release depending on input state */
	void LoopOrResolveChargedAttack();

	/** Callback when an attack montage completes or gets interrupted */
	void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Helper to get the owning character safely */
	ACharacter* GetCharacterOwner() const;

	/** Helper to get the owning character's mesh component */
	USkeletalMeshComponent* GetOwnerMesh() const;

protected:
	/** Montage played for light attack combo strings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Combo")
	TObjectPtr<UAnimMontage> ComboAttackMontage;

	/** Section names in ComboAttackMontage corresponding to consecutive combo hits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Combo")
	TArray<FName> ComboSectionNames;

	/** Montage played for heavy / charged attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Charged")
	TObjectPtr<UAnimMontage> ChargedAttackMontage;

	/** Montage section name for holding the charge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Charged")
	FName ChargeLoopSection;

	/** Montage section name for releasing the charged attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Charged")
	FName ChargeAttackSection;

	/** Base melee damage dealt per attack hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Damage", meta=(ClampMin=0.0f))
	float BaseMeleeDamage = 10.0f;

	/** Multiplier applied to BaseMeleeDamage (scaled by weapon equipped) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Damage", meta=(ClampMin=0.1f))
	float WeaponDamageMultiplier = 1.0f;

	/** Knockback impulse applied away from hit impact normal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Impulse", meta=(ClampMin=0.0f, Units="cm/s"))
	float MeleeKnockbackImpulse = 350.0f;

	/** Vertical impulse applied to launch hit enemies into the air */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Impulse", meta=(ClampMin=0.0f, Units="cm/s"))
	float MeleeLaunchImpulse = 300.0f;

	/** Sphere radius for melee hit trace sweeps */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Trace", meta=(ClampMin=1.0f, Units="cm"))
	float MeleeTraceRadius = 75.0f;

	/** Distance forward along character/bone forward vector to sweep */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Trace", meta=(ClampMin=1.0f, Units="cm"))
	float MeleeTraceDistance = 80.0f;

	/** Threat detection distance in front of character to notify AI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|AI Notice", meta=(ClampMin=10.0f, Units="cm"))
	float DangerTraceDistance = 300.0f;

	/** Threat detection radius to notify AI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|AI Notice", meta=(ClampMin=10.0f, Units="cm"))
	float DangerTraceRadius = 100.0f;

	/** Tolerance window in seconds where an input can be buffered to continue a combo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Timing", meta=(ClampMin=0.05f, Units="s"))
	float ComboInputCacheTimeTolerance = 0.45f;

	/** Tolerance window in seconds for non-combo attack input buffer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Timing", meta=(ClampMin=0.05f, Units="s"))
	float AttackInputCacheTimeTolerance = 1.0f;

	/** Default socket name to trace from on the skeletal mesh if not specified */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Weapon")
	FName WeaponSocketName = FName(TEXT("hand_r"));

private:
	/** True while playing an attack animation montage */
	bool bIsAttacking = false;

	/** True while holding down the charged attack input */
	bool bIsChargingAttack = false;

	/** True once the charge animation loop section has started */
	bool bHasLoopedChargedAttack = false;

	/** True once the user releases the charge button */
	bool bHasReleasedChargedAttack = false;

	/** Current stage index in the combo string */
	int32 ComboCount = 0;

	/** Timestamp when attack input was last registered */
	float CachedAttackInputTime = 0.0f;

	/** Cached delegate for montage completion */
	FOnMontageEnded AttackMontageEndedDelegate;
};
