// Copyright Ganapati Project. All Rights Reserved.

#include "Enemies/GanapatiAsuraCaptain.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
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

	// Setup attack telegraph point light
	AttackTelegraphLightComp = CreateDefaultSubobject<UPointLightComponent>(TEXT("AttackTelegraphLight"));
	if (AttackTelegraphLightComp)
	{
		AttackTelegraphLightComp->SetupAttachment(RootComponent);
		AttackTelegraphLightComp->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
		AttackTelegraphLightComp->SetIntensity(0.0f);
		AttackTelegraphLightComp->SetAttenuationRadius(650.0f);
		AttackTelegraphLightComp->SetCastShadows(false);
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

void AGanapatiAsuraCaptain::BeginPlay()
{
	Super::BeginPlay();
	InitialSpawnTransform = GetActorTransform();
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

	if (AttackTelegraphLightComp)
	{
		if (CurrentAttackPattern == ECaptainAttackPattern::HeavyCleave)
		{
			// Amber / fiery orange warning glow
			AttackTelegraphLightComp->SetLightColor(FLinearColor(1.0f, 0.40f, 0.05f));
			AttackTelegraphLightComp->SetIntensity(bIsEnraged ? 8500.0f : 6000.0f);
		}
		else
		{
			// Deep crimson danger glow for Overhead Smash
			AttackTelegraphLightComp->SetLightColor(FLinearColor(1.0f, 0.05f, 0.05f));
			AttackTelegraphLightComp->SetIntensity(bIsEnraged ? 11000.0f : 8500.0f);
		}
	}
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
		// Pattern 1: Heavy Cleave (1.0s normal, 0.82s enraged; 40 damage, 220 reach / 90 radius)
		AttackWindupTime = bIsEnraged ? 0.82f : 1.0f;
		AttackCooldown = bIsEnraged ? 2.8f : 3.5f;
		AttackDamage = 40.0f;
		AttackReach = 220.0f;
		AttackRadius = 90.0f;
		MeleeStopDistance = 190.0f;
		break;

	case ECaptainAttackPattern::OverheadSmash:
		// Pattern 2: Overhead Smash (1.3s normal, 1.05s enraged; 50 damage, 250 reach / 110 radius)
		AttackWindupTime = bIsEnraged ? 1.05f : 1.3f;
		AttackCooldown = bIsEnraged ? 2.8f : 3.5f;
		AttackDamage = 50.0f;
		AttackReach = 250.0f;
		AttackRadius = 110.0f;
		MeleeStopDistance = 210.0f;
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

	TSet<AActor*> DamagedActors;

	if (bHit)
	{
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

	// ── Enraged Overhead Smash: 220cm Ground Shockwave (Phase 5C Subsystem 4) ──
	if (bIsEnraged && CurrentAttackPattern == ECaptainAttackPattern::OverheadSmash)
	{
		const FVector ShockwaveCenter = GetActorLocation() + (GetActorForwardVector() * 140.0f);
		TArray<FHitResult> ShockwaveHits;
		FCollisionShape ShockwaveSphere = FCollisionShape::MakeSphere(220.0f);

		const bool bShockHit = World->SweepMultiByObjectType(
			ShockwaveHits,
			ShockwaveCenter,
			ShockwaveCenter + FVector(0.0f, 0.0f, 10.0f),
			FQuat::Identity,
			ObjectQueryParams,
			ShockwaveSphere,
			QueryParams
		);

		if (bShockHit)
		{
			for (const FHitResult& SwHit : ShockwaveHits)
			{
				AActor* SwActor = SwHit.GetActor();
				if (!SwActor || SwActor == this || DamagedActors.Contains(SwActor))
				{
					continue;
				}
				DamagedActors.Add(SwActor);

				if (SwActor->ActorHasTag(FName(TEXT("Player"))) || Cast<AGanapatiPlayerCharacter>(SwActor))
				{
					if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(SwActor))
					{
						const FVector UpwardImpulse = (FVector::UpVector * 280.0f) + (GetActorForwardVector() * 200.0f);
						Damageable->ApplyDamage(20.0f, this, SwHit.ImpactPoint, UpwardImpulse);

						UE_LOG(LogTemp, Warning, TEXT("AGanapatiAsuraCaptain [%s]: Enraged Ground Shockwave hit %s for 20 damage!"),
							*GetName(), *SwActor->GetName());
					}
				}
			}
		}
	}

	// Turn off telegraph light upon attack execution
	if (AttackTelegraphLightComp)
	{
		AttackTelegraphLightComp->SetIntensity(0.0f);
	}

	// Overhead smash ground shock camera shake on player
	if (CurrentAttackPattern == ECaptainAttackPattern::OverheadSmash)
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (AGanapatiPlayerCharacter* PlayerChar = Cast<AGanapatiPlayerCharacter>(PlayerPawn))
			{
				const float Dist = FVector::Dist(GetActorLocation(), PlayerChar->GetActorLocation());
				if (Dist < 1200.0f)
				{
					const float ShakeScale = FMath::Clamp(1.0f - (Dist / 1200.0f), 0.2f, 1.0f) * 0.85f;
					PlayerChar->PlayCameraShake(PlayerChar->GetHeavyAttackCameraShakeClass(), ShakeScale);
				}
			}
		}
	}
}

