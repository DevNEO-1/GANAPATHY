// Copyright Ganapati Project. All Rights Reserved.

#include "World/GanapatiWorldSubsystem.h"
#include "NPCs/GanapatiNPC.h"
#include "Interaction/GanapatiInteractable.h"
#include "Enemies/GanapatiTrainingDummy.h"
#include "Enemies/GanapatiAsuraMinion.h"
#include "Enemies/GanapatiAsuraCaptain.h"
#include "Environment/FestivalStreetBuilder.h"
#include "Engine/Engine.h"

UGanapatiWorldSubsystem::UGanapatiWorldSubsystem()
{
}

UGanapatiWorldSubsystem* UGanapatiWorldSubsystem::GetGanapatiWorldSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	if (const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
	{
		return World->GetSubsystem<UGanapatiWorldSubsystem>();
	}

	return nullptr;
}

void UGanapatiWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Initialized runtime actor registry."));
}

void UGanapatiWorldSubsystem::Deinitialize()
{
	RegisteredNPCs.Empty();
	RegisteredInteractables.Empty();
	RegisteredTrainingDummies.Empty();
	RegisteredAsuras.Empty();
	RegisteredCaptain.Reset();
	RegisteredStreetBuilder.Reset();

	Super::Deinitialize();
	UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Deinitialized runtime actor registry."));
}

void UGanapatiWorldSubsystem::RegisterNPC(AGanapatiNPC* InNPC)
{
	if (!IsValid(InNPC))
	{
		return;
	}

	if (!RegisteredNPCs.Contains(InNPC))
	{
		RegisteredNPCs.Add(InNPC);
		OnNPCRegistered.Broadcast(InNPC);
		UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Registered NPC [%s] (Total: %d)"),
			*InNPC->GetNPCName(), RegisteredNPCs.Num());
	}
}

void UGanapatiWorldSubsystem::UnregisterNPC(AGanapatiNPC* InNPC)
{
	if (!InNPC)
	{
		return;
	}

	if (RegisteredNPCs.Remove(InNPC) > 0)
	{
		OnNPCUnregistered.Broadcast(InNPC);
		UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Unregistered NPC [%s] (Remaining: %d)"),
			*InNPC->GetNPCName(), RegisteredNPCs.Num());
	}
}

void UGanapatiWorldSubsystem::RegisterInteractable(AGanapatiInteractable* InInteractable)
{
	if (!IsValid(InInteractable))
	{
		return;
	}

	if (!RegisteredInteractables.Contains(InInteractable))
	{
		RegisteredInteractables.Add(InInteractable);
		OnInteractableRegistered.Broadcast(InInteractable);
		UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Registered Interactable [%s] (Total: %d)"),
			*InInteractable->GetName(), RegisteredInteractables.Num());
	}
}

void UGanapatiWorldSubsystem::UnregisterInteractable(AGanapatiInteractable* InInteractable)
{
	if (!InInteractable)
	{
		return;
	}

	if (RegisteredInteractables.Remove(InInteractable) > 0)
	{
		OnInteractableUnregistered.Broadcast(InInteractable);
		UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Unregistered Interactable [%s] (Remaining: %d)"),
			*InInteractable->GetName(), RegisteredInteractables.Num());
	}
}

void UGanapatiWorldSubsystem::RegisterTrainingDummy(AGanapatiTrainingDummy* InDummy)
{
	if (!IsValid(InDummy))
	{
		return;
	}

	if (!RegisteredTrainingDummies.Contains(InDummy))
	{
		RegisteredTrainingDummies.Add(InDummy);
		OnTrainingDummyRegistered.Broadcast(InDummy);
		UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Registered Training Dummy [%s] (Total: %d)"),
			*InDummy->GetName(), RegisteredTrainingDummies.Num());
	}
}

void UGanapatiWorldSubsystem::UnregisterTrainingDummy(AGanapatiTrainingDummy* InDummy)
{
	if (!InDummy)
	{
		return;
	}

	if (RegisteredTrainingDummies.Remove(InDummy) > 0)
	{
		OnTrainingDummyUnregistered.Broadcast(InDummy);
		UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Unregistered Training Dummy [%s] (Remaining: %d)"),
			*InDummy->GetName(), RegisteredTrainingDummies.Num());
	}
}

void UGanapatiWorldSubsystem::RegisterAsuraMinion(AGanapatiAsuraMinion* InAsura)
{
	if (!IsValid(InAsura))
	{
		return;
	}

	if (!RegisteredAsuras.Contains(InAsura))
	{
		RegisteredAsuras.Add(InAsura);
		OnAsuraRegistered.Broadcast(InAsura);
		UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Registered Asura Minion [%s] (Total: %d)"),
			*InAsura->GetName(), RegisteredAsuras.Num());
	}

	// If this minion is an Asura Captain, also register in Captain slot
	if (AGanapatiAsuraCaptain* Captain = Cast<AGanapatiAsuraCaptain>(InAsura))
	{
		RegisterCaptain(Captain);
	}
}

