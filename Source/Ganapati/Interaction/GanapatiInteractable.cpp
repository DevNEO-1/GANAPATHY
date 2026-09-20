// Copyright Ganapati Project. All Rights Reserved.

#include "Interaction/GanapatiInteractable.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AGanapatiInteractable::AGanapatiInteractable()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->SetupAttachment(RootScene);
	TriggerSphere->InitSphereRadius(200.0f);
	TriggerSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerSphere->SetGenerateOverlapEvents(true);

	InteractableMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InteractableMesh"));
	InteractableMesh->SetupAttachment(RootScene);
	InteractableMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// Try loading default cylinder mesh for shrine/pedestal base
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(
		TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		InteractableMesh->SetStaticMesh(CylinderMeshFinder.Object);
		InteractableMesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.4f));
	}
}

void AGanapatiInteractable::BeginPlay()
{
	Super::BeginPlay();
}

void AGanapatiInteractable::TriggerInteraction(AActor* Interactor)
{
	if (bSingleUse && bHasBeenTriggered)
	{
		return;
	}

	bHasBeenTriggered = true;

	// ── Phase 4B Subsystem 2B: Shrine Blessing & Divine Energy Recharge ──
	if (AGanapatiPlayerCharacter* PlayerChar = Cast<AGanapatiPlayerCharacter>(Interactor))
	{
		if (bRestoresHealth)
		{
			PlayerChar->ResetHealth();
		}

		if (DivineEnergyGranted > 0.0f)
		{
			PlayerChar->AddDivineEnergy(DivineEnergyGranted);
		}

		OnShrineBlessed.Broadcast(PlayerChar, DivineEnergyGranted);
		BP_OnShrineBlessingGranted(PlayerChar, DivineEnergyGranted);
		BP_OnModakPrasadamReceived(PlayerChar, DivineEnergyGranted);
	}

	OnInteracted.Broadcast(Interactor, InteractionMessage);
	BP_OnInteracted(Interactor);
}