void AGanapatiAsuraCaptain::FinishAttack()
{
	if (AttackTelegraphLightComp)
	{
		AttackTelegraphLightComp->SetIntensity(0.0f);
	}

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

	// ── Phase 2: Enrage Transition at <= 50% HP (<= 125 HP) ──
	if (!bIsEnraged && CurrentHP <= (MaxHP * 0.5f) && CurrentHP > 0.0f)
	{
		TriggerEnrage();
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

void AGanapatiAsuraCaptain::TriggerEnrage()
{
	if (bIsEnraged || CurrentState == EAsuraAIState::Dead)
	{
		return;
	}

	bIsEnraged = true;

	// Enraged movement speed 225 cm/s
	ChaseSpeed = 225.0f;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = ChaseSpeed;
	}

	// Update attack pattern timings for enrage (Cleave 0.82s, Smash 1.05s, Cooldown 2.8s)
	ApplyAttackPatternSettings();

	// Enraged camera pulse on player
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (AGanapatiPlayerCharacter* PlayerChar = Cast<AGanapatiPlayerCharacter>(PlayerPawn))
		{
			PlayerChar->PlayCameraShake(PlayerChar->GetHeavyAttackCameraShakeClass(), 1.25f);
		}
	}

	// Dynamic fiery crimson surge on telegraph light
	if (AttackTelegraphLightComp)
	{
		AttackTelegraphLightComp->SetLightColor(FLinearColor(1.0f, 0.02f, 0.02f));
		AttackTelegraphLightComp->SetIntensity(11000.0f);
	}

	// Broadcast Blueprint implementable hook
	BP_OnCaptainEnraged();

	UpdateHealthText();

	UE_LOG(LogTemp, Warning, TEXT("AGanapatiAsuraCaptain [%s]: ENRAGED! Speed: %.0f, attack windups tightened (Cleave 0.82s, Smash 1.05s)."), *GetName(), ChaseSpeed);
}

void AGanapatiAsuraCaptain::ResetBossState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CaptainRecoveryTimerHandle);
		World->GetTimerManager().ClearTimer(AttackWindupTimerHandle);
		World->GetTimerManager().ClearTimer(AttackRecoveryTimerHandle);
		World->GetTimerManager().ClearTimer(StaggerTimerHandle);
		World->GetTimerManager().ClearTimer(HitStopTimerHandle);
	}

	if (AttackTelegraphLightComp)
	{
		AttackTelegraphLightComp->SetIntensity(0.0f);
	}

	bIsRecovering = false;
	bIsEnraged = false;
	CurrentHP = MaxHP; // 250 HP
	CurrentState = EAsuraAIState::Idle;
	ChaseSpeed = 190.0f;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->MaxWalkSpeed = ChaseSpeed;
		MoveComp->StopMovementImmediately();
	}

	// Restore spawn transform
	SetActorTransform(InitialSpawnTransform, false, nullptr, ETeleportType::ResetPhysics);

	// Ensure capsule collision is active
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));
		MeshComp->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -145.0f));
		MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	ApplyAttackPatternSettings();
	UpdateHealthText();

	UE_LOG(LogTemp, Log, TEXT("AGanapatiAsuraCaptain [%s]: Reset to pristine spawn state (250 HP, Dormant)."), *GetName());
}

void AGanapatiAsuraCaptain::HandleDeath()
{
	if (CurrentState == EAsuraAIState::Dead)
	{
		return;
	}

	CurrentState = EAsuraAIState::Dead;
	bIsRecovering = false;

	// Extinguish attack telegraph light
	if (AttackTelegraphLightComp)
	{
		AttackTelegraphLightComp->SetIntensity(0.0f);
	}

	// Dramatic defeat impact camera shake on player
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (AGanapatiPlayerCharacter* PlayerChar = Cast<AGanapatiPlayerCharacter>(PlayerPawn))
		{
			PlayerChar->PlayCameraShake(PlayerChar->GetHeavyAttackCameraShakeClass(), 1.4f);
		}
	}

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

	if (bIsEnraged && CurrentState != EAsuraAIState::Dead)
	{
		StateStr = FString::Printf(TEXT("[ENRAGED] %s"), *StateStr);
		if (CurrentState != EAsuraAIState::Attacking)
		{
			StateColor = FColor(255, 60, 20); // Intense fiery orange-red
		}
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
	const FString DisplayTitle = bIsEnraged ? TEXT("★ ASURA CAPTAIN (ENRAGED) ★") : FString::Printf(TEXT("★ %s ★"), *EnemyDisplayName);

	FloatingHealthText->SetText(FText::FromString(
		FString::Printf(TEXT("%s\n[%s] %.0f/%.0f HP (%.0f%%)\n%s"),
			*DisplayTitle, *BarStr, CurrentHP, MaxHP, HealthPct, *StateStr)
	));

	FloatingHealthText->SetTextRenderColor(StateColor);
}
