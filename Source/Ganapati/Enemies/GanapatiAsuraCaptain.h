// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enemies/GanapatiAsuraMinion.h"
#include "GanapatiAsuraCaptain.generated.h"

/**
 * Combat Attack Patterns for the Asura Captain.
 */
UENUM(BlueprintType)
enum class ECaptainAttackPattern : uint8
{
	HeavyCleave UMETA(DisplayName="Heavy Cleave"),
	OverheadSmash UMETA(DisplayName="Overhead Smash")
};

/**
 * AGanapatiAsuraCaptain
 *
 * First Asura Mini-Boss archetype in the festival courtyard arena.
 * - Inherits directly from AGanapatiAsuraMinion.
 * - Significantly larger and more durable (250 HP, 1.55x visual scale, 65 radius / 145 half-height capsule).
 * - Slower, deliberate pacing (190 cm/s movement speed).
 * - High knockback resistance (85% impulse mitigation against player strikes and Divine Shockwave).
 * - Short stagger recovery (0.30s).
 * - 5-second ragdoll and despawn lifespan on defeat.
 * - Multi-pattern melee attack foundation:
 *     1. Heavy Cleave: 1.0s telegraph, 40 damage, 220 reach / 90 radius.
 *     2. Overhead Smash: 1.3s telegraph, 50 damage, 250 reach / 110 radius.
 * - Distinct 3D in-world health and status display ("ASURA CAPTAIN").
 * - Explicitly isolated from the 2-target courtyard encounter counter (bCountsTowardEncounter = false).
 */
UCLASS()
class GANAPATI_API AGanapatiAsuraCaptain : public AGanapatiAsuraMinion
{
	GENERATED_BODY()

public:
	AGanapatiAsuraCaptain();

	// ~begin ICombatDamageable interface
	virtual void HandleDeath() override;
	// ~end ICombatDamageable interface

	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	ECaptainAttackPattern GetCurrentAttackPattern() const { return CurrentAttackPattern; }

	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat")
	void SelectNextAttack();

protected:
	virtual void StartAttack() override;
	virtual void UpdateHealthText() override;

	void ApplyAttackPatternSettings();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Combat")
	ECaptainAttackPattern CurrentAttackPattern = ECaptainAttackPattern::HeavyCleave;
};
