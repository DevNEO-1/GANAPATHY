#include "GameModes/GanapatiFestivalGameMode.h"
#include "Environment/FestivalStreetBuilder.h"
#include "UI/GanapatiGameHUD.h"
#include "Camera/CameraActor.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "NPCs/GanapatiNPC.h"
#include "Interaction/GanapatiInteractable.h"
#include "Enemies/GanapatiTrainingDummy.h"
#include "Enemies/GanapatiAsuraMinion.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"

AGanapatiFestivalGameMode::AGanapatiFestivalGameMode()
{
	// Set custom festival HUD
	HUDClass = AGanapatiGameHUD::StaticClass();
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

	// Bind quest event listeners (slightly deferred to ensure spawned street actors have initialized)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			QuestBindTimerHandle,
			this,
			&AGanapatiFestivalGameMode::BindQuestListeners,
			0.2f,
			false
		);

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
	TArray<AActor*> ExistingBuilders;
	UGameplayStatics::GetAllActorsOfClass(World, AFestivalStreetBuilder::StaticClass(), ExistingBuilders);

	if (ExistingBuilders.Num() == 0)
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
}

FString AGanapatiFestivalGameMode::GetCurrentObjectiveTitle() const
{
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

	// 1. Bind to Devotee NPCs for Step 1 (Halwai Anand dialogue)
	TArray<AActor*> NPCs;
	UGameplayStatics::GetAllActorsOfClass(World, AGanapatiNPC::StaticClass(), NPCs);
	for (AActor* Actor : NPCs)
	{
		if (AGanapatiNPC* NPC = Cast<AGanapatiNPC>(Actor))
		{
			NPC->OnNPCDialogueSpoken.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleNPCDialogueSpoken);
			NPC->OnNPCDialogueSpoken.AddDynamic(this, &AGanapatiFestivalGameMode::HandleNPCDialogueSpoken);
		}
	}

	// 2. Bind to Interactables for Step 2 (Modak Stall) and Step 3 (Grand Pandal Prayer)
	TArray<AActor*> Interactables;
	UGameplayStatics::GetAllActorsOfClass(World, AGanapatiInteractable::StaticClass(), Interactables);
	for (AActor* Actor : Interactables)
	{
		if (AGanapatiInteractable* Interactable = Cast<AGanapatiInteractable>(Actor))
		{
			Interactable->OnInteracted.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleInteractableInteracted);
			Interactable->OnInteracted.AddDynamic(this, &AGanapatiFestivalGameMode::HandleInteractableInteracted);

			Interactable->OnPrayerCompleted.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandlePrayerCompleted);
			Interactable->OnPrayerCompleted.AddDynamic(this, &AGanapatiFestivalGameMode::HandlePrayerCompleted);
		}
	}

	// 3. Bind to Training Dummy for Step 4 (Confirmed Combat Damage)
	TArray<AActor*> Dummies;
	UGameplayStatics::GetAllActorsOfClass(World, AGanapatiTrainingDummy::StaticClass(), Dummies);
	for (AActor* Actor : Dummies)
	{
		if (AGanapatiTrainingDummy* Dummy = Cast<AGanapatiTrainingDummy>(Actor))
		{
			Dummy->OnDamageConfirmed.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleDummyDamageConfirmed);
			Dummy->OnDamageConfirmed.AddDynamic(this, &AGanapatiFestivalGameMode::HandleDummyDamageConfirmed);
		}
	}

	// 4. Bind to Asura Minions for Courtyard Skirmish Encounter (Phase 5B Subsystem 1)
	TArray<AActor*> Asuras;
	UGameplayStatics::GetAllActorsOfClass(World, AGanapatiAsuraMinion::StaticClass(), Asuras);
	TotalAsurasSpawned = 0;
	DefeatedAsurasCount = 0;

	for (AActor* Actor : Asuras)
	{
		if (AGanapatiAsuraMinion* Asura = Cast<AGanapatiAsuraMinion>(Actor))
		{
			if (Asura->CountsTowardEncounter())
			{
				++TotalAsurasSpawned;
				Asura->OnAsuraDied.RemoveDynamic(this, &AGanapatiFestivalGameMode::HandleAsuraDied);
				Asura->OnAsuraDied.AddDynamic(this, &AGanapatiFestivalGameMode::HandleAsuraDied);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Sacred Darshan quest listeners bound to %d NPCs, %d Interactables, %d Dummies, %d Asuras (%d encounter targets)."),
		NPCs.Num(), Interactables.Num(), Dummies.Num(), Asuras.Num(), TotalAsurasSpawned);
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
	if (CurrentQuestStep != ESacredDarshanStep::Step2_ReceiveModakPrasadam)
	{
		return;
	}

	// Modak Stall interaction broadcasts this with "Prasadam" or tag
	const FString MsgStr = Message.ToString();
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
	++DefeatedAsurasCount;

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
		HUD->ShowQuestToast(FText::FromString(TEXT("⚔ Victory! Courtyard Cleared: All Asura Minions Banished! ⚔")), 4.5f);
	}
	else
	{
		HUD->ShowQuestToast(FText::FromString(FString::Printf(TEXT("⚔ Asura Banished! (%d/%d) ⚔"), DefeatedAsurasCount, TotalAsurasSpawned)), 2.5f);
	}
}

void AGanapatiFestivalGameMode::CheckCourtyardProximity()
{
	if (bCourtyardAlertTriggered)
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
				HUD->ShowQuestToast(FText::FromString(TEXT("⚔ Courtyard Skirmish: 2 Corrupted Asura Minions Detected! ⚔")), 3.5f);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("AGanapatiFestivalGameMode: Courtyard Skirmish alert triggered."));
	}
}
