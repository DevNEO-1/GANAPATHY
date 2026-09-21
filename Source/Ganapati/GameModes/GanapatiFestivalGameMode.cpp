#include "GameModes/GanapatiFestivalGameMode.h"
#include "Environment/FestivalStreetBuilder.h"
#include "UI/GanapatiGameHUD.h"
#include "Camera/CameraActor.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "NPCs/GanapatiNPC.h"
#include "Interaction/GanapatiInteractable.h"
#include "Enemies/GanapatiTrainingDummy.h"
#include "Enemies/GanapatiAsuraMinion.h"
#include "Enemies/GanapatiAsuraCaptain.h"
#include "World/GanapatiWorldSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "Camera/CameraShakeBase.h"
#include "UObject/ConstructorHelpers.h"

AGanapatiFestivalGameMode::AGanapatiFestivalGameMode()
{
	// Set custom festival HUD
	HUDClass = AGanapatiGameHUD::StaticClass();

	static ConstructorHelpers::FClassFinder<UCameraShakeBase> ShakeFinder(
		TEXT("/Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy"));
	if (ShakeFinder.Succeeded())
	{
		EncounterStartCameraShakeClass = ShakeFinder.Class;
	}
}

AActor* AGanapatiFestivalGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), PlayerStarts);

	for (AActor* StartActor : PlayerStarts)
	{
		if (APlayerStart* Start = Cast<APlayerStart>(StartActor))
		{
			if (Start->PlayerStartTag == FName(TEXT("FestivalStart")))
			{
				return Start;
			}
		}
	}

	if (PlayerStarts.Num() > 0)
	{
		AActor* BestStart = PlayerStarts[0];
		float BestDistSq = FVector::DistSquared(BestStart->GetActorLocation(), FVector(-1300.0f, 0.0f, 50.0f));
		for (AActor* StartActor : PlayerStarts)
		{
			float DistSq = FVector::DistSquared(StartActor->GetActorLocation(), FVector(-1300.0f, 0.0f, 50.0f));
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestStart = StartActor;
			}
		}
		return BestStart;
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void AGanapatiFestivalGameMode::BeginPlay()
{
	Super::BeginPlay();

	EnsureFestivalEnvironment();
	PlayOpeningCinematic();

	// Bind quest and encounter listeners via streaming-safe subsystem
	BindQuestListeners();

	if (UWorld* World = GetWorld())
	{
		// Synchronize session world state with UGanapatiWorldSubsystem
		if (UGanapatiWorldSubsystem* Subsystem = World->GetSubsystem<UGanapatiWorldSubsystem>())
		{
			if (Subsystem->GetStoryProgressionState() != EStoryProgressionState::FestivalBeginning)
			{
				CurrentStoryState = Subsystem->GetStoryProgressionState();
			}
			else
			{
				Subsystem->SetStoryProgressionState(CurrentStoryState);
			}

			if (Subsystem->IsSacredPathUnlocked())
			{
				bSacredJourneyUnlocked = true;
			}
			if (Subsystem->IsCourtyardPurified())
			{
				bCourtyardPurified = true;
			}
		}

		// Periodically check if player enters courtyard skirmish zone
		World->GetTimerManager().SetTimer(
			CourtyardAlertTimerHandle,
			this,
			&AGanapatiFestivalGameMode::CheckCourtyardProximity,
			0.5f,
			true
		);
	}
}

void AGanapatiFestivalGameMode::EnsureFestivalEnvironment()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1. Ensure PlayerStart is positioned at the beginning of the festival street
	TArray<AActor*> ExistingStarts;
	UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), ExistingStarts);
	bool bHasFestivalStart = false;
	for (AActor* StartActor : ExistingStarts)
	{
		if (APlayerStart* Start = Cast<APlayerStart>(StartActor))
		{
			if (Start->PlayerStartTag == FName(TEXT("FestivalStart")))
			{
				bHasFestivalStart = true;
				break;
			}
		}
	}

	if (!bHasFestivalStart)
	{
		FActorSpawnParameters StartSpawnParams;
		StartSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APlayerStart* FestivalStart = World->SpawnActor<APlayerStart>(
			APlayerStart::StaticClass(),
			FVector(-1300.0f, 0.0f, 50.0f),
			FRotator(0.0f, 0.0f, 0.0f),
			StartSpawnParams
		);
		if (FestivalStart)
		{
			FestivalStart->PlayerStartTag = FName(TEXT("FestivalStart"));
		}
	}

	// 2. Check if a FestivalStreetBuilder already exists in the level
	AFestivalStreetBuilder* ExistingBuilder = nullptr;
	if (UGanapatiWorldSubsystem* Subsystem = World->GetSubsystem<UGanapatiWorldSubsystem>())
	{
		ExistingBuilder = Subsystem->GetRegisteredStreetBuilder();
	}

	if (!ExistingBuilder)
	{
		TArray<AActor*> ExistingBuilders;
		UGameplayStatics::GetAllActorsOfClass(World, AFestivalStreetBuilder::StaticClass(), ExistingBuilders);
		if (ExistingBuilders.Num() > 0)
		{
			ExistingBuilder = Cast<AFestivalStreetBuilder>(ExistingBuilders[0]);
		}
	}

	if (!ExistingBuilder)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		World->SpawnActor<AFestivalStreetBuilder>(
			AFestivalStreetBuilder::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);
	}
}

void AGanapatiFestivalGameMode::PlayOpeningCinematic()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	bIsPlayingOpeningCinematic = true;

	// High panoramic vantage point looking down the festival street towards the Ganesh Pandal
	const FVector CamLoc(-1200.0f, -400.0f, 650.0f);
	const FRotator CamRot(-16.0f, 25.0f, 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CinematicCamera = World->SpawnActor<ACameraActor>(
		ACameraActor::StaticClass(),
		CamLoc,
		CamRot,
		SpawnParams
	);

	if (CinematicCamera)
	{
		// Cut immediately to cinematic camera
		PC->SetViewTarget(CinematicCamera);

		// Schedule transition to player control
		GetWorldTimerManager().SetTimer(
			CinematicTimerHandle,
			this,
			&AGanapatiFestivalGameMode::TransitionToPlayerControl,
			CinematicHoldDuration,
			false
		);
	}
}