void UGanapatiWorldSubsystem::UnregisterAsuraMinion(AGanapatiAsuraMinion* InAsura)
{
	if (!InAsura)
	{
		return;
	}

	if (AGanapatiAsuraCaptain* Captain = Cast<AGanapatiAsuraCaptain>(InAsura))
	{
		UnregisterCaptain(Captain);
	}

	if (RegisteredAsuras.Remove(InAsura) > 0)
	{
		OnAsuraUnregistered.Broadcast(InAsura);
		UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Unregistered Asura Minion [%s] (Remaining: %d)"),
			*InAsura->GetName(), RegisteredAsuras.Num());
	}
}

void UGanapatiWorldSubsystem::RegisterCaptain(AGanapatiAsuraCaptain* InCaptain)
{
	if (!IsValid(InCaptain))
	{
		return;
	}

	if (RegisteredCaptain != InCaptain)
	{
		RegisteredCaptain = InCaptain;
		OnCaptainRegistered.Broadcast(InCaptain);
		UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Registered Asura Captain [%s]"), *InCaptain->GetName());
	}
}

void UGanapatiWorldSubsystem::UnregisterCaptain(AGanapatiAsuraCaptain* InCaptain)
{
	if (!InCaptain)
	{
		return;
	}

	if (RegisteredCaptain == InCaptain)
	{
		RegisteredCaptain.Reset();
		OnCaptainUnregistered.Broadcast(InCaptain);
		UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Unregistered Asura Captain [%s]"), *InCaptain->GetName());
	}
}

void UGanapatiWorldSubsystem::RegisterStreetBuilder(AFestivalStreetBuilder* InStreetBuilder)
{
	if (!IsValid(InStreetBuilder))
	{
		return;
	}

	if (RegisteredStreetBuilder != InStreetBuilder)
	{
		RegisteredStreetBuilder = InStreetBuilder;
		OnStreetBuilderRegistered.Broadcast(InStreetBuilder);
		UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Registered FestivalStreetBuilder [%s]"), *InStreetBuilder->GetName());
	}
}

void UGanapatiWorldSubsystem::UnregisterStreetBuilder(AFestivalStreetBuilder* InStreetBuilder)
{
	if (!InStreetBuilder)
	{
		return;
	}

	if (RegisteredStreetBuilder == InStreetBuilder)
	{
		RegisteredStreetBuilder.Reset();
		OnStreetBuilderUnregistered.Broadcast(InStreetBuilder);
		UE_LOG(LogTemp, Log, TEXT("UGanapatiWorldSubsystem: Unregistered FestivalStreetBuilder [%s]"), *InStreetBuilder->GetName());
	}
}

TArray<AGanapatiNPC*> UGanapatiWorldSubsystem::GetRegisteredNPCs() const
{
	TArray<AGanapatiNPC*> Result;
	Result.Reserve(RegisteredNPCs.Num());
	for (const TWeakObjectPtr<AGanapatiNPC>& WeakNPC : RegisteredNPCs)
	{
		if (WeakNPC.IsValid())
		{
			Result.Add(WeakNPC.Get());
		}
	}
	return Result;
}

TArray<AGanapatiInteractable*> UGanapatiWorldSubsystem::GetRegisteredInteractables() const
{
	TArray<AGanapatiInteractable*> Result;
	Result.Reserve(RegisteredInteractables.Num());
	for (const TWeakObjectPtr<AGanapatiInteractable>& WeakInteractable : RegisteredInteractables)
	{
		if (WeakInteractable.IsValid())
		{
			Result.Add(WeakInteractable.Get());
		}
	}
	return Result;
}

TArray<AGanapatiTrainingDummy*> UGanapatiWorldSubsystem::GetRegisteredTrainingDummies() const
{
	TArray<AGanapatiTrainingDummy*> Result;
	Result.Reserve(RegisteredTrainingDummies.Num());
	for (const TWeakObjectPtr<AGanapatiTrainingDummy>& WeakDummy : RegisteredTrainingDummies)
	{
		if (WeakDummy.IsValid())
		{
			Result.Add(WeakDummy.Get());
		}
	}
	return Result;
}

TArray<AGanapatiAsuraMinion*> UGanapatiWorldSubsystem::GetRegisteredAsuras() const
{
	TArray<AGanapatiAsuraMinion*> Result;
	Result.Reserve(RegisteredAsuras.Num());
	for (const TWeakObjectPtr<AGanapatiAsuraMinion>& WeakAsura : RegisteredAsuras)
	{
		if (WeakAsura.IsValid())
		{
			Result.Add(WeakAsura.Get());
		}
	}
	return Result;
}

AGanapatiAsuraCaptain* UGanapatiWorldSubsystem::GetRegisteredCaptain() const
{
	return RegisteredCaptain.Get();
}

AFestivalStreetBuilder* UGanapatiWorldSubsystem::GetRegisteredStreetBuilder() const
{
	return RegisteredStreetBuilder.Get();
}
