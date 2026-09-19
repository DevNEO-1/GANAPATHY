// Copyright Ganapati Project. All Rights Reserved.

#include "Enemies/GanapatiTrainingDummy.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"

AGanapatiTrainingDummy::AGanapatiTrainingDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(45.0f, 95.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->SetGenerateOverlapEvents(true);
	SetRootComponent(CapsuleComponent);

	BasePlate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BasePlate"));
	BasePlate->SetupAttachment(CapsuleComponent);
	BasePlate->SetRelativeLocation(FVector(0.0f, 0.0f, -95.0f));
	BasePlate->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.15f));
	BasePlate->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DummyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMesh"));
	DummyMesh->SetupAttachment(CapsuleComponent);
	DummyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -95.0f));
	DummyMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	DummyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FloatingHealthText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("FloatingHealthText"));
	FloatingHealthText->SetupAttachment(CapsuleComponent);
	FloatingHealthText->SetRelativeLocation(FVector(0.0f, 0.0f, 115.0f));
	FloatingHealthText->SetHorizontalAlignment(EHTA_Center);
	FloatingHealthText->SetVerticalAlignment(EVRTA_TextCenter);
	FloatingHealthText->SetWorldSize(28.0f);
	FloatingHealthText->SetTextRenderColor(FColor(255, 215, 0)); // Golden yellow

	// Load prototype mesh for cylinder body
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(
		TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		DummyMesh->SetStaticMesh(CylinderMeshFinder.Object);
		BasePlate->SetStaticMesh(CylinderMeshFinder.Object);
	}

	Tags.Add(FName(TEXT("Enemy")));
	Tags.Add(FName(TEXT("Target")));
}

void AGanapatiTrainingDummy::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = MaxHP;
	InitialLocation = GetActorLocation();
	InitialRotation = GetActorRotation();

	UpdateHealthText();
}

void AGanapatiTrainingDummy::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	if (bIsDefeated)
	{
		return;
	}

	CurrentHP = FMath::Clamp(CurrentHP - Damage, 0.0f, MaxHP);
	OnHealthChanged.Broadcast(CurrentHP, MaxHP);
	UpdateHealthText();

	// Slight physical knockback nudge on hit
	if (!DamageImpulse.IsNearlyZero())
	{
		FVector Nudge = DamageImpulse.GetSafeNormal() * FMath::Clamp(Damage * 5.0f, 20.0f, 150.0f);
		Nudge.Z = FMath::Max(Nudge.Z, 30.0f);
		SetActorLocation(GetActorLocation() + Nudge * 0.1f, true);
	}

	if (CurrentHP <= 0.0f)
	{
		HandleDeath();
	}
}

void AGanapatiTrainingDummy::HandleDeath()
{
	if (bIsDefeated)
	{
		return;
	}

	bIsDefeated = true;
	OnDummyDestroyed.Broadcast();

	FloatingHealthText->SetText(FText::FromString(TEXT("DEFEATED!\n(Respawning...)")));
	FloatingHealthText->SetTextRenderColor(FColor::Red);

	// Hide dummy temporarily
	DummyMesh->SetVisibility(false);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AGanapatiTrainingDummy::Respawn, RespawnDelay, false);
}

void AGanapatiTrainingDummy::Respawn()
{
	CurrentHP = MaxHP;
	bIsDefeated = false;

	SetActorLocation(InitialLocation);
	SetActorRotation(InitialRotation);

	DummyMesh->SetVisibility(true);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	FloatingHealthText->SetTextRenderColor(FColor(255, 215, 0));
	UpdateHealthText();
}

void AGanapatiTrainingDummy::ApplyHealing(float Healing, AActor* Healer)
{
	if (bIsDefeated)
	{
		return;
	}

	CurrentHP = FMath::Clamp(CurrentHP + Healing, 0.0f, MaxHP);
	UpdateHealthText();
}

void AGanapatiTrainingDummy::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// Training dummy registers incoming strike danger
}

void AGanapatiTrainingDummy::UpdateHealthText()
{
	FString HealthString = FString::Printf(TEXT("[Training Target]\nHP: %.0f / %.0f"), CurrentHP, MaxHP);
	FloatingHealthText->SetText(FText::FromString(HealthString));
}
