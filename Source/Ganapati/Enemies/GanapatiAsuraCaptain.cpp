// Copyright Ganapati Project. All Rights Reserved.

#include "Enemies/GanapatiAsuraCaptain.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AGanapatiAsuraCaptain::AGanapatiAsuraCaptain()
{
	// Explicitly exclude from standard courtyard encounter counter (do NOT count toward 2-Asura requirement)
	bCountsTowardEncounter = false;

	// Scale capsule for mini-boss archetype (radius 65, half-height 145)
	GetCapsuleComponent()->InitCapsuleSize(65.0f, 145.0f);

	// Scale skeletal mesh 1.55x and align base to capsule bottom
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeScale3D(FVector(1.55f, 1.55f, 1.55f));
		MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -145.0f));
	}

	// Adjust 3D overhead text location for towering captain archetype
	if (FloatingHealthText)
	{
		FloatingHealthText->SetRelativeLocation(FVector(0.0f, 0.0f, 175.0f));
		FloatingHealthText->SetWorldSize(30.0f);
	}

	// Movement tuning: imposing, deliberate advance (190 cm/s)
	ChaseSpeed = 190.0f;
	GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 280.0f, 0.0f);

	// Health pool tuning: 250 HP mini-boss
	MaxHP = 250.0f;
	CurrentHP = 250.0f;

	// Knockback resistance: 85% physics impulse reduction against melee hits & Divine Shockwave
	KnockbackResistance = 0.85f;

	// Hit reaction and feel tuning: brief stagger, crisp hit-stop
	StaggerDuration = 0.30f;
	HitStopDuration = 0.08f;

	// AI pacing & range tuning
	AttackCooldown = 3.5f;
	DetectionRadius = 1500.0f;
	MeleeStopDistance = 200.0f;

	// Initial default attack pattern configuration
	CurrentAttackPattern = ECaptainAttackPattern::HeavyCleave;
	ApplyAttackPatternSettings();

	// Identity and encounter tracking
	EnemyDisplayName = TEXT("ASURA CAPTAIN");

	Tags.Add(FName(TEXT("Captain")));
}

void AGanapatiAsuraCaptain::HandleDeath()
{
	Super::HandleDeath();

	// Extended 5-second corpse lifespan for mini-boss defeat presentation
	SetLifeSpan(5.0f);
}

void AGanapatiAsuraCaptain::StartAttack()
{
	SelectNextAttack();
	Super::StartAttack();
}

void AGanapatiAsuraCaptain::SelectNextAttack()
{
	// Alternate attack patterns for dynamic combat variety
	if (CurrentAttackPattern == ECaptainAttackPattern::HeavyCleave)
	{
		CurrentAttackPattern = ECaptainAttackPattern::OverheadSmash;
	}
	else
	{
		CurrentAttackPattern = ECaptainAttackPattern::HeavyCleave;
	}

	ApplyAttackPatternSettings();
}

void AGanapatiAsuraCaptain::ApplyAttackPatternSettings()
{
	switch (CurrentAttackPattern)
	{
	case ECaptainAttackPattern::HeavyCleave:
		// Pattern 1: Heavy Cleave (1.0s telegraph, 40 damage, 220 reach / 90 radius)
		AttackWindupTime = 1.0f;
		AttackDamage = 40.0f;
		AttackReach = 220.0f;
		AttackRadius = 90.0f;
		break;

	case ECaptainAttackPattern::OverheadSmash:
		// Pattern 2: Overhead Smash (1.3s telegraph, 50 damage, 250 reach / 110 radius)
		AttackWindupTime = 1.3f;
		AttackDamage = 50.0f;
		AttackReach = 250.0f;
		AttackRadius = 110.0f;
		break;
	}
}

void AGanapatiAsuraCaptain::UpdateHealthText()
{
	if (!FloatingHealthText)
	{
		return;
	}

	if (CurrentState == EAsuraAIState::Dead)
	{
		FloatingHealthText->SetText(FText::FromString(TEXT("CAPTAIN DEFEATED")));
		FloatingHealthText->SetTextRenderColor(FColor(120, 120, 120));
		return;
	}

	FString StateStr;
	switch (CurrentState)
	{
	case EAsuraAIState::Chasing:
		StateStr = TEXT("ADVANCING");
		break;
	case EAsuraAIState::Attacking:
		StateStr = (CurrentAttackPattern == ECaptainAttackPattern::HeavyCleave) ? TEXT("HEAVY CLEAVE!") : TEXT("OVERHEAD SMASH!");
		break;
	case EAsuraAIState::Staggered:
		StateStr = TEXT("STAGGERED");
		break;
	default:
		StateStr = TEXT("ENGAGED");
		break;
	}

	// Generate 12-segment ASCII health gauge bar [============]
	const float HealthFrac = FMath::Clamp(MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f, 0.0f, 1.0f);
	const int32 TotalBars = 12;
	const int32 FilledBars = FMath::Clamp(FMath::RoundToInt(HealthFrac * TotalBars), 0, TotalBars);

	FString BarStr;
	for (int32 i = 0; i < FilledBars; ++i)
	{
		BarStr.AppendChar(TEXT('='));
	}
	for (int32 i = FilledBars; i < TotalBars; ++i)
	{
		BarStr.AppendChar(TEXT('-'));
	}

	FloatingHealthText->SetText(FText::FromString(
		FString::Printf(TEXT("%s\n[%s] %.0f/%.0f\n[%s]"), *EnemyDisplayName, *BarStr, CurrentHP, MaxHP, *StateStr)
	));

	if (CurrentState == EAsuraAIState::Attacking)
	{
		FloatingHealthText->SetTextRenderColor(FColor(255, 20, 20));
	}
	else if (CurrentState == EAsuraAIState::Staggered)
	{
		FloatingHealthText->SetTextRenderColor(FColor(255, 140, 30));
	}
	else
	{
		FloatingHealthText->SetTextRenderColor(FColor(255, 60, 60));
	}
}
