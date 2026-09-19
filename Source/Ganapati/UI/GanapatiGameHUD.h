// Copyright Ganapati Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GanapatiGameHUD.generated.h"

class AGanapatiPlayerCharacter;
class UGanapatiInteractionComponent;

/**
 * AGanapatiGameHUD
 *
 * Cinematic festival HUD drawn directly through the Unreal Canvas pipeline.
 * Clean, lightweight, self-contained, and provides instant visual feedback for:
 * - Quest objectives & festival atmosphere banner
 * - Health bar & Anti-gravity mode status
 * - Contextual interaction prompt when near pandal/shrine
 * - Interaction blessing dialog display
 * - Controls reminder overlay
 */
UCLASS()
class GANAPATI_API AGanapatiGameHUD : public AHUD
{
	GENERATED_BODY()

public:
	AGanapatiGameHUD();

	virtual void DrawHUD() override;

protected:
	/** Draws the top cinematic festival objective banner */
	void DrawObjectiveBanner(float ScreenW, float ScreenH);

	/** Draws the health and anti-gravity status bar at bottom left */
	void DrawPlayerStatus(float ScreenW, float ScreenH, AGanapatiPlayerCharacter* PlayerChar);

	/** Draws the contextual interaction prompt or recent blessing message */
	void DrawInteractionOverlay(float ScreenW, float ScreenH, AGanapatiPlayerCharacter* PlayerChar);

	/** Draws the controls reminder card at bottom right */
	void DrawControlsOverlay(float ScreenW, float ScreenH);

	/** Helper to draw a semi-transparent tinted rectangle */
	void DrawTintedBox(float X, float Y, float W, float H, const FLinearColor& Color);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|HUD")
	FLinearColor PrimaryFestiveColor = FLinearColor(1.0f, 0.65f, 0.15f, 1.0f); // Saffron Gold

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ganapati|HUD")
	FLinearColor AccentCyan = FLinearColor(0.2f, 0.9f, 1.0f, 1.0f); // Divine Anti-Grav Cyan
};
