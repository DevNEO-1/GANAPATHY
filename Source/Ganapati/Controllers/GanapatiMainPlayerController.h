// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "GanapatiMainPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class AGanapatiPlayerCharacter;

/**
 * AGanapatiMainPlayerController
 *
 * The concrete PlayerController for the main Ganapati game.
 * Creates all Enhanced Input Actions and Mapping Context programmatically,
 * then routes input to AGanapatiPlayerCharacter's Do*() action hook API.
 *
 * Keybindings:
 *   WASD          → Move
 *   Mouse         → Look / Camera
 *   Space         → Jump / Double Jump
 *   Left Shift    → Sprint (hold)
 *   Left Ctrl     → Dash
 *   Left Click    → Light Attack
 *   Right Click   → Charged Attack (hold + release)
 *   G             → Toggle Anti-Gravity
 *   V             → Toggle Camera Shoulder Side
 */
UCLASS()
class GANAPATI_API AGanapatiMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGanapatiMainPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

	// ── Enhanced Input Actions (created programmatically) ──
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> IA_Jump;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> IA_Sprint;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> IA_Dash;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> IA_LightAttack;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> IA_ChargedAttack;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> IA_AntiGravity;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> IA_CameraSide;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> IA_Interact;

	// ── Mapping Context ──
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> IMC_Ganapati;

private:
	/** Creates all UInputAction objects and builds the UInputMappingContext with key bindings */
	void CreateInputActionsAndMappingContext();

	/** Helper to get the possessed Ganapati character */
	AGanapatiPlayerCharacter* GetGanapatiCharacter() const;

	// ── Input handlers ──
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpCompleted();
	void HandleSprintStarted();
	void HandleSprintCompleted();
	void HandleDash();
	void HandleLightAttackStarted();
	void HandleLightAttackCompleted();
	void HandleChargedAttackStarted();
	void HandleChargedAttackCompleted();
	void HandleAntiGravityToggle();
	void HandleCameraSideToggle();
	void HandleInteract();
};
