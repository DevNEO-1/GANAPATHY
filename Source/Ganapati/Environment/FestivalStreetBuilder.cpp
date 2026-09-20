// Copyright Ganapati Project. All Rights Reserved.

#include "Environment/FestivalStreetBuilder.h"
#include "Interaction/GanapatiInteractable.h"
#include "NPCs/GanapatiNPC.h"
#include "Enemies/GanapatiTrainingDummy.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ChamferFinder(
		TEXT("/Game/LevelPrototyping/Meshes/SM_ChamferCube.SM_ChamferCube"));
	if (ChamferFinder.Succeeded())
	{
		ChamferCubeMesh = ChamferFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> QCylFinder(
		TEXT("/Game/LevelPrototyping/Meshes/SM_QuarterCylinder.SM_QuarterCylinder"));
	if (QCylFinder.Succeeded())
	{
		QuarterCylinderMesh = QCylFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(
		TEXT("/Game/LevelPrototyping/Meshes/SM_Plane.SM_Plane"));
	if (PlaneFinder.Succeeded())
	{
		PlaneMesh = PlaneFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CircBandFinder(
		TEXT("/Game/LevelPrototyping/Interactable/JumpPad/Assets/Meshes/SM_CircularBand.SM_CircularBand"));
	if (CircBandFinder.Succeeded())
	{
		CircularBandMesh = CircBandFinder.Object;
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

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GrayMatFinder(
		TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray"));
	if (GrayMatFinder.Succeeded())
	{
		GrayMaterial = GrayMatFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlatColorFinder(
		TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (FlatColorFinder.Succeeded())
	{
		FlatColorMaterial = FlatColorFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowFinder(
		TEXT("/Game/LevelPrototyping/Interactable/JumpPad/Assets/Materials/M_SimpleGlow.M_SimpleGlow"));
	if (GlowFinder.Succeeded())
	{
		GlowMaterial = GlowFinder.Object;
	}
}

void AFestivalStreetBuilder::BeginPlay()
{
	Super::BeginPlay();

	if (!bHasConstructed)
	{
		InitializeFestivalMaterials();
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
	bool bEnableCollision,
	UMaterialInterface* CustomMaterial)
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

	if (CustomMaterial)
	{
		Comp->SetMaterial(0, CustomMaterial);
	}
	else if (PrototypeGridMaterial)
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
	BuildEntrancePlaza();
	BuildStreetAndBuildings();
	BuildCentralPlaza();
	BuildSidePaths();
	BuildTownhouses();
	BuildBazaarStalls();
	BuildGaneshPandal();
	BuildFestivalLighting();
	BuildCombatAndParkourCourtyard();
	BuildAntiGravityDemonstrationArea();
}

void AFestivalStreetBuilder::BuildEntrancePlaza()
{
	// ── 1. Arrival Threshold & Entrance Ground ──
	// Ground slab leading into the Maha-Dwar (X = -1600 to -1380, Y: -600 to +600)
	CreateMeshPiece(
		TEXT("Entrance_ArrivalFloor"),
		CubeMesh,
		FVector(-1490.0f, 0.0f, -20.0f),
		FRotator::ZeroRotator,
		FVector(3.2f, 13.0f, 0.4f),
		true,
		GrayMaterial
	);

	// Rear Boundary Wall (behind the player start to enclose the district)
	CreateMeshPiece(
		TEXT("Entrance_RearWall"),
		CubeMesh,
		FVector(-1620.0f, 0.0f, 250.0f),
		FRotator::ZeroRotator,
		FVector(0.8f, 16.0f, 5.0f)
	);

	// Flanking Boundary Walls framing the entrance gate
	CreateMeshPiece(
		TEXT("Entrance_LeftWall"),
		CubeMesh,
		FVector(-1380.0f, -850.0f, 250.0f),
		FRotator::ZeroRotator,
		FVector(0.8f, 7.0f, 5.0f)
	);
	CreateMeshPiece(
		TEXT("Entrance_RightWall"),
		CubeMesh,
		FVector(-1380.0f, 850.0f, 250.0f),
		FRotator::ZeroRotator,
		FVector(0.8f, 7.0f, 5.0f)
	);

	// ── 2. Toran Maha-Dwar (Grand Welcome Gate at X = -1380) ──
	const float PillarY = 480.0f;

	// Left Pillar: Chamfer Base, Cylinder Shaft, and Top Finial
	if (ChamferCubeMesh)
	{
		CreateMeshPiece(TEXT("Gate_Plinth_L"), ChamferCubeMesh, FVector(-1380.0f, -PillarY, 25.0f), FRotator::ZeroRotator, FVector(1.8f, 1.8f, 0.5f));
		CreateMeshPiece(TEXT("Gate_Plinth_R"), ChamferCubeMesh, FVector(-1380.0f, PillarY, 25.0f), FRotator::ZeroRotator, FVector(1.8f, 1.8f, 0.5f));
	}
	CreateMeshPiece(TEXT("Gate_Pillar_L"), CylinderMesh, FVector(-1380.0f, -PillarY, 300.0f), FRotator::ZeroRotator, FVector(1.4f, 1.4f, 5.6f));
	CreateMeshPiece(TEXT("Gate_Pillar_R"), CylinderMesh, FVector(-1380.0f, PillarY, 300.0f), FRotator::ZeroRotator, FVector(1.4f, 1.4f, 5.6f));

	// Overhead Arch Beam / Lintel
	CreateMeshPiece(
		TEXT("Gate_MainBeam"),
		CubeMesh,
		FVector(-1380.0f, 0.0f, 560.0f),
		FRotator::ZeroRotator,
		FVector(1.4f, 12.0f, 0.8f),
		true,
		GrayMaterial
	);

	// Decorative Festive Toran Banner / Marigold Band
	CreateMeshPiece(
		TEXT("Gate_ToranBand"),
		CubeMesh,
		FVector(-1380.0f, 0.0f, 505.0f),
		FRotator::ZeroRotator,
		FVector(0.4f, 9.6f, 0.35f),
		false
	);

	// Top Ceremonial Kalash Finials on Pillars
	CreateMeshPiece(TEXT("Gate_Finial_L"), CylinderMesh, FVector(-1380.0f, -PillarY, 605.0f), FRotator::ZeroRotator, FVector(0.7f, 0.7f, 0.6f), false);
	CreateMeshPiece(TEXT("Gate_Finial_R"), CylinderMesh, FVector(-1380.0f, PillarY, 605.0f), FRotator::ZeroRotator, FVector(0.7f, 0.7f, 0.6f), false);
}

void AFestivalStreetBuilder::BuildStreetAndBuildings()
{
	// ── 1. Main Festival Street (Raja Marg / Procession Route) ──
	// West Segment: From Entrance Arch (X = -1380) to Central Chowk (X = +500)
	// Road runs along X, width = 950 cm (Y: -475 to +475)
	UStaticMeshComponent* RoadWest = CreateMeshPiece(
		TEXT("MainRoad_West"),
		CubeMesh,
		FVector(-440.0f, 0.0f, -20.0f),
		FRotator::ZeroRotator,
		FVector(18.8f, 9.5f, 0.4f),
		true,
		DarkMaterial
	);

	// East Segment: From Central Chowk (X = +1100) to Grand Pandal (X = +1900)
	UStaticMeshComponent* RoadEast = CreateMeshPiece(
		TEXT("MainRoad_East"),
		CubeMesh,
		FVector(1500.0f, 0.0f, -20.0f),
		FRotator::ZeroRotator,
		FVector(8.0f, 9.5f, 0.4f),
		true,
		DarkMaterial
	);

	// ── 2. Curbside Stone Sidewalk Plinths (Otlas / Verandas) ──
	// Elevated walkways (Otlas) characteristic of Indian streets where devotees walk and sit
	// West North Otla (Left)
	CreateMeshPiece(
		TEXT("Otla_West_North"),
		CubeMesh,
		FVector(-440.0f, -575.0f, 12.0f),
		FRotator::ZeroRotator,
		FVector(18.8f, 1.8f, 0.25f),
		true,
		GrayMaterial
	);
	// West South Otla (Right)
	CreateMeshPiece(
		TEXT("Otla_West_South"),
		CubeMesh,
		FVector(-440.0f, 575.0f, 12.0f),
		FRotator::ZeroRotator,
		FVector(18.8f, 1.8f, 0.25f),
		true,
		GrayMaterial
	);

	// East North Otla (Left)
	CreateMeshPiece(
		TEXT("Otla_East_North"),
		CubeMesh,
		FVector(1500.0f, -575.0f, 12.0f),
		FRotator::ZeroRotator,
		FVector(8.0f, 1.8f, 0.25f),
		true,
		GrayMaterial
	);
	// East South Otla (Right)
	CreateMeshPiece(
		TEXT("Otla_East_South"),
		CubeMesh,
		FVector(1500.0f, 575.0f, 12.0f),
		FRotator::ZeroRotator,
		FVector(8.0f, 1.8f, 0.25f),
		true,
		GrayMaterial
	);
}

void AFestivalStreetBuilder::BuildCentralPlaza()
{
	// ── Central Festival Plaza (The Chowk / Rangoli Circle) ──
	// Located at X = 800, Y = 0. Connects Entrance Street, Pandal, Mandir Gali, and Bazaar Gali
	const FVector ChowkCenter(800.0f, 0.0f, 0.0f);

	// 1. Plaza Paving (1500cm wide open crossroads)
	CreateMeshPiece(
		TEXT("Chowk_Paving"),
		CubeMesh,
		ChowkCenter + FVector(0.0f, 0.0f, -20.0f),
		FRotator::ZeroRotator,
		FVector(7.0f, 15.5f, 0.4f),
		true,
		GrayMaterial
	);

	// 2. Central Ceremonial Rangoli Medallion & Dhwaja Stambha (Festival Mast)
	// Outer concentric Rangoli base ring
	CreateMeshPiece(
		TEXT("Chowk_Rangoli_Outer"),
		CylinderMesh,
		ChowkCenter + FVector(0.0f, 0.0f, 4.0f),
		FRotator::ZeroRotator,
		FVector(3.8f, 3.8f, 0.12f),
		true,
		DarkMaterial
	);

	// Inner decorative medallion
	CreateMeshPiece(
		TEXT("Chowk_Rangoli_Inner"),
		CylinderMesh,
		ChowkCenter + FVector(0.0f, 0.0f, 14.0f),
		FRotator::ZeroRotator,
		FVector(2.4f, 2.4f, 0.15f),
		true,
		GrayMaterial
	);

	// Chamfer pedestal for festival flag mast
	if (ChamferCubeMesh)
	{
		CreateMeshPiece(
			TEXT("Chowk_Mast_Base"),
			ChamferCubeMesh,
			ChowkCenter + FVector(0.0f, 0.0f, 32.0f),
			FRotator::ZeroRotator,
			FVector(1.2f, 1.2f, 0.35f)
		);
	}

	// Dhwaja Stambha (7.5-meter tall ceremonial festival pole)
	CreateMeshPiece(
		TEXT("Chowk_Dhwaja_Mast"),
		CylinderMesh,
		ChowkCenter + FVector(0.0f, 0.0f, 410.0f),
		FRotator::ZeroRotator,
		FVector(0.35f, 0.35f, 7.5f)
	);

	// Golden Kalash Finial atop mast
	CreateMeshPiece(
		TEXT("Chowk_Dhwaja_Finial"),
		CylinderMesh,
		ChowkCenter + FVector(0.0f, 0.0f, 785.0f),
		FRotator::ZeroRotator,
		FVector(0.7f, 0.7f, 0.7f),
		false
	);

	// 3. Procession Dhol & Performance Stage (North-East corner of Chowk)
	// Raised performance platform for musicians and aarti performers
	const FVector StageLoc = ChowkCenter + FVector(-50.0f, -580.0f, 0.0f);
	CreateMeshPiece(
		TEXT("Chowk_Stage_Plat"),
		CubeMesh,
		StageLoc + FVector(0.0f, 0.0f, 35.0f),
		FRotator::ZeroRotator,
		FVector(4.2f, 2.8f, 0.7f),
		true,
		GrayMaterial
	);

	// Stage steps
	if (RampMesh)
	{
		CreateMeshPiece(
			TEXT("Chowk_Stage_Steps"),
			RampMesh,
			StageLoc + FVector(-260.0f, 0.0f, 0.0f),
			FRotator(0.0f, 0.0f, 0.0f),
			FVector(1.0f, 2.8f, 0.7f)
		);
	}

	// Stage canopy poles and roof
	CreateMeshPiece(TEXT("Stage_Pole_1"), CylinderMesh, StageLoc + FVector(-180.0f, -120.0f, 150.0f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 2.4f), false);
	CreateMeshPiece(TEXT("Stage_Pole_2"), CylinderMesh, StageLoc + FVector(180.0f, -120.0f, 150.0f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 2.4f), false);
	CreateMeshPiece(TEXT("Stage_Pole_3"), CylinderMesh, StageLoc + FVector(-180.0f, 120.0f, 150.0f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 2.4f), false);
	CreateMeshPiece(TEXT("Stage_Pole_4"), CylinderMesh, StageLoc + FVector(180.0f, 120.0f, 150.0f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 2.4f), false);

	CreateMeshPiece(
		TEXT("Stage_Canopy_Roof"),
		CubeMesh,
		StageLoc + FVector(0.0f, 0.0f, 280.0f),
		FRotator(5.0f, 0.0f, 0.0f),
		FVector(4.4f, 3.0f, 0.15f)
	);
}

void AFestivalStreetBuilder::BuildSidePaths()
{
	// ── 1. North Mandir Gali (Residential & Quiet Temple Lane) ──
	// Branches off Chowk at X = 800, leading North from Y = -775 to Y = -1650
	const FVector NorthGaliCenter(800.0f, -1210.0f, 0.0f);

	// Mandir Gali stone paving (420cm wide alley)
	CreateMeshPiece(
		TEXT("Gali_North_Floor"),
		CubeMesh,
		NorthGaliCenter + FVector(0.0f, 0.0f, -20.0f),
		FRotator::ZeroRotator,
		FVector(4.2f, 8.8f, 0.4f),
		true,
		DarkMaterial
	);

	// West wall along North Gali
	CreateMeshPiece(
		TEXT("Gali_North_Wall_W"),
		CubeMesh,
		NorthGaliCenter + FVector(-230.0f, 0.0f, 250.0f),
		FRotator::ZeroRotator,
		FVector(0.6f, 8.8f, 5.0f)
	);

	// East wall along North Gali
	CreateMeshPiece(
		TEXT("Gali_North_Wall_E"),
		CubeMesh,
		NorthGaliCenter + FVector(230.0f, 0.0f, 250.0f),
		FRotator::ZeroRotator,
		FVector(0.6f, 8.8f, 5.0f)
	);

	// North Gali End Shrine Niche (Cozy small temple bell alcove at the end of the lane)
	CreateMeshPiece(
		TEXT("Gali_North_EndWall"),
		CubeMesh,
		FVector(800.0f, -1650.0f, 200.0f),
		FRotator::ZeroRotator,
		FVector(4.2f, 0.8f, 4.0f)
	);
	CreateMeshPiece(
		TEXT("Gali_North_Bell_Altar"),
		CylinderMesh,
		FVector(800.0f, -1570.0f, 40.0f),
		FRotator::ZeroRotator,
		FVector(1.4f, 1.4f, 0.8f)
	);
	CreateMeshPiece(
		TEXT("Gali_North_Bell_Arch"),
		CubeMesh,
		FVector(800.0f, -1570.0f, 160.0f),
		FRotator::ZeroRotator,
		FVector(0.3f, 1.4f, 1.6f)
	);

	// ── 2. South Bazaar Gali (Courtyard Lane) ──
	// Branches off Chowk at X = 800, leading South from Y = +775 to Y = +1450 into the Combat Courtyard
	const FVector SouthGaliCenter(800.0f, 1110.0f, 0.0f);

	CreateMeshPiece(
		TEXT("Gali_South_Floor"),
		CubeMesh,
		SouthGaliCenter + FVector(0.0f, 0.0f, -20.0f),
		FRotator::ZeroRotator,
		FVector(4.8f, 6.8f, 0.4f),
		true,
		DarkMaterial
	);

	// West wall along South Gali
	CreateMeshPiece(
		TEXT("Gali_South_Wall_W"),
		CubeMesh,
		SouthGaliCenter + FVector(-260.0f, 0.0f, 250.0f),
		FRotator::ZeroRotator,
		FVector(0.6f, 6.8f, 5.0f)
	);

	// East wall along South Gali
	CreateMeshPiece(
		TEXT("Gali_South_Wall_E"),
		CubeMesh,
		SouthGaliCenter + FVector(260.0f, 0.0f, 250.0f),
		FRotator::ZeroRotator,
		FVector(0.6f, 6.8f, 5.0f)
	);
}

void AFestivalStreetBuilder::BuildTownhouses()
{
	// ── Multi-tier Vernacular Indian Townhouses (Wadas & Havelis) ──
	// Distinct facades with staggered heights (450cm to 750cm), ground-floor verandas,
	// overhanging balconies (jharokhas), and pitched parapet trims along both street flanks.

	// --- NORTH FLANK BUILDINGS (Y = -1050 to -1150) ---
	struct FTownhouseSpec
	{
		float X;
		float Y;
		float Width;
		float Depth;
		float Height;
		int32 Floors;
	};

	const FTownhouseSpec NorthHouses[] = {
		{ -1100.0f, -1050.0f, 6.5f, 6.0f, 5.2f, 2 }, // Entrance Wada
		{  -650.0f, -1050.0f, 7.0f, 6.0f, 7.2f, 3 }, // Merchant Haveli
		{  -200.0f, -1050.0f, 6.0f, 6.0f, 5.8f, 2 }, // Sweet Vendor House
		{   250.0f, -1050.0f, 6.5f, 6.0f, 6.8f, 3 }, // Chowk Corner Haveli
		// (Opening for North Mandir Gali at X = 800)
		{  1350.0f, -1050.0f, 6.0f, 6.0f, 6.2f, 2 }, // Temple Street Wada A
		{  1700.0f, -1050.0f, 6.0f, 6.0f, 5.0f, 2 }  // Pandal Approach House
	};

	for (int32 i = 0; i < UE_ARRAY_COUNT(NorthHouses); ++i)
	{
		const FTownhouseSpec& House = NorthHouses[i];
		const FVector HouseLoc(House.X, House.Y, House.Height * 50.0f);

		// Main Building Body
		CreateMeshPiece(
			FString::Printf(TEXT("Townhouse_N_%d_Body"), i),
			CubeMesh,
			HouseLoc,
			FRotator::ZeroRotator,
			FVector(House.Width, House.Depth, House.Height)
		);

		// Ground Floor Veranda / Front Porch plinth
		CreateMeshPiece(
			FString::Printf(TEXT("Townhouse_N_%d_Porch"), i),
			CubeMesh,
			FVector(House.X, House.Y + 220.0f, 20.0f),
			FRotator::ZeroRotator,
			FVector(House.Width * 0.85f, 1.6f, 0.4f),
			true,
			GrayMaterial
		);

		// First Floor Overhanging Balcony (Jharokha Lintel)
		if (House.Floors >= 2)
		{
			CreateMeshPiece(
				FString::Printf(TEXT("Townhouse_N_%d_Balcony"), i),
				CubeMesh,
				FVector(House.X, House.Y + 280.0f, 260.0f),
				FRotator::ZeroRotator,
				FVector(House.Width * 0.65f, 0.8f, 0.35f),
				true,
				GrayMaterial
			);
		}

		// Roof Parapet / Eaves
		CreateMeshPiece(
			FString::Printf(TEXT("Townhouse_N_%d_Parapet"), i),
			CubeMesh,
			FVector(House.X, House.Y, House.Height * 100.0f + 15.0f),
			FRotator::ZeroRotator,
			FVector(House.Width + 0.4f, House.Depth + 0.4f, 0.3f),
			true,
			GrayMaterial
		);
	}

	// --- SOUTH FLANK BUILDINGS (Y = +1050 to +1150) ---
	const FTownhouseSpec SouthHouses[] = {
		{ -1100.0f, 1050.0f, 6.5f, 6.0f, 5.6f, 2 }, // Entrance House South
		{  -650.0f, 1050.0f, 7.0f, 6.0f, 6.8f, 3 }, // Flower Merchant Haveli
		{  -200.0f, 1050.0f, 6.0f, 6.0f, 5.4f, 2 }, // Diya Craftsman Wada
		{   250.0f, 1050.0f, 6.5f, 6.0f, 7.4f, 3 }, // Chowk South Haveli
		// (Opening for South Bazaar Gali at X = 800)
		{  1350.0f, 1050.0f, 6.0f, 6.0f, 6.0f, 2 }, // Temple Wada South A
		{  1700.0f, 1050.0f, 6.0f, 6.0f, 5.2f, 2 }  // Temple Wada South B
	};

	for (int32 i = 0; i < UE_ARRAY_COUNT(SouthHouses); ++i)
	{
		const FTownhouseSpec& House = SouthHouses[i];
		const FVector HouseLoc(House.X, House.Y, House.Height * 50.0f);

		// Main Building Body
		CreateMeshPiece(
			FString::Printf(TEXT("Townhouse_S_%d_Body"), i),
			CubeMesh,
			HouseLoc,
			FRotator::ZeroRotator,
			FVector(House.Width, House.Depth, House.Height)
		);

		// Ground Floor Veranda / Front Porch
		CreateMeshPiece(
			FString::Printf(TEXT("Townhouse_S_%d_Porch"), i),
			CubeMesh,
			FVector(House.X, House.Y - 220.0f, 20.0f),
			FRotator::ZeroRotator,
			FVector(House.Width * 0.85f, 1.6f, 0.4f),
			true,
			GrayMaterial
		);

		// First Floor Balcony
		if (House.Floors >= 2)
		{
			CreateMeshPiece(
				FString::Printf(TEXT("Townhouse_S_%d_Balcony"), i),
				CubeMesh,
				FVector(House.X, House.Y - 280.0f, 260.0f),
				FRotator::ZeroRotator,
				FVector(House.Width * 0.65f, 0.8f, 0.35f),
				true,
				GrayMaterial
			);
		}

		// Roof Parapet
		CreateMeshPiece(
			FString::Printf(TEXT("Townhouse_S_%d_Parapet"), i),
			CubeMesh,
			FVector(House.X, House.Y, House.Height * 100.0f + 15.0f),
			FRotator::ZeroRotator,
			FVector(House.Width + 0.4f, House.Depth + 0.4f, 0.3f),
			true,
			GrayMaterial
		);
	}
}

void AFestivalStreetBuilder::BuildBazaarStalls()
{
	// ── Bazaar Zones: Modak Sweets, Puja Flower, and Diya Stalls ──
	// Arranged along the curbs in dedicated vendor nooks

	// --- 1. Modak / Sweets Stall Zone (North Side, X = -450 and X = -150) ---
	const float NorthStallX[] = { -450.0f, -150.0f };
	for (int32 i = 0; i < UE_ARRAY_COUNT(NorthStallX); ++i)
	{
		float X = NorthStallX[i];

		// Stone Table Counter
		CreateMeshPiece(
			FString::Printf(TEXT("Stall_Sweets_%d_Table"), i),
			CubeMesh,
			FVector(X, -580.0f, 45.0f),
			FRotator::ZeroRotator,
			FVector(2.8f, 1.5f, 0.9f)
		);
		// Multi-tier sweets display shelf
		CreateMeshPiece(
			FString::Printf(TEXT("Stall_Sweets_%d_Shelf"), i),
			CubeMesh,
			FVector(X, -620.0f, 85.0f),
			FRotator::ZeroRotator,
			FVector(2.4f, 0.6f, 0.5f)
		);
		// Festive Canopy Roof (tilted forward)
		CreateMeshPiece(
			FString::Printf(TEXT("Stall_Sweets_%d_Canopy"), i),
			CubeMesh,
			FVector(X, -580.0f, 235.0f),
			FRotator(12.0f, 0.0f, 0.0f),
			FVector(3.2f, 2.0f, 0.12f)
		);
		// Wooden support poles
		CreateMeshPiece(FString::Printf(TEXT("Stall_Sweets_%d_Pole1"), i), CylinderMesh, FVector(X - 120.0f, -500.0f, 120.0f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 2.4f), false);
		CreateMeshPiece(FString::Printf(TEXT("Stall_Sweets_%d_Pole2"), i), CylinderMesh, FVector(X + 120.0f, -500.0f, 120.0f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 2.4f), false);

		// Phase 5A Subsystem 1: Decorate the primary stall (X = -450) with the Sacred Modak Prasadam Thali
		if (i == 0)
		{
			BuildModakPrasadamTray(FVector(X, -580.0f, 0.0f));
		}
	}

	// --- 2. Flower & Garland Bazaar Zone (South Side, X = -450 and X = -150) ---
	const float SouthStallX[] = { -450.0f, -150.0f };
	for (int32 i = 0; i < UE_ARRAY_COUNT(SouthStallX); ++i)
	{
		float X = SouthStallX[i];

		// Counter
		CreateMeshPiece(
			FString::Printf(TEXT("Stall_Flower_%d_Table"), i),
			CubeMesh,
			FVector(X, 580.0f, 45.0f),
			FRotator::ZeroRotator,
			FVector(2.8f, 1.5f, 0.9f)
		);
		// Stepped Garland Hanging Beam
		CreateMeshPiece(
			FString::Printf(TEXT("Stall_Flower_%d_Beam"), i),
			CubeMesh,
			FVector(X, 580.0f, 175.0f),
			FRotator::ZeroRotator,
			FVector(2.6f, 0.25f, 0.2f),
			false
		);
		// Canopy Roof (tilted forward towards street)
		CreateMeshPiece(
			FString::Printf(TEXT("Stall_Flower_%d_Canopy"), i),
			CubeMesh,
			FVector(X, 580.0f, 235.0f),
			FRotator(-12.0f, 0.0f, 0.0f),
			FVector(3.2f, 2.0f, 0.12f)
		);
		CreateMeshPiece(FString::Printf(TEXT("Stall_Flower_%d_Pole1"), i), CylinderMesh, FVector(X - 120.0f, 500.0f, 120.0f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 2.4f), false);
		CreateMeshPiece(FString::Printf(TEXT("Stall_Flower_%d_Pole2"), i), CylinderMesh, FVector(X + 120.0f, 500.0f, 120.0f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 2.4f), false);
	}

	// --- 3. Puja & Diya Craft Stalls (Approaching Chowk, X = 250) ---
	// North Diya Stand
	CreateMeshPiece(TEXT("Stall_Diya_N_Table"), CubeMesh, FVector(250.0f, -580.0f, 45.0f), FRotator::ZeroRotator, FVector(2.6f, 1.4f, 0.9f));
	CreateMeshPiece(TEXT("Stall_Diya_N_Canopy"), CubeMesh, FVector(250.0f, -580.0f, 235.0f), FRotator(10.0f, 0.0f, 0.0f), FVector(3.0f, 1.8f, 0.12f));

	// South Diya Stand
	CreateMeshPiece(TEXT("Stall_Diya_S_Table"), CubeMesh, FVector(250.0f, 580.0f, 45.0f), FRotator::ZeroRotator, FVector(2.6f, 1.4f, 0.9f));
	CreateMeshPiece(TEXT("Stall_Diya_S_Canopy"), CubeMesh, FVector(250.0f, 580.0f, 235.0f), FRotator(-10.0f, 0.0f, 0.0f), FVector(3.0f, 1.8f, 0.12f));
}

void AFestivalStreetBuilder::BuildModakPrasadamTray(const FVector& StallCenter)
{
	// Center of the counter top: X = StallCenter.X, Y = StallCenter.Y + 30.0f (-550.0f), Z = 90.0f
	const FVector CounterCenter = StallCenter + FVector(0.0f, 30.0f, 90.0f);

	// ── 1. Sacred Brass Prasadam Thali (Platter) ──
	CreateMeshPiece(
		TEXT("Modak_BrassThali"),
		CylinderMesh,
		CounterCenter + FVector(0.0f, 0.0f, 2.0f),
		FRotator::ZeroRotator,
		FVector(1.4f, 1.4f, 0.04f),
		true,
		GoldMat
	);

	if (CircularBandMesh)
	{
		CreateMeshPiece(
			TEXT("Modak_ThaliRim"),
			CircularBandMesh,
			CounterCenter + FVector(0.0f, 0.0f, 3.5f),
			FRotator::ZeroRotator,
			FVector(1.4f, 1.4f, 0.12f),
			false,
			GoldMat
		);
	}

	// Fresh green banana-leaf lining on the thali
	CreateMeshPiece(
		TEXT("Modak_LeafLiner"),
		CylinderMesh,
		CounterCenter + FVector(0.0f, 0.0f, 3.0f),
		FRotator::ZeroRotator,
		FVector(1.2f, 1.2f, 0.02f),
		false,
		GreenMat
	);

	// ── 2. Steamed Ukadiche Modak Offering Mound ──
	// Central Maha-Modak (King Modak)
	CreateMeshPiece(
		TEXT("Modak_Center_Base"),
		CylinderMesh,
		CounterCenter + FVector(0.0f, 0.0f, 10.0f),
		FRotator::ZeroRotator,
		FVector(0.4f, 0.4f, 0.16f),
		false,
		WhiteMat
	);
	if (ChamferCubeMesh)
	{
		CreateMeshPiece(
			TEXT("Modak_Center_Spire"),
			ChamferCubeMesh,
			CounterCenter + FVector(0.0f, 0.0f, 22.0f),
			FRotator(0.0f, 45.0f, 0.0f),
			FVector(0.24f, 0.24f, 0.24f),
			false,
			WhiteMat
		);
	}
	// Saffron Kesar Tilak dot on central modak apex
	CreateMeshPiece(
		TEXT("Modak_Center_Kesar"),
		CylinderMesh,
		CounterCenter + FVector(0.0f, 0.0f, 34.0f),
		FRotator::ZeroRotator,
		FVector(0.07f, 0.07f, 0.04f),
		false,
		MarigoldMat
	);

	// Ring of 8 freshly steamed Modaks encircling the center
	const float RingRadius = 32.0f;
	for (int32 m = 0; m < 8; ++m)
	{
		const float AngleDeg = m * 45.0f;
		const float Rad = FMath::DegreesToRadians(AngleDeg);
		const FVector Offset(RingRadius * FMath::Cos(Rad), RingRadius * FMath::Sin(Rad), 6.0f);

		CreateMeshPiece(
			FString::Printf(TEXT("Modak_Ring_%d_Base"), m),
			CylinderMesh,
			CounterCenter + Offset,
			FRotator::ZeroRotator,
			FVector(0.22f, 0.22f, 0.10f),
			false,
			WhiteMat
		);

		if (ChamferCubeMesh)
		{
			CreateMeshPiece(
				FString::Printf(TEXT("Modak_Ring_%d_Tip"), m),
				ChamferCubeMesh,
				CounterCenter + Offset + FVector(0.0f, 0.0f, 8.0f),
				FRotator(0.0f, AngleDeg + 22.5f, 0.0f),
				FVector(0.14f, 0.14f, 0.14f),
				false,
				WhiteMat
			);
		}

		CreateMeshPiece(
			FString::Printf(TEXT("Modak_Ring_%d_Dot"), m),
			CylinderMesh,
			CounterCenter + Offset + FVector(0.0f, 0.0f, 15.0f),
			FRotator::ZeroRotator,
			FVector(0.04f, 0.04f, 0.03f),
			false,
			SindoorMat
		);
	}

	// ── 3. Sweets Display Bowls on Rear Shelf ──
	// Brass bowl of Motichoor Ladoos on the shelf
	const FVector ShelfPos = StallCenter + FVector(-60.0f, -40.0f, 85.0f + 25.0f);
	CreateMeshPiece(
		TEXT("Modak_ShelfBowl1"),
		CylinderMesh,
		ShelfPos,
		FRotator::ZeroRotator,
		FVector(0.6f, 0.6f, 0.12f),
		false,
		GoldMat
	);
	CreateMeshPiece(
		TEXT("Modak_ShelfLadoos1"),
		CylinderMesh,
		ShelfPos + FVector(0.0f, 0.0f, 8.0f),
		FRotator::ZeroRotator,
		FVector(0.5f, 0.5f, 0.10f),
		false,
		MarigoldMat
	);

	// Brass bowl of white sweets on the shelf
	const FVector ShelfPos2 = StallCenter + FVector(60.0f, -40.0f, 85.0f + 25.0f);
	CreateMeshPiece(
		TEXT("Modak_ShelfBowl2"),
		CylinderMesh,
		ShelfPos2,
		FRotator::ZeroRotator,
		FVector(0.6f, 0.6f, 0.12f),
		false,
		GoldMat
	);
	CreateMeshPiece(
		TEXT("Modak_ShelfSweets2"),
		CylinderMesh,
		ShelfPos2 + FVector(0.0f, 0.0f, 8.0f),
		FRotator::ZeroRotator,
		FVector(0.5f, 0.5f, 0.10f),
		false,
		WhiteMat
	);

	// ── 4. Aarti Brass Diya & Festive Illumination ──
	const FVector DiyaPos = CounterCenter + FVector(-95.0f, 0.0f, 2.0f);
	CreateMeshPiece(
		TEXT("Modak_DiyaStand"),
		CylinderMesh,
		DiyaPos,
		FRotator::ZeroRotator,
		FVector(0.25f, 0.25f, 0.08f),
		false,
		GoldMat
	);
	if (ChamferCubeMesh)
	{
		CreateMeshPiece(
			TEXT("Modak_DiyaFlame"),
			ChamferCubeMesh,
			DiyaPos + FVector(0.0f, 0.0f, 8.0f),
			FRotator(0.0f, 45.0f, 0.0f),
			FVector(0.08f, 0.08f, 0.14f),
			false,
			DiyaGlowMat
		);
	}

	// Warm golden glow illuminating the prasadam stall
	CreateFestivalLight(
		TEXT("ModakStall_WarmLight"),
		CounterCenter + FVector(0.0f, 20.0f, 60.0f),
		FLinearColor(1.0f, 0.78f, 0.28f),
		2800.0f,
		450.0f
	);

	// ── 5. Decorative Marigold Garland along Counter Front ──
	const float GarlandY = StallCenter.Y + 75.0f;
	CreateMeshPiece(
		TEXT("Modak_CounterGarland"),
		CylinderMesh,
		FVector(StallCenter.X, GarlandY, 80.0f),
		FRotator(0.0f, 0.0f, 90.0f),
		FVector(0.10f, 0.10f, 2.7f),
		false,
		MarigoldMat
	);

	// Decorative Canopy Festive Fringe
	CreateMeshPiece(
		TEXT("Modak_CanopyFringe"),
		CubeMesh,
		FVector(StallCenter.X, StallCenter.Y + 20.0f, 240.0f),
		FRotator(12.0f, 0.0f, 0.0f),
		FVector(3.25f, 0.12f, 0.25f),
		false,
		MarigoldMat
	);
}

void AFestivalStreetBuilder::InitializeFestivalMaterials()
{
	if (FlatColorMaterial)
	{
		MarigoldMat = UMaterialInstanceDynamic::Create(FlatColorMaterial, this, TEXT("Mat_Marigold"));
		if (MarigoldMat)
		{
			MarigoldMat->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(1.0f, 0.42f, 0.02f));
			MarigoldMat->SetVectorParameterValue(FName(TEXT("BaseColor")), FLinearColor(1.0f, 0.42f, 0.02f));
		}

		SindoorMat = UMaterialInstanceDynamic::Create(FlatColorMaterial, this, TEXT("Mat_Sindoor"));
		if (SindoorMat)
		{
			SindoorMat->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.85f, 0.08f, 0.08f));
			SindoorMat->SetVectorParameterValue(FName(TEXT("BaseColor")), FLinearColor(0.85f, 0.08f, 0.08f));
		}

		GoldMat = UMaterialInstanceDynamic::Create(FlatColorMaterial, this, TEXT("Mat_Gold"));
		if (GoldMat)
		{
			GoldMat->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.98f, 0.78f, 0.15f));
			GoldMat->SetVectorParameterValue(FName(TEXT("BaseColor")), FLinearColor(0.98f, 0.78f, 0.15f));
		}

		WhiteMat = UMaterialInstanceDynamic::Create(FlatColorMaterial, this, TEXT("Mat_White"));
		if (WhiteMat)
		{
			WhiteMat->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.94f, 0.93f, 0.90f));
			WhiteMat->SetVectorParameterValue(FName(TEXT("BaseColor")), FLinearColor(0.94f, 0.93f, 0.90f));
		}

		GreenMat = UMaterialInstanceDynamic::Create(FlatColorMaterial, this, TEXT("Mat_Green"));
		if (GreenMat)
		{
			GreenMat->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(0.08f, 0.52f, 0.18f));
			GreenMat->SetVectorParameterValue(FName(TEXT("BaseColor")), FLinearColor(0.08f, 0.52f, 0.18f));
		}
	}

	if (GlowMaterial)
	{
		DiyaGlowMat = UMaterialInstanceDynamic::Create(GlowMaterial, this, TEXT("Mat_DiyaGlow"));
		if (DiyaGlowMat)
		{
			DiyaGlowMat->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(1.0f, 0.70f, 0.12f));
			DiyaGlowMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), FLinearColor(1.0f, 0.70f, 0.12f) * 16.0f);
		}
	}
	else if (FlatColorMaterial)
	{
		DiyaGlowMat = UMaterialInstanceDynamic::Create(FlatColorMaterial, this, TEXT("Mat_DiyaGlow"));
		if (DiyaGlowMat)
		{
			DiyaGlowMat->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(1.0f, 0.85f, 0.25f));
			DiyaGlowMat->SetVectorParameterValue(FName(TEXT("BaseColor")), FLinearColor(1.0f, 0.85f, 0.25f));
		}
	}
}

