// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CombatAttacker.h"
#include "CombatDamageable.h"
#include "GanapatiPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UGanapatiCombatComponent;
class UGanapatiMovementComponent;
class UGanapatiInteractionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGanapatiHealthChangedSignature, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGanapatiCharacterDiedSignature);

/**
 * AGanapatiPlayerCharacter
 *
 * The unified third-person player character for the Ganapati project.
 * Designed with a clean, modular component-driven architecture:
 * - UGanapatiCombatComponent: Handles combo strings, charged attacks, traces, and danger notifications.
 * - UGanapatiMovementComponent: Handles sprinting, dash bursts, double jump, coyote time, and wall jumps.
 * - Implements ICombatAttacker: Directly compatible with existing combat animation notifies.
 * - Implements ICombatDamageable: Directly compatible with existing AI attacks, EQS, and hazards.
 * - Health / Damage / Ragdoll / Death lifecycle foundation.
 * - Input-agnostic action hook API ready for Enhanced Input or standard controller binding.
 */
UCLASS(Blueprintable)
class GANAPATI_API AGanapatiPlayerCharacter : public ACharacter, public ICombatAttacker, public ICombatDamageable
{
	GENERATED_BODY()

public:
	AGanapatiPlayerCharacter();

	// ~begin ICombatAttacker interface
	virtual void DoAttackTrace(FName DamageSourceBone) override;
	virtual void CheckCombo() override;
	virtual void CheckChargedAttack() override;
	// ~end ICombatAttacker interface

	// ~begin ICombatDamageable interface
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;
	// ~end ICombatDamageable interface

public:
	/** Handles directional move inputs (from controller or UI) */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs (from controller or UI) */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump press */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoJumpStart();

	/** Handles jump release */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoJumpEnd();

	/** Handles sprint press */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoSprintStart();

	/** Handles sprint release */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoSprintEnd();

	/** Handles dash press */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoDash();

	/** Handles light attack press */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoLightAttackStart();

	/** Handles light attack release */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoLightAttackEnd();

	/** Handles charged heavy attack press */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoChargedAttackStart();

	/** Handles charged heavy attack release */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoChargedAttackEnd();

	/** Toggles the supernatural anti-gravity float mode */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoToggleAntiGravity();

	/** Toggles the camera between left and right shoulder */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoToggleCameraSide();

	/** Handles interaction with nearby shrines, stalls, and NPCs */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Input")
	virtual void DoInteract();

	/** Resets current health to MaxHP */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Health")
	void ResetHealth();

public:
	/** Returns current health amount */
	UFUNCTION(BlueprintPure, Category="Ganapati|Health")
	float GetCurrentHealth() const { return CurrentHP; }

	/** Returns maximum health amount */
	UFUNCTION(BlueprintPure, Category="Ganapati|Health")
	float GetMaxHealth() const { return MaxHP; }

	/** Returns true if character is dead */
	UFUNCTION(BlueprintPure, Category="Ganapati|Health")
	bool IsDead() const { return bIsDead; }

	/** Returns CameraBoom subobject */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Returns Ganapati Combat Component */
	FORCEINLINE UGanapatiCombatComponent* GetCombatComponent() const { return CombatComponent; }

	/** Returns Ganapati Movement Component */
	FORCEINLINE UGanapatiMovementComponent* GetGanapatiMovementComponent() const { return MovementComponent; }

	/** Returns Ganapati Interaction Component */
	FORCEINLINE UGanapatiInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

public:
	/** Broadcast when health changes */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Health|Events")
	FOnGanapatiHealthChangedSignature OnHealthChanged;

	/** Broadcast when character dies */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Health|Events")
	FOnGanapatiCharacterDiedSignature OnCharacterDied;

protected:
	virtual void BeginPlay() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Internal callback from CombatComponent OnDamageDealt */
	UFUNCTION()
	void HandleDamageDealt(float Damage, const FVector& ImpactPoint);

	/** Blueprint implementable hook to trigger VFX/audio when damage is received */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Combat")
	void BP_OnReceivedDamage(float Damage, const FVector& ImpactPoint, const FVector& DamageDirection);

	/** Blueprint implementable hook to trigger VFX/audio when damage is dealt */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Combat")
	void BP_OnDealtDamage(float Damage, const FVector& ImpactPoint);

	/** Blueprint implementable hook when healing is applied */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Combat")
	void BP_OnReceivedHealing(float Healing);

	/** Blueprint implementable hook when character dies */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Combat")
	void BP_OnCharacterDeath();

	/** Blueprint implementable hook when camera side switches */
	UFUNCTION(BlueprintImplementableEvent, Category="Ganapati|Camera")
	void BP_OnCameraSideToggled(bool bIsRightSide);

protected:
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Modular Combat Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UGanapatiCombatComponent> CombatComponent;

	/** Modular Movement Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UGanapatiMovementComponent> MovementComponent;

	/** Modular Interaction Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UGanapatiInteractionComponent> InteractionComponent;

	/** Maximum hit points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Health", meta=(ClampMin=1.0f))
	float MaxHP = 100.0f;

	/** Current hit points */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Health")
	float CurrentHP = 100.0f;

	/** Pelvis bone name used for damage hit physics */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Health")
	FName PelvisBoneName = FName(TEXT("pelvis"));

	/** Default camera boom distance in combat/exploration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Camera", meta=(ClampMin=50.0f, Units="cm"))
	float DefaultCameraDistance = 350.0f;

	/** Camera boom distance pulled back when character dies */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Camera", meta=(ClampMin=100.0f, Units="cm"))
	float DeathCameraDistance = 500.0f;

	/** Lateral shoulder offset distance for camera side toggling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Camera", meta=(ClampMin=0.0f, Units="cm"))
	float ShoulderOffsetDistance = 60.0f;

private:
	bool bIsDead = false;
	bool bIsRightShoulder = true;
};
