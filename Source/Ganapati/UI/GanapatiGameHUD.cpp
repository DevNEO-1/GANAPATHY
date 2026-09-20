// Copyright Ganapati Project. All Rights Reserved.

#include "UI/GanapatiGameHUD.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "Interaction/GanapatiInteractionComponent.h"
#include "Components/GanapatiMovementComponent.h"
#include "GameModes/GanapatiFestivalGameMode.h"
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

void AGanapatiGameHUD::ShowQuestToast(const FText& InToastText, float InDuration)
{
	ActiveQuestToast = InToastText;
	QuestToastDuration = FMath::Max(0.5f, InDuration);
	QuestToastRemainingTime = QuestToastDuration;
}

void AGanapatiGameHUD::DrawObjectiveBanner(float ScreenW, float ScreenH)
{
	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
	if (QuestToastRemainingTime > 0.0f)
	{
		QuestToastRemainingTime = FMath::Max(0.0f, QuestToastRemainingTime - DeltaSeconds);
	}

	AGanapatiFestivalGameMode* FestGM = Cast<AGanapatiFestivalGameMode>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr);

	// ── 1. Quest Completion Toast (Pulsing celebratory banner when a step completes) ──
	if (QuestToastRemainingTime > 0.0f && !ActiveQuestToast.IsEmpty())
	{
		const float ToastW = 760.0f;
		const float ToastH = 46.0f;
		const float ToastX = (ScreenW - ToastW) * 0.5f;
		const float ToastY = 18.0f;

		const float AlphaFade = FMath::Clamp(QuestToastRemainingTime / 0.5f, 0.0f, 1.0f);
		const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		const float Pulse = 0.85f + 0.15f * FMath::Abs(FMath::Sin(TimeSec * 6.0f));

		// Glowing toast background
		DrawTintedBox(ToastX, ToastY, ToastW, ToastH, FLinearColor(0.04f, 0.03f, 0.01f, 0.90f * AlphaFade));
		DrawTintedBox(ToastX, ToastY, ToastW, 3.0f, FLinearColor(1.0f * Pulse, 0.85f * Pulse, 0.2f, AlphaFade));
		DrawTintedBox(ToastX, ToastY + ToastH - 2.0f, ToastW, 2.0f, FLinearColor(1.0f * Pulse, 0.7f * Pulse, 0.1f, AlphaFade));

		DrawText(ActiveQuestToast.ToString(), FLinearColor(1.0f, 0.92f, 0.45f, AlphaFade), ToastX + 25.0f, ToastY + 12.0f, nullptr, 1.15f);
	}

	// ── 2. Dynamic Sacred Darshan Objective Card ──
	const float BannerW = 780.0f;
	const float BannerH = 68.0f;
	const float BannerX = (ScreenW - BannerW) * 0.5f;
	const float BannerY = (QuestToastRemainingTime > 0.0f) ? 70.0f : 24.0f;

	// Dark semi-transparent background
	DrawTintedBox(BannerX, BannerY, BannerW, BannerH, FLinearColor(0.02f, 0.02f, 0.04f, 0.78f));

	if (FestGM && FestGM->IsQuestCompleted())
	{
		// Celebratory Full Completion Banner
		const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		const float GoldPulse = 0.85f + 0.15f * FMath::Abs(FMath::Sin(TimeSec * 4.0f));

		// Golden border top and bottom
		DrawTintedBox(BannerX, BannerY, BannerW, 3.0f, DivineFullColor * GoldPulse);
		DrawTintedBox(BannerX, BannerY + BannerH - 2.0f, BannerW, 2.0f, PrimaryFestiveColor);

		const FString CompleteTitle = TEXT("✦ SACRED DARSHAN COMPLETED — LORD GANESHA HAS BLESSED YOUR PATH! ✦");
		DrawText(CompleteTitle, DivineFullColor * GoldPulse, BannerX + 60.0f, BannerY + 12.0f, nullptr, 1.20f);

		const FString CompleteDesc = TEXT("All devotional rites performed! Divine energy fully restored. Explore freely with Lord Vighnaharta's grace.");
		DrawText(CompleteDesc, FLinearColor(0.9f, 0.9f, 0.95f, 0.95f), BannerX + 35.0f, BannerY + 40.0f, nullptr, 0.90f);
	}
	else
	{
		// Golden top accent line
		DrawTintedBox(BannerX, BannerY, BannerW, 3.0f, PrimaryFestiveColor);

		FString HeaderText = TEXT("✦ THE SACRED DARSHAN — FESTIVAL PILGRIMAGE ✦");
		FString ObjTitle = FestGM ? FestGM->GetCurrentObjectiveTitle() : TEXT("Explore the festival street and seek Lord Ganesha's blessings.");
		FString ObjDesc = FestGM ? FestGM->GetCurrentObjectiveDescription() : TEXT("Talk to Devotees [E], taste Modak [E], pray at Pandal [E], and practice combat.");

		// Header
		DrawText(HeaderText, PrimaryFestiveColor, BannerX + 160.0f, BannerY + 10.0f, nullptr, 1.15f);

		// Current objective line with step indicator
		DrawText(ObjTitle, FLinearColor(1.0f, 0.95f, 0.8f, 1.0f), BannerX + 35.0f, BannerY + 32.0f, nullptr, 1.0f);

		// Helper prompt text
		DrawText(ObjDesc, FLinearColor(0.75f, 0.75f, 0.8f, 0.85f), BannerX + 35.0f, BannerY + 49.0f, nullptr, 0.80f);
	}

	// ── 3. Active Courtyard Skirmish Combat Card (Phase 5B Subsystem 4) ──
	if (FestGM && (FestGM->IsEncounterActive() || FestGM->IsEncounterCompleted()))
	{
		const float SkirmishW = 660.0f;
		const float SkirmishH = 46.0f;
		const float SkirmishX = (ScreenW - SkirmishW) * 0.5f;
		const float SkirmishY = BannerY + BannerH + 8.0f;

		const bool bCompleted = FestGM->IsEncounterCompleted();
		const int32 Defeated = FestGM->GetEncounterDefeatedCount();
		const int32 Total = FestGM->GetEncounterTotalCount();

		// Dark background
		DrawTintedBox(SkirmishX, SkirmishY, SkirmishW, SkirmishH, FLinearColor(0.03f, 0.02f, 0.02f, 0.85f));

		if (bCompleted)
		{
			// Emerald victory border
			DrawTintedBox(SkirmishX, SkirmishY, SkirmishW, 2.0f, FLinearColor(0.2f, 0.9f, 0.35f, 0.95f));
			DrawTintedBox(SkirmishX, SkirmishY + SkirmishH - 2.0f, SkirmishW, 2.0f, FLinearColor(0.2f, 0.9f, 0.35f, 0.95f));

			const FString VictoryText = FString::Printf(TEXT("⚔ COURTYARD CLEARED — ALL %d ASURA MINIONS BANISHED [ %d / %d ] ⚔"), Total, Defeated, Total);
			DrawText(VictoryText, FLinearColor(0.35f, 1.0f, 0.5f, 1.0f), SkirmishX + 40.0f, SkirmishY + 14.0f, nullptr, 1.05f);
		}
		else
		{
			// Hostile crimson/amber combat border
			DrawTintedBox(SkirmishX, SkirmishY, SkirmishW, 2.0f, FLinearColor(1.0f, 0.3f, 0.15f, 0.95f));
			DrawTintedBox(SkirmishX, SkirmishY + SkirmishH - 2.0f, SkirmishW, 2.0f, FLinearColor(1.0f, 0.5f, 0.15f, 0.85f));

			const FString ObjectiveText = FString::Printf(TEXT("⚔ COURTYARD SKIRMISH: Defeat Corrupted Minions  [ %d / %d ] ⚔"), Defeated, Total);
			DrawText(ObjectiveText, FLinearColor(1.0f, 0.85f, 0.3f, 1.0f), SkirmishX + 55.0f, SkirmishY + 14.0f, nullptr, 1.05f);
		}
	}
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
