// Copyright Ganapati Project. All Rights Reserved.

#include "UI/GanapatiGameHUD.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "Interaction/GanapatiInteractionComponent.h"
#include "Components/GanapatiMovementComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"

AGanapatiGameHUD::AGanapatiGameHUD()
{
}

void AGanapatiGameHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const float ScreenW = Canvas->ClipX;
	const float ScreenH = Canvas->ClipY;

	AGanapatiPlayerCharacter* PlayerChar = Cast<AGanapatiPlayerCharacter>(GetOwningPawn());

	DrawObjectiveBanner(ScreenW, ScreenH);
	DrawControlsOverlay(ScreenW, ScreenH);

	if (PlayerChar)
	{
		DrawPlayerStatus(ScreenW, ScreenH, PlayerChar);
		DrawInteractionOverlay(ScreenW, ScreenH, PlayerChar);
	}
}

void AGanapatiGameHUD::DrawTintedBox(float X, float Y, float W, float H, const FLinearColor& Color)
{
	if (!Canvas) return;
	DrawRect(Color, X, Y, W, H);
}

void AGanapatiGameHUD::DrawObjectiveBanner(float ScreenW, float ScreenH)
{
	const float BannerW = 750.0f;
	const float BannerH = 65.0f;
	const float BannerX = (ScreenW - BannerW) * 0.5f;
	const float BannerY = 25.0f;

	// Dark semi-transparent background
	DrawTintedBox(BannerX, BannerY, BannerW, BannerH, FLinearColor(0.02f, 0.02f, 0.04f, 0.75f));
	// Golden top accent line
	DrawTintedBox(BannerX, BannerY, BannerW, 3.0f, PrimaryFestiveColor);

	// Title
	const FString Title = TEXT("✦ VINAYAKA CHATURTHI — FESTIVAL STREET ✦");
	DrawText(Title, PrimaryFestiveColor, BannerX + 130.0f, BannerY + 12.0f, nullptr, 1.25f);

	// Sub-objective
	const FString SubText = TEXT("Explore the bazaar, offer prayers at Lord Ganesha's Pandal [E], and test combat in the courtyard.");
	DrawText(SubText, FLinearColor(0.85f, 0.85f, 0.9f, 0.9f), BannerX + 45.0f, BannerY + 38.0f, nullptr, 0.9f);
}

