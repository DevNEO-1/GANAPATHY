// Copyright Ganapati Project. All Rights Reserved.

#include "UI/GanapatiGameHUD.h"
#include "Characters/GanapatiPlayerCharacter.h"
#include "Interaction/GanapatiInteractionComponent.h"
#include "Components/GanapatiMovementComponent.h"
#include "GameModes/GanapatiFestivalGameMode.h"
#include "Enemies/GanapatiAsuraCaptain.h"
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
	AGanapatiFestivalGameMode* FestGM = Cast<AGanapatiFestivalGameMode>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr);

	if (FestGM && FestGM->IsPlayingOpeningCinematic())
	{
		DrawOpeningCinematicOverlay(ScreenW, ScreenH);
		return;
	}

	DrawObjectiveBanner(ScreenW, ScreenH);
	DrawCaptainBossBar(ScreenW, ScreenH, FestGM);
	DrawControlsOverlay(ScreenW, ScreenH);

	if (PlayerChar)
	{
		DrawPlayerStatus(ScreenW, ScreenH, PlayerChar);
		DrawInteractionOverlay(ScreenW, ScreenH, PlayerChar);
	}

	if (FestGM && FestGM->IsMountainShrineActivated())
	{
		DrawSummitVictoryCard(ScreenW, ScreenH, FestGM);
	}
}

