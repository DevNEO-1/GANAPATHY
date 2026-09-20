// Copyright Ganapati Project. All Rights Reserved.

#include "Controllers/GanapatiMainPlayerController.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "Engine/LocalPlayer.h"

AGanapatiMainPlayerController::AGanapatiMainPlayerController()
{
	// Allow mouse cursor to be hidden in-game for camera look
	bShowMouseCursor = false;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AGanapatiMainPlayerController::CreateInputActionsAndMappingContext()
{
	if (IMC_Ganapati)
	{
		return;
	}

	// ── Create Input Actions ──

	// Move: Axis2D (X = right/left, Y = forward/back)
	IA_Move = NewObject<UInputAction>(this, TEXT("IA_Ganapati_Move"));
	IA_Move->ValueType = EInputActionValueType::Axis2D;

	// Look: Axis2D (X = yaw, Y = pitch)
	IA_Look = NewObject<UInputAction>(this, TEXT("IA_Ganapati_Look"));
	IA_Look->ValueType = EInputActionValueType::Axis2D;

	// Jump: Boolean (press/release)
	IA_Jump = NewObject<UInputAction>(this, TEXT("IA_Ganapati_Jump"));
	IA_Jump->ValueType = EInputActionValueType::Boolean;

	// Sprint: Boolean (hold)
	IA_Sprint = NewObject<UInputAction>(this, TEXT("IA_Ganapati_Sprint"));
	IA_Sprint->ValueType = EInputActionValueType::Boolean;

	// Dash: Boolean (press)
	IA_Dash = NewObject<UInputAction>(this, TEXT("IA_Ganapati_Dash"));
	IA_Dash->ValueType = EInputActionValueType::Boolean;

	// Light Attack: Boolean (press/release)
	IA_LightAttack = NewObject<UInputAction>(this, TEXT("IA_Ganapati_LightAttack"));
	IA_LightAttack->ValueType = EInputActionValueType::Boolean;

	// Charged Attack: Boolean (hold + release)
	IA_ChargedAttack = NewObject<UInputAction>(this, TEXT("IA_Ganapati_ChargedAttack"));
	IA_ChargedAttack->ValueType = EInputActionValueType::Boolean;

	// Anti-Gravity: Boolean (press)
	IA_AntiGravity = NewObject<UInputAction>(this, TEXT("IA_Ganapati_AntiGravity"));
	IA_AntiGravity->ValueType = EInputActionValueType::Boolean;

	// Camera Side: Boolean (press)
	IA_CameraSide = NewObject<UInputAction>(this, TEXT("IA_Ganapati_CameraSide"));
	IA_CameraSide->ValueType = EInputActionValueType::Boolean;

	// Interact: Boolean (press)
	IA_Interact = NewObject<UInputAction>(this, TEXT("IA_Ganapati_Interact"));
	IA_Interact->ValueType = EInputActionValueType::Boolean;

	// Divine Shockwave: Boolean (press)
	IA_DivineShockwave = NewObject<UInputAction>(this, TEXT("IA_Ganapati_DivineShockwave"));
	IA_DivineShockwave->ValueType = EInputActionValueType::Boolean;

	// ── Create Mapping Context ──
	IMC_Ganapati = NewObject<UInputMappingContext>(this, TEXT("IMC_Ganapati"));

	// --- WASD Movement ---
	// W = forward (+Y in 2D)
	{
		FEnhancedActionKeyMapping& Mapping = IMC_Ganapati->MapKey(IA_Move, EKeys::W);
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(IMC_Ganapati);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		Mapping.Modifiers.Add(Swizzle);
	}

	// S = backward (-Y in 2D)
	{
		FEnhancedActionKeyMapping& Mapping = IMC_Ganapati->MapKey(IA_Move, EKeys::S);
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(IMC_Ganapati);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		Mapping.Modifiers.Add(Swizzle);
		UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(IMC_Ganapati);
		Mapping.Modifiers.Add(Negate);
	}

	// D = right (+X in 2D)
	{
		IMC_Ganapati->MapKey(IA_Move, EKeys::D);
	}

	// A = left (-X in 2D)
	{
		FEnhancedActionKeyMapping& Mapping = IMC_Ganapati->MapKey(IA_Move, EKeys::A);
		UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(IMC_Ganapati);
		Mapping.Modifiers.Add(Negate);
	}

	// --- Gamepad Left Stick (two separate axis keys combined into Axis2D) ---
	{
		// Left stick X → move right/left
		IMC_Ganapati->MapKey(IA_Move, EKeys::Gamepad_LeftX);
	}
	{
		// Left stick Y → move forward/back
		FEnhancedActionKeyMapping& Mapping = IMC_Ganapati->MapKey(IA_Move, EKeys::Gamepad_LeftY);
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(IMC_Ganapati);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		Mapping.Modifiers.Add(Swizzle);
	}

	// --- Mouse Look ---
	{
		IMC_Ganapati->MapKey(IA_Look, EKeys::Mouse2D);
	}

	// --- Gamepad Right Stick (two separate axis keys for camera look) ---
	{
		// Right stick X → look yaw
		IMC_Ganapati->MapKey(IA_Look, EKeys::Gamepad_RightX);
	}
	{
		// Right stick Y → look pitch
		FEnhancedActionKeyMapping& Mapping = IMC_Ganapati->MapKey(IA_Look, EKeys::Gamepad_RightY);
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(IMC_Ganapati);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		Mapping.Modifiers.Add(Swizzle);
	}

	// --- Jump (Space / Gamepad FaceButton Bottom) ---
	{
		IMC_Ganapati->MapKey(IA_Jump, EKeys::SpaceBar);
	}
	{
		IMC_Ganapati->MapKey(IA_Jump, EKeys::Gamepad_FaceButton_Bottom);
	}

	// --- Sprint (Left Shift / Gamepad Left Thumbstick Button) ---
	{
		IMC_Ganapati->MapKey(IA_Sprint, EKeys::LeftShift);
	}
	{
		IMC_Ganapati->MapKey(IA_Sprint, EKeys::Gamepad_LeftThumbstick);
	}

	// --- Dash (Left Ctrl / Gamepad FaceButton Right) ---
	{
		IMC_Ganapati->MapKey(IA_Dash, EKeys::LeftControl);
	}
	{
		IMC_Ganapati->MapKey(IA_Dash, EKeys::Gamepad_FaceButton_Right);
	}

	// --- Light Attack (Left Mouse Button / Gamepad Right Shoulder) ---
	{
		IMC_Ganapati->MapKey(IA_LightAttack, EKeys::LeftMouseButton);
	}
	{
		IMC_Ganapati->MapKey(IA_LightAttack, EKeys::Gamepad_RightShoulder);
	}

	// --- Charged Attack (Right Mouse Button / Gamepad Right Trigger) ---
	{
		IMC_Ganapati->MapKey(IA_ChargedAttack, EKeys::RightMouseButton);
	}
	{
		IMC_Ganapati->MapKey(IA_ChargedAttack, EKeys::Gamepad_RightTrigger);
	}

	// --- Anti-Gravity (G / Gamepad Left Trigger) ---
	{
		IMC_Ganapati->MapKey(IA_AntiGravity, EKeys::G);
	}
	{
		IMC_Ganapati->MapKey(IA_AntiGravity, EKeys::Gamepad_LeftTrigger);
	}

	// --- Camera Side Toggle (V / Gamepad Right Thumbstick Button) ---
	{
		IMC_Ganapati->MapKey(IA_CameraSide, EKeys::V);
	}
	{
		IMC_Ganapati->MapKey(IA_CameraSide, EKeys::Gamepad_RightThumbstick);
	}

	// --- Interact (E / Gamepad FaceButton Left) ---
	{
		IMC_Ganapati->MapKey(IA_Interact, EKeys::E);
	}
	{
		IMC_Ganapati->MapKey(IA_Interact, EKeys::Gamepad_FaceButton_Left);
	}

	// --- Divine Shockwave (Q / Gamepad FaceButton Top) ---
	{
		IMC_Ganapati->MapKey(IA_DivineShockwave, EKeys::Q);
	}
	{
		IMC_Ganapati->MapKey(IA_DivineShockwave, EKeys::Gamepad_FaceButton_Top);
	}
}

void AGanapatiMainPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("=== GANAPATI: AGanapatiMainPlayerController::BeginPlay ==="));
	UE_LOG(LogTemp, Warning, TEXT("  Controller Class: %s"), *GetClass()->GetName());
	if (GetPawn())
	{
		UE_LOG(LogTemp, Warning, TEXT("  Possessed Pawn: %s (Class: %s)"), *GetPawn()->GetName(), *GetPawn()->GetClass()->GetName());
	}

	// Create input system programmatically if not already created
	CreateInputActionsAndMappingContext();

	// Clear conflicting template contexts (e.g. IMC_MouseLook, IMC_Default) and register IMC_Ganapati exclusively
	if (IsLocalPlayerController() && IMC_Ganapati)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(IMC_Ganapati, 100);
			UE_LOG(LogTemp, Warning, TEXT("  IMC_Ganapati REGISTERED with priority 100"));
		}
	}
}

void AGanapatiMainPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CreateInputActionsAndMappingContext();

	if (IsLocalPlayerController() && IMC_Ganapati)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(IMC_Ganapati, 100);
		}
	}
}

void AGanapatiMainPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Ensure input actions exist before binding
	CreateInputActionsAndMappingContext();

	// Register mapping context exclusively
	if (IsLocalPlayerController() && IMC_Ganapati)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(IMC_Ganapati, 100);
		}
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Movement
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AGanapatiMainPlayerController::HandleMove);

		// Camera Look
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AGanapatiMainPlayerController::HandleLook);

		// Jump
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &AGanapatiMainPlayerController::HandleJumpStarted);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AGanapatiMainPlayerController::HandleJumpCompleted);

		// Sprint
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &AGanapatiMainPlayerController::HandleSprintStarted);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AGanapatiMainPlayerController::HandleSprintCompleted);

		// Dash
		EIC->BindAction(IA_Dash, ETriggerEvent::Started, this, &AGanapatiMainPlayerController::HandleDash);

		// Light Attack
		EIC->BindAction(IA_LightAttack, ETriggerEvent::Started, this, &AGanapatiMainPlayerController::HandleLightAttackStarted);
		EIC->BindAction(IA_LightAttack, ETriggerEvent::Completed, this, &AGanapatiMainPlayerController::HandleLightAttackCompleted);

		// Charged Attack
		EIC->BindAction(IA_ChargedAttack, ETriggerEvent::Started, this, &AGanapatiMainPlayerController::HandleChargedAttackStarted);
		EIC->BindAction(IA_ChargedAttack, ETriggerEvent::Completed, this, &AGanapatiMainPlayerController::HandleChargedAttackCompleted);

		// Anti-Gravity Toggle
		EIC->BindAction(IA_AntiGravity, ETriggerEvent::Started, this, &AGanapatiMainPlayerController::HandleAntiGravityToggle);

		// Camera Side Toggle
		EIC->BindAction(IA_CameraSide, ETriggerEvent::Started, this, &AGanapatiMainPlayerController::HandleCameraSideToggle);

		// Interact
		EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AGanapatiMainPlayerController::HandleInteract);
		EIC->BindAction(IA_Interact, ETriggerEvent::Triggered, this, &AGanapatiMainPlayerController::HandleInteract);

		// Divine Shockwave (Q)
		EIC->BindAction(IA_DivineShockwave, ETriggerEvent::Started, this, &AGanapatiMainPlayerController::HandleDivineShockwave);
	}
}