void AGanapatiFestivalGameMode::TransitionToPlayerControl()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	APawn* PlayerPawn = PC->GetPawn();
	if (!PlayerPawn)
	{
		return;
	}

	// Smoothly swoop from cinematic wide angle back into the third-person player camera
	PC->SetViewTargetWithBlend(PlayerPawn, CinematicBlendDuration, VTBlend_EaseInOut, 2.0f);

	bIsPlayingOpeningCinematic = false;

	// Welcome celebratory toast on player control handover
	if (AGanapatiGameHUD* HUD = Cast<AGanapatiGameHUD>(PC->GetHUD()))
	{
		HUD->ShowQuestToast(FText::FromString(TEXT("✦ PILGRIMAGE BEGUN: Seek the blessings of Halwai Anand ✦")), 4.0f);
	}

	// Advance initial story state to SacredDarshan when player control begins
	if (CurrentStoryState == EStoryProgressionState::FestivalBeginning)
	{
		const EStoryProgressionState PrevState = CurrentStoryState;
		CurrentStoryState = EStoryProgressionState::SacredDarshan;
		if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
		{
			Subsystem->SetStoryProgressionState(CurrentStoryState);
		}
		OnStoryProgressionChanged.Broadcast(PrevState, CurrentStoryState);
		BP_OnStoryProgressionChanged(PrevState, CurrentStoryState);
	}
}

void AGanapatiFestivalGameMode::SkipOpeningCinematic()
{
	if (!bIsPlayingOpeningCinematic)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(CinematicTimerHandle);
	TransitionToPlayerControl();
}

FString AGanapatiFestivalGameMode::GetCurrentObjectiveTitle() const
{
	if (CurrentStoryState == EStoryProgressionState::SacredJourney)
	{
		if (IsMountainShrineActivated())
		{
			return TEXT("✦ KAILASH COMMUNION ACHIEVED: Sacred Peak Blessed ✦");
		}
		if (IsSacredMountainDiscovered())
		{
			return TEXT("OBJECTIVE: Ascend Mount Kailash to the Summit Shrine");
		}
		if (IsMountainThresholdReached())
		{
			return TEXT("✦ PILGRIMAGE MILESTONE: Threshold of Mount Kailash Reached ✦");
		}
		if (IsDivineAscensionDiscovered())
		{
			return TEXT("OBJECTIVE: Traverse the Ascent to the Mountain Threshold");
		}
		return TEXT("OBJECTIVE: Follow the Sacred Path");
	}

	switch (CurrentQuestStep)
	{
	case ESacredDarshanStep::Step1_SpeakWithAnand:
		return TEXT("[1/4] Seek the blessings of Halwai Anand (Sweetmaker).");
	case ESacredDarshanStep::Step2_ReceiveModakPrasadam:
		return TEXT("[2/4] Taste the Sacred Modak Prasadam at the Sweet Stall.");
	case ESacredDarshanStep::Step3_GrandPandalPrayer:
		return TEXT("[3/4] Offer Prayers before Lord Ganesha at the Grand Pandal.");
	case ESacredDarshanStep::Step4_DamageTrainingDummy:
		return TEXT("[4/4] Test your divine strength on the Courtyard Training Dummy.");
	case ESacredDarshanStep::Completed:
		return TEXT("✦ SACRED DARSHAN COMPLETED — PILGRIMAGE FULFILLED ✦");
	default:
		return TEXT("Explore the festival street.");
	}
}

FString AGanapatiFestivalGameMode::GetCurrentObjectiveDescription() const
{
	if (CurrentStoryState == EStoryProgressionState::SacredJourney)
	{
		if (IsMountainShrineActivated())
		{
			return TEXT("The divine radiance of Mount Kailash blesses you. Your spirit and strength are fully replenished!");
		}
		if (IsSacredMountainDiscovered())
		{
			return TEXT("Use Divine Anti-Gravity [G] across the floating platforms and avoid miasma hazards to reach the summit.");
		}
		if (IsMountainThresholdReached())
		{
			return TEXT("You stand at the sacred gateway to the divine peaks. Ascend into the sacred mountain crags!");
		}
		if (IsDivineAscensionDiscovered())
		{
			return TEXT("Divine Anti-Gravity awakened [G]. Leap across the sacred crags toward the mountain threshold.");
		}
		return TEXT("The courtyard is purified. Proceed through the eastern gate onto the sacred path.");
	}

	switch (CurrentQuestStep)
	{
	case ESacredDarshanStep::Step1_SpeakWithAnand:
		return TEXT("Find Anand near the market entrance stalls and press [E] to talk.");
	case ESacredDarshanStep::Step2_ReceiveModakPrasadam:
		return TEXT("Approach the Modak thali table at the sweets stall and press [E] to partake.");
	case ESacredDarshanStep::Step3_GrandPandalPrayer:
		return TEXT("Walk to the grand altar at the end of the street and press [E] to perform the prayer.");
	case ESacredDarshanStep::Step4_DamageTrainingDummy:
		return TEXT("Enter the courtyard and strike the dummy with a melee combo [LMB] or Divine Shockwave [Q].");
	case ESacredDarshanStep::Completed:
		return TEXT("Lord Vighnaharta's grace is upon you. All divine energies are fully replenished!");
	default:
		return TEXT("");
	}
}