void AGanapatiGameHUD::DrawOpeningCinematicOverlay(float ScreenW, float ScreenH)
{
	const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float Pulse = 0.85f + 0.15f * FMath::Abs(FMath::Sin(TimeSec * 3.0f));

	// 1. Cinematic Widescreen Letterbox Bars (Top and Bottom)
	const float BarH = FMath::Clamp(ScreenH * 0.12f, 75.0f, 135.0f);
	DrawTintedBox(0.0f, 0.0f, ScreenW, BarH, FLinearColor(0.01f, 0.01f, 0.015f, 0.96f));
	DrawTintedBox(0.0f, ScreenH - BarH, ScreenW, BarH, FLinearColor(0.01f, 0.01f, 0.015f, 0.96f));

	// Golden dividing lines along letterbox inner edges
	DrawTintedBox(0.0f, BarH - 2.5f, ScreenW, 2.5f, FLinearColor(1.0f * Pulse, 0.75f * Pulse, 0.2f, 0.9f));
	DrawTintedBox(0.0f, ScreenH - BarH, ScreenW, 2.5f, FLinearColor(1.0f * Pulse, 0.75f * Pulse, 0.2f, 0.9f));

	// 2. Top Location & Event Header
	const FString HeaderLocation = TEXT("✦ GANESH CHATURTHI FESTIVAL  |  SACRED STREETS OF MAHARASHTRA ✦");
	const float HeaderW = 760.0f;
	const float HeaderX = FMath::Max(20.0f, (ScreenW - HeaderW) * 0.5f);
	DrawText(HeaderLocation, FLinearColor(1.0f, 0.80f, 0.30f, 0.95f), HeaderX, BarH * 0.40f, nullptr, 1.05f);

	// 3. Central Cinematic Title & Narrative Prologue Card
	const float CardW = FMath::Min(860.0f, ScreenW - 60.0f);
	const float CardH = 200.0f;
	const float CardX = (ScreenW - CardW) * 0.5f;
	const float CardY = ScreenH - BarH - CardH - 25.0f;

	// Dark semi-transparent card backing with saffron border
	DrawTintedBox(CardX, CardY, CardW, CardH, FLinearColor(0.02f, 0.02f, 0.035f, 0.82f));
	DrawTintedBox(CardX, CardY, CardW, 3.0f, DivineFullColor * Pulse);
	DrawTintedBox(CardX, CardY + CardH - 2.0f, CardW, 2.0f, PrimaryFestiveColor);

	// Title
	const FString MainTitle = TEXT("GANAPATI: THE DIVINE JOURNEY");
	DrawText(MainTitle, DivineFullColor * Pulse, CardX + 35.0f, CardY + 16.0f, nullptr, 1.55f);

	// Sanskrit Blessing & Subtitle
	const FString Subtitle = TEXT("ॐ श्री गणेशाय नमः  •  PILGRIMAGE OF THE SACRED ASCENT");
	DrawText(Subtitle, PrimaryFestiveColor, CardX + 35.0f, CardY + 54.0f, nullptr, 1.05f);

	// Prologue lines
	const FString LoreLine1 = TEXT("The sacred festival has begun. Devotees gather to celebrate Lord Vighnaharta's divine grace.");
	const FString LoreLine2 = TEXT("Seek the holy blessings, purify the sanctuary from darkness, and ascend Mount Kailash.");
	DrawText(LoreLine1, FLinearColor(0.92f, 0.92f, 0.96f, 0.92f), CardX + 35.0f, CardY + 84.0f, nullptr, 0.88f);
	DrawText(LoreLine2, FLinearColor(0.85f, 0.85f, 0.90f, 0.85f), CardX + 35.0f, CardY + 106.0f, nullptr, 0.88f);

	// Objective Callout Box
	const float ObjBoxW = CardW - 70.0f;
	const float ObjBoxH = 38.0f;
	const float ObjBoxX = CardX + 35.0f;
	const float ObjBoxY = CardY + 138.0f;

	DrawTintedBox(ObjBoxX, ObjBoxY, ObjBoxW, ObjBoxH, FLinearColor(0.12f, 0.08f, 0.02f, 0.85f));
	DrawTintedBox(ObjBoxX, ObjBoxY, 3.0f, ObjBoxH, DivineFullColor);

	const FString FirstObjective = TEXT("FIRST RITE: Seek the blessings of sweetmaker Halwai Anand at the Sweet Stall");
	DrawText(FirstObjective, FLinearColor(1.0f, 0.95f, 0.45f, 1.0f), ObjBoxX + 15.0f, ObjBoxY + 10.0f, nullptr, 1.0f);

	// 4. Skip prompt in bottom letterbox
	const FString SkipPrompt = TEXT("[ SPACE ]  Begin Journey");
	const float SkipW = 240.0f;
	const float SkipX = (ScreenW - SkipW) * 0.5f;
	DrawText(SkipPrompt, FLinearColor(0.85f, 0.85f, 0.85f, 0.75f * Pulse), SkipX, ScreenH - (BarH * 0.60f), nullptr, 0.95f);
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

bool AGanapatiGameHUD::IsDisplayingSacredJourney() const
{
	AGanapatiFestivalGameMode* FestGM = Cast<AGanapatiFestivalGameMode>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr);
	return FestGM && FestGM->IsSacredJourneyUnlocked();
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

	// ── Phase 6C: World Region Discovery Badge ──
	if (FestGM)
	{
		const EWorldRegion Region = FestGM->GetActiveRegion();
		FString RegionName;
		switch (Region)
		{
		case EWorldRegion::FestivalStreet:
			RegionName = TEXT("REGION: FESTIVAL CITY BAZAAR");
			break;
		case EWorldRegion::CourtyardSanctuary:
			RegionName = TEXT("REGION: COURTYARD SANCTUARY");
			break;
		case EWorldRegion::SacredPathAscent:
			RegionName = TEXT("REGION: SACRED PATH ASCENT");
			break;
		case EWorldRegion::MountainThreshold:
			RegionName = TEXT("REGION: THRESHOLD OF MOUNT KAILASH");
			break;
		case EWorldRegion::SacredMountainAscent:
			RegionName = TEXT("REGION: SACRED MOUNTAIN ASCENT");
			break;
		case EWorldRegion::KailashSummitShrine:
			RegionName = TEXT("REGION: KAILASH SUMMIT SHRINE");
			break;
		default:
			break;
		}

		if (!RegionName.IsEmpty())
		{
			DrawText(FString::Printf(TEXT("✦ %s ✦"), *RegionName), FLinearColor(1.0f, 0.85f, 0.45f, 0.85f), BannerX + 15.0f, BannerY - 18.0f, nullptr, 0.85f);
		}
	}

	if (FestGM && FestGM->GetStoryProgressionState() == EStoryProgressionState::SacredJourney)
	{
		// Sacred Journey Objective Banner (Phase 5D Subsystem 1)
		const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		const float GoldPulse = 0.85f + 0.15f * FMath::Abs(FMath::Sin(TimeSec * 3.5f));

		// Golden border top and bottom
		DrawTintedBox(BannerX, BannerY, BannerW, 3.0f, DivineFullColor * GoldPulse);
		DrawTintedBox(BannerX, BannerY + BannerH - 2.0f, BannerW, 2.0f, PrimaryFestiveColor);

		if (FestGM->IsMountainShrineActivated())
		{
			const FString CompleteHeader = TEXT("✦ SACRED PILGRIMAGE FULFILLED — KAILASH COMMUNION ATTAINED ✦");
			const FString CompleteTitle = TEXT("✦ Divine Communion Blessed: Lord Vighnaharta's Eternal Grace Radiant! ✦");
			const FString CompleteDesc = TEXT("The pilgrimage of Mount Kailash is fulfilled! Vertical slice complete. Explore freely with [G] Anti-Gravity.");

			DrawText(CompleteHeader, DivineFullColor * GoldPulse, BannerX + 110.0f, BannerY + 10.0f, nullptr, 1.15f);
			DrawText(CompleteTitle, FLinearColor(1.0f, 0.95f, 0.45f, 1.0f), BannerX + 35.0f, BannerY + 32.0f, nullptr, 1.05f);
			DrawText(CompleteDesc, FLinearColor(0.9f, 0.9f, 0.95f, 0.95f), BannerX + 35.0f, BannerY + 49.0f, nullptr, 0.82f);
		}
		else
		{
			const FString JourneyHeader = TEXT("✦ THE SACRED JOURNEY — PILGRIMAGE OF THE ASCENT ✦");
			const FString ObjTitle = FestGM->GetCurrentObjectiveTitle();
			const FString ObjDesc = FestGM->GetCurrentObjectiveDescription();

			DrawText(JourneyHeader, DivineFullColor * GoldPulse, BannerX + 165.0f, BannerY + 10.0f, nullptr, 1.15f);
			DrawText(ObjTitle, FLinearColor(1.0f, 0.95f, 0.45f, 1.0f), BannerX + 35.0f, BannerY + 32.0f, nullptr, 1.05f);
			DrawText(ObjDesc, FLinearColor(0.9f, 0.9f, 0.95f, 0.95f), BannerX + 35.0f, BannerY + 49.0f, nullptr, 0.82f);
		}
	}
	else if (FestGM && FestGM->IsQuestCompleted())
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

		if (FestGM->IsMountainShrineActivated())
		{
			// Radiant divine gold Kailash Summit Blessed card (Phase 6D Subsystem 4)
			const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
			const float GoldPulse = 0.85f + 0.15f * FMath::Abs(FMath::Sin(TimeSec * 3.5f));

			DrawTintedBox(SkirmishX, SkirmishY, SkirmishW, 2.5f, FLinearColor(1.0f, 0.85f, 0.25f, 0.95f) * GoldPulse);
			DrawTintedBox(SkirmishX, SkirmishY + SkirmishH - 2.0f, SkirmishW, 2.0f, FLinearColor(1.0f, 0.65f, 0.15f, 0.85f));

			const FString SummitBlessedText = TEXT("✦ KAILASH SUMMIT BLESSED — THE SACRED PILGRIMAGE IS FULFILLED ✦");
			DrawText(SummitBlessedText, FLinearColor(1.0f, 0.95f, 0.45f, 1.0f), SkirmishX + 36.0f, SkirmishY + 14.0f, nullptr, 1.05f);
		}
		else if (FestGM->GetStoryProgressionState() == EStoryProgressionState::SacredJourney)
		{
			// Radiant divine gold Sacred Path unlocked card (Phase 5D Subsystem 1)
			const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
			const float GoldPulse = 0.85f + 0.15f * FMath::Abs(FMath::Sin(TimeSec * 3.5f));

			DrawTintedBox(SkirmishX, SkirmishY, SkirmishW, 2.5f, FLinearColor(1.0f, 0.85f, 0.25f, 0.95f) * GoldPulse);
			DrawTintedBox(SkirmishX, SkirmishY + SkirmishH - 2.0f, SkirmishW, 2.0f, FLinearColor(1.0f, 0.65f, 0.15f, 0.85f));

			const FString PathOpenText = TEXT("✦ SACRED PATH UNLOCKED — PROCEED BEYOND THE COURTYARD ✦");
			DrawText(PathOpenText, FLinearColor(1.0f, 0.95f, 0.45f, 1.0f), SkirmishX + 42.0f, SkirmishY + 14.0f, nullptr, 1.05f);
		}
		else if (FestGM->IsCourtyardPurified())
		{
			// Luminous divine gold courtyard purified card (Phase 5C Subsystem 4)
			const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
			const float GoldPulse = 0.85f + 0.15f * FMath::Abs(FMath::Sin(TimeSec * 3.5f));

			DrawTintedBox(SkirmishX, SkirmishY, SkirmishW, 2.5f, FLinearColor(1.0f, 0.85f, 0.25f, 0.95f) * GoldPulse);
			DrawTintedBox(SkirmishX, SkirmishY + SkirmishH - 2.0f, SkirmishW, 2.0f, FLinearColor(1.0f, 0.65f, 0.15f, 0.85f));

			const FString PurifiedText = TEXT("✦ COURTYARD PURIFIED — THE DIVINE SANCTUARY RESTORED ✦");
			DrawText(PurifiedText, FLinearColor(1.0f, 0.92f, 0.45f, 1.0f), SkirmishX + 48.0f, SkirmishY + 14.0f, nullptr, 1.05f);
		}
		else if (bCompleted)
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
	const float BoxY = ScreenH - 155.0f;
	const float BoxW = 295.0f;
	const float BoxH = 125.0f;

	// Background card
	DrawTintedBox(BoxX, BoxY, BoxW, BoxH, FLinearColor(0.02f, 0.02f, 0.04f, 0.78f));
	DrawTintedBox(BoxX, BoxY, 3.0f, BoxH, PrimaryFestiveColor);

	// ── 1. Player Health Bar ──
	const float CurrentHP = PlayerChar->GetCurrentHealth();
	const float MaxHP = FMath::Max(1.0f, PlayerChar->GetMaxHealth());
	const float HealthPercent = FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f);

	DrawText(FString::Printf(TEXT("HEALTH  %.0f / %.0f"), CurrentHP, MaxHP), FLinearColor::White, BoxX + 15.0f, BoxY + 8.0f, nullptr, 0.90f);

	const float BarX = BoxX + 15.0f;
	const float BarW = 260.0f;
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
	const float Pulse = bIsFull ? (0.80f + 0.20f * FMath::Abs(FMath::Sin(TimeSec * 4.5f))) : 1.0f;

	const FLinearColor MeterTextColor = bIsFull
		? FLinearColor(DivineFullColor.R * Pulse, DivineFullColor.G * Pulse, DivineFullColor.B * Pulse, 1.0f)
		: DivineEnergyColor;

	const FString EnergyText = bIsFull
		? TEXT("✦ [Q] DIVINE SHOCKWAVE READY ✦")
		: FString::Printf(TEXT("DIVINE ENERGY  %.0f / %.0f"), CachedDivineEnergy, CachedMaxDivineEnergy);

	DrawText(EnergyText, MeterTextColor, BoxX + 15.0f, BoxY + 38.0f, nullptr, 0.90f);

	const float EnergyBarY = BoxY + 54.0f;

	// Glowing aura border when Divine Shockwave is primed
	if (bIsFull)
	{
		DrawTintedBox(BarX - 2.0f, EnergyBarY - 2.0f, BarW + 4.0f, BarH + 4.0f, FLinearColor(1.0f * Pulse, 0.85f * Pulse, 0.25f, 0.90f * Pulse));
	}

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

	// ── 3. Action Cue or Anti-Gravity Status ──
	UGanapatiMovementComponent* MovComp = PlayerChar->GetGanapatiMovementComponent();
	const bool bAntiGrav = MovComp && MovComp->IsAntiGravityActive();
	AGanapatiFestivalGameMode* FestGM = Cast<AGanapatiFestivalGameMode>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr);
	const bool bAscension = FestGM && FestGM->IsDivineAscensionDiscovered();

	if (bIsFull)
	{
		// Prominent readiness directive
		DrawText(TEXT("PRESS [Q] TO UNLEASH GANESHA'S RADIANCE!"), FLinearColor(1.0f, 0.95f, 0.45f, Pulse), BoxX + 15.0f, BoxY + 68.0f, nullptr, 0.78f);
	}
	else if (bAntiGrav)
	{
		const FString ActiveText = bAscension ? TEXT("✦ DIVINE ASCENSION: ACTIVE [G] ✦") : TEXT("✦ DIVINE LEVITATION: ACTIVE [G] ✦");
		DrawText(ActiveText, AccentCyan, BoxX + 15.0f, BoxY + 68.0f, nullptr, 0.88f);
	}
	else
	{
		const FString StandbyText = bAscension ? TEXT("✦ Divine Ascension Standby [G] ✦") : TEXT("Levitation: Standby (Press [G])");
		const FLinearColor StandbyColor = bAscension ? FLinearColor(0.85f, 0.85f, 0.95f, 0.9f) : FLinearColor(0.6f, 0.6f, 0.65f, 0.8f);
		DrawText(StandbyText, StandbyColor, BoxX + 15.0f, BoxY + 68.0f, nullptr, 0.82f);
	}

	// ── 4. Hint Line ──
	if (bIsFull)
	{
		DrawText(TEXT("Devastating radial divine blast breaks enemy poise"), FLinearColor(0.95f, 0.90f, 0.70f, 0.85f), BoxX + 15.0f, BoxY + 86.0f, nullptr, 0.72f);
	}
	else
	{
		DrawText(TEXT("Melee hits & Shrine offerings restore Divine Energy"), FLinearColor(0.70f, 0.70f, 0.75f, 0.75f), BoxX + 15.0f, BoxY + 86.0f, nullptr, 0.72f);
	}
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
	const float BoxW = 285.0f;
	const float BoxH = 168.0f;
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
		TEXT("Q: Divine Shockwave  |  G: Anti-Grav"),
		TEXT("E: Interact / Prayer  |  V: Shoulder Cam")
	};

	float LineY = BoxY + 28.0f;
	for (const FString& Line : Lines)
	{
		DrawText(Line, FLinearColor(0.85f, 0.85f, 0.85f, 0.85f), BoxX + 15.0f, LineY, nullptr, 0.8f);
		LineY += 19.0f;
	}
}