void AFestivalStreetBuilder::BuildGaneshPandal()
{
	// ── Grand Ganesh Pandal Temple Sanctuary ──
	// The sacred culmination of the festival street at X = 2200, Y = 0
	const FVector PandalCenter(2200.0f, 0.0f, 0.0f);

	// 1. Grand Elevated Temple Dais / Mandap Platform
	CreateMeshPiece(
		TEXT("Pandal_MainPlatform"),
		CubeMesh,
		PandalCenter + FVector(0.0f, 0.0f, 40.0f),
		FRotator::ZeroRotator,
		FVector(18.0f, 20.0f, 0.8f),
		true,
		DarkMaterial
	);

	// Upper Marble Sanctum Dais
	CreateMeshPiece(
		TEXT("Pandal_UpperPlatform"),
		CubeMesh,
		PandalCenter + FVector(120.0f, 0.0f, 85.0f),
		FRotator::ZeroRotator,
		FVector(13.5f, 15.5f, 0.25f),
		true,
		WhiteMat ? WhiteMat : GrayMaterial
	);

	// 2. Ceremonial Approach Steps (Grand multi-tiered entrance)
	for (int32 StepIdx = 0; StepIdx < 4; ++StepIdx)
	{
		float StepX = 1260.0f + (StepIdx * 50.0f);
		float StepZ = 15.0f + (StepIdx * 18.0f);
		float StepDepth = 3.2f - (StepIdx * 0.4f);
		CreateMeshPiece(
			FString::Printf(TEXT("Pandal_Step_%d"), StepIdx),
			CubeMesh,
			FVector(StepX, 0.0f, StepZ),
			FRotator::ZeroRotator,
			FVector(StepDepth, 9.0f, 0.35f),
			true,
			DarkMaterial
		);
	}

	// 3. Sacred Ceremonial Red & Gold Carpet Runner (Leading from Chowk to Altar)
	CreateMeshPiece(
		TEXT("Pandal_Carpet_Main"),
		CubeMesh,
		FVector(1500.0f, 0.0f, 82.0f),
		FRotator::ZeroRotator,
		FVector(15.0f, 2.6f, 0.06f),
		false,
		SindoorMat ? SindoorMat : GrayMaterial
	);
	// Carpet Gold Borders
	CreateMeshPiece(
		TEXT("Pandal_Carpet_Border_L"),
		CubeMesh,
		FVector(1500.0f, -135.0f, 83.0f),
		FRotator::ZeroRotator,
		FVector(15.0f, 0.18f, 0.07f),
		false,
		GoldMat ? GoldMat : GrayMaterial
	);
	CreateMeshPiece(
		TEXT("Pandal_Carpet_Border_R"),
		CubeMesh,
		FVector(1500.0f, 135.0f, 83.0f),
		FRotator::ZeroRotator,
		FVector(15.0f, 0.18f, 0.07f),
		false,
		GoldMat ? GoldMat : GrayMaterial
	);

	// 4. Eight Grand Carved Temple Pillars (4 Front Portico + 4 Inner Sanctum)
	const FVector PillarLocations[] = {
		// Portico (Entrance)
		FVector(1420.0f, -680.0f, 0.0f),
		FVector(1420.0f, -260.0f, 0.0f),
		FVector(1420.0f,  260.0f, 0.0f),
		FVector(1420.0f,  680.0f, 0.0f),
		// Inner Sanctum
		FVector(2050.0f, -560.0f, 0.0f),
		FVector(2050.0f,  560.0f, 0.0f),
		FVector(2680.0f, -560.0f, 0.0f),
		FVector(2680.0f,  560.0f, 0.0f)
	};

	const float PillarHeight = 6.6f;
	for (int32 i = 0; i < UE_ARRAY_COUNT(PillarLocations); ++i)
	{
		const FVector& Pos = PillarLocations[i];

		// Base Plinth
		if (ChamferCubeMesh)
		{
			CreateMeshPiece(FString::Printf(TEXT("Pandal_PillarBase_%d"), i), ChamferCubeMesh, Pos + FVector(0.0f, 0.0f, 50.0f), FRotator::ZeroRotator, FVector(1.9f, 1.9f, 0.7f), true, DarkMaterial);
		}

		// Main Pillar Shaft (White marble / carved stone)
		CreateMeshPiece(FString::Printf(TEXT("Pandal_PillarShaft_%d"), i), CylinderMesh, Pos + FVector(0.0f, 0.0f, 380.0f), FRotator::ZeroRotator, FVector(1.4f, 1.4f, PillarHeight), true, WhiteMat ? WhiteMat : GrayMaterial);

		// Mid-Height Marigold Floral Ring
		CreateMeshPiece(FString::Printf(TEXT("Pandal_PillarGarland_%d"), i), CylinderMesh, Pos + FVector(0.0f, 0.0f, 320.0f), FRotator::ZeroRotator, FVector(1.6f, 1.6f, 0.25f), false, MarigoldMat ? MarigoldMat : GrayMaterial);

		// Stepped Capital Bracket (Golden brass trim)
		if (ChamferCubeMesh)
		{
			CreateMeshPiece(FString::Printf(TEXT("Pandal_PillarCapital_%d"), i), ChamferCubeMesh, Pos + FVector(0.0f, 0.0f, 700.0f), FRotator::ZeroRotator, FVector(1.8f, 1.8f, 0.5f), true, GoldMat ? GoldMat : GrayMaterial);
		}
	}

	// 5. Grand Mandap Canopy / Multi-Tiered Temple Shikhara Roof
	// Tie Beams connecting columns
	CreateMeshPiece(TEXT("Pandal_Beam_Front"), CubeMesh, FVector(1420.0f, 0.0f, 705.0f), FRotator::ZeroRotator, FVector(1.2f, 15.0f, 0.7f), true, DarkMaterial);
	CreateMeshPiece(TEXT("Pandal_Beam_Back"), CubeMesh, FVector(2680.0f, 0.0f, 705.0f), FRotator::ZeroRotator, FVector(1.2f, 15.0f, 0.7f), true, DarkMaterial);
	CreateMeshPiece(TEXT("Pandal_Beam_Left"), CubeMesh, FVector(2050.0f, -560.0f, 705.0f), FRotator::ZeroRotator, FVector(13.0f, 1.2f, 0.7f), true, DarkMaterial);
	CreateMeshPiece(TEXT("Pandal_Beam_Right"), CubeMesh, FVector(2050.0f, 560.0f, 705.0f), FRotator::ZeroRotator, FVector(13.0f, 1.2f, 0.7f), true, DarkMaterial);

	// Tier 1: Overhanging Lower Eaves
	CreateMeshPiece(TEXT("Pandal_Roof_Tier1"), CubeMesh, PandalCenter + FVector(0.0f, 0.0f, 740.0f), FRotator::ZeroRotator, FVector(19.0f, 21.0f, 0.6f), true, DarkMaterial);
	// Eaves Saffron Decorative Trim
	CreateMeshPiece(TEXT("Pandal_Roof_Trim"), CubeMesh, PandalCenter + FVector(0.0f, 0.0f, 715.0f), FRotator::ZeroRotator, FVector(19.6f, 21.6f, 0.2f), false, GoldMat ? GoldMat : GrayMaterial);

	// Tier 2: Stepped Saffron Pyramid Tier (Mandap superstructure)
	CreateMeshPiece(TEXT("Pandal_Roof_Tier2"), CubeMesh, PandalCenter + FVector(0.0f, 0.0f, 810.0f), FRotator::ZeroRotator, FVector(14.0f, 16.0f, 1.1f), true, MarigoldMat ? MarigoldMat : GrayMaterial);

	// Tier 3: Upper Shikhara Pavilion
	CreateMeshPiece(TEXT("Pandal_Roof_Tier3"), CubeMesh, PandalCenter + FVector(0.0f, 0.0f, 910.0f), FRotator::ZeroRotator, FVector(9.0f, 11.0f, 1.4f), true, SindoorMat ? SindoorMat : GrayMaterial);

	// Central Golden Kalash Pinnacle (Temple Stupa Spire)
	CreateMeshPiece(TEXT("Pandal_Kalash_Base"), CylinderMesh, PandalCenter + FVector(0.0f, 0.0f, 1010.0f), FRotator::ZeroRotator, FVector(3.2f, 3.2f, 0.8f), false, GoldMat ? GoldMat : GrayMaterial);
	CreateMeshPiece(TEXT("Pandal_Kalash_Pot"), CylinderMesh, PandalCenter + FVector(0.0f, 0.0f, 1080.0f), FRotator::ZeroRotator, FVector(1.8f, 1.8f, 1.0f), false, GoldMat ? GoldMat : GrayMaterial);
	CreateMeshPiece(TEXT("Pandal_Kalash_Finial"), CylinderMesh, PandalCenter + FVector(0.0f, 0.0f, 1150.0f), FRotator::ZeroRotator, FVector(0.8f, 0.8f, 1.2f), false, GoldMat ? GoldMat : GrayMaterial);

	// Corner Kalash Finials on roof eaves
	const FVector CornerKalashPos[] = {
		PandalCenter + FVector(-850.0f, -950.0f, 770.0f),
		PandalCenter + FVector(-850.0f,  950.0f, 770.0f),
		PandalCenter + FVector( 850.0f, -950.0f, 770.0f),
		PandalCenter + FVector( 850.0f,  950.0f, 770.0f)
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(CornerKalashPos); ++i)
	{
		CreateMeshPiece(FString::Printf(TEXT("Pandal_CornerKalash_%d"), i), CylinderMesh, CornerKalashPos[i], FRotator::ZeroRotator, FVector(0.7f, 0.7f, 0.8f), false, GoldMat ? GoldMat : GrayMaterial);
		// Ceremonial Saffron Flag on corner
		CreateMeshPiece(FString::Printf(TEXT("Pandal_FlagPole_%d"), i), CylinderMesh, CornerKalashPos[i] + FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, FVector(0.08f, 0.08f, 2.2f), false, GoldMat ? GoldMat : GrayMaterial);
		CreateMeshPiece(FString::Printf(TEXT("Pandal_Flag_%d"), i), CubeMesh, CornerKalashPos[i] + FVector(35.0f, 0.0f, 210.0f), FRotator(0.0f, 15.0f, 0.0f), FVector(0.7f, 0.05f, 0.45f), false, MarigoldMat ? MarigoldMat : GrayMaterial);
	}

	// 6. Sacred Rear Wall (Garbhagriha Enclosure & Prabhavali Wall)
	CreateMeshPiece(
		TEXT("Pandal_BackWall"),
		CubeMesh,
		PandalCenter + FVector(700.0f, 0.0f, 400.0f),
		FRotator::ZeroRotator,
		FVector(0.9f, 17.0f, 7.2f),
		true,
		DarkMaterial
	);
	// Flanking Side Enclosure Walls
	CreateMeshPiece(TEXT("Pandal_SideWall_L"), CubeMesh, PandalCenter + FVector(250.0f, -800.0f, 380.0f), FRotator::ZeroRotator, FVector(9.5f, 0.8f, 6.8f), true, DarkMaterial);
	CreateMeshPiece(TEXT("Pandal_SideWall_R"), CubeMesh, PandalCenter + FVector(250.0f,  800.0f, 380.0f), FRotator::ZeroRotator, FVector(9.5f, 0.8f, 6.8f), true, DarkMaterial);

	// 7. Sub-systems: Deity Sculpture, Diyas, Rangolis, Garlands
	BuildGaneshaShrine(PandalCenter);
	BuildFestivalDiyas(PandalCenter);
	BuildFestivalRangolis(PandalCenter);
	BuildGarlandsAndBanners(PandalCenter);
}

