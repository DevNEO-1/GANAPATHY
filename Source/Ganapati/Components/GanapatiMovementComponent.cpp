// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/GanapatiMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"

UGanapatiMovementComponent::UGanapatiMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> DashMontageFinder(
		TEXT("/Game/Variant_Platforming/Anims/AM_Dash.AM_Dash"));
	if (DashMontageFinder.Succeeded())
	{
		DashMontage = DashMontageFinder.Object;
	}
}

void UGanapatiMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	DashMontageEndedDelegate.BindUObject(this, &UGanapatiMovementComponent::HandleDashMontageEnded);

	if (UCharacterMovementComponent* CMC = GetMovementComponent())
	{
		DefaultGravityScale = CMC->GravityScale;
		CMC->MaxWalkSpeed = NormalWalkSpeed;
		CMC->bOrientRotationToMovement = true;
	}
}

ACharacter* UGanapatiMovementComponent::GetCharacterOwner() const
{
	return Cast<ACharacter>(GetOwner());
}

UCharacterMovementComponent* UGanapatiMovementComponent::GetMovementComponent() const
{
	if (ACharacter* Character = GetCharacterOwner())
	{
		return Character->GetCharacterMovement();
	}
	return nullptr;
}

void UGanapatiMovementComponent::StartSprint()
{
	if (UCharacterMovementComponent* CMC = GetMovementComponent())
	{
		bIsSprinting = true;
		CMC->MaxWalkSpeed = SprintWalkSpeed;
		OnSprintStateChanged.Broadcast(true);
	}
}

void UGanapatiMovementComponent::StopSprint()
{
	if (UCharacterMovementComponent* CMC = GetMovementComponent())
	{
		bIsSprinting = false;
		CMC->MaxWalkSpeed = NormalWalkSpeed;
		OnSprintStateChanged.Broadcast(false);
	}
}

void UGanapatiMovementComponent::StartDash()
{
	if (bIsDashing || bHasDashed)
	{
		return;
	}

	ACharacter* Character = GetCharacterOwner();
	UCharacterMovementComponent* CMC = GetMovementComponent();
	if (!Character || !CMC || !GetWorld())
	{
		return;
	}

	bIsDashing = true;
	bHasDashed = true;
	OnDashStateChanged.Broadcast(true);

	// Zero gravity during dash burst
	CMC->GravityScale = 0.0f;

	// Determine dash direction from movement input or forward vector
	FVector DashDirection = CMC->Velocity.GetSafeNormal2D();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = Character->GetActorForwardVector();
	}

	// Apply immediate burst velocity
	CMC->Velocity = DashDirection * DashImpulse;

	// Play dash montage if assigned
	bool bPlayedMontage = false;
	if (DashMontage)
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				const float MontageLength = AnimInstance->Montage_Play(DashMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
				if (MontageLength > 0.0f)
				{
					AnimInstance->Montage_SetEndDelegate(DashMontageEndedDelegate, DashMontage);
					bPlayedMontage = true;
				}
			}
		}
	}

	// Use fallback timer if no montage or montage failed to play
	if (!bPlayedMontage)
	{
		GetWorld()->GetTimerManager().SetTimer(DashFallbackTimerHandle, this, &UGanapatiMovementComponent::EndDash, DashDurationFallback, false);
	}
}

void UGanapatiMovementComponent::HandleDashMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndDash();
}

void UGanapatiMovementComponent::EndDash()
{
	if (!bIsDashing)
	{
		return;
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DashFallbackTimerHandle);
	}

	bIsDashing = false;

	if (UCharacterMovementComponent* CMC = GetMovementComponent())
	{
		// Restore appropriate gravity depending on anti-gravity mode
		CMC->GravityScale = bIsAntiGravityActive ? AntiGravityScale : DefaultGravityScale;

		// Dampen extreme leftover dash momentum
		CMC->Velocity = CMC->Velocity.GetClampedToMaxSize(bIsSprinting ? SprintWalkSpeed : NormalWalkSpeed);

		if (CMC->IsMovingOnGround())
		{
			bHasDashed = false;
		}
	}

	OnDashStateChanged.Broadcast(false);
}