void AGanapatiGameHUD::DrawCaptainBossBar(float ScreenW, float ScreenH, AGanapatiFestivalGameMode* FestGM)
{
	if (!FestGM)
	{
		return;
	}

	const ECaptainEncounterState EncounterState = FestGM->GetCaptainEncounterState();
	if (EncounterState == ECaptainEncounterState::Dormant && BossBarFadeAlpha <= 0.0f)
	{
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;

	// Handle alpha fade-in on active/intro and graceful fade-out on defeat
	if (EncounterState == ECaptainEncounterState::Intro || EncounterState == ECaptainEncounterState::Active)
	{
		BossBarFadeAlpha = FMath::Min(1.0f, BossBarFadeAlpha + DeltaSeconds * 2.5f);
		BossBarDefeatTimer = 0.0f;
	}
	else if (EncounterState == ECaptainEncounterState::Defeated)
	{
		BossBarDefeatTimer += DeltaSeconds;
		if (BossBarDefeatTimer > 1.2f)
		{
			BossBarFadeAlpha = FMath::Max(0.0f, 1.0f - ((BossBarDefeatTimer - 1.2f) / 2.8f));
		}
	}
	else
	{
		BossBarFadeAlpha = FMath::Max(0.0f, BossBarFadeAlpha - DeltaSeconds * 2.0f);
	}

	if (BossBarFadeAlpha <= 0.001f)
	{
		return;
	}

	AGanapatiAsuraCaptain* Captain = FestGM->GetActiveCaptain();

	// Boss Bar geometry & positioning
	const float BarW = 620.0f;
	const float CardH = 62.0f;
	const float BarX = (ScreenW - BarW) * 0.5f;

	// Anchor below top objective banner; adjust if quest toast or skirmish card is active
	float BarY = (QuestToastRemainingTime > 0.0f) ? 146.0f : 100.0f;
	if (FestGM->IsEncounterActive() || FestGM->IsEncounterCompleted())
	{
		BarY += 48.0f;
	}

	// 1. Obsidian card background
	DrawTintedBox(BarX - 10.0f, BarY, BarW + 20.0f, CardH, FLinearColor(0.02f, 0.02f, 0.03f, 0.85f * BossBarFadeAlpha));

	// 2. Gold filigree frame accents
	DrawTintedBox(BarX - 10.0f, BarY, BarW + 20.0f, 2.5f, FLinearColor(1.0f, 0.75f, 0.2f, 0.95f * BossBarFadeAlpha));
	DrawTintedBox(BarX - 10.0f, BarY + CardH - 2.0f, BarW + 20.0f, 2.0f, FLinearColor(1.0f, 0.6f, 0.15f, 0.80f * BossBarFadeAlpha));

	const bool bIsDefeated = (EncounterState == ECaptainEncounterState::Defeated);

	// 3. Header title
	const bool bIsEnraged = (Captain && Captain->IsEnraged() && !bIsDefeated);
	const FString BossTitle = bIsEnraged
		? TEXT("★ ASURA CAPTAIN — CORRUPTED COMMANDER (ENRAGED) ★")
		: TEXT("★ ASURA CAPTAIN — CORRUPTED COMMANDER ★");
	const float TitleX = bIsEnraged ? (BarX + 70.0f) : (BarX + 115.0f);
	const FLinearColor TitleColor = bIsEnraged
		? FLinearColor(1.0f, 0.55f, 0.15f, BossBarFadeAlpha)
		: FLinearColor(1.0f, 0.88f, 0.4f, BossBarFadeAlpha);
	DrawText(BossTitle, TitleColor, TitleX, BarY + 6.0f, nullptr, 1.05f);

	// 4. Health Bar Slot & Fill
	const float SlotY = BarY + 26.0f;
	const float SlotH = 14.0f;

	// Dark slot frame
	DrawTintedBox(BarX, SlotY, BarW, SlotH, FLinearColor(0.08f, 0.03f, 0.03f, 0.90f * BossBarFadeAlpha));

	float CurrentHP = 0.0f;
	float MaxHP = 250.0f;
	float HealthPercent = 0.0f;

	if (Captain && !bIsDefeated)
	{
		CurrentHP = Captain->GetCurrentHP();
		MaxHP = FMath::Max(1.0f, Captain->GetMaxHP());
		HealthPercent = FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f);
	}

	if (HealthPercent > 0.0f)
	{
		// Deep crimson gradient fill (intensified fiery red when enraged)
		const FLinearColor FillColor = bIsEnraged
			? FLinearColor(1.0f, 0.08f, 0.05f, 0.98f * BossBarFadeAlpha)
			: FLinearColor(0.92f, 0.15f, 0.15f, 0.95f * BossBarFadeAlpha);
		DrawTintedBox(BarX, SlotY, BarW * HealthPercent, SlotH, FillColor);
	}

	// 5. Centered Health text over bar
	FString HPText;
	if (bIsDefeated)
	{
		HPText = TEXT("✦ BANISHED ✦");
	}
	else
	{
		HPText = FString::Printf(TEXT("%.0f / %.0f HP (%.0f%%)"), CurrentHP, MaxHP, HealthPercent * 100.0f);
	}
	DrawText(HPText, FLinearColor(1.0f, 1.0f, 1.0f, BossBarFadeAlpha), BarX + 240.0f, SlotY + 1.0f, nullptr, 0.80f);

	// 6. Live Combat State Badge
	FString StateBadgeText;
	FLinearColor StateBadgeColor = FLinearColor(0.85f, 0.85f, 0.85f, BossBarFadeAlpha);

	if (bIsDefeated)
	{
		StateBadgeText = TEXT("[BANISHED]");
		StateBadgeColor = FLinearColor(0.5f, 0.5f, 0.5f, BossBarFadeAlpha);
	}
	else if (Captain)
	{
		if (Captain->GetAIState() == EAsuraAIState::Attacking)
		{
			if (Captain->GetCurrentAttackPattern() == ECaptainAttackPattern::HeavyCleave)
			{
				StateBadgeText = bIsEnraged ? TEXT("[ENRAGED: HEAVY CLEAVE]") : TEXT("[HEAVY CLEAVE]");
				StateBadgeColor = bIsEnraged ? FLinearColor(1.0f, 0.35f, 0.05f, BossBarFadeAlpha) : FLinearColor(1.0f, 0.4f, 0.1f, BossBarFadeAlpha);
			}
			else
			{
				StateBadgeText = bIsEnraged ? TEXT("[ENRAGED: OVERHEAD SMASH]") : TEXT("[OVERHEAD SMASH]");
				StateBadgeColor = bIsEnraged ? FLinearColor(1.0f, 0.08f, 0.08f, BossBarFadeAlpha) : FLinearColor(1.0f, 0.15f, 0.15f, BossBarFadeAlpha);
			}
		}
		else if (Captain->IsInRecovery())
		{
			StateBadgeText = TEXT("[VULNERABLE - RECOVERY]");
			StateBadgeColor = FLinearColor(1.0f, 0.85f, 0.1f, BossBarFadeAlpha);
		}
		else if (Captain->GetAIState() == EAsuraAIState::Staggered)
		{
			StateBadgeText = TEXT("[STAGGER BREAK]");
			StateBadgeColor = FLinearColor(1.0f, 0.6f, 0.15f, BossBarFadeAlpha);
		}
		else if (Captain->GetAIState() == EAsuraAIState::Chasing)
		{
			StateBadgeText = bIsEnraged ? TEXT("[ENRAGED: ADVANCING]") : TEXT("[ADVANCING]");
			StateBadgeColor = bIsEnraged ? FLinearColor(1.0f, 0.20f, 0.15f, BossBarFadeAlpha) : FLinearColor(0.95f, 0.25f, 0.25f, BossBarFadeAlpha);
		}
		else
		{
			StateBadgeText = bIsEnraged ? TEXT("[ENRAGED]") : TEXT("[ENGAGED]");
			StateBadgeColor = FLinearColor(0.85f, 0.3f, 0.3f, BossBarFadeAlpha);
		}
	}

	if (!StateBadgeText.IsEmpty())
	{
		const float ApproxCharWidth = 8.0f;
		const float TextW = StateBadgeText.Len() * ApproxCharWidth;
		const float BadgeX = (ScreenW - TextW) * 0.5f;
		DrawText(StateBadgeText, StateBadgeColor, BadgeX, BarY + 43.0f, nullptr, 0.85f);
	}
}

