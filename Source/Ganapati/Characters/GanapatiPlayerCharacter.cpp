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
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "World/GanapatiWorldSubsystem.h"

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
	ResetDivineEnergy();

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
	AddDivineEnergy(DivineEnergyPerHit);
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

		PlayCameraShake(HeavyAttackCameraShakeClass, 0.4f);
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

void AGanapatiPlayerCharacter::AddDivineEnergy(float Amount)
{
	if (Amount <= 0.0f || bIsDead)
	{
		return;
	}

	const float OldEnergy = CurrentDivineEnergy;
	CurrentDivineEnergy = FMath::Clamp(CurrentDivineEnergy + Amount, 0.0f, MaxDivineEnergy);

	if (!FMath::IsNearlyEqual(OldEnergy, CurrentDivineEnergy))
	{
		OnDivineEnergyChanged.Broadcast(CurrentDivineEnergy, MaxDivineEnergy);
		BP_OnDivineEnergyChanged(CurrentDivineEnergy, MaxDivineEnergy);
	}
}

bool AGanapatiPlayerCharacter::ConsumeDivineEnergy(float Amount)
{
	if (Amount <= 0.0f || CurrentDivineEnergy < Amount || bIsDead)
	{
		return false;
	}

	CurrentDivineEnergy = FMath::Clamp(CurrentDivineEnergy - Amount, 0.0f, MaxDivineEnergy);
	OnDivineEnergyChanged.Broadcast(CurrentDivineEnergy, MaxDivineEnergy);
	BP_OnDivineEnergyChanged(CurrentDivineEnergy, MaxDivineEnergy);
	return true;
}

float AGanapatiPlayerCharacter::GetDivineEnergyPercent() const
{
	if (MaxDivineEnergy <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(CurrentDivineEnergy / MaxDivineEnergy, 0.0f, 1.0f);
}

void AGanapatiPlayerCharacter::ResetDivineEnergy()
{
	CurrentDivineEnergy = 0.0f;
	OnDivineEnergyChanged.Broadcast(CurrentDivineEnergy, MaxDivineEnergy);
	BP_OnDivineEnergyChanged(CurrentDivineEnergy, MaxDivineEnergy);
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
	if (bIsDead || bIsPraying || !Controller)
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
	if (!Controller || bIsPraying)
	{
		return;
	}

	AddControllerYawInput(Yaw);
	AddControllerPitchInput(Pitch);
}

void AGanapatiPlayerCharacter::DoJumpStart()
{
	if (bIsDead || bIsPraying || !MovementComponent)
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
	if (bIsDead || bIsPraying || !MovementComponent)
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
	if (bIsDead || bIsPraying || !MovementComponent)
	{
		return;
	}

	MovementComponent->StartDash();
	PlayCameraShake(DashCameraShakeClass, DashShakeScale);
}

void AGanapatiPlayerCharacter::DoLightAttackStart()
{
	if (bIsDead || bIsPraying || !CombatComponent)
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
	if (bIsDead || bIsPraying || !CombatComponent)
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
	if (bIsDead || bIsPraying || !MovementComponent)
	{
		return;
	}

	MovementComponent->ToggleAntiGravityMode();

	// Phase 6C: Divine Anti-Gravity Traversal discovery during Sacred Journey
	if (MovementComponent->IsAntiGravityActive())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGanapatiWorldSubsystem* Subsystem = World->GetSubsystem<UGanapatiWorldSubsystem>())
			{
				if ((Subsystem->GetActiveRegion() == EWorldRegion::SacredPathAscent || Subsystem->GetActiveRegion() == EWorldRegion::MountainThreshold)
					&& !Subsystem->IsDivineAscensionDiscovered())
				{
					Subsystem->SetDivineAscensionDiscovered(true);
				}
			}
		}
	}
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
	if (bIsDead || bIsPraying || !InteractionComponent)
	{
		return;
	}

	InteractionComponent->TryInteract();
}

