// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Animation/AnimInstance.h"
#include "GanapatiMovementComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGanapatiAntiGravityToggledSignature, bool, bIsActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGanapatiSprintStateChangedSignature, bool, bIsSprinting);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGanapatiDashStateChangedSignature, bool, bIsDashing);

/**
 * UGanapatiMovementComponent
 *
 * Encapsulates advanced locomotion capabilities for the Ganapati player character:
 * - Dynamic Sprinting (walk speed adjustments)
 * - Zero-Gravity Directional Dash (burst impulse + montage playback)
 * - Multi-Jump & Double Jump
 * - Coyote Time Jump buffering
 * - Wall Jump (wall detection sweep, character reorientation, launch impulse)
 * - Anti-Gravity / Divine Supernatural Mode Foundation (low-gravity float / levitation physics)
 * - Landed and movement mode notification hooks
 */
UCLASS(ClassGroup=(Ganapati), meta=(BlueprintSpawnableComponent))
class GANAPATI_API UGanapatiMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGanapatiMovementComponent();

	/** Called when jump input is pressed. Evaluates ground jump, wall jump, coyote jump, or double jump */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Movement|Jump")
	void HandleJumpStart();

	/** Called when jump input is released */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Movement|Jump")
	void HandleJumpEnd();

	/** Starts sprinting by increasing maximum walk speed */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Movement|Sprint")
	void StartSprint();

	/** Stops sprinting by restoring normal walk speed */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Movement|Sprint")
	void StopSprint();

	/** Triggers a zero-gravity directional dash burst */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Movement|Dash")
	void StartDash();

	/** Ends the dash state and restores gravity */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Movement|Dash")
	void EndDash();

	/** Toggles the supernatural anti-gravity / levitation mode */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Movement|AntiGravity")
	void ToggleAntiGravityMode();

	/** Sets the supernatural anti-gravity / levitation mode state */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Movement|AntiGravity")
	void SetAntiGravityMode(bool bEnable);

	/** Notifies component of landing events to reset air jumps, dash, and wall jump state */
	void NotifyLanded(const FHitResult& Hit);

	/** Notifies component of movement mode transitions to track coyote time */
	void NotifyMovementModeChanged(EMovementMode PrevMovementMode, EMovementMode NewMovementMode);

	/** Returns true if currently dashing */
	UFUNCTION(BlueprintPure, Category="Ganapati|Movement")
	bool IsDashing() const { return bIsDashing; }

	/** Returns true if currently sprinting */
	UFUNCTION(BlueprintPure, Category="Ganapati|Movement")
	bool IsSprinting() const { return bIsSprinting; }

	/** Returns true if anti-gravity mode is active */
	UFUNCTION(BlueprintPure, Category="Ganapati|Movement")
	bool IsAntiGravityActive() const { return bIsAntiGravityActive; }

	/** Returns true if the character has performed an air double jump */
	UFUNCTION(BlueprintPure, Category="Ganapati|Movement")
	bool HasDoubleJumped() const { return bHasDoubleJumped; }

	/** Returns true if the character has performed a wall jump */
	UFUNCTION(BlueprintPure, Category="Ganapati|Movement")
	bool HasWallJumped() const { return bHasWallJumped; }

public:
	/** Broadcast when anti-gravity mode is toggled */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Movement|Events")
	FOnGanapatiAntiGravityToggledSignature OnAntiGravityToggled;

	/** Broadcast when sprint state changes */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Movement|Events")
	FOnGanapatiSprintStateChangedSignature OnSprintStateChanged;

	/** Broadcast when dash state changes */
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Movement|Events")
	FOnGanapatiDashStateChangedSignature OnDashStateChanged;

protected:
	virtual void BeginPlay() override;

	/** Attempts to perform a wall jump if against a valid obstacle */
	bool TryWallJump();

	/** Resets the wall jump lockout timer */
	void ResetWallJump();

	/** Callback when dash montage ends */
	void HandleDashMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Helper to get the character owner safely */
	ACharacter* GetCharacterOwner() const;

	/** Helper to get character movement component safely */
	UCharacterMovementComponent* GetMovementComponent() const;

protected:
	/** Standard grounded walking speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|Sprint", meta=(ClampMin=100.0f, Units="cm/s"))
	float NormalWalkSpeed = 500.0f;

	/** Sprinting speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|Sprint", meta=(ClampMin=100.0f, Units="cm/s"))
	float SprintWalkSpeed = 850.0f;

	/** Impulse applied along forward/input vector during a dash */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|Dash", meta=(ClampMin=0.0f, Units="cm/s"))
	float DashImpulse = 1800.0f;

	/** Fallback duration in seconds for dash if no montage is assigned */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|Dash", meta=(ClampMin=0.05f, Units="s"))
	float DashDurationFallback = 0.25f;

	/** Optional animation montage for dashing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|Dash")
	TObjectPtr<UAnimMontage> DashMontage;

	/** Upward Z velocity applied on double jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|Jump", meta=(ClampMin=0.0f, Units="cm/s"))
	float DoubleJumpZVelocity = 550.0f;

	/** Maximum allowed time after walking off a ledge to still execute a jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|Jump", meta=(ClampMin=0.0f, Units="s"))
	float MaxCoyoteTime = 0.16f;

	/** Distance forward to sweep for wall jump surfaces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|Wall Jump", meta=(ClampMin=10.0f, Units="cm"))
	float WallJumpTraceDistance = 60.0f;

	/** Sphere radius for wall jump detection sweep */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|Wall Jump", meta=(ClampMin=5.0f, Units="cm"))
	float WallJumpTraceRadius = 30.0f;

	/** Impulse applied away from the wall normal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|Wall Jump", meta=(ClampMin=0.0f, Units="cm/s"))
	float WallJumpBounceImpulse = 800.0f;

	/** Upward vertical impulse applied during a wall jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|Wall Jump", meta=(ClampMin=0.0f, Units="cm/s"))
	float WallJumpVerticalImpulse = 900.0f;

	/** Cooldown lock time between wall jumps */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|Wall Jump", meta=(ClampMin=0.05f, Units="s"))
	float DelayBetweenWallJumps = 0.15f;

	/** Normal gravity scale restored when leaving zero-g or dash */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|AntiGravity", meta=(ClampMin=0.1f))
	float DefaultGravityScale = 1.75f;

	/** Reduced gravity scale used during anti-gravity / divine float mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|AntiGravity", meta=(ClampMin=0.0f))
	float AntiGravityScale = 0.15f;

	/** Air control multiplier while anti-gravity mode is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|AntiGravity", meta=(ClampMin=0.0f, ClampMax=1.0f))
	float AntiGravityAirControl = 1.0f;

	/** Falling braking deceleration applied while anti-gravity mode is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Movement|AntiGravity", meta=(ClampMin=0.0f))
	float AntiGravityBrakingDeceleration = 2000.0f;

private:
	bool bIsSprinting = false;
	bool bIsDashing = false;
	bool bHasDashed = false;
	bool bHasDoubleJumped = false;
	bool bHasWallJumped = false;
	bool bIsAntiGravityActive = false;

	float LastFallTime = 0.0f;

	FTimerHandle WallJumpTimerHandle;
	FTimerHandle DashFallbackTimerHandle;
	FOnMontageEnded DashMontageEndedDelegate;
};
