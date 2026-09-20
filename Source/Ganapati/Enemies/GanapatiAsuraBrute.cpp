// Copyright Ganapati Project. All Rights Reserved.

#include "Enemies/GanapatiAsuraBrute.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AGanapatiAsuraBrute::AGanapatiAsuraBrute()
{
	// Scale capsule for larger brute archetype
	GetCapsuleComponent()->InitCapsuleSize(55.0f, 125.0f);

	// Scale skeletal mesh 1.35x and adjust vertical offset to match capsule bottom
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeScale3D(FVector(1.35f, 1.35f, 1.35f));
		MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -125.0f));
	}

	// Adjust 3D overhead text location for taller character
	if (FloatingHealthText)
	{
		FloatingHealthText->SetRelativeLocation(FVector(0.0f, 0.0f, 155.0f));
		FloatingHealthText->SetWorldSize(28.0f);
	}

	// Movement tuning: slower, heavier advance
	ChaseSpeed = 220.0f;
	GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 320.0f, 0.0f);

	// Health tuning: 120 HP (vs 50 HP for Minion)
	MaxHP = 120.0f;
	CurrentHP = 120.0f;

	// Combat tuning: heavy, punishing strikes with longer telegraph window
	AttackDamage = 35.0f;
	AttackWindupTime = 0.85f;
	AttackCooldown = 3.2f;
	AttackReach = 200.0f;
	AttackRadius = 85.0f;
	MeleeStopDistance = 180.0f;

	// Knockback resistance: 65% reduction to physics impulses (melee hits & Divine Shockwave)
	KnockbackResistance = 0.65f;

	// Hit feel tuning
	HitStopDuration = 0.08f;
	StaggerDuration = 0.40f;

	// Identity and encounter tracking
	EnemyDisplayName = TEXT("ASURA BRUTE");
	bCountsTowardEncounter = false; // Preserves 2-Asura encounter count logic

	Tags.Add(FName(TEXT("Brute")));
}
