// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FestivalStreetBuilder.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;
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

	/** Toggles the temporary courtyard boss encounter ward barrier (Phase 5C Subsystem 3) */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Combat")
	void SetBossBarrierActive(bool bActive);

	/** Toggles the eastern courtyard Sacred Path progression gate (Phase 5D Subsystem 1) */
	UFUNCTION(BlueprintCallable, Category="Ganapati|Story")
	void SetSacredPathUnlocked(bool bUnlocked);

	/** Returns true if Sacred Path gate has been unlocked (Phase 5D Subsystem 1) */
	UFUNCTION(BlueprintPure, Category="Ganapati|Story")
	bool IsSacredPathUnlocked() const { return bSacredPathUnlocked; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	/** Builds the sacred Modak prasadam offering tray and stall decorations */
	void BuildModakPrasadamTray(const FVector& StallCenter);

	/** Builds the grand Ganesh Pandal temple pavilion */
	void BuildGaneshPandal();

	/** Initializes procedural festival materials (Marigold, Sindoor, Gold, Marble, Diya glow) */
	void InitializeFestivalMaterials();

	/** Builds the sacred Ganesha idol and altar inside the sanctum */
	void BuildGaneshaShrine(const FVector& PandalCenter);

	/** Builds celebratory Diyas (terracotta oil lamps) with glowing flames */
	void BuildFestivalDiyas(const FVector& PandalCenter);

	/** Builds traditional Rangoli floor mandalas */
	void BuildFestivalRangolis(const FVector& PandalCenter);

	/** Builds marigold flower garlands, mango leaf torans, and cloth banners */
	void BuildGarlandsAndBanners(const FVector& PandalCenter);

	/** Builds the open festival aerial plaza for anti-gravity showcase */
	void BuildAntiGravityDemonstrationArea();

	/** Builds festive street lantern lights */
	void BuildFestivalLighting();

	/** Builds the parkour and combat training courtyard */
	void BuildCombatAndParkourCourtyard();

	/** Builds the eastern Sacred Path extending beyond the courtyard (Phase 5D Subsystem 1) */
	void BuildSacredPath(const FVector& CourtCenter);

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
	TObjectPtr<UStaticMesh> PlaneMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CircularBandMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PrototypeGridMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DarkMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GrayMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> FlatColorMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GlowMaterial;

	// Dynamic Festival Materials
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MarigoldMat;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SindoorMat;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GoldMat;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WhiteMat;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GreenMat;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DiyaGlowMat;

	/** Temporary courtyard boss ward barrier component (Phase 5C Subsystem 3) */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BossBarrierComp;

	/** Eastern courtyard Sacred Path progression gate barrier component (Phase 5D Subsystem 1) */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> SacredPathBarrierComp;

	/** Ceremonial lanterns along the Sacred Path (Phase 5D Subsystem 1) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> SacredPathLights;

	/** Number of wandering NPCs to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|Spawning", meta=(ClampMin=1, ClampMax=20))
	int32 NPCSpawnCount = 5;

	// ── Phase 6C: World Region & Milestone Discovery Trigger Volumes ──
	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> CourtyardTriggerComp;

	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> SacredPathTriggerComp;

	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> MountainThresholdTriggerComp;

	UFUNCTION()
	void HandleCourtyardBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleSacredPathBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleMountainThresholdBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bHasConstructed = false;
	bool bSacredPathUnlocked = false;
};
