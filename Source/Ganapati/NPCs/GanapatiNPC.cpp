// Copyright Ganapati Project. All Rights Reserved.

#include "NPCs/GanapatiNPC.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/GanapatiInteractable.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
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

	// Populate default dialogue lines if none were set
	if (DialogueLines.Num() == 0)
	{
		DialogueLines.Add(FText::FromString(TEXT("Ganpati Bappa Morya! Welcome to our festival street!")));
		DialogueLines.Add(FText::FromString(TEXT("Have you offered prayers at the Grand Pandal? Lord Ganesha's idol looks magnificent!")));
		DialogueLines.Add(FText::FromString(TEXT("The sweetmaker's modaks are warm and fresh. Go taste the sacred prasadam!")));
		DialogueLines.Add(FText::FromString(TEXT("May Lord Vighnaharta remove all obstacles from your journey.")));
	}

	// Spawn and attach the interaction trigger
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		InteractionTrigger = World->SpawnActor<AGanapatiInteractable>(
			AGanapatiInteractable::StaticClass(),
			GetActorLocation(),
			GetActorRotation(),
			SpawnParams
		);

		if (InteractionTrigger)
		{
			InteractionTrigger->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			InteractionTrigger->Tags.Add(FName(TEXT("NPC")));
			InteractionTrigger->Tags.Add(FName(TEXT("Devotee")));
			InteractionTrigger->SetPromptText(FText::FromString(FString::Printf(TEXT("Press [E] to Speak with %s"), *NPCName)));
			InteractionTrigger->SetInteractionMessage(DialogueLines[0]);
			InteractionTrigger->SetDivineEnergyGranted(10.0f);
			InteractionTrigger->SetRestoresHealth(false);
			InteractionTrigger->SetSingleUse(false);
			InteractionTrigger->SetTriggersPrayerSequence(false);

			if (InteractionTrigger->GetTriggerSphere())
			{
				InteractionTrigger->GetTriggerSphere()->SetSphereRadius(180.0f);
			}

			if (InteractionTrigger->GetInteractableMesh())
			{
				InteractionTrigger->GetInteractableMesh()->SetVisibility(false);
				InteractionTrigger->GetInteractableMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}

			InteractionTrigger->OnInteracted.AddDynamic(this, &AGanapatiNPC::HandleInteraction);
		}
	}

	DecideNextAction();
}

void AGanapatiNPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ConversationTimerHandle);
		World->GetTimerManager().ClearTimer(StateTimerHandle);
	}

	if (InteractionTrigger)
	{
		InteractionTrigger->Destroy();
		InteractionTrigger = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AGanapatiNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EGanapatiNPCState::Talking)
	{
		if (InteractingPlayer.IsValid())
		{
			const FVector PlayerLoc = InteractingPlayer->GetActorLocation();
			const FVector NPCLoc = GetActorLocation();

			// If player walked away (> 350 cm), end conversation early
			if (FVector::DistSquared2D(PlayerLoc, NPCLoc) > FMath::Square(350.0f))
			{
				EndConversation();
				return;
			}

			// Smoothly rotate to face player
			FVector Direction = (PlayerLoc - NPCLoc);
			Direction.Z = 0.0f;
			if (!Direction.IsNearlyZero())
			{
				const FRotator TargetRot = Direction.Rotation();
				const FRotator CurrentRot = GetActorRotation();
				const FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, FacePlayerTurnSpeed);
				SetActorRotation(NewRot);
			}
		}
		else
		{
			EndConversation();
		}
	}
	else if (CurrentState == EGanapatiNPCState::Wandering)
	{
		MoveTowardsTarget(DeltaTime);
	}
}

void AGanapatiNPC::SetNPCProfile(const FString& InName, const TArray<FText>& InDialogueLines)
{
	NPCName = InName;
	DialogueLines = InDialogueLines;
	CurrentDialogueIndex = -1;

	if (InteractionTrigger)
	{
		InteractionTrigger->SetPromptText(FText::FromString(FString::Printf(TEXT("Press [E] to Speak with %s"), *NPCName)));
		if (DialogueLines.Num() > 0)
		{
			InteractionTrigger->SetInteractionMessage(DialogueLines[0]);
		}
	}
}

void AGanapatiNPC::HandleInteraction(AActor* Interactor, const FText& Message)
{
	if (!Interactor)
	{
		return;
	}

	InteractingPlayer = Interactor;
	CurrentState = EGanapatiNPCState::Talking;

	// Pause wandering and halt movement immediately
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StateTimerHandle);
	}
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
	}

	// Cycle dialogue line
	if (DialogueLines.Num() > 0)
	{
		CurrentDialogueIndex = (CurrentDialogueIndex + 1) % DialogueLines.Num();
		const FText& CurrentLine = DialogueLines[CurrentDialogueIndex];

		if (InteractionTrigger)
		{
			InteractionTrigger->SetInteractionMessage(CurrentLine);
		}

		BP_OnNPCDialogueSpoken(Interactor, CurrentLine);
		OnNPCDialogueSpoken.Broadcast(this, Interactor, CurrentLine);

		UE_LOG(LogTemp, Log, TEXT("AGanapatiNPC [%s]: Spoke line %d/%d: %s"),
			*NPCName, CurrentDialogueIndex + 1, DialogueLines.Num(), *CurrentLine.ToString());
	}

	// Reset conversation timeout
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ConversationTimerHandle);
		World->GetTimerManager().SetTimer(ConversationTimerHandle, this, &AGanapatiNPC::EndConversation, ConversationTimeout, false);
	}
}

void AGanapatiNPC::EndConversation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ConversationTimerHandle);
	}

	InteractingPlayer = nullptr;

	if (CurrentState == EGanapatiNPCState::Talking)
	{
		CurrentState = EGanapatiNPCState::Idle;
		DecideNextAction();
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
