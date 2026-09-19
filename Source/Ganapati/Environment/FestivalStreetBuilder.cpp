// Copyright Ganapati Project. All Rights Reserved.

#include "Environment/FestivalStreetBuilder.h"
#include "Interaction/GanapatiInteractable.h"
#include "NPCs/GanapatiNPC.h"
#include "Enemies/GanapatiTrainingDummy.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

AFestivalStreetBuilder::AFestivalStreetBuilder()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// Load LevelPrototyping assets for construction
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
		TEXT("/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"));
	if (CubeFinder.Succeeded())
	{
		CubeMesh = CubeFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylFinder(
		TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	if (CylFinder.Succeeded())
	{
		CylinderMesh = CylFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RampFinder(
		TEXT("/Game/LevelPrototyping/Meshes/SM_Ramp.SM_Ramp"));
	if (RampFinder.Succeeded())
	{
		RampMesh = RampFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(
		TEXT("/Game/LevelPrototyping/Materials/M_PrototypeGrid.M_PrototypeGrid"));
	if (MatFinder.Succeeded())
	{
		PrototypeGridMaterial = MatFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DarkMatFinder(
		TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark"));
	if (DarkMatFinder.Succeeded())
	{
		DarkMaterial = DarkMatFinder.Object;
	}
}

void AFestivalStreetBuilder::BeginPlay()
{
	Super::BeginPlay();

	if (!bHasConstructed)
	{
		BuildFestivalEnvironment();
		PopulateWorldActors();
		bHasConstructed = true;
	}
}

UStaticMeshComponent* AFestivalStreetBuilder::CreateMeshPiece(
	const FString& PieceName,
	UStaticMesh* Mesh,
	const FVector& RelativeLocation,
	const FRotator& RelativeRotation,
	const FVector& RelativeScale,
	bool bEnableCollision)
{
	if (!Mesh)
	{
		return nullptr;
	}

	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this, *PieceName);
	if (!Comp)
	{
		return nullptr;
	}

	Comp->SetStaticMesh(Mesh);
	Comp->AttachToComponent(RootScene, FAttachmentTransformRules::KeepRelativeTransform);
	Comp->SetRelativeLocation(RelativeLocation);
	Comp->SetRelativeRotation(RelativeRotation);
	Comp->SetRelativeScale3D(RelativeScale);

	if (bEnableCollision)
	{
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (PrototypeGridMaterial)
	{
		Comp->SetMaterial(0, PrototypeGridMaterial);
	}

	Comp->RegisterComponent();
	return Comp;
}

UPointLightComponent* AFestivalStreetBuilder::CreateFestivalLight(
	const FString& LightName,
	const FVector& RelativeLocation,
	const FLinearColor& LightColor,
	float Intensity,
	float AttenuationRadius)
{
	UPointLightComponent* Light = NewObject<UPointLightComponent>(this, *LightName);
	if (!Light)
	{
		return nullptr;
	}

	Light->AttachToComponent(RootScene, FAttachmentTransformRules::KeepRelativeTransform);
	Light->SetRelativeLocation(RelativeLocation);
	Light->SetLightColor(LightColor);
	Light->SetIntensity(Intensity);
	Light->SetAttenuationRadius(AttenuationRadius);
	Light->SetCastShadows(false); // Optimize performance for prototype
	Light->RegisterComponent();

	return Light;
}

void AFestivalStreetBuilder::BuildFestivalEnvironment()
{
	BuildStreetAndBuildings();
	BuildBazaarStalls();
	BuildGaneshPandal();
	BuildFestivalLighting();
	BuildCombatAndParkourCourtyard();
}

void AFestivalStreetBuilder::BuildStreetAndBuildings()
{
	// ── 1. Main Festival Street Road ──
	// Road runs along X from -1500 to +2500, width = 1400 cm (Y: -700 to +700)
	UStaticMeshComponent* Road = CreateMeshPiece(
		TEXT("MainRoad"),
		CubeMesh,
		FVector(500.0f, 0.0f, -20.0f),
		FRotator::ZeroRotator,
		FVector(42.0f, 15.0f, 0.4f)
	);
	if (Road && DarkMaterial)
	{
		Road->SetMaterial(0, DarkMaterial);
	}

	// ── 2. Flanking Buildings (Left Side: Y = -850) ──
	for (int32 i = 0; i < 4; ++i)
	{
		float XPos = -1000.0f + (i * 900.0f);
		float Height = 5.0f + (i % 2) * 2.0f;

		CreateMeshPiece(
			FString::Printf(TEXT("Building_Left_%d"), i),
			CubeMesh,
			FVector(XPos, -1150.0f, Height * 50.0f),
			FRotator::ZeroRotator,
			FVector(7.5f, 6.0f, Height)
		);
	}

	// ── 3. Flanking Buildings (Right Side: Y = +850) ──
	// Leave opening at X = 800 for Combat Courtyard entrance
	for (int32 i = 0; i < 4; ++i)
	{
		if (i == 2) continue; // Gap for courtyard entrance!

		float XPos = -1000.0f + (i * 900.0f);
		float Height = 5.5f + ((i + 1) % 2) * 1.5f;

		CreateMeshPiece(
			FString::Printf(TEXT("Building_Right_%d"), i),
			CubeMesh,
			FVector(XPos, 1150.0f, Height * 50.0f),
			FRotator::ZeroRotator,
			FVector(7.5f, 6.0f, Height)
		);
	}

	// Street Entrance Archway (at X = -1400)
	CreateMeshPiece(TEXT("EntrancePillar_L"), CylinderMesh, FVector(-1400.0f, -600.0f, 250.0f), FRotator::ZeroRotator, FVector(1.0f, 1.0f, 5.0f));
	CreateMeshPiece(TEXT("EntrancePillar_R"), CylinderMesh, FVector(-1400.0f, 600.0f, 250.0f), FRotator::ZeroRotator, FVector(1.0f, 1.0f, 5.0f));
	CreateMeshPiece(TEXT("EntranceBeam"), CubeMesh, FVector(-1400.0f, 0.0f, 520.0f), FRotator::ZeroRotator, FVector(1.2f, 13.0f, 0.8f));
}

void AFestivalStreetBuilder::BuildBazaarStalls()
{
	// Stalls along the street on left and right sides
	const float StallXPositions[] = { -600.0f, 0.0f, 600.0f, 1200.0f };

	for (int32 i = 0; i < 4; ++i)
	{
		float X = StallXPositions[i];

		// Left Stall: Modak / Sweets & Flowers
		CreateMeshPiece(
			FString::Printf(TEXT("Stall_L_%d_Table"), i),
			CubeMesh,
			FVector(X, -620.0f, 50.0f),
			FRotator::ZeroRotator,
			FVector(3.0f, 1.8f, 1.0f)
		);
		// Canopy Roof
		CreateMeshPiece(
			FString::Printf(TEXT("Stall_L_%d_Canopy"), i),
			CubeMesh,
			FVector(X, -620.0f, 240.0f),
			FRotator(10.0f, 0.0f, 0.0f),
			FVector(3.4f, 2.2f, 0.15f)
		);
		// Canopy Poles
		CreateMeshPiece(FString::Printf(TEXT("Stall_L_%d_Pole1"), i), CylinderMesh, FVector(X - 130.0f, -540.0f, 120.0f), FRotator::ZeroRotator, FVector(0.15f, 0.15f, 2.4f), false);
		CreateMeshPiece(FString::Printf(TEXT("Stall_L_%d_Pole2"), i), CylinderMesh, FVector(X + 130.0f, -540.0f, 120.0f), FRotator::ZeroRotator, FVector(0.15f, 0.15f, 2.4f), false);

		// Right Stall: Diya / Garlands (skip index 2 where courtyard entry is)
		if (i != 2)
		{
			CreateMeshPiece(
				FString::Printf(TEXT("Stall_R_%d_Table"), i),
				CubeMesh,
				FVector(X, 620.0f, 50.0f),
				FRotator::ZeroRotator,
				FVector(3.0f, 1.8f, 1.0f)
			);
			CreateMeshPiece(
				FString::Printf(TEXT("Stall_R_%d_Canopy"), i),
				CubeMesh,
				FVector(X, 620.0f, 240.0f),
				FRotator(-10.0f, 0.0f, 0.0f),
				FVector(3.4f, 2.2f, 0.15f)
			);
			CreateMeshPiece(FString::Printf(TEXT("Stall_R_%d_Pole1"), i), CylinderMesh, FVector(X - 130.0f, 540.0f, 120.0f), FRotator::ZeroRotator, FVector(0.15f, 0.15f, 2.4f), false);
			CreateMeshPiece(FString::Printf(TEXT("Stall_R_%d_Pole2"), i), CylinderMesh, FVector(X + 130.0f, 540.0f, 120.0f), FRotator::ZeroRotator, FVector(0.15f, 0.15f, 2.4f), false);
		}
	}
}

void AFestivalStreetBuilder::BuildGaneshPandal()
{
	// The Grand Ganesh Pandal sits at the end of the street (X = 2200, Y = 0)
	const FVector PandalCenter(2200.0f, 0.0f, 0.0f);

	// 1. Grand Raised Dais / Temple Platform
	CreateMeshPiece(
		TEXT("Pandal_Platform"),
		CubeMesh,
		PandalCenter + FVector(0.0f, 0.0f, 40.0f),
		FRotator::ZeroRotator,
		FVector(12.0f, 14.0f, 0.8f)
	);

	// 2. Approach Steps / Ramp
	if (RampMesh)
	{
		CreateMeshPiece(
			TEXT("Pandal_Ramp"),
			RampMesh,
			PandalCenter + FVector(-750.0f, 0.0f, 0.0f),
			FRotator(0.0f, 0.0f, 0.0f),
			FVector(3.0f, 6.0f, 0.8f)
		);
	}

	// 3. Four Grand Temple Pillars
	const float PillarOffsetX = 450.0f;
	const float PillarOffsetY = 550.0f;
	const float PillarHeight = 6.0f; // 600cm high

	CreateMeshPiece(TEXT("Pandal_Pillar_FL"), CylinderMesh, PandalCenter + FVector(-PillarOffsetX, -PillarOffsetY, 340.0f), FRotator::ZeroRotator, FVector(1.4f, 1.4f, PillarHeight));
	CreateMeshPiece(TEXT("Pandal_Pillar_FR"), CylinderMesh, PandalCenter + FVector(-PillarOffsetX, PillarOffsetY, 340.0f), FRotator::ZeroRotator, FVector(1.4f, 1.4f, PillarHeight));
	CreateMeshPiece(TEXT("Pandal_Pillar_BL"), CylinderMesh, PandalCenter + FVector(PillarOffsetX, -PillarOffsetY, 340.0f), FRotator::ZeroRotator, FVector(1.4f, 1.4f, PillarHeight));
	CreateMeshPiece(TEXT("Pandal_Pillar_BR"), CylinderMesh, PandalCenter + FVector(PillarOffsetX, PillarOffsetY, 340.0f), FRotator::ZeroRotator, FVector(1.4f, 1.4f, PillarHeight));

	// 4. Grand Roof / Mandap Canopy
	CreateMeshPiece(
		TEXT("Pandal_Roof"),
		CubeMesh,
		PandalCenter + FVector(0.0f, 0.0f, 660.0f),
		FRotator::ZeroRotator,
		FVector(13.0f, 15.0f, 0.8f)
	);

	// 5. Central Altar / Shrine Pedestal (where Ganesha idol sits)
	CreateMeshPiece(
		TEXT("Ganesh_Altar_Base"),
		CylinderMesh,
		PandalCenter + FVector(150.0f, 0.0f, 110.0f),
		FRotator::ZeroRotator,
		FVector(3.0f, 3.0f, 0.6f)
	);
	CreateMeshPiece(
		TEXT("Ganesh_Altar_Tier2"),
		CylinderMesh,
		PandalCenter + FVector(150.0f, 0.0f, 150.0f),
		FRotator::ZeroRotator,
		FVector(2.2f, 2.2f, 0.4f)
	);

	// Symbolic Divine Ganesha Idol Geometry (Tiered golden idol representation)
	UStaticMeshComponent* IdolBody = CreateMeshPiece(
		TEXT("Ganesh_Idol_Body"),
		CylinderMesh,
		PandalCenter + FVector(150.0f, 0.0f, 230.0f),
		FRotator::ZeroRotator,
		FVector(1.6f, 1.6f, 1.2f)
	);
	UStaticMeshComponent* IdolHead = CreateMeshPiece(
		TEXT("Ganesh_Idol_Head"),
		CylinderMesh,
		PandalCenter + FVector(150.0f, 0.0f, 320.0f),
		FRotator(0.0f, 0.0f, 90.0f),
		FVector(1.1f, 1.1f, 0.9f)
	);
	// Golden Crown (Mukut)
	CreateMeshPiece(
		TEXT("Ganesh_Idol_Crown"),
		CylinderMesh,
		PandalCenter + FVector(150.0f, 0.0f, 390.0f),
		FRotator::ZeroRotator,
		FVector(0.7f, 0.7f, 0.8f)
	);

	// Pandal Back Wall
	CreateMeshPiece(
		TEXT("Pandal_BackWall"),
		CubeMesh,
		PandalCenter + FVector(550.0f, 0.0f, 360.0f),
		FRotator::ZeroRotator,
		FVector(0.8f, 14.0f, 6.0f)
	);
}

void AFestivalStreetBuilder::BuildFestivalLighting()
{
	// ── Warm Saffron & Golden Festival Lights Along Street ──
	const FLinearColor SaffronGold(1.0f, 0.65f, 0.15f);
	const FLinearColor DeepAmber(1.0f, 0.45f, 0.08f);
	const FLinearColor DivineGold(1.0f, 0.85f, 0.35f);

	// Street Lanterns
	for (int32 i = -2; i <= 3; ++i)
	{
		float X = i * 600.0f;

		// Left lantern post & light
		CreateMeshPiece(FString::Printf(TEXT("LanternPost_L_%d"), i), CylinderMesh, FVector(X, -500.0f, 180.0f), FRotator::ZeroRotator, FVector(0.2f, 0.2f, 3.6f), false);
		CreateFestivalLight(FString::Printf(TEXT("Light_L_%d"), i), FVector(X, -500.0f, 370.0f), (i % 2 == 0) ? SaffronGold : DeepAmber, 6000.0f, 1000.0f);

		// Right lantern post & light
		CreateMeshPiece(FString::Printf(TEXT("LanternPost_R_%d"), i), CylinderMesh, FVector(X, 500.0f, 180.0f), FRotator::ZeroRotator, FVector(0.2f, 0.2f, 3.6f), false);
		CreateFestivalLight(FString::Printf(TEXT("Light_R_%d"), i), FVector(X, 500.0f, 370.0f), (i % 2 == 0) ? DeepAmber : SaffronGold, 6000.0f, 1000.0f);
	}

	// ── Divine Illumination Inside Ganesh Pandal ──
	CreateFestivalLight(TEXT("Pandal_Main_Light"), FVector(2200.0f, 0.0f, 400.0f), DivineGold, 12000.0f, 1800.0f);
	CreateFestivalLight(TEXT("Pandal_Altar_Glow"), FVector(2350.0f, 0.0f, 250.0f), SaffronGold, 8000.0f, 900.0f);
	CreateFestivalLight(TEXT("Pandal_Entrance_L"), FVector(1700.0f, -400.0f, 300.0f), DeepAmber, 5000.0f, 800.0f);
	CreateFestivalLight(TEXT("Pandal_Entrance_R"), FVector(1700.0f, 400.0f, 300.0f), DeepAmber, 5000.0f, 800.0f);
}

void AFestivalStreetBuilder::BuildCombatAndParkourCourtyard()
{
	// Courtyard branches off to the right (Y = 1900, X = 800)
	const FVector CourtCenter(800.0f, 2000.0f, 0.0f);

	// Courtyard Floor (large arena)
	CreateMeshPiece(
		TEXT("Court_Floor"),
		CubeMesh,
		CourtCenter + FVector(0.0f, 0.0f, -15.0f),
		FRotator::ZeroRotator,
		FVector(18.0f, 18.0f, 0.3f)
	);

	// Surrounding Walls (great for Wall Jump tests)
	// North Wall
	CreateMeshPiece(TEXT("Court_Wall_N"), CubeMesh, CourtCenter + FVector(900.0f, 0.0f, 250.0f), FRotator::ZeroRotator, FVector(0.6f, 18.0f, 5.0f));
	// South Wall
	CreateMeshPiece(TEXT("Court_Wall_S"), CubeMesh, CourtCenter + FVector(-900.0f, 0.0f, 250.0f), FRotator::ZeroRotator, FVector(0.6f, 18.0f, 5.0f));
	// East Wall
	CreateMeshPiece(TEXT("Court_Wall_E"), CubeMesh, CourtCenter + FVector(0.0f, 900.0f, 250.0f), FRotator::ZeroRotator, FVector(18.0f, 0.6f, 5.0f));

	// Stepped Parkour Platforms for Double-Jump & Anti-Gravity tests
	CreateMeshPiece(TEXT("Parkour_Plat_1"), CubeMesh, CourtCenter + FVector(400.0f, -400.0f, 80.0f), FRotator::ZeroRotator, FVector(3.0f, 3.0f, 1.6f));
	CreateMeshPiece(TEXT("Parkour_Plat_2"), CubeMesh, CourtCenter + FVector(400.0f, 0.0f, 180.0f), FRotator::ZeroRotator, FVector(2.5f, 2.5f, 3.6f));
	CreateMeshPiece(TEXT("Parkour_Plat_3"), CubeMesh, CourtCenter + FVector(400.0f, 400.0f, 300.0f), FRotator::ZeroRotator, FVector(2.0f, 2.0f, 6.0f));

	// High Beam for Leaping
	CreateMeshPiece(TEXT("Parkour_HighBeam"), CubeMesh, CourtCenter + FVector(-300.0f, 300.0f, 380.0f), FRotator::ZeroRotator, FVector(6.0f, 1.2f, 0.4f));

	// Courtyard Lighting
	CreateFestivalLight(TEXT("Court_Light_Center"), CourtCenter + FVector(0.0f, 0.0f, 300.0f), FLinearColor(1.0f, 0.7f, 0.3f), 8000.0f, 1500.0f);
}

void AFestivalStreetBuilder::PopulateWorldActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector ActorOrigin = GetActorLocation();

	// ── 1. Spawn Ganesh Pandal Shrine Interactable ──
	// At the altar in front of the idol
	FVector ShrineLocation = ActorOrigin + FVector(2250.0f, 0.0f, 90.0f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGanapatiInteractable* Shrine = World->SpawnActor<AGanapatiInteractable>(
		AGanapatiInteractable::StaticClass(),
		ShrineLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	// ── 2. Spawn Training Dummy in Combat Courtyard ──
	FVector DummyLocation = ActorOrigin + FVector(800.0f, 1900.0f, 50.0f);
	World->SpawnActor<AGanapatiTrainingDummy>(
		AGanapatiTrainingDummy::StaticClass(),
		DummyLocation,
		FRotator(0.0f, 180.0f, 0.0f),
		SpawnParams
	);

	// ── 3. Spawn Devotee NPCs along the Festival Street ──
	const FVector NPCSpawnPositions[] = {
		FVector(-400.0f, -200.0f, 50.0f),
		FVector(100.0f, 250.0f, 50.0f),
		FVector(700.0f, -180.0f, 50.0f),
		FVector(1400.0f, 200.0f, 50.0f),
		FVector(1800.0f, -100.0f, 50.0f)
	};

	int32 Count = FMath::Min(NPCSpawnCount, static_cast<int32>(UE_ARRAY_COUNT(NPCSpawnPositions)));
	for (int32 i = 0; i < Count; ++i)
	{
		FVector SpawnLoc = ActorOrigin + NPCSpawnPositions[i];
		FRotator SpawnRot(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);

		World->SpawnActor<AGanapatiNPC>(
			AGanapatiNPC::StaticClass(),
			SpawnLoc,
			SpawnRot,
			SpawnParams
		);
	}
}
