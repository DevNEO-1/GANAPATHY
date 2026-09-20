// Copyright Ganapati Project. All Rights Reserved.

#include "Enemies/GanapatiAsuraMinion.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGanapatiAsuraMinion::AGanapatiAsuraMinion()
{
	PrimaryActorTick.bCanEverTick = true;

	// Capsule configuration
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	// Character movement configuration
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 480.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;

	// Load mannequin skeletal mesh for prototype visual
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMeshFinder(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MannyMeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannyMeshFinder.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	// Load combat animation blueprint
	static ConstructorHelpers::FClassFinder<UAnimInstance> CombatAnimBPFinder(
		TEXT("/Game/Variant_Combat/Anims/ABP_Manny_Combat"));
	if (CombatAnimBPFinder.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(CombatAnimBPFinder.Class);
	}

	// 3D floating health and status text
	FloatingHealthText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("FloatingHealthText"));
	FloatingHealthText->SetupAttachment(RootComponent);
	FloatingHealthText->SetRelativeLocation(FVector(0.0f, 0.0f, 115.0f));
	FloatingHealthText->SetHorizontalAlignment(EHTA_Center);
	FloatingHealthText->SetVerticalAlignment(EVRTA_TextCenter);
	FloatingHealthText->SetWorldSize(24.0f);
	FloatingHealthText->SetTextRenderColor(FColor(255, 60, 60)); // Hostile red

	// Hostile enemy identification tags
	Tags.Add(FName(TEXT("Enemy")));
	Tags.Add(FName(TEXT("Asura")));
	Tags.Add(FName(TEXT("Target")));
}

void AGanapatiAsuraMinion::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = MaxHP;
	SpawnLocation = GetActorLocation();
	CurrentState = EAsuraAIState::Idle;

	UpdateHealthText();
}

void AGanapatiAsuraMinion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EAsuraAIState::Dead)
	{
		return;
	}

	UpdateAI(DeltaTime);
}

void AGanapatiAsuraMinion::UpdateAI(float DeltaTime)
{
	// Acquire player target if not already cached
	if (!TargetPlayer.IsValid())
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (AGanapatiPlayerCharacter* PlayerChar = Cast<AGanapatiPlayerCharacter>(PlayerPawn))
		{
			TargetPlayer = PlayerChar;
		}
	}

	if (!TargetPlayer.IsValid() || TargetPlayer->IsDead())
	{
		CurrentState = EAsuraAIState::Idle;
		UpdateHealthText();
		return;
	}

	const FVector MyLoc = GetActorLocation();
	const FVector PlayerLoc = TargetPlayer->GetActorLocation();
	const float DistToPlayer = FVector::Dist2D(MyLoc, PlayerLoc);

	// If currently executing an attack or staggered, do not override state
	if (CurrentState == EAsuraAIState::Attacking || CurrentState == EAsuraAIState::Staggered)
	{
		return;
	}

	// 1. Detection Check
	if (DistToPlayer <= DetectionRadius)
	{
		// 2. Melee Range Check
		if (DistToPlayer <= MeleeStopDistance)
		{
			// Halt and face player
			FVector Direction = (PlayerLoc - MyLoc).GetSafeNormal2D();
			if (!Direction.IsNearlyZero())
			{
				SetActorRotation(Direction.Rotation());
			}

			const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
			if (CurrentTime - LastAttackTime >= AttackCooldown)
			{
				StartAttack();
			}
			else
			{
				CurrentState = EAsuraAIState::Idle;
			}
		}
		else
		{
			// 3. Chase Player
			CurrentState = EAsuraAIState::Chasing;
			FVector MoveDir = (PlayerLoc - MyLoc).GetSafeNormal2D();
			AddMovementInput(MoveDir, 1.0f);
		}
	}
	else
	{
		CurrentState = EAsuraAIState::Idle;
	}

	UpdateHealthText();
}

void AGanapatiAsuraMinion::StartAttack()
{
	CurrentState = EAsuraAIState::Attacking;
	LastAttackTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
	}

	UpdateHealthText();

	// Broadcast attack telegraph hook for visual/audio tell
	BP_OnAsuraAttackTelegraphed();

	// Schedule physical hit check at the peak of the swing
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AttackWindupTimerHandle,
			this,
			&AGanapatiAsuraMinion::PerformAttackHitCheck,
			AttackWindupTime,
			false
		);
	}
}

void AGanapatiAsuraMinion::PerformAttackHitCheck()
{
	if (CurrentState != EAsuraAIState::Attacking)
	{
		return;
	}

	DoAttackTrace(NAME_None);

	// Schedule attack recovery
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AttackRecoveryTimerHandle,
			this,
			&AGanapatiAsuraMinion::FinishAttack,
			0.4f,
			false
		);
	}
}

void AGanapatiAsuraMinion::DoAttackTrace(FName DamageSourceBone)
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

				// Only deal damage if the hit actor is the player or damageable target with Player tag
				if (HitActor->ActorHasTag(FName(TEXT("Player"))) || Cast<AGanapatiPlayerCharacter>(HitActor))
				{
					if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor))
					{
						const FVector HitDir = (HitActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
						const FVector Impulse = (HitDir * 400.0f) + (FVector::UpVector * 200.0f);

						Damageable->ApplyDamage(AttackDamage, this, Hit.ImpactPoint, Impulse);

						UE_LOG(LogTemp, Warning, TEXT("AGanapatiAsuraMinion [%s]: Dealt %f damage to %s!"),
							*GetName(), AttackDamage, *HitActor->GetName());
					}
				}
			}
	}
}

