// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/GanapatiPlayerCharacter.h"
#include "Components/GanapatiCombatComponent.h"
#include "Components/GanapatiMovementComponent.h"
#include "Interaction/GanapatiInteractionComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AGanapatiPlayerCharacter::AGanapatiPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// Set capsule size for standard humanoid character
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// Character doesn't rotate with controller; camera orbits independently
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure responsive third-person movement defaults
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 550.0f;
	GetCharacterMovement()->AirControl = 0.4f;
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// ── Auto-load Manny skeletal mesh so the character is immediately playable ──
	// This removes the need for a Blueprint just to assign a mesh for prototyping.
	// A Blueprint override can still replace these defaults later.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMeshFinder(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MannyMeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannyMeshFinder.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	// ── Auto-load Combat animation blueprint (with fallback to Unarmed) ──
	static ConstructorHelpers::FClassFinder<UAnimInstance> CombatAnimBPFinder(
		TEXT("/Game/Variant_Combat/Anims/ABP_Manny_Combat"));
	if (CombatAnimBPFinder.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(CombatAnimBPFinder.Class);
	}
	else
	{
		static ConstructorHelpers::FClassFinder<UAnimInstance> UnarmedAnimBPFinder(
			TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
		if (UnarmedAnimBPFinder.Succeeded())
		{
			GetMesh()->SetAnimInstanceClass(UnarmedAnimBPFinder.Class);
		}
	}

	// Camera boom configuration
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = DefaultCameraDistance;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.0f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 12.0f;
	CameraBoom->SocketOffset = FVector(0.0f, ShoulderOffsetDistance, 45.0f);

	// Follow camera configuration
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Modular components
	CombatComponent = CreateDefaultSubobject<UGanapatiCombatComponent>(TEXT("CombatComponent"));
	MovementComponent = CreateDefaultSubobject<UGanapatiMovementComponent>(TEXT("MovementComponent"));
	InteractionComponent = CreateDefaultSubobject<UGanapatiInteractionComponent>(TEXT("InteractionComponent"));

	// Tag for AI and EQS recognition
	Tags.Add(FName(TEXT("Player")));
}

void AGanapatiPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	ResetHealth();

	if (CombatComponent)
	{
		CombatComponent->OnDamageDealt.AddDynamic(this, &AGanapatiPlayerCharacter::HandleDamageDealt);
	}
}

void AGanapatiPlayerCharacter::HandleDamageDealt(float Damage, const FVector& ImpactPoint)
{
	BP_OnDealtDamage(Damage, ImpactPoint);
}

// ~begin ICombatAttacker interface
void AGanapatiPlayerCharacter::DoAttackTrace(FName DamageSourceBone)
{
	if (CombatComponent)
	{
		CombatComponent->DoAttackTrace(DamageSourceBone);
	}
}

void AGanapatiPlayerCharacter::CheckCombo()
{
	if (CombatComponent)
	{
		CombatComponent->CheckCombo();
	}
}

void AGanapatiPlayerCharacter::CheckChargedAttack()
{
	if (CombatComponent)
	{
		CombatComponent->CheckChargedAttack();
	}
}
// ~end ICombatAttacker interface

// ~begin ICombatDamageable interface
void AGanapatiPlayerCharacter::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	if (bIsDead)
	{
		return;
	}

	FDamageEvent DamageEvent;
	const float ActualDamage = TakeDamage(Damage, DamageEvent, nullptr, DamageCauser);

	if (ActualDamage > 0.0f)
	{
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->AddImpulse(DamageImpulse, true);
		}

		if (USkeletalMeshComponent* CharacterMesh = GetMesh())
		{
			if (CharacterMesh->IsSimulatingPhysics())
			{
				CharacterMesh->AddImpulseAtLocation(DamageImpulse * CharacterMesh->GetMass(), DamageLocation);
			}
		}

		BP_OnReceivedDamage(ActualDamage, DamageLocation, DamageImpulse.GetSafeNormal());
	}
}

void AGanapatiPlayerCharacter::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetSimulatePhysics(true);
	}

	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = DeathCameraDistance;
	}

	OnCharacterDied.Broadcast();
	BP_OnCharacterDeath();
}

void AGanapatiPlayerCharacter::ApplyHealing(float Healing, AActor* Healer)
{
	if (bIsDead || Healing <= 0.0f)
	{
		return;
	}

	CurrentHP = FMath::Clamp(CurrentHP + Healing, 0.0f, MaxHP);
	OnHealthChanged.Broadcast(CurrentHP, MaxHP);
	BP_OnReceivedHealing(Healing);
}

