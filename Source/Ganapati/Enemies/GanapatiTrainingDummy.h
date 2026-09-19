// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatDamageable.h"
#include "GanapatiTrainingDummy.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDummyHealthChangedSignature, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDummyDestroyedSignature);

/**
 * AGanapatiTrainingDummy
 *
 * Concrete combat training dummy implementing ICombatDamageable.
 * Places in the combat courtyard of the festival street.
 * Reacts to light combo hits, heavy charged strikes, and knockbacks.
 * Automatically respawns after being defeated.
 */
UCLASS()
class GANAPATI_API AGanapatiTrainingDummy : public AActor, public ICombatDamageable
{
	GENERATED_BODY()

public:
	AGanapatiTrainingDummy();

	// ~begin ICombatDamageable interface
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;
	// ~end ICombatDamageable interface

	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	bool IsDefeated() const { return bIsDefeated; }

public:
	UPROPERTY(BlueprintAssignable, Category="Ganapati|Combat|Events")
	FOnDummyHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Ganapati|Combat|Events")
	FOnDummyDestroyedSignature OnDummyDestroyed;

protected:
	virtual void BeginPlay() override;

	/** Respawns the dummy after death */
	void Respawn();

	/** Updates the 3D overhead floating text showing health */
	void UpdateHealthText();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components")
	TObjectPtr<UStaticMeshComponent> DummyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components")
	TObjectPtr<UStaticMeshComponent> BasePlate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components")
	TObjectPtr<UTextRenderComponent> FloatingHealthText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat", meta=(ClampMin=1.0f))
	float MaxHP = 120.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Combat")
	float CurrentHP = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat", meta=(ClampMin=1.0f, Units="s"))
	float RespawnDelay = 3.5f;

private:
	bool bIsDefeated = false;
	FVector InitialLocation;
	FRotator InitialRotation;
	FTimerHandle RespawnTimerHandle;
};
