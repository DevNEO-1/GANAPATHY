// Copyright Ganapati Project. All Rights Reserved.

#include "Enemies/GanapatiAsuraCaptain.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

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

	// Hit reaction and feel tuning: brief stagger, crisp hit-stop, stagger cooldown
	StaggerDuration = 0.30f;
	HitStopDuration = 0.08f;
	StaggerCooldown = 4.0f;
	PostAttackRecoveryDuration = 0.65f;

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

void AGanapatiAsuraCaptain::UpdateAI(float DeltaTime)
{
	// If currently paused in post-attack vulnerability window, maintain halt
	if (bIsRecovering)
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}
		return;
	}

	Super::UpdateAI(DeltaTime);
}

void AGanapatiAsuraCaptain::StartAttack()
{
	SelectAttackForDistance();
	Super::StartAttack();
}

void AGanapatiAsuraCaptain::SelectAttackForDistance()
{
	float DistToTarget = 150.0f;
	if (TargetPlayer.IsValid())
	{
		DistToTarget = FVector::Dist2D(GetActorLocation(), TargetPlayer->GetActorLocation());
	}

	// Distance-aware selection:
	// If target is farther away (> 180 cm), deploy the longer-reach Overhead Smash (reach 250)
	// If target is close (<= 140 cm), deploy Heavy Cleave (reach 220, faster 1.0s windup)
	// In mid-range (140 to 180 cm), alternate patterns to maintain combat unpredictability
	if (DistToTarget > 180.0f)
	{
		CurrentAttackPattern = ECaptainAttackPattern::OverheadSmash;
	}
	else if (DistToTarget <= 140.0f)
	{
		CurrentAttackPattern = ECaptainAttackPattern::HeavyCleave;
	}
	else
	{
		if (CurrentAttackPattern == ECaptainAttackPattern::HeavyCleave)
		{
			CurrentAttackPattern = ECaptainAttackPattern::OverheadSmash;
		}
		else
		{
			CurrentAttackPattern = ECaptainAttackPattern::HeavyCleave;
		}
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

void AGanapatiAsuraCaptain::DoAttackTrace(FName DamageSourceBone)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector TraceStart = GetActorLocation() + (GetActorForwardVector() * 30.0f);
	const FVector TraceEnd = TraceStart + (GetActorForwardVector() * AttackReach);

	TArray<FHitResult> HitResults;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(AttackRadius);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = World->SweepMultiByObjectType(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ObjectQueryParams,
		SphereShape,
		QueryParams
	);

	if (bHit)
	{
		TSet<AActor*> DamagedActors;

		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActor == this || DamagedActors.Contains(HitActor))
			{
				continue;
			}

			DamagedActors.Add(HitActor);

			// Deal damage to player with pattern-specific physical impact impulses
			if (HitActor->ActorHasTag(FName(TEXT("Player"))) || Cast<AGanapatiPlayerCharacter>(HitActor))
			{
				if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor))
				{
					FVector Impulse = FVector::ZeroVector;

					if (CurrentAttackPattern == ECaptainAttackPattern::HeavyCleave)
					{
						// Heavy Cleave: Lateral sweep impulse 850 cm/s + upward 180 cm/s
						const FVector RightDir = GetActorRightVector();
						const FVector ForwardDir = GetActorForwardVector();
						const FVector SweepDir = (RightDir * 0.7f + ForwardDir * 0.3f).GetSafeNormal();
						Impulse = (SweepDir * 850.0f) + (FVector::UpVector * 180.0f);
					}
					else
					{
						// Overhead Smash: Forward impulse 500 cm/s + grounded/downward component
						const FVector ForwardDir = GetActorForwardVector();
						Impulse = (ForwardDir * 500.0f) + (FVector::DownVector * 120.0f);
					}

					Damageable->ApplyDamage(AttackDamage, this, Hit.ImpactPoint, Impulse);

					UE_LOG(LogTemp, Warning, TEXT("AGanapatiAsuraCaptain [%s]: Dealt %f damage via %s to %s!"),
						*GetName(), AttackDamage,
						(CurrentAttackPattern == ECaptainAttackPattern::HeavyCleave ? TEXT("HeavyCleave") : TEXT("OverheadSmash")),
						*HitActor->GetName());
				}
			}
		}
	}
}