void AFestivalStreetBuilder::BuildGaneshaShrine(const FVector& PandalCenter)
{
	// Altar / Garbhagriha Center (elevated sacred sanctum)
	const FVector AltarCenter = PandalCenter + FVector(240.0f, 0.0f, 85.0f);

	// ── 1. The Singhasan (Multi-Tier Lotus Throne) ──
	// Base Octagonal Dais
	if (ChamferCubeMesh)
	{
		CreateMeshPiece(TEXT("Shrine_Dais_Base"), ChamferCubeMesh, AltarCenter + FVector(0.0f, 0.0f, 25.0f), FRotator::ZeroRotator, FVector(4.8f, 4.8f, 0.6f), true, DarkMaterial);
	}
	// Middle Marble Tier
	CreateMeshPiece(TEXT("Shrine_Dais_Marble"), CylinderMesh, AltarCenter + FVector(0.0f, 0.0f, 65.0f), FRotator::ZeroRotator, FVector(3.8f, 3.8f, 0.45f), true, WhiteMat ? WhiteMat : GrayMaterial);

	// Sacred Lotus Tier (Kamala Asana)
	CreateMeshPiece(TEXT("Shrine_Lotus_Core"), CylinderMesh, AltarCenter + FVector(0.0f, 0.0f, 95.0f), FRotator::ZeroRotator, FVector(3.2f, 3.2f, 0.35f), true, SindoorMat ? SindoorMat : GrayMaterial);

	// Concentric Golden Deity Mat
	CreateMeshPiece(TEXT("Shrine_Golden_Asana"), CylinderMesh, AltarCenter + FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, FVector(2.7f, 2.7f, 0.28f), true, GoldMat ? GoldMat : GrayMaterial);

	// ── 2. The Sacred Form of Lord Ganesha (Vakratunda Mahakaya) ──
	// A. Seated Folded Legs (Padmasana / Lalitasana) with Saffron Dhoti
	CreateMeshPiece(
		TEXT("Ganesh_Legs_Base"),
		CylinderMesh,
		AltarCenter + FVector(0.0f, 0.0f, 160.0f),
		FRotator::ZeroRotator,
		FVector(2.6f, 2.2f, 0.65f),
		true,
		MarigoldMat ? MarigoldMat : GrayMaterial
	);

	// B. Torso (Lambodara - The Great Cosmic Belly)
	CreateMeshPiece(
		TEXT("Ganesh_Torso"),
		CylinderMesh,
		AltarCenter + FVector(0.0f, 0.0f, 240.0f),
		FRotator::ZeroRotator,
		FVector(1.95f, 1.8f, 1.35f),
		true,
		GoldMat ? GoldMat : GrayMaterial
	);
	// Sacred Thread (Janeu) draping diagonally
	CreateMeshPiece(
		TEXT("Ganesh_Janeu"),
		CubeMesh,
		AltarCenter + FVector(30.0f, 0.0f, 250.0f),
		FRotator(25.0f, -30.0f, 0.0f),
		FVector(0.08f, 1.8f, 0.08f),
		false,
		WhiteMat ? WhiteMat : GrayMaterial
	);
	// Sacred Chest Garland (Kanthi Haar)
	CreateMeshPiece(
		TEXT("Ganesh_Haar"),
		CylinderMesh,
		AltarCenter + FVector(10.0f, 0.0f, 280.0f),
		FRotator::ZeroRotator,
		FVector(1.65f, 1.65f, 0.18f),
		false,
		MarigoldMat ? MarigoldMat : GrayMaterial
	);

	// C. Four Divine Arms (Chaturbhuja)
	// Lower Right Arm (Abhaya Mudra — Blessing of Protection)
	CreateMeshPiece(TEXT("Ganesh_Arm_LowerR"), CylinderMesh, AltarCenter + FVector(60.0f, 110.0f, 235.0f), FRotator(45.0f, 25.0f, 0.0f), FVector(0.45f, 0.45f, 1.05f), false, GoldMat ? GoldMat : GrayMaterial);
	CreateMeshPiece(TEXT("Ganesh_Hand_LowerR"), ChamferCubeMesh ? ChamferCubeMesh : CubeMesh, AltarCenter + FVector(115.0f, 140.0f, 265.0f), FRotator::ZeroRotator, FVector(0.35f, 0.45f, 0.5f), false, GoldMat ? GoldMat : GrayMaterial);

	// Lower Left Arm (Holding Golden Modak Bowl)
	CreateMeshPiece(TEXT("Ganesh_Arm_LowerL"), CylinderMesh, AltarCenter + FVector(60.0f, -110.0f, 220.0f), FRotator(40.0f, -35.0f, 0.0f), FVector(0.45f, 0.45f, 1.0f), false, GoldMat ? GoldMat : GrayMaterial);
	// Modak Bowl (Golden Katori)
	CreateMeshPiece(TEXT("Ganesh_ModakBowl"), CylinderMesh, AltarCenter + FVector(105.0f, -115.0f, 225.0f), FRotator::ZeroRotator, FVector(0.6f, 0.6f, 0.22f), false, GoldMat ? GoldMat : GrayMaterial);
	// Miniature Sculpted Modak inside the hand
	CreateMeshPiece(TEXT("Ganesh_HandModak"), CylinderMesh, AltarCenter + FVector(105.0f, -115.0f, 240.0f), FRotator::ZeroRotator, FVector(0.28f, 0.28f, 0.3f), false, GoldMat ? GoldMat : GrayMaterial);

	// Upper Right Arm (Holding Divine Ankusha / Goad)
	CreateMeshPiece(TEXT("Ganesh_Arm_UpperR"), CylinderMesh, AltarCenter + FVector(-20.0f, 125.0f, 280.0f), FRotator(-50.0f, 15.0f, 0.0f), FVector(0.38f, 0.38f, 1.1f), false, GoldMat ? GoldMat : GrayMaterial);
	CreateMeshPiece(TEXT("Ganesh_Ankusha"), CylinderMesh, AltarCenter + FVector(-20.0f, 170.0f, 325.0f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 0.9f), false, GoldMat ? GoldMat : GrayMaterial);

	// Upper Left Arm (Holding Sacred Parashu / Axe)
	CreateMeshPiece(TEXT("Ganesh_Arm_UpperL"), CylinderMesh, AltarCenter + FVector(-20.0f, -125.0f, 280.0f), FRotator(-50.0f, -15.0f, 0.0f), FVector(0.38f, 0.38f, 1.1f), false, GoldMat ? GoldMat : GrayMaterial);
	CreateMeshPiece(TEXT("Ganesh_Parashu"), CylinderMesh, AltarCenter + FVector(-20.0f, -170.0f, 325.0f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 0.9f), false, GoldMat ? GoldMat : GrayMaterial);

	// D. The Divine Elephant Head (Gajanana)
	CreateMeshPiece(
		TEXT("Ganesh_Head"),
		CylinderMesh,
		AltarCenter + FVector(10.0f, 0.0f, 335.0f),
		FRotator::ZeroRotator,
		FVector(1.4f, 1.4f, 1.15f),
		true,
		GoldMat ? GoldMat : GrayMaterial
	);
	// Forehead Crown Bumps (Kumbha)
	if (ChamferCubeMesh)
	{
		CreateMeshPiece(TEXT("Ganesh_Kumbha"), ChamferCubeMesh, AltarCenter + FVector(35.0f, 0.0f, 380.0f), FRotator::ZeroRotator, FVector(0.85f, 0.95f, 0.45f), false, GoldMat ? GoldMat : GrayMaterial);
	}
	// Sacred Tilak Mark on Forehead (Sindoor Crescent)
	CreateMeshPiece(
		TEXT("Ganesh_Tilak"),
		CubeMesh,
		AltarCenter + FVector(95.0f, 0.0f, 375.0f),
		FRotator::ZeroRotator,
		FVector(0.1f, 0.35f, 0.5f),
		false,
		SindoorMat ? SindoorMat : GrayMaterial
	);

	// Broad Fan Ears (Supakarna)
	if (QuarterCylinderMesh)
	{
		CreateMeshPiece(TEXT("Ganesh_Ear_L"), QuarterCylinderMesh, AltarCenter + FVector(0.0f, -125.0f, 335.0f), FRotator(0.0f, -70.0f, 0.0f), FVector(0.3f, 1.5f, 1.3f), false, GoldMat ? GoldMat : GrayMaterial);
		CreateMeshPiece(TEXT("Ganesh_Ear_R"), QuarterCylinderMesh, AltarCenter + FVector(0.0f,  125.0f, 335.0f), FRotator(0.0f, 70.0f, 0.0f), FVector(0.3f, 1.5f, 1.3f), false, GoldMat ? GoldMat : GrayMaterial);
	}

	// Curved Elephant Trunk (Vakratunda) curling towards His left hand
	CreateMeshPiece(TEXT("Ganesh_Trunk_Upper"), CylinderMesh, AltarCenter + FVector(75.0f, -15.0f, 295.0f), FRotator(35.0f, -20.0f, 0.0f), FVector(0.48f, 0.48f, 0.95f), false, GoldMat ? GoldMat : GrayMaterial);
	CreateMeshPiece(TEXT("Ganesh_Trunk_Tip"), CylinderMesh, AltarCenter + FVector(90.0f, -65.0f, 245.0f), FRotator(15.0f, -65.0f, 0.0f), FVector(0.35f, 0.35f, 0.7f), false, GoldMat ? GoldMat : GrayMaterial);

	// Single Ivory Tusk (Ekadanta)
	CreateMeshPiece(TEXT("Ganesh_Tusk_R"), CylinderMesh, AltarCenter + FVector(75.0f, 40.0f, 285.0f), FRotator(45.0f, 15.0f, 0.0f), FVector(0.14f, 0.14f, 0.5f), false, WhiteMat ? WhiteMat : GrayMaterial);

	// E. Golden Regal Mukut (Temple Crown)
	CreateMeshPiece(TEXT("Ganesh_Mukut_Tier1"), CylinderMesh, AltarCenter + FVector(10.0f, 0.0f, 410.0f), FRotator::ZeroRotator, FVector(1.2f, 1.2f, 0.45f), false, GoldMat ? GoldMat : GrayMaterial);
	if (ChamferCubeMesh)
	{
		CreateMeshPiece(TEXT("Ganesh_Mukut_Tier2"), ChamferCubeMesh, AltarCenter + FVector(10.0f, 0.0f, 450.0f), FRotator::ZeroRotator, FVector(0.85f, 0.85f, 0.5f), false, GoldMat ? GoldMat : GrayMaterial);
	}
	CreateMeshPiece(TEXT("Ganesh_Mukut_Spire"), CylinderMesh, AltarCenter + FVector(10.0f, 0.0f, 495.0f), FRotator::ZeroRotator, FVector(0.45f, 0.45f, 0.7f), false, GoldMat ? GoldMat : GrayMaterial);

	// F. Prabhavali (Divine Golden Aureole Halo Arch)
	if (CircularBandMesh)
	{
		CreateMeshPiece(
			TEXT("Ganesh_Prabhavali_Ring"),
			CircularBandMesh,
			AltarCenter + FVector(-60.0f, 0.0f, 350.0f),
			FRotator(90.0f, 0.0f, 0.0f),
			FVector(4.2f, 0.35f, 4.2f),
			false,
			GoldMat ? GoldMat : GrayMaterial
		);
	}
	else
	{
		CreateMeshPiece(
			TEXT("Ganesh_Prabhavali_Disc"),
			CylinderMesh,
			AltarCenter + FVector(-60.0f, 0.0f, 350.0f),
			FRotator(90.0f, 0.0f, 0.0f),
			FVector(4.0f, 0.2f, 4.0f),
			false,
			GoldMat ? GoldMat : GrayMaterial
		);
	}

	// ── 3. Altar Puja Offerings ──
	// A. Ceremonial Modak Thali (Golden Offering Platter)
	const FVector ThaliPos = AltarCenter + FVector(115.0f, 0.0f, 85.0f);
	CreateMeshPiece(TEXT("Altar_ModakThali"), CylinderMesh, ThaliPos, FRotator::ZeroRotator, FVector(1.4f, 1.4f, 0.09f), false, GoldMat ? GoldMat : GrayMaterial);
	// Piled Modak Sweets on the Thali (7 sacred modaks)
	for (int32 m = 0; m < 6; ++m)
	{
		float Angle = m * 60.0f;
		float Rad = FMath::DegreesToRadians(Angle);
		FVector ModakOffset(FMath::Cos(Rad) * 35.0f, FMath::Sin(Rad) * 35.0f, 12.0f);
		CreateMeshPiece(FString::Printf(TEXT("Altar_Modak_%d"), m), CylinderMesh, ThaliPos + ModakOffset, FRotator::ZeroRotator, FVector(0.24f, 0.24f, 0.35f), false, GoldMat ? GoldMat : GrayMaterial);
	}
	// Apex Modak
	CreateMeshPiece(TEXT("Altar_Modak_Apex"), CylinderMesh, ThaliPos + FVector(0.0f, 0.0f, 26.0f), FRotator::ZeroRotator, FVector(0.28f, 0.28f, 0.4f), false, GoldMat ? GoldMat : GrayMaterial);

	// B. Brass Puja Bell (Ghanti)
	CreateMeshPiece(TEXT("Altar_PujaBell_Base"), CylinderMesh, AltarCenter + FVector(125.0f, -85.0f, 85.0f), FRotator::ZeroRotator, FVector(0.35f, 0.35f, 0.4f), false, GoldMat ? GoldMat : GrayMaterial);
	CreateMeshPiece(TEXT("Altar_PujaBell_Handle"), CylinderMesh, AltarCenter + FVector(125.0f, -85.0f, 110.0f), FRotator::ZeroRotator, FVector(0.1f, 0.1f, 0.6f), false, GoldMat ? GoldMat : GrayMaterial);

	// C. Incense Holder (Dhoop Stand)
	CreateMeshPiece(TEXT("Altar_DhoopStand"), CylinderMesh, AltarCenter + FVector(125.0f, 85.0f, 85.0f), FRotator::ZeroRotator, FVector(0.35f, 0.35f, 0.3f), false, GoldMat ? GoldMat : GrayMaterial);

	// D. Twin Ceremonial Brass Samai Lamps (Standing Oil Lamps)
	const FVector SamaiPositions[] = {
		AltarCenter + FVector(60.0f, -190.0f, 0.0f),
		AltarCenter + FVector(60.0f,  190.0f, 0.0f)
	};
	for (int32 s = 0; s < 2; ++s)
	{
		const FVector& SPos = SamaiPositions[s];
		CreateMeshPiece(FString::Printf(TEXT("Samai_Base_%d"), s), CylinderMesh, SPos + FVector(0.0f, 0.0f, 15.0f), FRotator::ZeroRotator, FVector(0.7f, 0.7f, 0.2f), false, GoldMat ? GoldMat : GrayMaterial);
		CreateMeshPiece(FString::Printf(TEXT("Samai_Stem_%d"), s), CylinderMesh, SPos + FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 2.2f), false, GoldMat ? GoldMat : GrayMaterial);
		CreateMeshPiece(FString::Printf(TEXT("Samai_Bowl_%d"), s), CylinderMesh, SPos + FVector(0.0f, 0.0f, 230.0f), FRotator::ZeroRotator, FVector(0.75f, 0.75f, 0.25f), false, GoldMat ? GoldMat : GrayMaterial);
		CreateMeshPiece(FString::Printf(TEXT("Samai_Flame_%d"), s), CylinderMesh, SPos + FVector(0.0f, 0.0f, 250.0f), FRotator::ZeroRotator, FVector(0.25f, 0.25f, 0.35f), false, DiyaGlowMat ? DiyaGlowMat : GoldMat);
		CreateFestivalLight(FString::Printf(TEXT("Samai_Light_%d"), s), SPos + FVector(0.0f, 0.0f, 260.0f), FLinearColor(1.0f, 0.60f, 0.12f), 3500.0f, 650.0f);
	}
}