AGanapatiPlayerCharacter* AGanapatiMainPlayerController::GetGanapatiCharacter() const
{
	return Cast<AGanapatiPlayerCharacter>(GetPawn());
}

// ── Input handlers route to the character's Do*() API ──

void AGanapatiMainPlayerController::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoMove(MoveInput.X, MoveInput.Y);
	}
	else if (APawn* CurrentPawn = GetPawn())
	{
		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		CurrentPawn->AddMovementInput(ForwardDirection, MoveInput.Y);
		CurrentPawn->AddMovementInput(RightDirection, MoveInput.X);
	}
}

void AGanapatiMainPlayerController::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoLook(LookInput.X, LookInput.Y);
	}
	else
	{
		AddYawInput(LookInput.X);
		AddPitchInput(LookInput.Y);
	}
}

void AGanapatiMainPlayerController::HandleJumpStarted()
{
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoJumpStart();
	}
}

void AGanapatiMainPlayerController::HandleJumpCompleted()
{
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoJumpEnd();
	}
}

void AGanapatiMainPlayerController::HandleSprintStarted()
{
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoSprintStart();
	}
}

void AGanapatiMainPlayerController::HandleSprintCompleted()
{
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoSprintEnd();
	}
}

void AGanapatiMainPlayerController::HandleDash()
{
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoDash();
	}
}

void AGanapatiMainPlayerController::HandleLightAttackStarted()
{
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoLightAttackStart();
	}
}

void AGanapatiMainPlayerController::HandleLightAttackCompleted()
{
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoLightAttackEnd();
	}
}

void AGanapatiMainPlayerController::HandleChargedAttackStarted()
{
	UE_LOG(LogTemp, Warning, TEXT("GANAPATI: HandleChargedAttackStarted (RMB Pressed)"));
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoChargedAttackStart();
	}
}

void AGanapatiMainPlayerController::HandleChargedAttackCompleted()
{
	UE_LOG(LogTemp, Warning, TEXT("GANAPATI: HandleChargedAttackCompleted (RMB Released)"));
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoChargedAttackEnd();
	}
}

void AGanapatiMainPlayerController::HandleAntiGravityToggle()
{
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoToggleAntiGravity();
	}
}

void AGanapatiMainPlayerController::HandleCameraSideToggle()
{
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoToggleCameraSide();
	}
}

void AGanapatiMainPlayerController::HandleInteract()
{
	UE_LOG(LogTemp, Warning, TEXT("GANAPATI: HandleInteract (E Pressed)"));
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoInteract();
	}
}

void AGanapatiMainPlayerController::HandleDivineShockwave()
{
	UE_LOG(LogTemp, Warning, TEXT("GANAPATI: HandleDivineShockwave (Q Pressed)"));
	if (AGanapatiPlayerCharacter* Char = GetGanapatiCharacter())
	{
		Char->DoDivineShockwave();
	}
}