void AGanapatiAsuraCaptain::FinishAttack()
{
	if (CurrentState == EAsuraAIState::Attacking)
	{
		// Enter explicit 0.65s post-attack vulnerability / recovery window
		bIsRecovering = true;
		CurrentState = EAsuraAIState::Idle;

		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}

		UpdateHealthText();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CaptainRecoveryTimerHandle);
			World->GetTimerManager().SetTimer(
				CaptainRecoveryTimerHandle,
				this,
				&AGanapatiAsuraCaptain::CompleteRecovery,
				PostAttackRecoveryDuration,
				false
			);
		}
	}
}

void AGanapatiAsuraCaptain::CompleteRecovery()
{
	bIsRecovering = false;

	// Stamp LastAttackTime to current time so the full 3.5s cooldown begins AFTER recovery
	LastAttackTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	UpdateHealthText();
}

void AGanapatiAsuraCaptain::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	if (CurrentState == EAsuraAIState::Dead)
	{
		return;
	}

	CurrentHP = FMath::Clamp(CurrentHP - Damage, 0.0f, MaxHP);
	OnHealthChanged.Broadcast(CurrentHP, MaxHP);

	// Physics impulse feedback scaled by 85% knockback resistance (only 15% applied)
	if (!DamageImpulse.IsNearlyZero())
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			const float ImpulseScale = FMath::Clamp(1.0f - KnockbackResistance, 0.0f, 1.0f);
			MoveComp->AddImpulse(DamageImpulse * ImpulseScale, true);
		}
	}

	// Controlled hit-stop animation freeze on skeletal mesh (0.08s)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->GlobalAnimRateScale = 0.0f;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(HitStopTimerHandle);
			World->GetTimerManager().SetTimer(
				HitStopTimerHandle,
				this,
				&AGanapatiAsuraCaptain::ResetHitStop,
				HitStopDuration,
				false
			);
		}
	}

	// Trigger hit reaction blueprint hook
	BP_OnAsuraHitReact(Damage, DamageLocation, DamageImpulse.GetSafeNormal());

	if (CurrentHP <= 0.0f)
	{
		HandleDeath();
		return;
	}

	// ── Hyper-Armor during Attack Windup ──
	// If the Captain is actively executing an attack, light hits must NOT cancel the attack!
	if (CurrentState == EAsuraAIState::Attacking)
	{
		UpdateHealthText();
		return;
	}

	// ── Stagger Cooldown & Poise (Prevents Stun-Locking) ──
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bCanStagger = (CurrentTime - LastStaggerTime >= StaggerCooldown);

	if (bCanStagger)
	{
		LastStaggerTime = CurrentTime;

		// Cancel recovery window if staggered
		if (bIsRecovering)
		{
			bIsRecovering = false;
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(CaptainRecoveryTimerHandle);
			}
		}

		CurrentState = EAsuraAIState::Staggered;
		UpdateHealthText();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AttackWindupTimerHandle);
			World->GetTimerManager().ClearTimer(AttackRecoveryTimerHandle);
			World->GetTimerManager().ClearTimer(StaggerTimerHandle);

			World->GetTimerManager().SetTimer(
				StaggerTimerHandle,
				this,
				&AGanapatiAsuraCaptain::RecoverFromStagger,
				StaggerDuration,
				false
			);
		}
	}
	else
	{
		// Stagger on cooldown: update health display without dropping current action
		UpdateHealthText();
	}
}

