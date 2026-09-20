// Copyright Ganapati Project. All Rights Reserved.

#include "GameModes/GanapatiFestivalGameMode.h"
#include "Environment/FestivalStreetBuilder.h"
#include "UI/GanapatiGameHUD.h"
#include "Camera/CameraActor.h"
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