void AGanapatiGameHUD::DrawPlayerStatus(float ScreenW, float ScreenH, AGanapatiPlayerCharacter* PlayerChar)
{
	// Bind to OnDivineEnergyChanged delegate on the player character
	if (BoundPlayerChar.Get() != PlayerChar)
	{
		if (BoundPlayerChar.IsValid())
		{
			BoundPlayerChar->OnDivineEnergyChanged.RemoveDynamic(this, &AGanapatiGameHUD::HandleDivineEnergyChanged);
		}
		BoundPlayerChar = PlayerChar;
		if (PlayerChar)
		{
			PlayerChar->OnDivineEnergyChanged.AddDynamic(this, &AGanapatiGameHUD::HandleDivineEnergyChanged);
			CachedDivineEnergy = PlayerChar->GetCurrentDivineEnergy();
			CachedMaxDivineEnergy = FMath::Max(1.0f, PlayerChar->GetMaxDivineEnergy());
			CachedDivineEnergyPercent = PlayerChar->GetDivineEnergyPercent();
		}
	}

	const float BoxX = 35.0f;
	const float BoxY = ScreenH - 145.0f;
	const float BoxW = 290.0f;
	const float BoxH = 115.0f;

	// Background card
	DrawTintedBox(BoxX, BoxY, BoxW, BoxH, FLinearColor(0.02f, 0.02f, 0.04f, 0.78f));
	DrawTintedBox(BoxX, BoxY, 3.0f, BoxH, PrimaryFestiveColor);

	// ── 1. Player Health Bar ──
	const float CurrentHP = PlayerChar->GetCurrentHealth();
	const float MaxHP = FMath::Max(1.0f, PlayerChar->GetMaxHealth());
	const float HealthPercent = FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f);

	DrawText(FString::Printf(TEXT("HEALTH  %.0f / %.0f"), CurrentHP, MaxHP), FLinearColor::White, BoxX + 15.0f, BoxY + 8.0f, nullptr, 0.90f);

	const float BarX = BoxX + 15.0f;
	const float BarW = 255.0f;
	const float BarH = 8.0f;

	// Health Bar BG
	const float HealthBarY = BoxY + 24.0f;
	DrawTintedBox(BarX, HealthBarY, BarW, BarH, FLinearColor(0.12f, 0.12f, 0.14f, 0.85f));
	// Health Bar Fill
	const FLinearColor HealthColor = (HealthPercent > 0.3f) ? FLinearColor(0.2f, 0.85f, 0.3f, 1.0f) : FLinearColor(0.9f, 0.2f, 0.2f, 1.0f);
	DrawTintedBox(BarX, HealthBarY, BarW * HealthPercent, BarH, HealthColor);

	// ── 2. Divine Energy (Modak) Meter ──
	const bool bIsFull = CachedDivineEnergyPercent >= 0.999f;
	const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float Pulse = bIsFull ? (0.80f + 0.20f * FMath::Abs(FMath::Sin(TimeSec * 4.0f))) : 1.0f;

	const FLinearColor MeterTextColor = bIsFull
		? FLinearColor(DivineFullColor.R * Pulse, DivineFullColor.G * Pulse, DivineFullColor.B * Pulse, 1.0f)
		: DivineEnergyColor;

	const FString EnergyText = bIsFull
		? TEXT("✦ DIVINE POWER: 100% READY ✦")
		: FString::Printf(TEXT("DIVINE ENERGY  %.0f / %.0f"), CachedDivineEnergy, CachedMaxDivineEnergy);

	DrawText(EnergyText, MeterTextColor, BoxX + 15.0f, BoxY + 38.0f, nullptr, 0.90f);

	const float EnergyBarY = BoxY + 54.0f;
	// Energy Bar BG
	DrawTintedBox(BarX, EnergyBarY, BarW, BarH, FLinearColor(0.12f, 0.12f, 0.14f, 0.85f));

	// Energy Bar Fill
	FLinearColor CurrentMeterFill = bIsFull ? (DivineFullColor * Pulse) : DivineEnergyColor;
	CurrentMeterFill.A = 1.0f;
	DrawTintedBox(BarX, EnergyBarY, BarW * FMath::Clamp(CachedDivineEnergyPercent, 0.0f, 1.0f), BarH, CurrentMeterFill);

	// Subtle glowing accent cap on filled edge
	if (CachedDivineEnergyPercent > 0.02f)
	{
		const float CapX = BarX + (BarW * FMath::Clamp(CachedDivineEnergyPercent, 0.0f, 1.0f)) - 2.0f;
		DrawTintedBox(CapX, EnergyBarY - 1.0f, 3.0f, BarH + 2.0f, bIsFull ? FLinearColor::White : FLinearColor(1.0f, 0.95f, 0.7f, 1.0f));
	}

	// ── 3. Anti-Gravity Indicator ──
	UGanapatiMovementComponent* MovComp = PlayerChar->GetGanapatiMovementComponent();
	const bool bAntiGrav = MovComp && MovComp->IsAntiGravityActive();

	if (bAntiGrav)
	{
		DrawText(TEXT("✦ DIVINE LEVITATION: ACTIVE [G] ✦"), AccentCyan, BoxX + 15.0f, BoxY + 70.0f, nullptr, 0.90f);
	}
	else
	{
		DrawText(TEXT("Levitation: Standby (Press [G])"), FLinearColor(0.6f, 0.6f, 0.65f, 0.8f), BoxX + 15.0f, BoxY + 70.0f, nullptr, 0.85f);
	}

	// ── 4. Modak / Offering Hint ──
	DrawText(TEXT("Melee hits & Shrine offerings generate Divine Energy"), FLinearColor(0.70f, 0.70f, 0.75f, 0.75f), BoxX + 15.0f, BoxY + 90.0f, nullptr, 0.75f);
}