int32 AGanapatiFestivalGameMode::GetCurrentStepNumber() const
{
	switch (CurrentQuestStep)
	{
	case ESacredDarshanStep::Step1_SpeakWithAnand:
		return 1;
	case ESacredDarshanStep::Step2_ReceiveModakPrasadam:
		return 2;
	case ESacredDarshanStep::Step3_GrandPandalPrayer:
		return 3;
	case ESacredDarshanStep::Step4_DamageTrainingDummy:
	case ESacredDarshanStep::Completed:
		return 4;
	default:
		return 1;
	}
}

void AGanapatiFestivalGameMode::BindQuestListeners()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UGanapatiWorldSubsystem* Subsystem = World->GetSubsystem<UGanapatiWorldSubsystem>();
	if (Subsystem)
	{
		// 1. Bind to Subsystem registration / unregistration multicast delegates
		Subsystem->OnNPCRegistered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleNPCRegistered);
		Subsystem->OnNPCRegistered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleNPCRegistered);
		Subsystem->OnNPCUnregistered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleNPCUnregistered);
		Subsystem->OnNPCUnregistered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleNPCUnregistered);

		Subsystem->OnInteractableRegistered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleInteractableRegistered);
		Subsystem->OnInteractableRegistered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleInteractableRegistered);
		Subsystem->OnInteractableUnregistered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleInteractableUnregistered);
		Subsystem->OnInteractableUnregistered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleInteractableUnregistered);

		Subsystem->OnTrainingDummyRegistered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleTrainingDummyRegistered);
		Subsystem->OnTrainingDummyRegistered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleTrainingDummyRegistered);
		Subsystem->OnTrainingDummyUnregistered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleTrainingDummyUnregistered);
		Subsystem->OnTrainingDummyUnregistered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleTrainingDummyUnregistered);

		Subsystem->OnAsuraRegistered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleAsuraRegistered);
		Subsystem->OnAsuraRegistered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleAsuraRegistered);
		Subsystem->OnAsuraUnregistered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleAsuraUnregistered);
		Subsystem->OnAsuraUnregistered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleAsuraUnregistered);

		Subsystem->OnCaptainRegistered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleCaptainRegistered);
		Subsystem->OnCaptainRegistered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleCaptainRegistered);
		Subsystem->OnCaptainUnregistered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleCaptainUnregistered);
		Subsystem->OnCaptainUnregistered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleCaptainUnregistered);

		Subsystem->OnStreetBuilderRegistered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleStreetBuilderRegistered);
		Subsystem->OnStreetBuilderRegistered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleStreetBuilderRegistered);
		Subsystem->OnStreetBuilderUnregistered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleStreetBuilderUnregistered);
		Subsystem->OnStreetBuilderUnregistered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleStreetBuilderUnregistered);

		// Phase 6C: World Region & Milestone Delegates
		Subsystem->OnWorldRegionChanged.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleWorldRegionChanged);
		Subsystem->OnWorldRegionChanged.AddDynamic(this, &AGanapatiFestivalGameMode::HandleWorldRegionChanged);
		Subsystem->OnDivineAscensionDiscovered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleDivineAscensionDiscovered);
		Subsystem->OnDivineAscensionDiscovered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleDivineAscensionDiscovered);
		Subsystem->OnPilgrimageMilestoneReached.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandlePilgrimageMilestoneReached);
		Subsystem->OnPilgrimageMilestoneReached.AddDynamic(this, &AGanapatiFestivalGameMode::HandlePilgrimageMilestoneReached);
		Subsystem->OnSacredMountainDiscovered.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleSacredMountainDiscovered);
		Subsystem->OnSacredMountainDiscovered.AddDynamic(this, &AGanapatiFestivalGameMode::HandleSacredMountainDiscovered);
		Subsystem->OnMountainShrineActivated.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleMountainShrineActivated);
		Subsystem->OnMountainShrineActivated.AddDynamic(this, &AGanapatiFestivalGameMode::HandleMountainShrineActivated);

		// 2. Process all actors already registered before GameMode initialized
		for (AGanapatiNPC* NPC : Subsystem->GetRegisteredNPCs())
		{
			HandleNPCRegistered(NPC);
		}
		for (AGanapatiInteractable* Interactable : Subsystem->GetRegisteredInteractables())
		{
			HandleInteractableRegistered(Interactable);
		}
		for (AGanapatiTrainingDummy* Dummy : Subsystem->GetRegisteredTrainingDummies())
		{
			HandleTrainingDummyRegistered(Dummy);
		}
		for (AGanapatiAsuraMinion* Asura : Subsystem->GetRegisteredAsuras())
		{
			HandleAsuraRegistered(Asura);
		}
		if (AGanapatiAsuraCaptain* Captain = Subsystem->GetRegisteredCaptain())
		{
			HandleCaptainRegistered(Captain);
		}
		if (AFestivalStreetBuilder* Builder = Subsystem->GetRegisteredStreetBuilder())
		{
			HandleStreetBuilderRegistered(Builder);
		}
	}

	// 3. Bind to Player Character death event for boss fight reset (Phase 5C Subsystem 4)
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
	{
		if (AGanapatiPlayerCharacter* PlayerChar = Cast<AGanapatiPlayerCharacter>(PlayerPawn))
		{
			PlayerChar->OnCharacterDied.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandlePlayerDied);
			PlayerChar->OnCharacterDied.AddDynamic(this, &AGanapatiFestivalGameMode::HandlePlayerDied);
		}
	}

	// 4. Start Captain proximity monitoring timer
	World->GetTimerManager().SetTimer(
		CaptainProximityTimerHandle,
		this,
		&AGanapatiFestivalGameMode::CheckCaptainProximity,
		0.25f,
		true
	);

	UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Reactive streaming listeners bound via UGanapatiWorldSubsystem."));
}

void AGanapatiFestivalGameMode::HandleNPCRegistered(AGanapatiNPC* NPC)
{
	if (!NPC)
	{
		return;
	}

	NPC->OnNPCDialogueSpoken.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleNPCDialogueSpoken);
	NPC->OnNPCDialogueSpoken.AddDynamic(this, &AGanapatiFestivalGameMode::HandleNPCDialogueSpoken);
}

