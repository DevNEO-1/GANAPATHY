// Copyright Ganapati Project. All Rights Reserved.

#include "Environment/FestivalStreetBuilder.h"
#include "Interaction/GanapatiInteractable.h"
#include "NPCs/GanapatiNPC.h"
#include "Enemies/GanapatiTrainingDummy.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
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

void AFestivalStreetBuilder::BuildGaneshPandal()
{
	// ── Grand Ganesh Pandal Temple Sanctuary ──
	// The sacred culmination of the festival street at X = 2200, Y = 0
	const FVector PandalCenter(2200.0f, 0.0f, 0.0f);

	// 1. Grand Elevated Temple Dais / Mandap Platform
	CreateMeshPiece(
		TEXT("Pandal_Platform"),
		CubeMesh,
		PandalCenter + FVector(0.0f, 0.0f, 40.0f),
		FRotator::ZeroRotator,
		FVector(13.0f, 15.0f, 0.8f),
		true,
		GrayMaterial
	);

	// 2. Ceremonial Approach Steps / Ramp
	if (RampMesh)
	{
		CreateMeshPiece(
			TEXT("Pandal_Ramp"),
			RampMesh,
			PandalCenter + FVector(-750.0f, 0.0f, 0.0f),
			FRotator(0.0f, 0.0f, 0.0f),
			FVector(3.5f, 7.0f, 0.8f)
		);
	}

	// 3. Four Grand Carved Temple Pillars
	const float PillarOffsetX = 480.0f;
	const float PillarOffsetY = 580.0f;
	const float PillarHeight = 6.5f;

	const FVector PillarPositions[] = {
		PandalCenter + FVector(-PillarOffsetX, -PillarOffsetY, 0.0f),
		PandalCenter + FVector(-PillarOffsetX,  PillarOffsetY, 0.0f),
		PandalCenter + FVector( PillarOffsetX, -PillarOffsetY, 0.0f),
		PandalCenter + FVector( PillarOffsetX,  PillarOffsetY, 0.0f)
	};

	for (int32 i = 0; i < 4; ++i)
	{
		const FVector& Pos = PillarPositions[i];

		// Pillar Base Plinth
		if (ChamferCubeMesh)
		{
			CreateMeshPiece(FString::Printf(TEXT("Pandal_PillarBase_%d"), i), ChamferCubeMesh, Pos + FVector(0.0f, 0.0f, 50.0f), FRotator::ZeroRotator, FVector(1.8f, 1.8f, 0.6f));
		}
		// Main Pillar Shaft
		CreateMeshPiece(FString::Printf(TEXT("Pandal_PillarShaft_%d"), i), CylinderMesh, Pos + FVector(0.0f, 0.0f, 365.0f), FRotator::ZeroRotator, FVector(1.4f, 1.4f, PillarHeight));
	}

	// 4. Grand Mandap Canopy / Temple Roof
	CreateMeshPiece(
		TEXT("Pandal_Roof_Main"),
		CubeMesh,
		PandalCenter + FVector(0.0f, 0.0f, 700.0f),
		FRotator::ZeroRotator,
		FVector(14.5f, 16.5f, 0.9f),
		true,
		GrayMaterial
	);

	// Eaves overhang band
	CreateMeshPiece(
		TEXT("Pandal_Roof_Eaves"),
		CubeMesh,
		PandalCenter + FVector(0.0f, 0.0f, 650.0f),
		FRotator::ZeroRotator,
		FVector(15.2f, 17.2f, 0.3f),
		true,
		DarkMaterial
	);

	// 5. Sacred Central Altar / Shrine Pedestal (Garbhagriha)
	CreateMeshPiece(
		TEXT("Ganesh_Altar_Base"),
		CylinderMesh,
		PandalCenter + FVector(150.0f, 0.0f, 110.0f),
		FRotator::ZeroRotator,
		FVector(3.2f, 3.2f, 0.6f),
		true,
		DarkMaterial
	);
	CreateMeshPiece(
		TEXT("Ganesh_Altar_Tier2"),
		CylinderMesh,
		PandalCenter + FVector(150.0f, 0.0f, 150.0f),
		FRotator::ZeroRotator,
		FVector(2.4f, 2.4f, 0.4f),
		true,
		GrayMaterial
	);

	// Symbolic Divine Ganesha Idol Representation
	CreateMeshPiece(
		TEXT("Ganesh_Idol_Body"),
		CylinderMesh,
		PandalCenter + FVector(150.0f, 0.0f, 230.0f),
		FRotator::ZeroRotator,
		FVector(1.6f, 1.6f, 1.2f)
	);
	CreateMeshPiece(
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
		FVector(0.75f, 0.75f, 0.8f)
	);

	// Sacred Back Wall (Prabhavali Arch Backdrop)
	CreateMeshPiece(
		TEXT("Pandal_BackWall"),
		CubeMesh,
		PandalCenter + FVector(580.0f, 0.0f, 380.0f),
		FRotator::ZeroRotator,
		FVector(0.9f, 15.0f, 6.8f)
	);
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

	// 5. Divine Illumination Inside Ganesh Pandal
	CreateFestivalLight(TEXT("Pandal_Main_Light"), FVector(2200.0f, 0.0f, 450.0f), DivineGold, 14000.0f, 2000.0f);
	CreateFestivalLight(TEXT("Pandal_Altar_Glow"), FVector(2350.0f, 0.0f, 260.0f), SaffronGold, 9000.0f, 950.0f);
	CreateFestivalLight(TEXT("Pandal_Entrance_L"), FVector(1700.0f, -400.0f, 320.0f), DeepAmber, 5500.0f, 850.0f);
	CreateFestivalLight(TEXT("Pandal_Entrance_R"), FVector(1700.0f,  400.0f, 320.0f), DeepAmber, 5500.0f, 850.0f);
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

	// ── 1. Spawn Ganesh Pandal Shrine Interactable ──
	// At the altar in front of the idol
	FVector ShrineLocation = ActorOrigin + FVector(2250.0f, 0.0f, 90.0f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<AActor*> ExistingInteractables;
	UGameplayStatics::GetAllActorsOfClass(World, AGanapatiInteractable::StaticClass(), ExistingInteractables);

	if (ExistingInteractables.Num() == 0)
	{
		AGanapatiInteractable* Shrine = World->SpawnActor<AGanapatiInteractable>(
			AGanapatiInteractable::StaticClass(),
			ShrineLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (Shrine)
		{
			UE_LOG(LogTemp, Warning, TEXT("GANAPATI: Ganesh Pandal Shrine SPAWNED at %s"), *ShrineLocation.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GANAPATI: FAILED to spawn Ganesh Pandal Shrine!"));
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