void AGanapatiAsuraMinion::FinishAttack()
{
	if (CurrentState == EAsuraAIState::Attacking)
	{
		CurrentState = EAsuraAIState::Idle;
		UpdateHealthText();
	}
}

void AGanapatiAsuraMinion::ResetHitStop()
{
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->GlobalAnimRateScale = 1.0f;
	}
}

void AGanapatiAsuraMinion::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	if (CurrentState == EAsuraAIState::Dead)
	{
		return;
	}

	CurrentHP = FMath::Clamp(CurrentHP - Damage, 0.0f, MaxHP);
	OnHealthChanged.Broadcast(CurrentHP, MaxHP);

	// Physics impulse feedback on hit
	if (!DamageImpulse.IsNearlyZero())
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->AddImpulse(DamageImpulse, true);
		}
	}

	// Hit-stop animation freeze on skeletal mesh
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->GlobalAnimRateScale = 0.0f;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(HitStopTimerHandle);
			World->GetTimerManager().SetTimer(
				HitStopTimerHandle,
				this,
				&AGanapatiAsuraMinion::ResetHitStop,
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
	}
	else
	{
		// Enter stagger state
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
				&AGanapatiAsuraMinion::RecoverFromStagger,
				StaggerDuration,
				false
			);
		}
	}
}

void AGanapatiAsuraMinion::RecoverFromStagger()
{
	if (CurrentState == EAsuraAIState::Staggered)
	{
		CurrentState = EAsuraAIState::Idle;
		UpdateHealthText();
	}
}

void AGanapatiAsuraMinion::HandleDeath()
{
	if (CurrentState == EAsuraAIState::Dead)
	{
		return;
	}

	CurrentState = EAsuraAIState::Dead;

	// Clear active timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackWindupTimerHandle);
		World->GetTimerManager().ClearTimer(AttackRecoveryTimerHandle);
		World->GetTimerManager().ClearTimer(StaggerTimerHandle);
		World->GetTimerManager().ClearTimer(HitStopTimerHandle);
	}

	// Disable movement
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
		MoveComp->StopMovementImmediately();
	}

	// Disable root capsule collisions so it doesn't block player or interfere with ragdoll
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Activate ragdoll physics on skeletal mesh
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->GlobalAnimRateScale = 1.0f;
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComp->SetSimulatePhysics(true);
		MeshComp->AddImpulse(FVector::UpVector * 250.0f, NAME_None, true);
	}

	FloatingHealthText->SetText(FText::FromString(TEXT("DEFEATED")));
	FloatingHealthText->SetTextRenderColor(FColor(120, 120, 120));

	BP_OnAsuraDeathSequence(FVector::UpVector * 250.0f);
	OnAsuraDied.Broadcast(this);

	UE_LOG(LogTemp, Warning, TEXT("AGanapatiAsuraMinion [%s]: Defeated and banished!"), *GetName());

	// Clean destruction after brief fade delay
	SetLifeSpan(3.5f);
}

void AGanapatiAsuraMinion::ApplyHealing(float Healing, AActor* Healer)
{
	if (CurrentState == EAsuraAIState::Dead || Healing <= 0.0f)
	{
		return;
	}

	CurrentHP = FMath::Clamp(CurrentHP + Healing, 0.0f, MaxHP);
	OnHealthChanged.Broadcast(CurrentHP, MaxHP);
	UpdateHealthText();
}

void AGanapatiAsuraMinion::UpdateHealthText()
{
	if (!FloatingHealthText)
	{
		return;
	}

	if (CurrentState == EAsuraAIState::Dead)
	{
		FloatingHealthText->SetText(FText::FromString(TEXT("DEFEATED")));
		FloatingHealthText->SetTextRenderColor(FColor(120, 120, 120));
		return;
	}

	FString StateStr;
	switch (CurrentState)
	{
	case EAsuraAIState::Chasing:
		StateStr = TEXT("CHASING");
		break;
	case EAsuraAIState::Attacking:
		StateStr = TEXT("ATTACKING!");
		break;
	case EAsuraAIState::Staggered:
		StateStr = TEXT("STAGGERED");
		break;
	default:
		StateStr = TEXT("ALERT");
		break;
	}

	// Generate ASCII health gauge bar [==========]
	const float HealthFrac = FMath::Clamp(MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f, 0.0f, 1.0f);
	const int32 TotalBars = 10;
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
		FString::Printf(TEXT("ASURA MINION\n[%s] %.0f/%.0f\n[%s]"), *BarStr, CurrentHP, MaxHP, *StateStr)
	));

	if (CurrentState == EAsuraAIState::Attacking)
	{
		FloatingHealthText->SetTextRenderColor(FColor(255, 30, 30));
	}
	else if (CurrentState == EAsuraAIState::Staggered)
	{
		FloatingHealthText->SetTextRenderColor(FColor(255, 160, 40));
	}
	else
	{
		FloatingHealthText->SetTextRenderColor(FColor(230, 50, 50));
	}
}