void AGanapatiFestivalGameMode::HandleNPCUnregistered(AGanapatiNPC* NPC)
{
	if (!NPC)
	{
		return;
	}

	NPC->OnNPCDialogueSpoken.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleNPCDialogueSpoken);
}

void AGanapatiFestivalGameMode::HandleInteractableRegistered(AGanapatiInteractable* Interactable)
{
	if (!Interactable)
	{
		return;
	}

	Interactable->OnInteracted.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleInteractableInteracted);
	Interactable->OnInteracted.AddDynamic(this, &AGanapatiFestivalGameMode::HandleInteractableInteracted);

	Interactable->OnPrayerCompleted.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandlePrayerCompleted);
	Interactable->OnPrayerCompleted.AddDynamic(this, &AGanapatiFestivalGameMode::HandlePrayerCompleted);
}

void AGanapatiFestivalGameMode::HandleInteractableUnregistered(AGanapatiInteractable* Interactable)
{
	if (!Interactable)
	{
		return;
	}

	Interactable->OnInteracted.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleInteractableInteracted);
	Interactable->OnPrayerCompleted.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandlePrayerCompleted);
}

void AGanapatiFestivalGameMode::HandleTrainingDummyRegistered(AGanapatiTrainingDummy* Dummy)
{
	if (!Dummy)
	{
		return;
	}

	Dummy->OnDamageConfirmed.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleDummyDamageConfirmed);
	Dummy->OnDamageConfirmed.AddDynamic(this, &AGanapatiFestivalGameMode::HandleDummyDamageConfirmed);
}

void AGanapatiFestivalGameMode::HandleTrainingDummyUnregistered(AGanapatiTrainingDummy* Dummy)
{
	if (!Dummy)
	{
		return;
	}

	Dummy->OnDamageConfirmed.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleDummyDamageConfirmed);
}

void AGanapatiFestivalGameMode::HandleAsuraRegistered(AGanapatiAsuraMinion* Asura)
{
	if (!Asura)
	{
		return;
	}

	if (Asura->CountsTowardEncounter())
	{
		TrackedEncounterAsuras.Add(Asura);
		TotalAsurasSpawned = TrackedEncounterAsuras.Num();

		Asura->OnAsuraDied.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleAsuraDied);
		Asura->OnAsuraDied.AddDynamic(this, &AGanapatiFestivalGameMode::HandleAsuraDied);

		UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Registered encounter target [%s] (Total: %d)"),
			*Asura->GetName(), TotalAsurasSpawned);
	}
}

void AGanapatiFestivalGameMode::HandleAsuraUnregistered(AGanapatiAsuraMinion* Asura)
{
	if (!Asura)
	{
		return;
	}

	Asura->OnAsuraDied.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleAsuraDied);
}

void AGanapatiFestivalGameMode::HandleCaptainRegistered(AGanapatiAsuraCaptain* Captain)
{
	if (!Captain)
	{
		return;
	}

	CachedCaptain = Captain;
	Captain->OnAsuraDied.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleCaptainDied);
	Captain->OnAsuraDied.AddDynamic(this, &AGanapatiFestivalGameMode::HandleCaptainDied);

	UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Registered and cached Asura Captain [%s]"), *Captain->GetName());
}

void AGanapatiFestivalGameMode::HandleCaptainUnregistered(AGanapatiAsuraCaptain* Captain)
{
	if (CachedCaptain == Captain)
	{
		if (Captain)
		{
			Captain->OnAsuraDied.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleCaptainDied);
		}
		CachedCaptain.Reset();
		UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Unregistered Asura Captain"));
	}
}

void AGanapatiFestivalGameMode::HandleStreetBuilderRegistered(AFestivalStreetBuilder* StreetBuilder)
{
	if (!StreetBuilder)
	{
		return;
	}

	CachedStreetBuilder = StreetBuilder;
	UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Registered and cached FestivalStreetBuilder [%s]"), *StreetBuilder->GetName());
}

void AGanapatiFestivalGameMode::HandleStreetBuilderUnregistered(AFestivalStreetBuilder* StreetBuilder)
{
	if (CachedStreetBuilder == StreetBuilder)
	{
		CachedStreetBuilder.Reset();
		UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Unregistered FestivalStreetBuilder"));
	}
}

void AGanapatiFestivalGameMode::AdvanceQuestStep(ESacredDarshanStep ExpectedCurrentStep, ESacredDarshanStep NextStep, const FText& CompletionToastText)
{
	if (CurrentQuestStep != ExpectedCurrentStep)
	{
		return;
	}

	const ESacredDarshanStep PrevStep = CurrentQuestStep;
	CurrentQuestStep = NextStep;

	// Display completion toast on HUD
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		if (AGanapatiGameHUD* HUD = Cast<AGanapatiGameHUD>(PC->GetHUD()))
		{
			HUD->ShowQuestToast(CompletionToastText, 4.0f);
		}
	}

	OnQuestStepAdvanced.Broadcast(PrevStep, NextStep);
	BP_OnQuestStepAdvanced(PrevStep, NextStep);

	UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Quest advanced from Step %d to Step %d: %s"),
		static_cast<uint8>(PrevStep) + 1, static_cast<uint8>(NextStep) + 1, *CompletionToastText.ToString());

	// If final completion
	if (NextStep == ESacredDarshanStep::Completed)
	{
		// Restore Divine Energy to full 100/100
		if (PC)
		{
			if (AGanapatiPlayerCharacter* PlayerChar = Cast<AGanapatiPlayerCharacter>(PC->GetPawn()))
			{
				PlayerChar->AddDivineEnergy(PlayerChar->GetMaxDivineEnergy());
			}
		}

		OnQuestCompleted.Broadcast();
		BP_OnQuestCompleted();

		UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: ✦ SACRED DARSHAN QUEST FULLY COMPLETED! ✦"));
	}
}

