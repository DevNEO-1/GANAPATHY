// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/GanapatiCombatComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "CombatDamageable.h"
#include "UObject/ConstructorHelpers.h"

UGanapatiCombatComponent::UGanapatiCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Default combo section names matching standard UE character montages
	ComboSectionNames.Add(FName(TEXT("Attack1")));
	ComboSectionNames.Add(FName(TEXT("Attack2")));
	ComboSectionNames.Add(FName(TEXT("Attack3")));

	ChargeLoopSection = FName(TEXT("Charge"));
	ChargeAttackSection = FName(TEXT("Attack"));

	// Auto-load combat animation montages from project assets
	static ConstructorHelpers::FObjectFinder<UAnimMontage> ComboMontageFinder(
		TEXT("/Game/Variant_Combat/Anims/AM_ComboAttack.AM_ComboAttack"));
	if (ComboMontageFinder.Succeeded())
	{
		ComboAttackMontage = ComboMontageFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> ChargedMontageFinder(
		TEXT("/Game/Variant_Combat/Anims/AM_ChargedAttack.AM_ChargedAttack"));
	if (ChargedMontageFinder.Succeeded())
	{
		ChargedAttackMontage = ChargedMontageFinder.Object;
	}
}

void UGanapatiCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// Bind the montage end delegate
	AttackMontageEndedDelegate.BindUObject(this, &UGanapatiCombatComponent::HandleAttackMontageEnded);
}

ACharacter* UGanapatiCombatComponent::GetCharacterOwner() const
{
	return Cast<ACharacter>(GetOwner());
}

USkeletalMeshComponent* UGanapatiCombatComponent::GetOwnerMesh() const
{
	if (ACharacter* CharacterOwner = GetCharacterOwner())
	{
		return CharacterOwner->GetMesh();
	}
	return nullptr;
}

void UGanapatiCombatComponent::StartLightAttack()
{
	ACharacter* CharacterOwner = GetCharacterOwner();
	if (!CharacterOwner || !GetWorld())
	{
		return;
	}

	// If already in an attack animation, buffer the input timestamp for combo evaluation
	if (bIsAttacking)
	{
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();
		return;
	}

	ExecuteComboAttack();
}

void UGanapatiCombatComponent::StopLightAttack()
{
	// Reserved for channeled/hold light attack variations
}

void UGanapatiCombatComponent::StartChargedAttack()
{
	ACharacter* CharacterOwner = GetCharacterOwner();
	if (!CharacterOwner || !GetWorld())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("GANAPATI: StartChargedAttack() — bIsAttacking=%d, ChargedAttackMontage=%s"),
		bIsAttacking, ChargedAttackMontage ? *ChargedAttackMontage->GetName() : TEXT("NULL"));

	bIsChargingAttack = true;
	bHasReleasedChargedAttack = false;
	bHasLoopedChargedAttack = false;

	if (bIsAttacking)
	{
		CachedAttackInputTime = GetWorld()->GetTimeSeconds();
		return;
	}

	ExecuteChargedAttack();
}

void UGanapatiCombatComponent::StopChargedAttack()
{
	bIsChargingAttack = false;
	bHasReleasedChargedAttack = true;

	UE_LOG(LogTemp, Warning, TEXT("GANAPATI: StopChargedAttack() — jumping to section '%s'"), *ChargeAttackSection.ToString());

	// Jump directly to the release heavy strike section if montage is playing
	if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
	{
		if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
		{
			if (ChargedAttackMontage && AnimInstance->Montage_IsPlaying(ChargedAttackMontage))
			{
				AnimInstance->Montage_JumpToSection(ChargeAttackSection, ChargedAttackMontage);
				return;
			}
		}
	}

	// Montage wasn't playing — reset attack state cleanly
	// The AnimNotify_DoAttackTrace in the montage is responsible for the trace
	bIsAttacking = false;
	OnAttackStateChanged.Broadcast(false);
}

