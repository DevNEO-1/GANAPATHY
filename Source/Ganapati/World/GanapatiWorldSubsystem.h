// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GanapatiWorldSubsystem.generated.h"

class AGanapatiNPC;
class AGanapatiInteractable;
class AGanapatiTrainingDummy;
class AGanapatiAsuraMinion;
class AGanapatiAsuraCaptain;
class AFestivalStreetBuilder;

// Multicast dynamic delegate signatures for streaming and registration events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldNPCRegisteredSignature, AGanapatiNPC*, NPC);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldNPCUnregisteredSignature, AGanapatiNPC*, NPC);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldInteractableRegisteredSignature, AGanapatiInteractable*, Interactable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldInteractableUnregisteredSignature, AGanapatiInteractable*, Interactable);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldTrainingDummyRegisteredSignature, AGanapatiTrainingDummy*, Dummy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldTrainingDummyUnregisteredSignature, AGanapatiTrainingDummy*, Dummy);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldAsuraRegisteredSignature, AGanapatiAsuraMinion*, Asura);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldAsuraUnregisteredSignature, AGanapatiAsuraMinion*, Asura);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldCaptainRegisteredSignature, AGanapatiAsuraCaptain*, Captain);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldCaptainUnregisteredSignature, AGanapatiAsuraCaptain*, Captain);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldStreetBuilderRegisteredSignature, AFestivalStreetBuilder*, StreetBuilder);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldStreetBuilderUnregisteredSignature, AFestivalStreetBuilder*, StreetBuilder);

/**
 * UGanapatiWorldSubsystem
 *
 * Central runtime actor registry for "GANAPATI: The Divine Journey".
 * Manages world actors reactively without reliance on global actor scans (GetAllActorsOfClass).
 * Ensures streaming-safe lifecycle handling across World Partition, level streaming,
 * and dynamic spawning.
 */
UCLASS()
class GANAPATI_API UGanapatiWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UGanapatiWorldSubsystem();

	/** Static accessor for obtaining the subsystem from any world context object */
	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem", meta=(WorldContext="WorldContextObject", DisplayName="Get Ganapati World Subsystem"))
	static UGanapatiWorldSubsystem* GetGanapatiWorldSubsystem(const UObject* WorldContextObject);

	// ~begin USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// ~end USubsystem interface

	// ── NPC Registration ──
	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Registration")
	void RegisterNPC(AGanapatiNPC* InNPC);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Registration")
	void UnregisterNPC(AGanapatiNPC* InNPC);

	// ── Interactable Registration ──
	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Registration")
	void RegisterInteractable(AGanapatiInteractable* InInteractable);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Registration")
	void UnregisterInteractable(AGanapatiInteractable* InInteractable);

	// ── Training Dummy Registration ──
	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Registration")
	void RegisterTrainingDummy(AGanapatiTrainingDummy* InDummy);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Registration")
	void UnregisterTrainingDummy(AGanapatiTrainingDummy* InDummy);

	// ── Asura Minion / Brute / Captain Registration ──
	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Registration")
	void RegisterAsuraMinion(AGanapatiAsuraMinion* InAsura);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Registration")
	void UnregisterAsuraMinion(AGanapatiAsuraMinion* InAsura);

	// ── Asura Captain Specific Registration ──
	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Registration")
	void RegisterCaptain(AGanapatiAsuraCaptain* InCaptain);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Registration")
	void UnregisterCaptain(AGanapatiAsuraCaptain* InCaptain);

	// ── Festival Street Builder Registration ──
	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Registration")
	void RegisterStreetBuilder(AFestivalStreetBuilder* InStreetBuilder);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|Registration")
	void UnregisterStreetBuilder(AFestivalStreetBuilder* InStreetBuilder);

	// ── Registry Queries ──
	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|Registry")
	TArray<AGanapatiNPC*> GetRegisteredNPCs() const;

	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|Registry")
	TArray<AGanapatiInteractable*> GetRegisteredInteractables() const;

	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|Registry")
	TArray<AGanapatiTrainingDummy*> GetRegisteredTrainingDummies() const;

	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|Registry")
	TArray<AGanapatiAsuraMinion*> GetRegisteredAsuras() const;

	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|Registry")
	AGanapatiAsuraCaptain* GetRegisteredCaptain() const;

	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|Registry")
	AFestivalStreetBuilder* GetRegisteredStreetBuilder() const;

public:
	// ── Broadcast Delegates ──
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldNPCRegisteredSignature OnNPCRegistered;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldNPCUnregisteredSignature OnNPCUnregistered;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldInteractableRegisteredSignature OnInteractableRegistered;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldInteractableUnregisteredSignature OnInteractableUnregistered;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldTrainingDummyRegisteredSignature OnTrainingDummyRegistered;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldTrainingDummyUnregisteredSignature OnTrainingDummyUnregistered;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldAsuraRegisteredSignature OnAsuraRegistered;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldAsuraUnregisteredSignature OnAsuraUnregistered;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldCaptainRegisteredSignature OnCaptainRegistered;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldCaptainUnregisteredSignature OnCaptainUnregistered;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldStreetBuilderRegisteredSignature OnStreetBuilderRegistered;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldStreetBuilderUnregisteredSignature OnStreetBuilderUnregistered;

private:
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AGanapatiNPC>> RegisteredNPCs;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AGanapatiInteractable>> RegisteredInteractables;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AGanapatiTrainingDummy>> RegisteredTrainingDummies;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AGanapatiAsuraMinion>> RegisteredAsuras;

	UPROPERTY(Transient)
	TWeakObjectPtr<AGanapatiAsuraCaptain> RegisteredCaptain;

	UPROPERTY(Transient)
	TWeakObjectPtr<AFestivalStreetBuilder> RegisteredStreetBuilder;
};