void AGanapatiGameHUD::HandleDivineEnergyChanged(float NewEnergy, float MaxEnergy)
{
	CachedDivineEnergy = NewEnergy;
	CachedMaxDivineEnergy = FMath::Max(1.0f, MaxEnergy);
	CachedDivineEnergyPercent = (CachedMaxDivineEnergy > 0.0f) ? FMath::Clamp(NewEnergy / CachedMaxDivineEnergy, 0.0f, 1.0f) : 0.0f;
}

void AGanapatiGameHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundPlayerChar.IsValid())
	{
		BoundPlayerChar->OnDivineEnergyChanged.RemoveDynamic(this, &AGanapatiGameHUD::HandleDivineEnergyChanged);
		BoundPlayerChar.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void AGanapatiGameHUD::DrawInteractionOverlay(float ScreenW, float ScreenH, AGanapatiPlayerCharacter* PlayerChar)
{
	UGanapatiInteractionComponent* InteractComp = PlayerChar->FindComponentByClass<UGanapatiInteractionComponent>();
	if (!InteractComp)
	{
		return;
	}

	// 1. Interaction Message Banner (after pressing E)
	if (InteractComp->HasActiveMessage())
	{
		const FString Msg = InteractComp->GetLastInteractionMessage().ToString();
		const float MsgW = 600.0f;
		const float MsgH = 50.0f;
		const float MsgX = (ScreenW - MsgW) * 0.5f;
		const float MsgY = ScreenH * 0.68f;

		DrawTintedBox(MsgX, MsgY, MsgW, MsgH, FLinearColor(0.05f, 0.05f, 0.08f, 0.85f));
		DrawTintedBox(MsgX, MsgY, MsgW, 2.0f, PrimaryFestiveColor);
		DrawText(Msg, PrimaryFestiveColor, MsgX + 25.0f, MsgY + 16.0f, nullptr, 1.05f);
	}
	// 2. Proximity Interaction Prompt
	else
	{
		FText Prompt = InteractComp->GetCurrentPrompt();
		if (!Prompt.IsEmpty())
		{
			const FString PromptStr = FString::Printf(TEXT("✦ %s ✦"), *Prompt.ToString());
			const float PromptW = 420.0f;
			const float PromptH = 42.0f;
			const float PromptX = (ScreenW - PromptW) * 0.5f;
			const float PromptY = ScreenH * 0.72f;

			DrawTintedBox(PromptX, PromptY, PromptW, PromptH, FLinearColor(0.04f, 0.04f, 0.06f, 0.85f));
			DrawTintedBox(PromptX, PromptY + PromptH - 2.0f, PromptW, 2.0f, PrimaryFestiveColor);
			DrawText(PromptStr, PrimaryFestiveColor, PromptX + 20.0f, PromptY + 12.0f, nullptr, 1.05f);
		}
	}
}

void AGanapatiGameHUD::DrawControlsOverlay(float ScreenW, float ScreenH)
{
	const float BoxW = 280.0f;
	const float BoxH = 150.0f;
	const float BoxX = ScreenW - BoxW - 30.0f;
	const float BoxY = ScreenH - BoxH - 30.0f;

	// Background box
	DrawTintedBox(BoxX, BoxY, BoxW, BoxH, FLinearColor(0.02f, 0.02f, 0.04f, 0.7f));
	DrawTintedBox(BoxX + BoxW - 3.0f, BoxY, 3.0f, BoxH, PrimaryFestiveColor);

	// Header
	DrawText(TEXT("CONTROLS"), PrimaryFestiveColor, BoxX + 15.0f, BoxY + 8.0f, nullptr, 0.95f);

	// Keybind list
	const FString Lines[] = {
		TEXT("WASD: Move  |  Mouse: Orbit Look"),
		TEXT("Space: Jump / Double Jump"),
		TEXT("Shift: Sprint  |  Ctrl: Dash"),
		TEXT("LMB: Light Combo  |  RMB: Heavy Strike"),
		TEXT("G: Anti-Gravity  |  V: Shoulder Cam"),
		TEXT("E: Interact / Offer Prayers")
	};

	float LineY = BoxY + 28.0f;
	for (const FString& Line : Lines)
	{
		DrawText(Line, FLinearColor(0.85f, 0.85f, 0.85f, 0.85f), BoxX + 15.0f, LineY, nullptr, 0.8f);
		LineY += 19.0f;
	}
}