void AFestivalStreetBuilder::BuildFestivalDiyas(const FVector& PandalCenter)
{
	// Symmetrical Diya stations along the ceremonial entrance steps
	const float StepDiyaX[] = { 1260.0f, 1320.0f, 1380.0f, 1440.0f };
	const float StepDiyaZ[] = { 20.0f, 38.0f, 56.0f, 74.0f };

	for (int32 i = 0; i < 4; ++i)
	{
		float X = StepDiyaX[i];
		float Z = StepDiyaZ[i];

		// Left Step Diya
		CreateMeshPiece(FString::Printf(TEXT("StepDiya_L_Base_%d"), i), CylinderMesh, FVector(X, -300.0f, Z), FRotator::ZeroRotator, FVector(0.42f, 0.42f, 0.16f), false, DarkMaterial);
		CreateMeshPiece(FString::Printf(TEXT("StepDiya_L_Flame_%d"), i), CylinderMesh, FVector(X, -300.0f, Z + 12.0f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 0.26f), false, DiyaGlowMat ? DiyaGlowMat : GoldMat);
		CreateFestivalLight(FString::Printf(TEXT("StepDiya_L_Light_%d"), i), FVector(X, -300.0f, Z + 20.0f), FLinearColor(1.0f, 0.55f, 0.10f), 1200.0f, 400.0f);

		// Right Step Diya
		CreateMeshPiece(FString::Printf(TEXT("StepDiya_R_Base_%d"), i), CylinderMesh, FVector(X, 300.0f, Z), FRotator::ZeroRotator, FVector(0.42f, 0.42f, 0.16f), false, DarkMaterial);
		CreateMeshPiece(FString::Printf(TEXT("StepDiya_R_Flame_%d"), i), CylinderMesh, FVector(X, 300.0f, Z + 12.0f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 0.26f), false, DiyaGlowMat ? DiyaGlowMat : GoldMat);
		CreateFestivalLight(FString::Printf(TEXT("StepDiya_R_Light_%d"), i), FVector(X, 300.0f, Z + 20.0f), FLinearColor(1.0f, 0.55f, 0.10f), 1200.0f, 400.0f);
	}
}