void AGanapatiPlayerCharacter::DoDivineShockwave()
{
	if (bIsDead || bIsPraying)
	{
		return;
	}

	// Activation requires exactly DivineShockwaveCost (100.0f).
	// If insufficient, ConsumeDivineEnergy returns false, does nothing, and consumes nothing.
	if (!ConsumeDivineEnergy(DivineShockwaveCost))
	{
		UE_LOG(LogTemp, Verbose, TEXT("AGanapatiPlayerCharacter::DoDivineShockwave: Insufficient Divine Energy (requires %f, have %f)."),
			DivineShockwaveCost, CurrentDivineEnergy);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Origin = GetActorLocation();
	const float Radius = DivineShockwaveRadius;

	UE_LOG(LogTemp, Warning, TEXT("AGanapatiPlayerCharacter::DoDivineShockwave: ACTIVATED at %s (Radius: %f cm)"),
		*Origin.ToString(), Radius);

	// Re-use Phase 4A camera feedback for impactful feel
	PlayCameraShake(HeavyAttackCameraShakeClass, HeavyAttackShakeScale * 1.5f);

	// Broadcast delegate and invoke Blueprint hook for VFX/SFX
	OnDivineShockwaveTriggered.Broadcast(Origin, Radius);
	BP_OnDivineShockwaveTriggered(Origin, Radius);

	// Radial overlap query for valid combatants
	TArray<FOverlapResult> Overlaps;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(Radius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DivineShockwave), false, this);
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		ObjectQueryParams,
		SphereShape,
		QueryParams
	);

	TSet<AActor*> ProcessedActors;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor || HitActor == this || ProcessedActors.Contains(HitActor))
		{
			continue;
		}

		if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor))
		{
			ProcessedActors.Add(HitActor);

			const FVector TargetLocation = HitActor->GetActorLocation();
			FVector Direction2D = TargetLocation - Origin;
			Direction2D.Z = 0.0f;

			if (Direction2D.IsNearlyZero())
			{
				Direction2D = GetActorForwardVector();
				Direction2D.Z = 0.0f;
			}
			Direction2D.Normalize();

			// Radial knockback (1200 cm/s) with upward launch component (600 cm/s)
			const FVector LaunchImpulse = (Direction2D * DivineShockwaveKnockbackImpulse) + (FVector::UpVector * DivineShockwaveLaunchImpulse);
			const FVector HitPoint = Overlap.Component.IsValid() ? Overlap.Component->GetComponentLocation() : TargetLocation;

			// Apply combat damage and impulse through ICombatDamageable interface
			Damageable->ApplyDamage(DivineShockwaveDamage, this, HitPoint, LaunchImpulse);

			// If target is a Character, ensure clean physics launch transition into Falling mode
			if (ACharacter* TargetCharacter = Cast<ACharacter>(HitActor))
			{
				TargetCharacter->LaunchCharacter(LaunchImpulse, true, true);
			}

			// Broadcast hit enemy Blueprint hook
			BP_OnDivineShockwaveHitEnemy(HitActor, HitPoint, LaunchImpulse);

			UE_LOG(LogTemp, Log, TEXT("AGanapatiPlayerCharacter::DoDivineShockwave: Hit %s (Damage: %f, Impulse: %s)"),
				*HitActor->GetName(), DivineShockwaveDamage, *LaunchImpulse.ToString());
		}
	}
}

void AGanapatiPlayerCharacter::SetPraying(bool bNewPraying)
{
	if (bIsPraying == bNewPraying)
	{
		return;
	}

	bIsPraying = bNewPraying;

	if (bIsPraying)
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(bIsPraying);
		PC->SetIgnoreLookInput(bIsPraying);
	}

	BP_OnPrayerStateChanged(bIsPraying);

	UE_LOG(LogTemp, Log, TEXT("AGanapatiPlayerCharacter::SetPraying: %s"),
		bIsPraying ? TEXT("PRAYING (Input Locked)") : TEXT("PRAYER ENDED (Input Restored)"));
}