void AGanapatiAsuraCaptain::HandleDeath()
{
	if (CurrentState == EAsuraAIState::Dead)
	{
		return;
	}

	CurrentState = EAsuraAIState::Dead;
	bIsRecovering = false;

	// Clear active timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackWindupTimerHandle);
		World->GetTimerManager().ClearTimer(AttackRecoveryTimerHandle);
		World->GetTimerManager().ClearTimer(StaggerTimerHandle);
		World->GetTimerManager().ClearTimer(HitStopTimerHandle);
		World->GetTimerManager().ClearTimer(CaptainRecoveryTimerHandle);
	}

	// Disable movement before physics simulation
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
		MoveComp->StopMovementImmediately();
	}

	// Disable root capsule collisions
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Activate ragdoll physics with backward/upward collapse impulse
	const FVector CollapseImpulse = (GetActorForwardVector() * -320.0f) + (FVector::UpVector * 260.0f);
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->GlobalAnimRateScale = 1.0f;
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComp->SetSimulatePhysics(true);
		MeshComp->AddImpulse(CollapseImpulse, NAME_None, true);
	}

	UpdateHealthText();

	BP_OnAsuraDeathSequence(CollapseImpulse);
	OnAsuraDied.Broadcast(this);

	UE_LOG(LogTemp, Warning, TEXT("AGanapatiAsuraCaptain [%s]: Mini-boss banished from the festival courtyard!"), *GetName());

	// Extended 5-second corpse lifespan for mini-boss defeat presentation
	SetLifeSpan(5.0f);
}

void AGanapatiAsuraCaptain::UpdateHealthText()
{
	if (!FloatingHealthText)
	{
		return;
	}

	if (CurrentState == EAsuraAIState::Dead)
	{
		FloatingHealthText->SetText(FText::FromString(
			TEXT("★ ASURA CAPTAIN ★\n[----------------] BANISHED\n[FALLEN COMMANDER]")
		));
		FloatingHealthText->SetTextRenderColor(FColor(120, 120, 120));
		return;
	}

	FString StateStr;
	FColor StateColor = FColor(230, 50, 50);

	if (CurrentState == EAsuraAIState::Attacking)
	{
		if (CurrentAttackPattern == ECaptainAttackPattern::HeavyCleave)
		{
			StateStr = TEXT(">>> ! HEAVY CLEAVE ! <<<");
			StateColor = FColor(255, 60, 20); // Flame orange-red
		}
		else
		{
			StateStr = TEXT(">>> !!! OVERHEAD SMASH !!! <<<");
			StateColor = FColor(255, 20, 20); // Intense danger red
		}
	}
	else if (CurrentState == EAsuraAIState::Staggered)
	{
		StateStr = TEXT("[STAGGER BREAK]");
		StateColor = FColor(255, 140, 30); // Amber
	}
	else if (bIsRecovering)
	{
		StateStr = TEXT("[VULNERABLE - RECOVERY]");
		StateColor = FColor(255, 215, 0); // Gold / yellow opening
	}
	else if (CurrentState == EAsuraAIState::Chasing)
	{
		StateStr = TEXT("[ADVANCING]");
		StateColor = FColor(230, 50, 50); // Aggressive red
	}
	else
	{
		StateStr = TEXT("[ENGAGED]");
		StateColor = FColor(210, 60, 60);
	}

	// 16-segment ASCII health gauge bar [================]
	const float HealthFrac = FMath::Clamp(MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f, 0.0f, 1.0f);
	const int32 TotalBars = 16;
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

	const float HealthPct = HealthFrac * 100.0f;

	FloatingHealthText->SetText(FText::FromString(
		FString::Printf(TEXT("★ %s ★\n[%s] %.0f/%.0f HP (%.0f%%)\n%s"),
			*EnemyDisplayName, *BarStr, CurrentHP, MaxHP, HealthPct, *StateStr)
	));

	FloatingHealthText->SetTextRenderColor(StateColor);
}