void AFestivalStreetBuilder::BuildFestivalRangolis(const FVector& PandalCenter)
{
	// 1. Grand Pandal Entrance Mandala (X = 1520, Y = 0, Z = 86)
	const FVector EntranceRangoliPos(1520.0f, 0.0f, 86.0f);
	CreateMeshPiece(TEXT("Rangoli_Ent_Layer1"), CylinderMesh, EntranceRangoliPos, FRotator::ZeroRotator, FVector(5.2f, 5.2f, 0.015f), false, MarigoldMat ? MarigoldMat : GrayMaterial);
	CreateMeshPiece(TEXT("Rangoli_Ent_Layer2"), CylinderMesh, EntranceRangoliPos + FVector(0.0f, 0.0f, 0.5f), FRotator::ZeroRotator, FVector(4.0f, 4.0f, 0.02f), false, SindoorMat ? SindoorMat : GrayMaterial);
	if (ChamferCubeMesh)
	{
		CreateMeshPiece(TEXT("Rangoli_Ent_Layer3"), ChamferCubeMesh, EntranceRangoliPos + FVector(0.0f, 0.0f, 1.0f), FRotator(0.0f, 45.0f, 0.0f), FVector(2.8f, 2.8f, 0.025f), false, WhiteMat ? WhiteMat : GrayMaterial);
	}
	CreateMeshPiece(TEXT("Rangoli_Ent_Layer4"), CylinderMesh, EntranceRangoliPos + FVector(0.0f, 0.0f, 1.5f), FRotator::ZeroRotator, FVector(1.8f, 1.8f, 0.03f), false, GoldMat ? GoldMat : GrayMaterial);
	CreateMeshPiece(TEXT("Rangoli_Ent_Center"), CylinderMesh, EntranceRangoliPos + FVector(0.0f, 0.0f, 2.0f), FRotator::ZeroRotator, FVector(0.6f, 0.6f, 0.035f), false, SindoorMat ? SindoorMat : GrayMaterial);

	// 2. Central Chowk Plaza Festive Rangoli (X = 800, Y = 0, Z = 45)
	const FVector ChowkRangoliPos(800.0f, 0.0f, 45.0f);
	CreateMeshPiece(TEXT("Rangoli_Chowk_Layer1"), CylinderMesh, ChowkRangoliPos, FRotator::ZeroRotator, FVector(6.0f, 6.0f, 0.015f), false, MarigoldMat ? MarigoldMat : GrayMaterial);
	CreateMeshPiece(TEXT("Rangoli_Chowk_Layer2"), CylinderMesh, ChowkRangoliPos + FVector(0.0f, 0.0f, 0.5f), FRotator::ZeroRotator, FVector(4.5f, 4.5f, 0.02f), false, SindoorMat ? SindoorMat : GrayMaterial);
	if (ChamferCubeMesh)
	{
		CreateMeshPiece(TEXT("Rangoli_Chowk_Layer3"), ChamferCubeMesh, ChowkRangoliPos + FVector(0.0f, 0.0f, 1.0f), FRotator(0.0f, 22.5f, 0.0f), FVector(3.2f, 3.2f, 0.025f), false, GoldMat ? GoldMat : GrayMaterial);
	}
	CreateMeshPiece(TEXT("Rangoli_Chowk_Center"), CylinderMesh, ChowkRangoliPos + FVector(0.0f, 0.0f, 1.5f), FRotator::ZeroRotator, FVector(1.2f, 1.2f, 0.03f), false, WhiteMat ? WhiteMat : GrayMaterial);
}

