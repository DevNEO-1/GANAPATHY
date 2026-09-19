// Copyright Ganapati Project. All Rights Reserved.

#include "NPCs/GanapatiNPC.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AGanapatiNPC::AGanapatiNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 300.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 180.0f; // Gentle festive strolling pace
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.0f;

	// Auto-load Manny mesh for festival devotees
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMeshFinder(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MannyMeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannyMeshFinder.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	// Auto-load Unarmed animation blueprint
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBPFinder(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (AnimBPFinder.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimBPFinder.Class);
	}

	Tags.Add(FName(TEXT("NPC")));
	Tags.Add(FName(TEXT("Devotee")));
}

void AGanapatiNPC::BeginPlay()
{
	Super::BeginPlay();

	SpawnOrigin = GetActorLocation();
	TargetLocation = SpawnOrigin;

	DecideNextAction();
}

void AGanapatiNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EGanapatiNPCState::Wandering)
	{
		MoveTowardsTarget(DeltaTime);
	}
}

void AGanapatiNPC::DecideNextAction()
{
	const float RandomVal = FMath::FRand();

	if (RandomVal < 0.45f)
	{
		// Stay idle
		CurrentState = EGanapatiNPCState::Idle;
		const float Duration = FMath::FRandRange(MinIdleDuration, MaxIdleDuration);
		GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AGanapatiNPC::DecideNextAction, Duration, false);
	}
	else if (RandomVal < 0.85f)
	{
		// Pick random wander target around spawn origin
		CurrentState = EGanapatiNPCState::Wandering;
		const FVector2D RandomCircle = FMath::RandPointInCircle(WanderRadius);
		TargetLocation = SpawnOrigin + FVector(RandomCircle.X, RandomCircle.Y, 0.0f);

		// Timeout in case NPC gets stuck
		const float Timeout = FMath::FRandRange(6.0f, 10.0f);
		GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AGanapatiNPC::DecideNextAction, Timeout, false);
	}
	else
	{
		// Stand and pray/meditate towards nearest shrine direction
		CurrentState = EGanapatiNPCState::Praying;
		const float Duration = FMath::FRandRange(3.0f, 6.0f);
		GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AGanapatiNPC::DecideNextAction, Duration, false);
	}
}

void AGanapatiNPC::MoveTowardsTarget(float DeltaTime)
{
	const FVector CurrentLoc = GetActorLocation();
	FVector Direction = (TargetLocation - CurrentLoc);
	Direction.Z = 0.0f;

	const float Distance = Direction.Size();

	if (Distance < 100.0f)
	{
		// Reached target
		CurrentState = EGanapatiNPCState::Idle;
		DecideNextAction();
		return;
	}

	Direction.Normalize();
	AddMovementInput(Direction, 1.0f);
}
