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
 * - Hyper-armor during attack windups (uninterruptible by light hits).
 * - Controlled hit-stop (0.08s) and 0.30s stagger with 4.0s stagger cooldown to prevent stun-locking.
 * - 0.65s post-attack recovery / vulnerability opening window.
 * - Distance-aware multi-pattern melee combat:
 *     1. Heavy Cleave: 1.0s telegraph, 40 damage, 220 reach / 90 radius, sweeping lateral impulse.
 *     2. Overhead Smash: 1.3s telegraph, 50 damage, 250 reach / 110 radius, grounded slam impulse.
 * - Distinct 16-segment 3D in-world health and state display ("★ ASURA CAPTAIN ★").
 * - Backward-tumbling ragdoll defeat sequence with 5-second lifespan.
 * - Explicitly isolated from the 2-target courtyard encounter counter (bCountsTowardEncounter = false).
 */
UCLASS()
class GANAPATI_API AGanapatiAsuraCaptain : public AGanapatiAsuraMinion
{
	GENERATED_BODY()

public:
	AGanapatiAsuraCaptain();

	// ~begin ICombatAttacker interface
	virtual void DoAttackTrace(FName DamageSourceBone) override;
	// ~end ICombatAttacker interface

	// ~begin ICombatDamageable interface
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	// ~end ICombatDamageable interface

	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	ECaptainAttackPattern GetCurrentAttackPattern() const { return CurrentAttackPattern; }

	UFUNCTION(BlueprintPure, Category="Ganapati|Combat")
	bool IsInRecovery() const { return bIsRecovering; }

	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat")
	void SelectAttackForDistance();

protected:
	virtual void UpdateAI(float DeltaTime) override;
	virtual void StartAttack() override;
	virtual void FinishAttack() override;
	virtual void UpdateHealthText() override;

	void ApplyAttackPatternSettings();
	void CompleteRecovery();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Combat")
	ECaptainAttackPattern CurrentAttackPattern = ECaptainAttackPattern::HeavyCleave;

	/** Whether the Captain is currently pausing in post-attack recovery vulnerability */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Combat|Feel")
	bool bIsRecovering = false;

	/** Duration of post-attack vulnerability opening window */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Feel", meta=(ClampMin=0.1f, Units="s"))
	float PostAttackRecoveryDuration = 0.65f;

	/** Cooldown preventing back-to-back staggers and stun-locking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Combat|Feel", meta=(ClampMin=1.0f, Units="s"))
	float StaggerCooldown = 4.0f;

	float LastStaggerTime = -10.0f;

	FTimerHandle CaptainRecoveryTimerHandle;
};
