#include "Characters/GanapatiPlayerCharacter.h"
#include "Components/GanapatiCombatComponent.h"
#include "Components/GanapatiMovementComponent.h"
#include "Interaction/GanapatiInteractionComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AGanapatiPlayerCharacter::AGanapatiPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

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

	// ── Auto-load Camera Shake classes ──
	static ConstructorHelpers::FClassFinder<UCameraShakeBase> HitEnemyShakeFinder(
		TEXT("/Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy"));
	if (HitEnemyShakeFinder.Succeeded())
	{
		MeleeHitCameraShakeClass = HitEnemyShakeFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UCameraShakeBase> HitPlayerShakeFinder(
		TEXT("/Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Player"));
	if (HitPlayerShakeFinder.Succeeded())
	{
		HeavyAttackCameraShakeClass = HitPlayerShakeFinder.Class;
		HardLandingCameraShakeClass = HitPlayerShakeFinder.Class;
		DashCameraShakeClass = HitPlayerShakeFinder.Class;
	}
}

void AGanapatiPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	ResetHealth();

	if (FollowCamera)
	{
		FollowCamera->SetFieldOfView(DefaultFOV);
	}
	PeakFallVelocityZ = 0.0f;

	if (CombatComponent)
	{
		CombatComponent->OnDamageDealt.AddDynamic(this, &AGanapatiPlayerCharacter::HandleDamageDealt);
	}
}

void AGanapatiPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 1. Dynamic FOV smooth interpolation
	if (FollowCamera)
	{
		float TargetFOV = DefaultFOV;

		if (MovementComponent && MovementComponent->IsDashing())
		{
			TargetFOV = DashFOV;
		}
		else if (MovementComponent && MovementComponent->IsSprinting() && GetVelocity().SizeSquared2D() > 10000.0f)
		{
			TargetFOV = SprintFOV;
		}

		const float CurrentFOV = FollowCamera->FieldOfView;
		const float InterpSpeed = (TargetFOV > CurrentFOV) ? FOVInterpSpeedIn : FOVInterpSpeedOut;
		FollowCamera->SetFieldOfView(FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaSeconds, InterpSpeed));
	}

	// 2. Track peak downward falling velocity for landing impact feedback
	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		const float CurrentVelZ = GetVelocity().Z;
		if (CurrentVelZ < PeakFallVelocityZ)
		{
			PeakFallVelocityZ = CurrentVelZ;
		}
	}
}

void AGanapatiPlayerCharacter::PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (!ShakeClass || Scale <= 0.0f)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->ClientStartCameraShake(ShakeClass, Scale);
	}
}

void AGanapatiPlayerCharacter::TriggerHitStop(float Duration)
{
	if (Duration <= 0.0f)
	{
		return;
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->GlobalAnimRateScale = 0.0f;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(HitStopTimerHandle);
			World->GetTimerManager().SetTimer(HitStopTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (USkeletalMeshComponent* Mesh = GetMesh())
				{
					Mesh->GlobalAnimRateScale = 1.0f;
				}
			}), Duration, false);
		}
	}
}

void AGanapatiPlayerCharacter::HandleDamageDealt(float Damage, const FVector& ImpactPoint)
{
	PlayCameraShake(MeleeHitCameraShakeClass, MeleeHitShakeScale);
	TriggerHitStop(HitStopDuration);
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

	if (PeakFallVelocityZ <= HardLandingVelocityThreshold)
	{
		HandleHardLanding(Hit, PeakFallVelocityZ);
	}
	PeakFallVelocityZ = 0.0f;

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

void AGanapatiPlayerCharacter::HandleHardLanding(const FHitResult& Hit, float ImpactVelocityZ)
{
	PlayCameraShake(HardLandingCameraShakeClass, HardLandingShakeScale);
	BP_OnHardLanding(Hit, ImpactVelocityZ);
}

void AGanapatiPlayerCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (PrevMovementMode == MOVE_Falling && GetCharacterMovement() && !GetCharacterMovement()->IsFalling())
	{
		PeakFallVelocityZ = 0.0f;
	}

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
	PlayCameraShake(DashCameraShakeClass, DashShakeScale);
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
	const bool bWasCharging = CombatComponent && CombatComponent->IsChargingAttack();

	if (CombatComponent)
	{
		CombatComponent->StopChargedAttack();
	}

	if (bWasCharging)
	{
		PlayCameraShake(HeavyAttackCameraShakeClass, HeavyAttackShakeScale);
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