void AGanapatiGameHUD::DrawSummitVictoryCard(float ScreenW, float ScreenH, AGanapatiFestivalGameMode* FestGM)
{
	if (!Canvas || !FestGM)
	{
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;

	if (!bSummitVictoryTriggered)
	{
		bSummitVictoryTriggered = true;
		SummitVictoryCardRemainingTime = 14.0f;
	}

	if (SummitVictoryCardRemainingTime > 0.0f)
	{
		SummitVictoryCardRemainingTime = FMath::Max(0.0f, SummitVictoryCardRemainingTime - DeltaSeconds);
	}

	// Only render the full grand card while the celebration timer is active
	// After 14s, the top objective banner remains permanently active with the completion text
	if (SummitVictoryCardRemainingTime <= 0.0f)
	{
		return;
	}

	const float AlphaFade = FMath::Clamp(SummitVictoryCardRemainingTime / 1.0f, 0.0f, 1.0f);
	const float TimeSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float Pulse = 0.85f + 0.15f * FMath::Abs(FMath::Sin(TimeSec * 3.5f));

	const float CardW = FMath::Min(880.0f, ScreenW - 60.0f);
	const float CardH = 240.0f;
	const float CardX = (ScreenW - CardW) * 0.5f;
	const float CardY = ScreenH - CardH - 65.0f;

	// Dark semi-transparent card background
	DrawTintedBox(CardX, CardY, CardW, CardH, FLinearColor(0.02f, 0.02f, 0.035f, 0.90f * AlphaFade));

	// Glowing golden borders top and bottom
	DrawTintedBox(CardX, CardY, CardW, 3.5f, FLinearColor(DivineFullColor.R * Pulse, DivineFullColor.G * Pulse, DivineFullColor.B * Pulse, AlphaFade));
	DrawTintedBox(CardX, CardY + CardH - 2.5f, CardW, 2.5f, FLinearColor(PrimaryFestiveColor.R, PrimaryFestiveColor.G, PrimaryFestiveColor.B, AlphaFade));

	// 1. Grand Victory Title
	const FString VictoryTitle = TEXT("✦ DIVINE COMMUNION ATTAINED ✦");
	DrawText(VictoryTitle, FLinearColor(DivineFullColor.R * Pulse, DivineFullColor.G * Pulse, DivineFullColor.B * Pulse, AlphaFade), CardX + 35.0f, CardY + 16.0f, nullptr, 1.55f);

	// 2. Sanskrit Blessing & Subtitle
	const FString Subtitle = TEXT("ॐ गं गणपतये नमः  •  THE SACRED PILGRIMAGE OF MOUNT KAILASH IS FULFILLED");
	DrawText(Subtitle, FLinearColor(PrimaryFestiveColor.R, PrimaryFestiveColor.G, PrimaryFestiveColor.B, AlphaFade), CardX + 35.0f, CardY + 54.0f, nullptr, 1.05f);

	// 3. Lore narrative
	const FString LoreText = TEXT("Lord Vighnaharta's eternal grace has purified the realm. Darkness is dispelled, and your spirit is perfected.");
	DrawText(LoreText, FLinearColor(0.92f, 0.92f, 0.96f, 0.90f * AlphaFade), CardX + 35.0f, CardY + 82.0f, nullptr, 0.88f);

	// 4. Milestone Recap Badges (2 columns x 2 rows)
	const float BoxW = (CardW - 90.0f) * 0.5f;
	const float BoxH = 30.0f;
	const float Col1X = CardX + 35.0f;
	const float Col2X = CardX + 45.0f + BoxW;
	const float Row1Y = CardY + 112.0f;
	const float Row2Y = CardY + 148.0f;

	const FLinearColor CheckGreen = FLinearColor(0.25f, 1.0f, 0.45f, AlphaFade);
	const FLinearColor BadgeTextCol = FLinearColor(1.0f, 0.95f, 0.80f, AlphaFade);
	const FLinearColor BadgeBg = FLinearColor(0.08f, 0.08f, 0.12f, 0.80f * AlphaFade);

	// Badge 1: Darshan Rites Blessed
	DrawTintedBox(Col1X, Row1Y, BoxW, BoxH, BadgeBg);
	DrawTintedBox(Col1X, Row1Y, 3.0f, BoxH, CheckGreen);
	DrawText(TEXT("✓  Sacred Darshan Rites Blessed"), BadgeTextCol, Col1X + 12.0f, Row1Y + 7.0f, nullptr, 0.90f);

	// Badge 2: Courtyard Purified
	DrawTintedBox(Col1X, Row2Y, BoxW, BoxH, BadgeBg);
	DrawTintedBox(Col1X, Row2Y, 3.0f, BoxH, CheckGreen);
	DrawText(TEXT("✓  Festival Courtyard Purified"), BadgeTextCol, Col1X + 12.0f, Row2Y + 7.0f, nullptr, 0.90f);

	// Badge 3: Asura Captain Vanquished
	DrawTintedBox(Col2X, Row1Y, BoxW, BoxH, BadgeBg);
	DrawTintedBox(Col2X, Row1Y, 3.0f, BoxH, CheckGreen);
	DrawText(TEXT("✓  Asura Captain Vanquished"), BadgeTextCol, Col2X + 12.0f, Row1Y + 7.0f, nullptr, 0.90f);

	// Badge 4: Kailash Summit Conquered
	DrawTintedBox(Col2X, Row2Y, BoxW, BoxH, BadgeBg);
	DrawTintedBox(Col2X, Row2Y, 3.0f, BoxH, CheckGreen);
	DrawText(TEXT("✓  Kailash Summit Conquered"), BadgeTextCol, Col2X + 12.0f, Row2Y + 7.0f, nullptr, 0.90f);

	// 5. Vertical Slice Notice
	const FString CompletionNotice = TEXT("GANAPATI: THE DIVINE JOURNEY  —  Vertical Slice Complete  |  Free Exploration & Anti-Gravity [G] Active");
	DrawText(CompletionNotice, FLinearColor(AccentCyan.R, AccentCyan.G, AccentCyan.B, 0.95f * AlphaFade), CardX + 35.0f, CardY + 196.0f, nullptr, 0.88f);
}