void AFestivalStreetBuilder::BuildGarlandsAndBanners(const FVector& PandalCenter)
{
	// 1. Portico Entrance Toran (Overhead marigold & mango leaf garland across X = 1420)
	for (int32 g = 0; g < 14; ++g)
	{
		float Y = -650.0f + (g * 100.0f);
		float DropZ = 660.0f - (FMath::Sin((g / 13.0f) * PI) * 35.0f);
		UMaterialInterface* LeafOrFlower = (g % 2 == 0) ? (MarigoldMat ? MarigoldMat : GrayMaterial) : (GreenMat ? GreenMat : DarkMaterial);
		CreateMeshPiece(FString::Printf(TEXT("Portico_Toran_%d"), g), CylinderMesh, FVector(1420.0f, Y, DropZ), FRotator(0.0f, 0.0f, 90.0f), FVector(0.35f, 0.35f, 0.45f), false, LeafOrFlower);
	}

	// 2. Festive Cloth Shamiana Valances hanging beneath the front eaves
	CreateMeshPiece(
		TEXT("Shamiana_Valance_Front"),
		CubeMesh,
		FVector(1420.0f, 0.0f, 690.0f),
		FRotator::ZeroRotator,
		FVector(0.12f, 14.8f, 0.45f),
		false,
		SindoorMat ? SindoorMat : GrayMaterial
	);
	CreateMeshPiece(
		TEXT("Shamiana_Valance_Trim"),
		CubeMesh,
		FVector(1420.0f, 0.0f, 665.0f),
		FRotator::ZeroRotator,
		FVector(0.14f, 14.8f, 0.08f),
		false,
		GoldMat ? GoldMat : GrayMaterial
	);

	// 3. Approach Street Festive Banners (Lining the final stretch from X = 950 to 1350)
	const float BannerX[] = { 950.0f, 1100.0f, 1250.0f };
	for (int32 b = 0; b < 3; ++b)
	{
		float X = BannerX[b];
		// Left Street Banner Pole & Cloth
		CreateMeshPiece(FString::Printf(TEXT("Banner_L_Pole_%d"), b), CylinderMesh, FVector(X, -450.0f, 180.0f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 3.8f), false, DarkMaterial);
		CreateMeshPiece(FString::Printf(TEXT("Banner_L_Cloth_%d"), b), CubeMesh, FVector(X, -450.0f, 260.0f), FRotator::ZeroRotator, FVector(0.06f, 0.9f, 1.8f), false, MarigoldMat ? MarigoldMat : GrayMaterial);

		// Right Street Banner Pole & Cloth
		CreateMeshPiece(FString::Printf(TEXT("Banner_R_Pole_%d"), b), CylinderMesh, FVector(X, 450.0f, 180.0f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 3.8f), false, DarkMaterial);
		CreateMeshPiece(FString::Printf(TEXT("Banner_R_Cloth_%d"), b), CubeMesh, FVector(X, 450.0f, 260.0f), FRotator::ZeroRotator, FVector(0.06f, 0.9f, 1.8f), false, SindoorMat ? SindoorMat : GrayMaterial);
	}
}