void UGanapatiCombatComponent::ExecuteComboAttack()
{
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;

	bIsAttacking = true;
	ComboCount = 0;
	OnAttackStateChanged.Broadcast(true);

	NotifyEnemiesOfIncomingAttack();

	float MontageLength = 0.0f;
	if (AnimInstance && ComboAttackMontage)
	{
		MontageLength = AnimInstance->Montage_Play(ComboAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
		if (MontageLength > 0.0f)
		{
			AnimInstance->Montage_SetEndDelegate(AttackMontageEndedDelegate, ComboAttackMontage);
		}
	}

	// Direct trace fallback: ensures light attack ALWAYS hits and deals damage even without an active montage
	if (MontageLength <= 0.0f)
	{
		DoAttackTrace(WeaponSocketName);

		if (UWorld* World = GetWorld())
		{
			FTimerHandle AttackResetTimer;
			World->GetTimerManager().SetTimer(AttackResetTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				bIsAttacking = false;
				OnAttackStateChanged.Broadcast(false);
			}), 0.35f, false);
		}
		else
		{
			bIsAttacking = false;
			OnAttackStateChanged.Broadcast(false);
		}
	}
}

void UGanapatiCombatComponent::ExecuteChargedAttack()
{
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;

	bIsAttacking = true;
	bHasLoopedChargedAttack = false;
	bHasReleasedChargedAttack = false;
	OnAttackStateChanged.Broadcast(true);

	NotifyEnemiesOfIncomingAttack();

	float MontageLength = 0.0f;
	if (AnimInstance && ChargedAttackMontage)
	{
		MontageLength = AnimInstance->Montage_Play(ChargedAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
		if (MontageLength > 0.0f)
		{
			AnimInstance->Montage_SetEndDelegate(AttackMontageEndedDelegate, ChargedAttackMontage);
		}
	}

	// Direct trace fallback for heavy strike
	if (MontageLength <= 0.0f)
	{
		DoAttackTrace(WeaponSocketName);

		if (UWorld* World = GetWorld())
		{
			FTimerHandle AttackResetTimer;
			World->GetTimerManager().SetTimer(AttackResetTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				bIsAttacking = false;
				bIsChargingAttack = false;
				OnAttackStateChanged.Broadcast(false);
			}), 0.5f, false);
		}
		else
		{
			bIsAttacking = false;
			bIsChargingAttack = false;
			OnAttackStateChanged.Broadcast(false);
		}
	}
}

void UGanapatiCombatComponent::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsAttacking = false;
	OnAttackStateChanged.Broadcast(false);

	if (!GetWorld())
	{
		return;
	}

	// Check if a buffered attack input exists within the tolerance window
	const float TimeSinceInput = GetWorld()->GetTimeSeconds() - CachedAttackInputTime;
	if (CachedAttackInputTime > 0.0f && TimeSinceInput <= AttackInputCacheTimeTolerance)
	{
		CachedAttackInputTime = 0.0f;

		if (bIsChargingAttack)
		{
			ExecuteChargedAttack();
		}
		else
		{
			ExecuteComboAttack();
		}
	}
}