void UGanapatiMovementComponent::HandleJumpStart()
{
	if (bIsDashing)
	{
		return;
	}

	ACharacter* Character = GetCharacterOwner();
	UCharacterMovementComponent* CMC = GetMovementComponent();
	if (!Character || !CMC || !GetWorld())
	{
		return;
	}

	// Case 1: Grounded jump
	if (CMC->IsMovingOnGround())
	{
		Character->Jump();
		return;
	}

	// Case 2: In-air execution checks
	if (CMC->IsFalling())
	{
		// Check Wall Jump first
		if (!bHasWallJumped && TryWallJump())
		{
			return;
		}

		// Check Coyote Time jump second
		const float TimeSinceFalling = GetWorld()->GetTimeSeconds() - LastFallTime;
		if (TimeSinceFalling < MaxCoyoteTime)
		{
			Character->Jump();
			return;
		}

		// Check Air Double Jump third
		if (!bHasDoubleJumped)
		{
			bHasDoubleJumped = true;
			Character->LaunchCharacter(FVector(0.0f, 0.0f, DoubleJumpZVelocity), false, true);
		}
	}
}

void UGanapatiMovementComponent::HandleJumpEnd()
{
	if (ACharacter* Character = GetCharacterOwner())
	{
		Character->StopJumping();
	}
}

bool UGanapatiMovementComponent::TryWallJump()
{
	ACharacter* Character = GetCharacterOwner();
	if (!Character || !GetWorld())
	{
		return false;
	}

	const FVector TraceStart = Character->GetActorLocation();
	const FVector TraceEnd = TraceStart + (Character->GetActorForwardVector() * WallJumpTraceDistance);
	const FCollisionShape TraceShape = FCollisionShape::MakeSphere(WallJumpTraceRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	FHitResult HitResult;
	if (GetWorld()->SweepSingleByChannel(HitResult, TraceStart, TraceEnd, FQuat::Identity, ECC_Visibility, TraceShape, QueryParams))
	{
		// Orient character to face outward from wall
		FRotator WallOrientation = HitResult.ImpactNormal.ToOrientationRotator();
		WallOrientation.Pitch = 0.0f;
		WallOrientation.Roll = 0.0f;
		Character->SetActorRotation(WallOrientation);

		// Apply wall jump impulse: bounce away from normal + upward thrust
		const FVector WallJumpImpulse = (HitResult.ImpactNormal * WallJumpBounceImpulse) + (FVector::UpVector * WallJumpVerticalImpulse);
		Character->LaunchCharacter(WallJumpImpulse, true, true);

		bHasWallJumped = true;

		// Timer to reset wall jump lockout
		GetWorld()->GetTimerManager().SetTimer(WallJumpTimerHandle, this, &UGanapatiMovementComponent::ResetWallJump, DelayBetweenWallJumps, false);
		return true;
	}

	return false;
}

void UGanapatiMovementComponent::ResetWallJump()
{
	bHasWallJumped = false;
}

void UGanapatiMovementComponent::ToggleAntiGravityMode()
{
	SetAntiGravityMode(!bIsAntiGravityActive);
}

void UGanapatiMovementComponent::SetAntiGravityMode(bool bEnable)
{
	bIsAntiGravityActive = bEnable;

	if (UCharacterMovementComponent* CMC = GetMovementComponent())
	{
		if (bIsAntiGravityActive)
		{
			CMC->GravityScale = AntiGravityScale;
			CMC->AirControl = AntiGravityAirControl;
			CMC->BrakingDecelerationFalling = AntiGravityBrakingDeceleration;
		}
		else
		{
			CMC->GravityScale = DefaultGravityScale;
			CMC->AirControl = 0.35f;
			CMC->BrakingDecelerationFalling = 1500.0f;
		}
	}

	OnAntiGravityToggled.Broadcast(bIsAntiGravityActive);
}

void UGanapatiMovementComponent::NotifyLanded(const FHitResult& Hit)
{
	bHasDoubleJumped = false;
	bHasDashed = false;
	bHasWallJumped = false;
}

void UGanapatiMovementComponent::NotifyMovementModeChanged(EMovementMode PrevMovementMode, EMovementMode NewMovementMode)
{
	if (NewMovementMode == MOVE_Falling && GetWorld())
	{
		LastFallTime = GetWorld()->GetTimeSeconds();
	}
}