void AFestivalStreetBuilder::BuildAntiGravityDemonstrationArea()
{
	// North of the Pandal approach at X = 1400, Y = -1150
	const FVector PlazaCenter(1400.0f, -1150.0f, 0.0f);

	// 1. Open Festive Aerial Courtyard Slab (Utsav Rangmanch)
	CreateMeshPiece(
		TEXT("AG_Plaza_Floor"),
		CubeMesh,
		PlazaCenter + FVector(0.0f, 0.0f, 15.0f),
		FRotator::ZeroRotator,
		FVector(13.0f, 13.0f, 0.4f),
		true,
		GrayMaterial
	);

	// 2. Surrounding Carved Balustrades / Low Walls
	CreateMeshPiece(TEXT("AG_Plaza_Wall_W"), CubeMesh, PlazaCenter + FVector(-650.0f, 0.0f, 65.0f), FRotator::ZeroRotator, FVector(0.6f, 13.0f, 0.8f), true, DarkMaterial);
	CreateMeshPiece(TEXT("AG_Plaza_Wall_N"), CubeMesh, PlazaCenter + FVector(0.0f, -650.0f, 65.0f), FRotator::ZeroRotator, FVector(13.0f, 0.6f, 0.8f), true, DarkMaterial);
	CreateMeshPiece(TEXT("AG_Plaza_Wall_E"), CubeMesh, PlazaCenter + FVector(650.0f, 0.0f, 65.0f), FRotator::ZeroRotator, FVector(0.6f, 13.0f, 0.8f), true, DarkMaterial);

	// 3. Stepped Aerial Observation Plinths (Ideal for launching into Anti-Gravity float)
	CreateMeshPiece(TEXT("AG_LaunchPlinth_1"), CubeMesh, PlazaCenter + FVector(-350.0f, 350.0f, 75.0f), FRotator::ZeroRotator, FVector(2.8f, 2.8f, 1.2f), true, DarkMaterial);
	CreateMeshPiece(TEXT("AG_LaunchPlinth_2"), CubeMesh, PlazaCenter + FVector(-350.0f, 100.0f, 150.0f), FRotator::ZeroRotator, FVector(2.2f, 2.2f, 2.6f), true, DarkMaterial);
	CreateMeshPiece(TEXT("AG_LaunchPlinth_3"), CubeMesh, PlazaCenter + FVector(-350.0f, -150.0f, 240.0f), FRotator::ZeroRotator, FVector(1.8f, 1.8f, 4.4f), true, DarkMaterial);

	// 4. Floating Festive Sky Lanterns (Aakash Kandil) suspended at graceful heights
	const FVector LanternFloatingOffsets[] = {
		FVector(0.0f, 0.0f, 280.0f),
		FVector(250.0f, -200.0f, 420.0f),
		FVector(-200.0f, -250.0f, 540.0f),
		FVector(200.0f, 250.0f, 380.0f),
		FVector(0.0f, -300.0f, 650.0f)
	};

	for (int32 k = 0; k < UE_ARRAY_COUNT(LanternFloatingOffsets); ++k)
	{
		const FVector LanternPos = PlazaCenter + LanternFloatingOffsets[k];

		// Glowing cylindrical lantern body
		CreateMeshPiece(
			FString::Printf(TEXT("AG_SkyLantern_Body_%d"), k),
			CylinderMesh,
			LanternPos,
			FRotator(0.0f, k * 30.0f, 0.0f),
			FVector(0.7f, 0.7f, 0.9f),
			false,
			DiyaGlowMat ? DiyaGlowMat : GoldMat
		);
		// Festive tassel fringe hanging beneath
		CreateMeshPiece(
			FString::Printf(TEXT("AG_SkyLantern_Tassel_%d"), k),
			CylinderMesh,
			LanternPos - FVector(0.0f, 0.0f, 65.0f),
			FRotator::ZeroRotator,
			FVector(0.25f, 0.25f, 0.5f),
			false,
			MarigoldMat ? MarigoldMat : GrayMaterial
		);
		// Ambient warm light from each sky lantern
		CreateFestivalLight(
			FString::Printf(TEXT("AG_Lantern_Light_%d"), k),
			LanternPos,
			FLinearColor(1.0f, 0.65f, 0.15f),
			3000.0f,
			750.0f
		);
	}
}