void UGanapatiCombatComponent::DoAttackTrace(FName DamageSourceBone)
{
	ACharacter* CharacterOwner = GetCharacterOwner();
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!CharacterOwner || !Mesh || !GetWorld())
	{
		return;
	}

	// Determine origin: requested bone/socket -> default weapon socket -> actor forward fallback
	FName EffectiveSocket = DamageSourceBone;
	if (EffectiveSocket == NAME_None || !Mesh->DoesSocketExist(EffectiveSocket))
	{
		EffectiveSocket = WeaponSocketName;
	}

	FVector TraceStart = CharacterOwner->GetActorLocation() + (CharacterOwner->GetActorForwardVector() * 40.0f);
	if (EffectiveSocket != NAME_None && Mesh->DoesSocketExist(EffectiveSocket))
	{
		TraceStart = Mesh->GetSocketLocation(EffectiveSocket);
	}

	const FVector TraceEnd = TraceStart + (CharacterOwner->GetActorForwardVector() * MeleeTraceDistance);

	TArray<FHitResult> OutHits;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(MeleeTraceRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CharacterOwner);

	const bool bHit = GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams);

	if (bHit)
	{
		TSet<AActor*> ProcessedActors;

		for (const FHitResult& CurrentHit : OutHits)
		{
			AActor* HitActor = CurrentHit.GetActor();
			if (!HitActor || ProcessedActors.Contains(HitActor))
			{
				continue;
			}

			ProcessedActors.Add(HitActor);

			if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor))
			{
				const float FinalDamage = BaseMeleeDamage * WeaponDamageMultiplier;

				// Calculate directional knockback away from hit impact normal with upwards launch
				const FVector ImpactNormal = CurrentHit.ImpactNormal.IsNearlyZero() ? -CharacterOwner->GetActorForwardVector() : FVector(CurrentHit.ImpactNormal);
				const FVector Impulse = (ImpactNormal * -MeleeKnockbackImpulse) + (FVector::UpVector * MeleeLaunchImpulse);

				Damageable->ApplyDamage(FinalDamage, CharacterOwner, CurrentHit.ImpactPoint, Impulse);
				OnDamageDealt.Broadcast(FinalDamage, CurrentHit.ImpactPoint);
			}
		}
	}
}

void UGanapatiCombatComponent::CheckCombo()
{
	if (!bIsAttacking || bIsChargingAttack || !GetWorld())
	{
		return;
	}

	const float TimeSinceInput = GetWorld()->GetTimeSeconds() - CachedAttackInputTime;
	if (CachedAttackInputTime > 0.0f && TimeSinceInput <= ComboInputCacheTimeTolerance)
	{
		// Consume the buffered input
		CachedAttackInputTime = 0.0f;
		++ComboCount;

		if (ComboCount < ComboSectionNames.Num() && ComboAttackMontage)
		{
			NotifyEnemiesOfIncomingAttack();

			if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
			{
				if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
				{
					AnimInstance->Montage_JumpToSection(ComboSectionNames[ComboCount], ComboAttackMontage);
				}
			}
		}
	}
}

void UGanapatiCombatComponent::CheckChargedAttack()
{
	bHasLoopedChargedAttack = true;
	bHasReleasedChargedAttack = !bIsChargingAttack;

	if (bHasReleasedChargedAttack)
	{
		if (USkeletalMeshComponent* Mesh = GetOwnerMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				if (ChargedAttackMontage)
				{
					AnimInstance->Montage_JumpToSection(ChargeAttackSection, ChargedAttackMontage);
					return;
				}
			}
		}
	}
	else
	{
		LoopOrResolveChargedAttack();
	}
}

void UGanapatiCombatComponent::LoopOrResolveChargedAttack()
{
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!Mesh || !ChargedAttackMontage)
	{
		return;
	}

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	const FName TargetSection = bHasReleasedChargedAttack ? ChargeAttackSection : ChargeLoopSection;
	AnimInstance->Montage_JumpToSection(TargetSection, ChargedAttackMontage);
}

void UGanapatiCombatComponent::NotifyEnemiesOfIncomingAttack()
{
	ACharacter* CharacterOwner = GetCharacterOwner();
	if (!CharacterOwner || !GetWorld())
	{
		return;
	}

	const FVector TraceStart = CharacterOwner->GetActorLocation();
	const FVector TraceEnd = TraceStart + (CharacterOwner->GetActorForwardVector() * DangerTraceDistance);

	TArray<FHitResult> OutHits;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(DangerTraceRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CharacterOwner);

	if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd, FQuat::Identity, ObjectParams, CollisionShape, QueryParams))
	{
		TSet<AActor*> NotifiedActors;

		for (const FHitResult& CurrentHit : OutHits)
		{
			AActor* HitActor = CurrentHit.GetActor();
			if (HitActor && !NotifiedActors.Contains(HitActor))
			{
				NotifiedActors.Add(HitActor);
				if (ICombatDamageable* Damageable = Cast<ICombatDamageable>(HitActor))
				{
					Damageable->NotifyDanger(CharacterOwner->GetActorLocation(), CharacterOwner);
				}
			}
		}
	}
}