void AGanapatiFestivalGameMode::HandleNPCDialogueSpoken(AGanapatiNPC* NPC, AActor* Interactor, const FText& SpokenLine)
{
	if (CurrentQuestStep != ESacredDarshanStep::Step1_SpeakWithAnand || !NPC)
	{
		return;
	}

	// Verify the NPC is Halwai Anand (Sweetmaker)
	const FString Name = NPC->GetNPCName();
	if (Name.Contains(TEXT("Anand")) || Name.Contains(TEXT("Sweetmaker")))
	{
		AdvanceQuestStep(
			ESacredDarshanStep::Step1_SpeakWithAnand,
			ESacredDarshanStep::Step2_ReceiveModakPrasadam,
			FText::FromString(TEXT("✓ Step 1 Complete: Blessed by Halwai Anand!"))
		);
	}
}

void AGanapatiFestivalGameMode::HandleInteractableInteracted(AActor* Interactor, const FText& Message)
{
	const FString MsgStr = Message.ToString();

	// Phase 6D: Kailash Summit Shrine communion
	if (MsgStr.Contains(TEXT("Kailash")) || MsgStr.Contains(TEXT("Mount Kailash")))
	{
		if (UWorld* World = GetWorld())
		{
			if (UGanapatiWorldSubsystem* Subsystem = World->GetSubsystem<UGanapatiWorldSubsystem>())
			{
				if (!Subsystem->IsMountainShrineActivated())
				{
					Subsystem->SetMountainShrineActivated(true);
				}
			}
		}
		return;
	}

	if (CurrentQuestStep != ESacredDarshanStep::Step2_ReceiveModakPrasadam)
	{
		return;
	}

	// Modak Stall interaction broadcasts this with "Prasadam" or tag
	if (MsgStr.Contains(TEXT("Modak")) || MsgStr.Contains(TEXT("Prasadam")))
	{
		AdvanceQuestStep(
			ESacredDarshanStep::Step2_ReceiveModakPrasadam,
			ESacredDarshanStep::Step3_GrandPandalPrayer,
			FText::FromString(TEXT("✓ Step 2 Complete: Sacred Modak Prasadam received!"))
		);
	}
}

void AGanapatiFestivalGameMode::HandlePrayerCompleted(AActor* Interactor)
{
	if (CurrentQuestStep != ESacredDarshanStep::Step3_GrandPandalPrayer)
	{
		return;
	}

	AdvanceQuestStep(
		ESacredDarshanStep::Step3_GrandPandalPrayer,
		ESacredDarshanStep::Step4_DamageTrainingDummy,
		FText::FromString(TEXT("✓ Step 3 Complete: Sacred Darshan of Lord Ganesha Achieved!"))
	);
}

void AGanapatiFestivalGameMode::HandleDummyDamageConfirmed(AGanapatiTrainingDummy* Dummy, float DamageTaken, AActor* DamageCauser, const FVector& DamageLocation)
{
	if (CurrentQuestStep != ESacredDarshanStep::Step4_DamageTrainingDummy)
	{
		return;
	}

	// Strict requirement: damage must be confirmed and greater than 0
	if (DamageTaken > 0.0f)
	{
		AdvanceQuestStep(
			ESacredDarshanStep::Step4_DamageTrainingDummy,
			ESacredDarshanStep::Completed,
			FText::FromString(TEXT("✦ Step 4 Complete: Divine Strength Proven! Sacred Darshan Fulfilled! ✦"))
		);
	}
}

void AGanapatiFestivalGameMode::HandleAsuraDied(AGanapatiAsuraMinion* Asura)
{
	if (Asura && Asura->CountsTowardEncounter())
	{
		TrackedDefeatedAsuras.Add(Asura);
		DefeatedAsurasCount = TrackedDefeatedAsuras.Num();
	}
	else
	{
		++DefeatedAsurasCount;
	}

	UE_LOG(LogTemp, Warning, TEXT("AGanapatiFestivalGameMode: Asura defeated (%d/%d)"), DefeatedAsurasCount, TotalAsurasSpawned);

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	AGanapatiGameHUD* HUD = Cast<AGanapatiGameHUD>(PC->GetHUD());
	if (!HUD)
	{
		return;
	}

	if (TotalAsurasSpawned > 0 && DefeatedAsurasCount >= TotalAsurasSpawned)
	{
		EncounterState = ECourtyardEncounterState::Completed;
		HUD->ShowQuestToast(FText::FromString(TEXT("⚔ Victory! Courtyard Cleared: All 2 Asura Minions Banished! ⚔")), 5.0f);
		CheckCourtyardPurification();
	}
	else
	{
		const int32 Remaining = FMath::Max(0, TotalAsurasSpawned - DefeatedAsurasCount);
		HUD->ShowQuestToast(FText::FromString(FString::Printf(TEXT("⚔ Asura Minion Banished! [%d / %d] — %d Remaining ⚔"),
			DefeatedAsurasCount, TotalAsurasSpawned, Remaining)), 3.0f);
	}
}

void AGanapatiFestivalGameMode::StartCourtyardEncounter()
{
	if (EncounterState != ECourtyardEncounterState::NotStarted)
	{
		return;
	}

	EncounterState = ECourtyardEncounterState::Active;
	bCourtyardAlertTriggered = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CourtyardAlertTimerHandle);
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		if (AGanapatiGameHUD* HUD = Cast<AGanapatiGameHUD>(PC->GetHUD()))
		{
			HUD->ShowQuestToast(FText::FromString(TEXT("⚔ Courtyard Skirmish: 2 Corrupted Asura Minions Detected! ⚔")), 4.0f);
		}

		if (EncounterStartCameraShakeClass)
		{
			PC->ClientStartCameraShake(EncounterStartCameraShakeClass, 0.35f);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Courtyard Skirmish encounter started (Official Targets: %d)."), TotalAsurasSpawned);

	// Advance story progression state to CourtyardAttack (Phase 5D Subsystem 1)
	if (CurrentStoryState < EStoryProgressionState::CourtyardAttack)
	{
		const EStoryProgressionState PrevState = CurrentStoryState;
		CurrentStoryState = EStoryProgressionState::CourtyardAttack;
		if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
		{
			Subsystem->SetStoryProgressionState(CurrentStoryState);
		}
		OnStoryProgressionChanged.Broadcast(PrevState, CurrentStoryState);
		BP_OnStoryProgressionChanged(PrevState, CurrentStoryState);
	}
}