void AFestivalStreetBuilder::BuildFestivalLighting()
{
	// ── Warm Saffron, Deep Amber & Divine Gold Festive Lighting ──
	const FLinearColor SaffronGold(1.0f, 0.65f, 0.15f);
	const FLinearColor DeepAmber(1.0f, 0.45f, 0.08f);
	const FLinearColor DivineGold(1.0f, 0.85f, 0.35f);

	// 1. Entrance Gate Lanterns
	CreateFestivalLight(TEXT("Light_Gate_L"), FVector(-1380.0f, -480.0f, 450.0f), SaffronGold, 6000.0f, 1200.0f);
	CreateFestivalLight(TEXT("Light_Gate_R"), FVector(-1380.0f,  480.0f, 450.0f), SaffronGold, 6000.0f, 1200.0f);

	// 2. Main Street Procession Lanterns (Rhythmic posts along curbs)
	const float StreetLanternX[] = { -1100.0f, -800.0f, -500.0f, -200.0f, 100.0f, 350.0f, 1250.0f, 1550.0f };
	for (int32 i = 0; i < UE_ARRAY_COUNT(StreetLanternX); ++i)
	{
		float X = StreetLanternX[i];
		FLinearColor Color = (i % 2 == 0) ? SaffronGold : DeepAmber;

		// Left lantern post & light
		CreateMeshPiece(FString::Printf(TEXT("LanternPost_L_%d"), i), CylinderMesh, FVector(X, -490.0f, 180.0f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 3.6f), false);
		CreateFestivalLight(FString::Printf(TEXT("Light_Street_L_%d"), i), FVector(X, -490.0f, 370.0f), Color, 5500.0f, 1100.0f);

		// Right lantern post & light
		CreateMeshPiece(FString::Printf(TEXT("LanternPost_R_%d"), i), CylinderMesh, FVector(X, 490.0f, 180.0f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 3.6f), false);
		CreateFestivalLight(FString::Printf(TEXT("Light_Street_R_%d"), i), FVector(X, 490.0f, 370.0f), Color, 5500.0f, 1100.0f);
	}

	// 3. Central Plaza (Chowk) Illumination
	CreateFestivalLight(TEXT("Chowk_Mast_Light"), FVector(800.0f, 0.0f, 550.0f), DivineGold, 12000.0f, 1800.0f);
	CreateFestivalLight(TEXT("Chowk_Corner_NW"), FVector(550.0f, -600.0f, 300.0f), SaffronGold, 6000.0f, 1200.0f);
	CreateFestivalLight(TEXT("Chowk_Corner_NE"), FVector(1050.0f, -600.0f, 300.0f), DeepAmber, 6000.0f, 1200.0f);
	CreateFestivalLight(TEXT("Chowk_Corner_SW"), FVector(550.0f, 600.0f, 300.0f), DeepAmber, 6000.0f, 1200.0f);
	CreateFestivalLight(TEXT("Chowk_Corner_SE"), FVector(1050.0f, 600.0f, 300.0f), SaffronGold, 6000.0f, 1200.0f);

	// 4. Mandir Gali and Bazaar Gali Lanterns
	CreateFestivalLight(TEXT("Gali_North_Light1"), FVector(800.0f, -1000.0f, 280.0f), DeepAmber, 5000.0f, 1000.0f);
	CreateFestivalLight(TEXT("Gali_North_Light2"), FVector(800.0f, -1450.0f, 250.0f), SaffronGold, 5000.0f, 1000.0f);
	CreateFestivalLight(TEXT("Gali_South_Light1"), FVector(800.0f,  1050.0f, 280.0f), SaffronGold, 5000.0f, 1000.0f);

	// 5. Divine Illumination Inside Ganesh Pandal & Altar
	CreateFestivalLight(TEXT("Pandal_Main_Light"), FVector(2200.0f, 0.0f, 480.0f), DivineGold, 18000.0f, 2400.0f);
	CreateFestivalLight(TEXT("Pandal_Altar_Spot"), FVector(2400.0f, 0.0f, 460.0f), DivineGold, 22000.0f, 1500.0f);
	CreateFestivalLight(TEXT("Pandal_Sanctum_Glow"), FVector(2480.0f, 0.0f, 300.0f), SaffronGold, 14000.0f, 1100.0f);
	CreateFestivalLight(TEXT("Pandal_Entrance_L"), FVector(1420.0f, -400.0f, 380.0f), DeepAmber, 6500.0f, 1000.0f);
	CreateFestivalLight(TEXT("Pandal_Entrance_R"), FVector(1420.0f,  400.0f, 380.0f), DeepAmber, 6500.0f, 1000.0f);
}

void AFestivalStreetBuilder::BuildCombatAndParkourCourtyard()
{
	// Courtyard branches off to the right (Y = 2000, X = 800)
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
	CreateMeshPiece(TEXT("Court_Wall_N"), CubeMesh, CourtCenter + FVector(900.0f, 0.0f, 250.0f), FRotator::ZeroRotator, FVector(0.6f, 18.0f, 5.0f));
	CreateMeshPiece(TEXT("Court_Wall_S"), CubeMesh, CourtCenter + FVector(-900.0f, 0.0f, 250.0f), FRotator::ZeroRotator, FVector(0.6f, 18.0f, 5.0f));
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

	// ── 1. Check Existing Interactables in Level ──
	const FVector ShrineLocation = ActorOrigin + FVector(2300.0f, 0.0f, 90.0f);
	const FVector ModakStallLocation = ActorOrigin + FVector(-450.0f, -540.0f, 90.0f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<AActor*> ExistingInteractables;
	UGameplayStatics::GetAllActorsOfClass(World, AGanapatiInteractable::StaticClass(), ExistingInteractables);

	bool bHasShrine = false;
	bool bHasModakStall = false;

	for (AActor* Act : ExistingInteractables)
	{
		if (Act->ActorHasTag(TEXT("GrandShrine")))
		{
			bHasShrine = true;
			if (AGanapatiInteractable* ShrineAct = Cast<AGanapatiInteractable>(Act))
			{
				ShrineAct->SetTriggersPrayerSequence(true);
			}
		}
		else if (Act->ActorHasTag(TEXT("ModakStall")))
		{
			bHasModakStall = true;
			if (AGanapatiInteractable* ModakAct = Cast<AGanapatiInteractable>(Act))
			{
				ModakAct->SetTriggersPrayerSequence(false);
			}
		}
		else
		{
			// Distance fallback for existing untagged instances
			if (FVector::Dist(Act->GetActorLocation(), ShrineLocation) < 400.0f)
			{
				bHasShrine = true;
				Act->Tags.Add(FName(TEXT("GrandShrine")));
				if (AGanapatiInteractable* ShrineAct = Cast<AGanapatiInteractable>(Act))
				{
					ShrineAct->SetTriggersPrayerSequence(true);
				}
			}
			else if (FVector::Dist(Act->GetActorLocation(), ModakStallLocation) < 400.0f)
			{
				bHasModakStall = true;
				Act->Tags.Add(FName(TEXT("ModakStall")));
				if (AGanapatiInteractable* ModakAct = Cast<AGanapatiInteractable>(Act))
				{
					ModakAct->SetTriggersPrayerSequence(false);
				}
			}
		}
	}

	// ── 2. Spawn Ganesh Pandal Shrine Interactable ──
	if (!bHasShrine)
	{
		AGanapatiInteractable* Shrine = World->SpawnActor<AGanapatiInteractable>(
			AGanapatiInteractable::StaticClass(),
			ShrineLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (Shrine)
		{
			Shrine->Tags.Add(FName(TEXT("GrandShrine")));
			Shrine->SetPromptText(FText::FromString(TEXT("Press [E] to Offer Prayers & Modak to Lord Ganesha")));
			Shrine->SetInteractionMessage(FText::FromString(TEXT("You offered modak and prayers with deep devotion. Lord Vighnaharta blesses your journey!")));
			Shrine->SetRestoresHealth(true);
			Shrine->SetDivineEnergyGranted(50.0f);
			Shrine->SetTriggersPrayerSequence(true);
		}
	}

	// ── 3. Spawn Modak Prasadam Stall Interactable (Phase 5A Subsystem 1) ──
	if (!bHasModakStall)
	{
		AGanapatiInteractable* ModakStall = World->SpawnActor<AGanapatiInteractable>(
			AGanapatiInteractable::StaticClass(),
			ModakStallLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (ModakStall)
		{
			ModakStall->Tags.Add(FName(TEXT("ModakStall")));
			ModakStall->SetPromptText(FText::FromString(TEXT("Press [E] to Taste Sacred Modak Prasadam")));
			ModakStall->SetInteractionMessage(FText::FromString(TEXT("Blessed with Sacred Modak Prasadam! +25 Divine Energy")));
			ModakStall->SetRestoresHealth(false);
			ModakStall->SetDivineEnergyGranted(25.0f);
			ModakStall->SetSingleUse(false);
			ModakStall->SetTriggersPrayerSequence(false);

			if (ModakStall->GetTriggerSphere())
			{
				ModakStall->GetTriggerSphere()->SetSphereRadius(180.0f);
			}

			if (ModakStall->GetInteractableMesh())
			{
				ModakStall->GetInteractableMesh()->SetVisibility(false);
				ModakStall->GetInteractableMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}

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
