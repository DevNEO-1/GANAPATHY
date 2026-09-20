// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enemies/GanapatiAsuraMinion.h"
#include "GanapatiAsuraBrute.generated.h"

/**
 * AGanapatiAsuraBrute
 *
 * Heavy hostile enemy archetype in the festival courtyard arena.
 * - Derived from AGanapatiAsuraMinion with heavy brute specialization.
 * - Higher health pool (120 HP).
 * - Slower, deliberate march movement (220 cm/s).
 * - Extended heavy attack telegraph (0.85s windup) dealing heavy damage (35).
 * - Heavy attack cooldown (3.2s).
 * - High knockback resistance (65% reduction against melee & Divine Shockwave impulses).
 * - 1.35x visual and collision scale.
 * - Dynamic health gauge and status overhead display ("ASURA BRUTE").
 * - Ragdoll physics simulation upon defeat.
 * - Does not alter the existing 2-Asura encounter counter or victory condition.
 */
UCLASS()
class GANAPATI_API AGanapatiAsuraBrute : public AGanapatiAsuraMinion
{
	GENERATED_BODY()

public:
	AGanapatiAsuraBrute();
};