void AGanapatiFestivalGameMode::CheckCourtyardProximity()
{
	if (EncounterState != ECourtyardEncounterState::NotStarted)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CourtyardAlertTimerHandle);
		}
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	const FVector PlayerLoc = PlayerPawn->GetActorLocation();
	const FVector CourtyardCenter(800.0f, 1850.0f, 50.0f);
	if (FVector::Dist2D(PlayerLoc, CourtyardCenter) <= 1200.0f)
	{
		StartCourtyardEncounter();
	}
}

void AGanapatiFestivalGameMode::CheckCaptainProximity()
{
	if (CaptainEncounterState != ECaptainEncounterState::Dormant)
	{
		return;
	}

	if (!CachedCaptain.IsValid() || CachedCaptain->IsDead())
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	if (AGanapatiPlayerCharacter* PlayerChar = Cast<AGanapatiPlayerCharacter>(PlayerPawn))
	{
		if (PlayerChar->IsDead())
		{
			SetBossBarrierActive(false);
			return;
		}
	}

	const FVector PlayerLoc = PlayerPawn->GetActorLocation();
	const FVector CaptainLoc = CachedCaptain->GetActorLocation();

	if (FVector::Dist2D(PlayerLoc, CaptainLoc) <= 850.0f)
	{
		StartCaptainEncounter();
	}
}

void AGanapatiFestivalGameMode::StartCaptainEncounter()
{
	if (CaptainEncounterState != ECaptainEncounterState::Dormant)
	{
		return;
	}

	CaptainEncounterState = ECaptainEncounterState::Intro;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;

	// Subtle camera rumble on boss intro
	if (EncounterStartCameraShakeClass && PC)
	{
		PC->ClientStartCameraShake(EncounterStartCameraShakeClass, 0.5f);
	}

	// Announcement toast on HUD
	if (PC)
	{
		if (AGanapatiGameHUD* HUD = Cast<AGanapatiGameHUD>(PC->GetHUD()))
		{
			HUD->ShowQuestToast(FText::FromString(TEXT("⚔ ASURA CAPTAIN — CORRUPTED SHADOW COMMANDER ⚔")), 3.5f);
		}
	}

	// Safe arena ward activation: only seal barrier if player is safely inside courtyard (Y > 1500)
	if (PlayerPawn && PlayerPawn->GetActorLocation().Y > 1500.0f)
	{
		SetBossBarrierActive(true);
	}

	// Alert the Captain to acquire player and select initial pattern
	if (CachedCaptain.IsValid())
	{
		CachedCaptain->SelectAttackForDistance();
	}

	// Smooth transition to active combat after 1.5s intro (player controls remain 100% active throughout)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CaptainIntroTimerHandle,
			this,
			&AGanapatiFestivalGameMode::TransitionCaptainToActive,
			1.5f,
			false
		);
	}

	UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Asura Captain mini-boss encounter triggered!"));
}

void AGanapatiFestivalGameMode::TransitionCaptainToActive()
{
	if (CaptainEncounterState == ECaptainEncounterState::Intro)
	{
		CaptainEncounterState = ECaptainEncounterState::Active;
	}
}

void AGanapatiFestivalGameMode::HandleCaptainDied(AGanapatiAsuraMinion* Asura)
{
	if (CaptainEncounterState == ECaptainEncounterState::Defeated)
	{
		return;
	}

	CaptainEncounterState = ECaptainEncounterState::Defeated;

	// Clear encounter proximity timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CaptainProximityTimerHandle);
		World->GetTimerManager().ClearTimer(CaptainIntroTimerHandle);
	}

	// Dissolve arena ward barrier immediately
	SetBossBarrierActive(false);

	// Celebratory victory toast on HUD
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AGanapatiGameHUD* HUD = Cast<AGanapatiGameHUD>(PC->GetHUD()))
		{
			HUD->ShowQuestToast(FText::FromString(TEXT("✦ ASURA CAPTAIN BANISHED! THE INNER COURTYARD IS PURIFIED! ✦")), 4.0f);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Asura Captain mini-boss defeated and banished!"));

	// Advance story progression state to CaptainDefeated if not yet purified (Phase 5D Subsystem 1)
	if (!bCourtyardPurified && CurrentStoryState < EStoryProgressionState::CaptainDefeated)
	{
		const EStoryProgressionState PrevState = CurrentStoryState;
		CurrentStoryState = EStoryProgressionState::CaptainDefeated;
		if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
		{
			Subsystem->SetStoryProgressionState(CurrentStoryState);
		}
		OnStoryProgressionChanged.Broadcast(PrevState, CurrentStoryState);
		BP_OnStoryProgressionChanged(PrevState, CurrentStoryState);
	}

	// Check if full courtyard purification conditions are met (Phase 5C Subsystem 4)
	CheckCourtyardPurification();
}

