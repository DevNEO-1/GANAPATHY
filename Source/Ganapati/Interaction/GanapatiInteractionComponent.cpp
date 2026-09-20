// Copyright Ganapati Project. All Rights Reserved.

#include "Interaction/GanapatiInteractionComponent.h"
#include "Interaction/GanapatiInteractable.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "Kismet/GameplayStatics.h"

UGanapatiInteractionComponent::UGanapatiInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // Poll 10 times a second to be lightweight
}

void UGanapatiInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// Ensure the Ganesh Pandal shrine interactable exists in the world
	if (UWorld* World = GetWorld())
	{
		TArray<AActor*> ExistingInteractables;
		UGameplayStatics::GetAllActorsOfClass(World, AGanapatiInteractable::StaticClass(), ExistingInteractables);

		if (ExistingInteractables.Num() == 0)
		{
			// The Ganesh Pandal altar is located at (2300, 0, 90) at the end of the festival street
			const FVector PandalShrineLocation(2300.0f, 0.0f, 90.0f);
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			World->SpawnActor<AGanapatiInteractable>(
				AGanapatiInteractable::StaticClass(),
				PandalShrineLocation,
				FRotator::ZeroRotator,
				SpawnParams
			);
		}
	}
}

void UGanapatiInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFocusedInteractable();

	if (MessageTimer > 0.0f)
	{
		MessageTimer -= DeltaTime;
		if (MessageTimer <= 0.0f)
		{
			LastInteractionMessage = FText::GetEmpty();
		}
	}
}

void UGanapatiInteractionComponent::UpdateFocusedInteractable()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector OwnerLocation = Owner->GetActorLocation();
	const float RadiusSq = FMath::Square(InteractionRadius);

	AGanapatiInteractable* BestInteractable = nullptr;
	float ClosestDistSq = RadiusSq;

	// Query all interactable actors in the world directly to guarantee 100% reliable detection
	TArray<AActor*> Candidates;
	UGameplayStatics::GetAllActorsOfClass(World, AGanapatiInteractable::StaticClass(), Candidates);

	for (AActor* CandidateActor : Candidates)
	{
		if (AGanapatiInteractable* Candidate = Cast<AGanapatiInteractable>(CandidateActor))
		{
			float DistSq = FVector::DistSquared(OwnerLocation, Candidate->GetActorLocation());
			if (DistSq < ClosestDistSq)
			{
				ClosestDistSq = DistSq;
				BestInteractable = Candidate;
			}
		}
	}

	if (BestInteractable != FocusedInteractable)
	{
		FocusedInteractable = BestInteractable;
		OnFocusedInteractableChanged.Broadcast(FocusedInteractable);
	}
}

bool UGanapatiInteractionComponent::TryInteract()
{
	if (!FocusedInteractable)
	{
		UpdateFocusedInteractable();
	}

	if (!FocusedInteractable)
	{
		return false;
	}

	AActor* Owner = GetOwner();
	FocusedInteractable->TriggerInteraction(Owner);

	LastInteractionMessage = FocusedInteractable->GetInteractionMessage();
	MessageTimer = MessageDisplayDuration;

	OnInteractionCompleted.Broadcast(LastInteractionMessage);
	return true;
}

FText UGanapatiInteractionComponent::GetCurrentPrompt() const
{
	if (FocusedInteractable)
	{
		return FocusedInteractable->GetPromptText();
	}
	return FText::GetEmpty();
}
