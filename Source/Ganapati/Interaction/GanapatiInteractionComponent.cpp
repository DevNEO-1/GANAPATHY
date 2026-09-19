// Copyright Ganapati Project. All Rights Reserved.

#include "Interaction/GanapatiInteractionComponent.h"
#include "Interaction/GanapatiInteractable.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"

UGanapatiInteractionComponent::UGanapatiInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // Poll 10 times a second to be lightweight
}

void UGanapatiInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
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

#include "Kismet/GameplayStatics.h"

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