void AGanapatiFestivalGameMode::HandlePlayerDied()
{
	// If player dies before SacredJourney begins, reset pre-purification story state and keep gate locked
	if (!bSacredJourneyUnlocked)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DivineMomentTimerHandle);
		}

		if (bCourtyardPurified)
		{
			bCourtyardPurified = false;
			if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
			{
				Subsystem->SetCourtyardPurified(false);
			}
		}

		if (CurrentStoryState == EStoryProgressionState::CaptainDefeated || CurrentStoryState == EStoryProgressionState::CourtyardPurified)
		{
			const EStoryProgressionState PrevState = CurrentStoryState;
			CurrentStoryState = EStoryProgressionState::CourtyardAttack;
			if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
			{
				Subsystem->SetStoryProgressionState(CurrentStoryState);
			}
			OnStoryProgressionChanged.Broadcast(PrevState, CurrentStoryState);
			BP_OnStoryProgressionChanged(PrevState, CurrentStoryState);
		}

		SetSacredPathUnlocked(false);
	}

	if (CaptainEncounterState == ECaptainEncounterState::Active || CaptainEncounterState == ECaptainEncounterState::Intro)
	{
		CaptainEncounterState = ECaptainEncounterState::Dormant;
		SetBossBarrierActive(false);

		if (CachedCaptain.IsValid())
		{
			CachedCaptain->ResetBossState();
		}

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CaptainProximityTimerHandle);
			World->GetTimerManager().ClearTimer(CaptainIntroTimerHandle);
			World->GetTimerManager().SetTimer(
				CaptainProximityTimerHandle,
				this,
				&AGanapatiFestivalGameMode::CheckCaptainProximity,
				0.25f,
				true
			);
		}

		UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Player died during boss fight. Captain reset to pristine spawn state, barrier dissolved."));
	}
}

void AGanapatiFestivalGameMode::CheckCourtyardPurification()
{
	if (bCourtyardPurified)
	{
		return;
	}

	// Purification occurs ONLY when:
	// - 2/2 basic Minions defeated (IsEncounterCompleted)
	// - Captain defeated (CaptainEncounterState == ECaptainEncounterState::Defeated)
	if (IsEncounterCompleted() && CaptainEncounterState == ECaptainEncounterState::Defeated)
	{
		bCourtyardPurified = true;
		TriggerCourtyardPurification();
	}
}

void AGanapatiFestivalGameMode::TriggerCourtyardPurification()
{
	// 1. Restore player health to maximum
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (AGanapatiPlayerCharacter* PlayerChar = Cast<AGanapatiPlayerCharacter>(PlayerPawn))
		{
			PlayerChar->ResetHealth();
		}
	}

	// 2. Display approved purification toast and non-intrusive camera pulse
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AGanapatiGameHUD* HUD = Cast<AGanapatiGameHUD>(PC->GetHUD()))
		{
			HUD->ShowQuestToast(FText::FromString(TEXT("✦ INNER COURTYARD PURIFIED: THE DIVINE SANCTUARY RESTORED! ✦")), 5.0f);
		}

		// Non-intrusive camera feedback: player movement and camera look remain 100% active
		if (EncounterStartCameraShakeClass)
		{
			PC->ClientStartCameraShake(EncounterStartCameraShakeClass, 0.75f);
		}
	}

	// 3. Fire Blueprint implementable hook
	BP_OnCourtyardPurified();

	// 4. Advance overarching story progression state to CourtyardPurified (Phase 5D Subsystem 1)
	const EStoryProgressionState PrevState = CurrentStoryState;
	CurrentStoryState = EStoryProgressionState::CourtyardPurified;
	if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
	{
		Subsystem->SetCourtyardPurified(true);
		Subsystem->SetStoryProgressionState(CurrentStoryState);
	}
	OnStoryProgressionChanged.Broadcast(PrevState, CurrentStoryState);
	BP_OnStoryProgressionChanged(PrevState, CurrentStoryState);

	UE_LOG(LogTemp, Warning, TEXT("AGanapatiFestivalGameMode: ✦ COURTYARD PURIFIED! 2/2 Minions and Asura Captain banished! Player health restored. ✦"));

	// 5. One-time 2.5-second non-intrusive divine presentation before transitioning to SacredJourney
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DivineMomentTimerHandle);
		World->GetTimerManager().SetTimer(
			DivineMomentTimerHandle,
			this,
			&AGanapatiFestivalGameMode::OnDivineStoryMomentCompleted,
			2.5f,
			false
		);
	}
}

void AGanapatiFestivalGameMode::OnDivineStoryMomentCompleted()
{
	if (bSacredJourneyUnlocked)
	{
		return;
	}

	bSacredJourneyUnlocked = true;
	const EStoryProgressionState PrevState = CurrentStoryState;
	CurrentStoryState = EStoryProgressionState::SacredJourney;
	if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
	{
		Subsystem->SetSacredPathUnlocked(true);
		Subsystem->SetStoryProgressionState(CurrentStoryState);
	}

	// Unlock physical Sacred Path gate in world
	SetSacredPathUnlocked(true);

	// Display story progression message on HUD
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AGanapatiGameHUD* HUD = Cast<AGanapatiGameHUD>(PC->GetHUD()))
		{
			HUD->ShowQuestToast(FText::FromString(TEXT("✦ THE SACRED PATH HAS OPENED — FOLLOW THE SACRED PATH! ✦")), 5.0f);
		}
	}

	// Dispatch delegates and blueprint hooks
	OnStoryProgressionChanged.Broadcast(PrevState, CurrentStoryState);
	BP_OnStoryProgressionChanged(PrevState, CurrentStoryState);
	OnSacredJourneyUnlocked.Broadcast();
	BP_OnSacredJourneyUnlocked();

	UE_LOG(LogTemp, Warning, TEXT("AGanapatiFestivalGameMode: ✦ SACRED JOURNEY UNLOCKED! Eastern courtyard gate is passable. OBJECTIVE: Follow the Sacred Path ✦"));
}

void AGanapatiFestivalGameMode::SetBossBarrierActive(bool bActive)
{
	if (CachedStreetBuilder.IsValid())
	{
		CachedStreetBuilder->SetBossBarrierActive(bActive);
	}
}

