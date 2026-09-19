// Copyright Epic Games, Inc. All Rights Reserved.

#include "GanapatiGameMode.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "Controllers/GanapatiMainPlayerController.h"
#include "UI/GanapatiGameHUD.h"
#include "Environment/FestivalStreetBuilder.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AGanapatiGameMode::AGanapatiGameMode()
{
	DefaultPawnClass = AGanapatiPlayerCharacter::StaticClass();
	PlayerControllerClass = AGanapatiMainPlayerController::StaticClass();
	HUDClass = AGanapatiGameHUD::StaticClass();
}

void AGanapatiGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Force player character, controller, and HUD regardless of Blueprint property serialization
	DefaultPawnClass = AGanapatiPlayerCharacter::StaticClass();
	PlayerControllerClass = AGanapatiMainPlayerController::StaticClass();
	HUDClass = AGanapatiGameHUD::StaticClass();
}

UClass* AGanapatiGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	return AGanapatiPlayerCharacter::StaticClass();
}

void AGanapatiGameMode::BeginPlay()
{
	Super::BeginPlay();

	EnsureFestivalEnvironment();
}

void AGanapatiGameMode::EnsureFestivalEnvironment()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

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