void AGanapatiPlayerCharacter::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// Incoming threat notification hook for player awareness / UI warning indicators
}
// ~end ICombatDamageable interface

float AGanapatiPlayerCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || Damage <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHP = FMath::Clamp(CurrentHP - Damage, 0.0f, MaxHP);
	OnHealthChanged.Broadcast(CurrentHP, MaxHP);

	if (CurrentHP <= 0.0f)
	{
		HandleDeath();
	}
	else
	{
		// Partial ragdoll blend weight on hit reaction while preserving pelvis alignment
		if (USkeletalMeshComponent* CharacterMesh = GetMesh())
		{
			CharacterMesh->SetPhysicsBlendWeight(0.35f);
			CharacterMesh->SetBodySimulatePhysics(PelvisBoneName, false);
		}
	}

	return Damage;
}

void AGanapatiPlayerCharacter::ResetHealth()
{
	CurrentHP = MaxHP;
	bIsDead = false;

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetPhysicsBlendWeight(0.0f);
	}

	OnHealthChanged.Broadcast(CurrentHP, MaxHP);
}

void AGanapatiPlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (MovementComponent)
	{
		MovementComponent->NotifyLanded(Hit);
	}

	if (!bIsDead)
	{
		if (USkeletalMeshComponent* CharacterMesh = GetMesh())
		{
			CharacterMesh->SetPhysicsBlendWeight(0.0f);
		}
	}
}

void AGanapatiPlayerCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (MovementComponent && GetCharacterMovement())
	{
		MovementComponent->NotifyMovementModeChanged(PrevMovementMode, GetCharacterMovement()->MovementMode);
	}
}

// Input and Action Hook API
void AGanapatiPlayerCharacter::DoMove(float Right, float Forward)
{
	if (bIsDead || !Controller)
	{
		return;
	}

	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, Forward);
	AddMovementInput(RightDirection, Right);
}

void AGanapatiPlayerCharacter::DoLook(float Yaw, float Pitch)
{
	if (!Controller)
	{
		return;
	}

	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
}

void AGanapatiPlayerCharacter::DoJumpStart()
{
	if (bIsDead || !MovementComponent)
	{
		return;
	}

	MovementComponent->HandleJumpStart();
}

void AGanapatiPlayerCharacter::DoJumpEnd()
{
	if (MovementComponent)
	{
		MovementComponent->HandleJumpEnd();
	}
}

void AGanapatiPlayerCharacter::DoSprintStart()
{
	if (bIsDead || !MovementComponent)
	{
		return;
	}

	MovementComponent->StartSprint();
}

void AGanapatiPlayerCharacter::DoSprintEnd()
{
	if (MovementComponent)
	{
		MovementComponent->StopSprint();
	}
}

void AGanapatiPlayerCharacter::DoDash()
{
	if (bIsDead || !MovementComponent)
	{
		return;
	}

	MovementComponent->StartDash();
}

void AGanapatiPlayerCharacter::DoLightAttackStart()
{
	if (bIsDead || !CombatComponent)
	{
		return;
	}

	CombatComponent->StartLightAttack();
}

void AGanapatiPlayerCharacter::DoLightAttackEnd()
{
	if (CombatComponent)
	{
		CombatComponent->StopLightAttack();
	}
}

void AGanapatiPlayerCharacter::DoChargedAttackStart()
{
	if (bIsDead || !CombatComponent)
	{
		return;
	}

	CombatComponent->StartChargedAttack();
}

void AGanapatiPlayerCharacter::DoChargedAttackEnd()
{
	if (CombatComponent)
	{
		CombatComponent->StopChargedAttack();
	}
}

void AGanapatiPlayerCharacter::DoToggleAntiGravity()
{
	if (bIsDead || !MovementComponent)
	{
		return;
	}

	MovementComponent->ToggleAntiGravityMode();
}

void AGanapatiPlayerCharacter::DoToggleCameraSide()
{
	if (!CameraBoom)
	{
		return;
	}

	bIsRightShoulder = !bIsRightShoulder;
	CameraBoom->SocketOffset.Y = bIsRightShoulder ? ShoulderOffsetDistance : -ShoulderOffsetDistance;
	BP_OnCameraSideToggled(bIsRightShoulder);
}

void AGanapatiPlayerCharacter::DoInteract()
{
	if (bIsDead || !InteractionComponent)
	{
		return;
	}

	InteractionComponent->TryInteract();
}