void AGanapatiFestivalGameMode::SetSacredPathUnlocked(bool bUnlocked)
{
	bSacredJourneyUnlocked = bUnlocked;
	if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
	{
		Subsystem->SetSacredPathUnlocked(bUnlocked);
	}
	if (CachedStreetBuilder.IsValid())
	{
		CachedStreetBuilder->SetSacredPathUnlocked(bUnlocked);
	}
}

AGanapatiAsuraCaptain* AGanapatiFestivalGameMode::GetActiveCaptain() const
{
	return CachedCaptain.Get();
}

EWorldRegion AGanapatiFestivalGameMode::GetActiveRegion() const
{
	if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
	{
		return Subsystem->GetActiveRegion();
	}
	return EWorldRegion::FestivalStreet;
}

bool AGanapatiFestivalGameMode::IsDivineAscensionDiscovered() const
{
	if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
	{
		return Subsystem->IsDivineAscensionDiscovered();
	}
	return false;
}

bool AGanapatiFestivalGameMode::IsMountainThresholdReached() const
{
	if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
	{
		return Subsystem->IsMountainThresholdReached();
	}
	return false;
}

bool AGanapatiFestivalGameMode::IsSacredMountainDiscovered() const
{
	if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
	{
		return Subsystem->IsSacredMountainDiscovered();
	}
	return false;
}

bool AGanapatiFestivalGameMode::IsMountainShrineActivated() const
{
	if (UGanapatiWorldSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UGanapatiWorldSubsystem>() : nullptr)
	{
		return Subsystem->IsMountainShrineActivated();
	}
	return false;
}

void AGanapatiFestivalGameMode::HandleWorldRegionChanged(EWorldRegion PreviousRegion, EWorldRegion NewRegion)
{
	UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: World Region transitioned from %d to %d"),
		static_cast<uint8>(PreviousRegion), static_cast<uint8>(NewRegion));
}

void AGanapatiFestivalGameMode::HandleDivineAscensionDiscovered(bool bDiscovered)
{
	if (bDiscovered)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			if (AGanapatiGameHUD* HUD = Cast<AGanapatiGameHUD>(PC->GetHUD()))
			{
				HUD->ShowQuestToast(FText::FromString(TEXT("✦ DIVINE ASCENSION UNLOCKED: ANTI-GRAVITY TRAVERSAL [G] ✦")), 5.0f);
			}
		}

		BP_OnDivineAscensionDiscovered();
		UE_LOG(LogTemp, Warning, TEXT("AGanapatiFestivalGameMode: ✦ DIVINE ASCENSION DISCOVERED! Player can traverse vertical crags using Anti-Gravity [G]. ✦"));
	}
}

void AGanapatiFestivalGameMode::HandlePilgrimageMilestoneReached(bool bReached)
{
	if (bReached)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			if (AGanapatiGameHUD* HUD = Cast<AGanapatiGameHUD>(PC->GetHUD()))
			{
				HUD->ShowQuestToast(FText::FromString(TEXT("✦ PILGRIMAGE MILESTONE REACHED — THRESHOLD OF MOUNT KAILASH ✦")), 6.0f);
			}
		}

		BP_OnMountainThresholdReached();
		UE_LOG(LogTemp, Warning, TEXT("AGanapatiFestivalGameMode: ✦ PILGRIMAGE MILESTONE REACHED! Player reached the Mountain Threshold Dais. ✦"));
	}
}

void AGanapatiFestivalGameMode::HandleSacredMountainDiscovered(bool bDiscovered)
{
	if (bDiscovered)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			if (AGanapatiGameHUD* HUD = Cast<AGanapatiGameHUD>(PC->GetHUD()))
			{
				HUD->ShowQuestToast(FText::FromString(TEXT("✦ SACRED MOUNTAIN REACHED: ASCEND TO THE KAILASH SUMMIT ✦")), 6.0f);
			}
		}

		BP_OnSacredMountainDiscovered();
		UE_LOG(LogTemp, Warning, TEXT("AGanapatiFestivalGameMode: ✦ SACRED MOUNTAIN DISCOVERED! Player commenced ascent to Mount Kailash. ✦"));
	}
}

void AGanapatiFestivalGameMode::HandleMountainShrineActivated(bool bActivated)
{
	if (bActivated)
	{
		bIsSummitFinaleActive = true;

		// Replenish player divine energy and health to maximum
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (AGanapatiPlayerCharacter* PlayerChar = Cast<AGanapatiPlayerCharacter>(PlayerPawn))
			{
				PlayerChar->AddDivineEnergy(PlayerChar->GetMaxDivineEnergy());
				PlayerChar->ResetHealth();

				// Trigger celebratory camera pulse
				if (APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController()))
				{
					if (PlayerChar->GetMeleeHitCameraShakeClass())
					{
						PC->ClientStartCameraShake(PlayerChar->GetMeleeHitCameraShakeClass(), 0.6f);
					}
				}
			}
		}

		// Enhance summit divine lighting
		if (CachedStreetBuilder.IsValid())
		{
			CachedStreetBuilder->EnhanceSummitCommunionLighting();
		}

		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			if (AGanapatiGameHUD* HUD = Cast<AGanapatiGameHUD>(PC->GetHUD()))
			{
				HUD->ShowQuestToast(FText::FromString(TEXT("✦ KAILASH COMMUNION: Sacred Peak Blessed! Divine Grace Perfected! ✦")), 7.0f);
			}
		}

		// Set a timer to smoothly resolve the finale active flag after 14 seconds
		GetWorldTimerManager().ClearTimer(SummitFinaleTimerHandle);
		GetWorldTimerManager().SetTimer(
			SummitFinaleTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				bIsSummitFinaleActive = false;
			}),
			14.0f,
			false
		);

		BP_OnMountainShrineActivated();
		UE_LOG(LogTemp, Warning, TEXT("AGanapatiFestivalGameMode: ✦ KAILASH COMMUNION ACHIEVED! Player activated the Kailash Summit Shrine. ✦"));
	}
}
