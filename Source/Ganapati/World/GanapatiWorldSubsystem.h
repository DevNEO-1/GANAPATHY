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
 * World regions for open-world spatial tracking and discovery milestones (Phase 6C).
 */
UENUM(BlueprintType)
enum class EWorldRegion : uint8
{
	FestivalStreet     UMETA(DisplayName="Festival Street"),
	CourtyardSanctuary UMETA(DisplayName="Courtyard Sanctuary"),
	SacredPathAscent   UMETA(DisplayName="Sacred Path Ascent"),
	MountainThreshold  UMETA(DisplayName="Mountain Threshold")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWorldRegionChangedSignature, EWorldRegion, PreviousRegion, EWorldRegion, NewRegion);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDivineAscensionDiscoveredSignature, bool, bDiscovered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPilgrimageMilestoneReachedSignature, bool, bReached);

/**
 * Progression states for the overarching festival narrative.
 * Reused project-wide as the authoritative story/world progression enum.
 */
UENUM(BlueprintType)
enum class EStoryProgressionState : uint8
{
	FestivalBeginning UMETA(DisplayName="Festival Beginning"),
	SacredDarshan     UMETA(DisplayName="Sacred Darshan"),
	CourtyardAttack   UMETA(DisplayName="Courtyard Attack"),
	CaptainDefeated   UMETA(DisplayName="Captain Defeated"),
	CourtyardPurified UMETA(DisplayName="Courtyard Purified"),
	SacredJourney     UMETA(DisplayName="Sacred Journey")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStoryProgressionChangedSignature, EStoryProgressionState, PreviousState, EStoryProgressionState, NewState);

/**
 * Session-persistent world gameplay state.
 * Owned authoritatively by UGanapatiWorldSubsystem; persists across World Partition cell streaming.
 */
USTRUCT(BlueprintType)
struct FGanapatiWorldState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|WorldState")
	EStoryProgressionState StoryProgressionState = EStoryProgressionState::FestivalBeginning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|WorldState")
	bool bSacredPathUnlocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|WorldState")
	bool bCourtyardPurified = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|WorldState")
	EWorldRegion ActiveRegion = EWorldRegion::FestivalStreet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|WorldState")
	bool bDivineAscensionDiscovered = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|WorldState")
	bool bMountainThresholdReached = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldStateChangedSignature, const FGanapatiWorldState&, NewWorldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldSacredPathChangedSignature, bool, bUnlocked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldCourtyardPurifiedSignature, bool, bPurified);

/**
 * UGanapatiWorldSubsystem
 *
 * Central runtime actor registry and session world-state owner for "GANAPATI: The Divine Journey".
 * Manages world actors reactively without reliance on global actor scans (GetAllActorsOfClass).
 * Ensures streaming-safe lifecycle handling and authoritative session state persistence across
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

	// ── Session World State APIs ──
	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|WorldState")
	const FGanapatiWorldState& GetWorldState() const { return WorldState; }

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|WorldState")
	void SetWorldState(const FGanapatiWorldState& NewState);

	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|WorldState")
	EStoryProgressionState GetStoryProgressionState() const { return WorldState.StoryProgressionState; }

	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|WorldState")
	EStoryProgressionState GetWorldProgressionState() const { return WorldState.StoryProgressionState; }

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|WorldState")
	void SetStoryProgressionState(EStoryProgressionState NewState);

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|WorldState")
	void SetWorldProgressionState(EStoryProgressionState NewState) { SetStoryProgressionState(NewState); }

	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|WorldState")
	bool IsSacredPathUnlocked() const { return WorldState.bSacredPathUnlocked; }

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|WorldState")
	void SetSacredPathUnlocked(bool bUnlocked);

	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|WorldState")
	bool IsCourtyardPurified() const { return WorldState.bCourtyardPurified; }

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|WorldState")
	void SetCourtyardPurified(bool bPurified);

	// ── Phase 6C: World Region & Milestone APIs ──
	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|WorldState")
	EWorldRegion GetActiveRegion() const { return WorldState.ActiveRegion; }

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|WorldState")
	void SetActiveRegion(EWorldRegion NewRegion);

	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|WorldState")
	bool IsDivineAscensionDiscovered() const { return WorldState.bDivineAscensionDiscovered; }

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|WorldState")
	void SetDivineAscensionDiscovered(bool bDiscovered);

	UFUNCTION(BlueprintPure, Category="Ganapati|Subsystem|WorldState")
	bool IsMountainThresholdReached() const { return WorldState.bMountainThresholdReached; }

	UFUNCTION(BlueprintCallable, Category="Ganapati|Subsystem|WorldState")
	void SetMountainThresholdReached(bool bReached);

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

	// ── World State Delegates ──
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldStateChangedSignature OnWorldStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnStoryProgressionChangedSignature OnStoryProgressionChanged;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldSacredPathChangedSignature OnSacredPathChanged;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldCourtyardPurifiedSignature OnCourtyardPurified;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnWorldRegionChangedSignature OnWorldRegionChanged;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnDivineAscensionDiscoveredSignature OnDivineAscensionDiscovered;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Subsystem|Events")
	FOnPilgrimageMilestoneReachedSignature OnPilgrimageMilestoneReached;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Subsystem|WorldState", meta=(AllowPrivateAccess="true"))
	FGanapatiWorldState WorldState;

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
