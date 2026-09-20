// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FestivalStreetBuilder.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class AGanapatiInteractable;
class AGanapatiNPC;
class AGanapatiTrainingDummy;

/**
 * AFestivalStreetBuilder
 *
 * Procedural constructor and manager for the Ganesh Chaturthi festival environment.
 * Builds:
 * 1. Main Festive Street (stone road flanked by village buildings)
 * 2. Bazaar Stalls (Modak/sweet stalls, flower & garland stalls, diya stands)
 * 3. Grand Ganesh Pandal (elevated temple pavilion, grand arch, altar, golden glow)
 * 4. Festive Illuminations (warm saffron & golden lantern lights)
 * 5. Combat & Parkour Courtyard (pillars, wall-jump walls, training dummies)
 * 6. Populates ambient Devotee NPCs and Pandal Interactable shrine
 */
UCLASS()
class GANAPATI_API AFestivalStreetBuilder : public AActor
{
	GENERATED_BODY()

public:
	AFestivalStreetBuilder();

protected:
	virtual void BeginPlay() override;

	/** Constructs the festival street layout */
	void BuildFestivalEnvironment();

	/** Builds the entrance plaza, welcome arch (Maha-Dwar), and player threshold */
	void BuildEntrancePlaza();

	/** Builds the main street road, curbs, and plinths */
	void BuildStreetAndBuildings();

	/** Builds the central festival plaza (Chowk) with rangoli dais and performance stage */
	void BuildCentralPlaza();

	/** Builds side paths and galies (North Mandir Gali and South Bazaar Lane) */
	void BuildSidePaths();

	/** Builds multi-tier vernacular houses and wada facades along both flanks */
	void BuildTownhouses();

	/** Builds bazaar stalls along the street */
	void BuildBazaarStalls();

	/** Builds the grand Ganesh Pandal temple pavilion */
	void BuildGaneshPandal();

	/** Builds festive street lantern lights */
	void BuildFestivalLighting();

	/** Builds the parkour and combat training courtyard */
	void BuildCombatAndParkourCourtyard();

	/** Spawns festival NPCs, shrine interactable, and training dummies */
	void PopulateWorldActors();

	/** Helper to create static mesh component attached to Root */
	UStaticMeshComponent* CreateMeshPiece(
		const FString& PieceName,
		UStaticMesh* Mesh,
		const FVector& RelativeLocation,
		const FRotator& RelativeRotation,
		const FVector& RelativeScale,
		bool bEnableCollision = true,
		UMaterialInterface* CustomMaterial = nullptr
	);

	/** Helper to create warm festive point lights */
	UPointLightComponent* CreateFestivalLight(
		const FString& LightName,
		const FVector& RelativeLocation,
		const FLinearColor& LightColor,
		float Intensity = 5000.0f,
		float AttenuationRadius = 1200.0f
	);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ganapati|Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> RampMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ChamferCubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> QuarterCylinderMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PrototypeGridMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DarkMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GrayMaterial;

	/** Number of wandering NPCs to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Spawning", meta=(ClampMin=1, ClampMax=20))
	int32 NPCSpawnCount = 5;

private:
	bool bHasConstructed = false;
};
