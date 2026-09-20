// Copyright Ganapati Project. All Rights Reserved.

#include "Interaction/GanapatiInteractable.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "World/GanapatiWorldSubsystem.h"

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

	if (UWorld* World = GetWorld())
	{
		if (UGanapatiWorldSubsystem* Subsystem = World->GetSubsystem<UGanapatiWorldSubsystem>())
		{
			Subsystem->RegisterInteractable(this);
		}
	}
}

void AGanapatiInteractable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UGanapatiWorldSubsystem* Subsystem = World->GetSubsystem<UGanapatiWorldSubsystem>())
		{
			Subsystem->UnregisterInteractable(this);
		}
	}

	CleanupPrayerCamera();
	bIsPrayerActive = false;
	Super::EndPlay(EndPlayReason);
}

void AGanapatiInteractable::TriggerInteraction(AActor* Interactor)
{
	if (bSingleUse && bHasBeenTriggered)
	{
		return;
	}

	if (bIsPrayerActive)
	{
		return;
	}

	AGanapatiPlayerCharacter* PlayerChar = Cast<AGanapatiPlayerCharacter>(Interactor);

	// If this interactable triggers a cinematic prayer sequence and interactor is player
	if (bTriggersPrayerSequence && PlayerChar)
	{
		bHasBeenTriggered = true;
		StartPrayerSequence(PlayerChar);
		return;
	}

	bHasBeenTriggered = true;

	// Instant interaction path (Modak Stall, standard interactables)
	if (PlayerChar)
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

void AGanapatiInteractable::StartPrayerSequence(AGanapatiPlayerCharacter* PlayerChar)
{
	if (!PlayerChar || bIsPrayerActive)
	{
		return;
	}

	bIsPrayerActive = true;

	// 1. Temporarily lock player movement and combat input
	PlayerChar->SetPraying(true);

	// 2. Grant shrine blessings (health + Divine Energy)
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
	OnInteracted.Broadcast(PlayerChar, InteractionMessage);
	BP_OnInteracted(PlayerChar);

	// 3. Notify prayer started
	OnPrayerStarted.Broadcast(PlayerChar, PrayerHoldDuration);
	BP_OnPrayerStarted(PlayerChar, PrayerHoldDuration);

	// 4. Cinematic camera blend
	UWorld* World = GetWorld();
	APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController());

	if (World && PC)
	{
		CleanupPrayerCamera();

		const FVector ShrineLoc = GetActorLocation();
		const FRotator ShrineRot = GetActorRotation();
		const FVector CamWorldLoc = ShrineLoc + ShrineRot.RotateVector(PrayerCameraRelativeOffset);
		const FVector LookAtWorldLoc = ShrineLoc + ShrineRot.RotateVector(PrayerCameraLookAtOffset);
		const FRotator CamRot = (LookAtWorldLoc - CamWorldLoc).Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		PrayerCamera = World->SpawnActor<ACameraActor>(CamWorldLoc, CamRot, SpawnParams);
		if (PrayerCamera)
		{
			if (UCameraComponent* CamComp = PrayerCamera->GetCameraComponent())
			{
				CamComp->SetFieldOfView(65.0f);
			}

			PC->SetViewTargetWithBlend(PrayerCamera, PrayerBlendInDuration, VTBlend_EaseInOut, 2.0f);
		}

		TWeakObjectPtr<AGanapatiInteractable> WeakThis(this);
		TWeakObjectPtr<AGanapatiPlayerCharacter> WeakPlayer(PlayerChar);
		TWeakObjectPtr<APlayerController> WeakPC(PC);

		World->GetTimerManager().SetTimer(
			PrayerTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [WeakThis, WeakPlayer, WeakPC]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->FinishPrayerSequence(WeakPlayer, WeakPC);
				}
			}),
			PrayerHoldDuration,
			false
		);
	}
	else
	{
		FinishPrayerSequence(PlayerChar, PC);
	}
}

void AGanapatiInteractable::FinishPrayerSequence(TWeakObjectPtr<AGanapatiPlayerCharacter> WeakPlayerChar, TWeakObjectPtr<APlayerController> WeakPC)
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(PrayerTimerHandle);
	}

	AGanapatiPlayerCharacter* PlayerChar = WeakPlayerChar.Get();
	APlayerController* PC = WeakPC.Get();

	if (PlayerChar)
	{
		OnPrayerCompleted.Broadcast(PlayerChar);
		BP_OnPrayerCompleted(PlayerChar);
	}

	if (PC && PlayerChar)
	{
		// Smoothly blend camera back to the player character
		PC->SetViewTargetWithBlend(PlayerChar, PrayerBlendOutDuration, VTBlend_EaseInOut, 2.0f);

		TWeakObjectPtr<AGanapatiInteractable> WeakThis(this);
		if (World)
		{
			World->GetTimerManager().SetTimer(
				PrayerBlendTimerHandle,
				FTimerDelegate::CreateWeakLambda(this, [WeakThis, WeakPlayerChar]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->CleanupPrayerCamera();
						WeakThis->bIsPrayerActive = false;
					}
					if (WeakPlayerChar.IsValid())
					{
						WeakPlayerChar->SetPraying(false);
					}
				}),
				PrayerBlendOutDuration,
				false
			);
		}
		else
		{
			CleanupPrayerCamera();
			bIsPrayerActive = false;
			PlayerChar->SetPraying(false);
		}
	}
	else
	{
		CleanupPrayerCamera();
		bIsPrayerActive = false;
		if (PlayerChar)
		{
			PlayerChar->SetPraying(false);
		}
	}
}

void AGanapatiInteractable::CleanupPrayerCamera()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PrayerBlendTimerHandle);
		World->GetTimerManager().ClearTimer(PrayerTimerHandle);
	}

	if (PrayerCamera)
	{
		PrayerCamera->Destroy();
		PrayerCamera = nullptr;
	}
}
