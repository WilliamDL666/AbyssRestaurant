#include "AbyssHUD.h"
#include "AbyssGameMode.h"
#include "SubmarinePawn.h"
#include "DiverPawn.h"
#include "CatchableFish.h"
#include "RestaurantDome.h"
#include "RestaurantCustomer.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "EngineUtils.h"
#include "AbyssLoc.h"
#include "KitchenGame.h"
#include "KitchenCounter.h"
#include "TextureResource.h"
#include "Engine/Texture2D.h"

namespace
{
	const FLinearColor Cyan(0.35f, 0.95f, 1.f, 1.f);
	const FLinearColor CyanDim(0.35f, 0.95f, 1.f, 0.35f);
	const FLinearColor Gold(1.f, 0.78f, 0.32f, 1.f);
	const FLinearColor Green(0.35f, 1.f, 0.45f, 1.f);
	const FLinearColor Red(1.f, 0.32f, 0.25f, 1.f);
	const FLinearColor Purple(0.75f, 0.4f, 1.f, 1.f);
	const FLinearColor Muted(0.7f, 0.82f, 0.85f, 1.f);

	FLinearColor FishColor(int32 Type)
	{
		return Type == 0 ? FLinearColor(1.f, 0.85f, 0.25f) : Type == 1 ? FLinearColor(0.45f, 0.65f, 1.f) : Purple;
	}

	FString Stars(int32 N)
	{
		FString Out;
		for (int32 i = 0; i < N; ++i) Out += TEXT("*");
		return Out;
	}
}

// ---------------------------------------------------------------- primitives

void AAbyssHUD::Panel(float X, float Y, float W, float H, const FLinearColor& C)
{
	FCanvasTileItem T(FVector2D(X, Y), FVector2D(W, H), C);
	T.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(T);
}

void AAbyssHUD::Border(float X, float Y, float W, float H, const FLinearColor& C, float T)
{
	Line(X, Y, X + W, Y, C, T);
	Line(X + W, Y, X + W, Y + H, C, T);
	Line(X + W, Y + H, X, Y + H, C, T);
	Line(X, Y + H, X, Y, C, T);
}

void AAbyssHUD::Line(float X1, float Y1, float X2, float Y2, const FLinearColor& C, float T)
{
	FCanvasLineItem L(FVector2D(X1, Y1), FVector2D(X2, Y2));
	L.SetColor(C);
	L.LineThickness = T * S;
	L.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(L);
}

void AAbyssHUD::Circle(float CX, float CY, float R, const FLinearColor& C, float T, int32 Seg, float A0, float A1)
{
	FVector2D Prev(CX + FMath::Cos(A0) * R, CY + FMath::Sin(A0) * R);
	for (int32 i = 1; i <= Seg; ++i)
	{
		const float A = FMath::Lerp(A0, A1, (float)i / Seg);
		const FVector2D P(CX + FMath::Cos(A) * R, CY + FMath::Sin(A) * R);
		Line(Prev.X, Prev.Y, P.X, P.Y, C, T);
		Prev = P;
	}
}

float AAbyssHUD::TextW(const FString& Str, float Px)
{
	float XL = 0.f, YL = 0.f;
	const float Sc = Px * S / FontBaseH;
	Canvas->TextSize(Font, AbyssLoc::Translate(Str), XL, YL, Sc, Sc);
	return XL;
}

void AAbyssHUD::Text(const FString& Str, float X, float Y, float Px, const FLinearColor& C, bool bCenter, bool bRight)
{
	float DX = 0.f;
	if (bCenter) DX = -TextW(Str, Px) * 0.5f;
	else if (bRight) DX = -TextW(Str, Px);
	FCanvasTextItem T(FVector2D(X + DX, Y), FText::FromString(AbyssLoc::Translate(Str)), Font, C);
	T.Scale = FVector2D(Px * S / FontBaseH);
	T.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f), FVector2D(1.5f, 1.5f));
	T.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(T);
}

void AAbyssHUD::Bar(float X, float Y, float W, float H, float V, const FLinearColor& Fill, int32 Segments)
{
	Panel(X, Y, W, H, FLinearColor(0.f, 0.f, 0.f, 0.55f));
	V = FMath::Clamp(V, 0.f, 1.f);
	if (Segments <= 0)
	{
		Panel(X, Y, W * V, H, Fill);
	}
	else
	{
		const float Gap = 3.f * S;
		const float SW = (W - Gap * (Segments - 1)) / Segments;
		const int32 Lit = FMath::CeilToInt(V * Segments - 0.001f);
		for (int32 i = 0; i < Segments; ++i)
		{
			Panel(X + i * (SW + Gap), Y, SW, H, i < Lit ? Fill : FLinearColor(1.f, 1.f, 1.f, 0.06f));
		}
	}
	Border(X, Y, W, H, FLinearColor(1.f, 1.f, 1.f, 0.25f), 1.f);
}

// ---------------------------------------------------------------- main

void AAbyssHUD::DrawHUD()
{
	if (UITextures.Num() == 0) // warm up the cooking pictures at game start (they may need compiling)
	{
		// warm up every cooking picture the first time so nothing pops in mid-step
		static const TCHAR* Names[] = { TEXT("Panel"), TEXT("Board"), TEXT("Knife"), TEXT("Teppan"), TEXT("Steak_Raw"), TEXT("Steak_Cooked"), TEXT("Plate"),
			TEXT("Ticket"), TEXT("Card"), TEXT("Card_Sel"), TEXT("Slot"), TEXT("Button"), TEXT("Chip_On"), TEXT("Chip_Off"), TEXT("Chip_Done"),
			TEXT("Counter"), TEXT("TicketCard"), TEXT("TicketCard_Sel"), TEXT("Hanger"), TEXT("IngBar"), TEXT("IngSlot"), TEXT("IngSlot_Sel"), TEXT("SideBtn"), TEXT("Badge"),
			TEXT("PillWide"), TEXT("PillMid"), TEXT("Coin"), TEXT("Pearl"), TEXT("Sun"), TEXT("Gear"), TEXT("Plus"), TEXT("AvatarRing"), TEXT("SubBtn"), TEXT("OpenSign"),
			TEXT("XpBar"), TEXT("Steak_Burnt"),
			TEXT("S_Clipboard"), TEXT("S_TileEmpty"), TEXT("S_Tile_Lemon"), TEXT("S_Tile_Parsley"),
			TEXT("S_AvatarRing"), TEXT("S_LvPill"), TEXT("S_CoinPill"), TEXT("S_ShellPill"), TEXT("S_Gear"), TEXT("S_JellyPanel"), TEXT("S_BtnBack"), TEXT("S_SubBtn"),
			TEXT("S_SignOpen"), TEXT("S_SignClosed"), TEXT("S_Round_Serve"), TEXT("S_Badge"), TEXT("S_BtnOk"), TEXT("S_BarGreen"), TEXT("S_BarBlue"),
			TEXT("S_Side_Menu"), TEXT("S_Side_Shop"), TEXT("S_Side_Decor"), TEXT("S_Side_Tasks"), TEXT("S_Side_Codex"), TEXT("S_Side_Upgrade"),
			TEXT("S_Card_Sashimi"), TEXT("S_Card_Grill"), TEXT("S_Card_Platter"), TEXT("S_Card_Soup"),
			TEXT("H_DepthGauge"), TEXT("H_BatteryBar"), TEXT("H_Sonar"), TEXT("H_Compass"), TEXT("H_Cargo"), TEXT("H_Oxygen"), TEXT("H_ToolHarpoon"),
			TEXT("H_ToolNet"), TEXT("H_SelRing"), TEXT("H_ProgRing"), TEXT("H_Reticle"), TEXT("H_Objective"), TEXT("H_Hints"), TEXT("H_FishTag"),
			TEXT("H_Check"), TEXT("H_CheckOn"), TEXT("H_OrderList"), TEXT("H_OrderRow"), TEXT("H_Bubble"), TEXT("H_Happy"), TEXT("H_Angry"),
			TEXT("H_Toast"), TEXT("H_Fridge"), TEXT("H_Marker"), TEXT("H_Summary"), TEXT("H_Star"), TEXT("H_StarOff"), TEXT("H_UpgCard"),
			TEXT("H_UpgBattery"), TEXT("H_UpgLight"), TEXT("H_UpgOxygen"), TEXT("H_UpgDome"), TEXT("H_Coins"), TEXT("H_Logo"), TEXT("H_MenuBtn"),
			TEXT("H_Frame"), TEXT("H_Danger") };
		for (const TCHAR* N : Names) UITex(N);
		for (int32 i = 0; i < 9; ++i) UITex(FString::Printf(TEXT("Ing_%d"), i));
		for (int32 i = 0; i < 5; ++i) UITex(FString::Printf(TEXT("Dish_%d"), i));
		for (int32 i = 1; i < AbyssData::NumSeason; ++i) UITex(FString::Printf(TEXT("Sea_%d"), i));
		UITex(TEXT("Bait")); UITex(TEXT("Pot")); UITex(TEXT("S_ArrowL")); UITex(TEXT("S_ArrowR")); UITex(TEXT("S_Plus")); UITex(TEXT("S_Minus"));
		for (int32 i = 0; i < 3; ++i) UITex(FString::Printf(TEXT("Fillet_%d"), i));
	}

	Super::DrawHUD();
	if (!Canvas) return;
	if (!Font)
	{
		Font = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Font) Font = GEngine->GetLargeFont();
		float XL, YL;
		Canvas->TextSize(Font, TEXT("Ag"), XL, YL, 1.f, 1.f);
		FontBaseH = FMath::Max(YL, 1.f);
	}
	S = FMath::Min(Canvas->ClipY / 1080.f, Canvas->ClipX / 1500.f);
	Time = GetWorld()->GetTimeSeconds();

	AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	ASubmarinePawn* Sub = Cast<ASubmarinePawn>(GetOwningPawn());
	if (!GM) return;

	switch (GM->Phase)
	{
	case EAbyssPhase::Explore:
		if (ADiverPawn* D = Cast<ADiverPawn>(GetOwningPawn())) DrawDive(GM, D);
		else if (Sub) DrawExplore(GM, Sub);
		break;
	case EAbyssPhase::Prep:       DrawPrep(GM); break;
	case EAbyssPhase::Restaurant: DrawRestaurant(GM); break;
	case EAbyssPhase::Summary:    DrawSummary(GM); break;
	case EAbyssPhase::Ending:     DrawEnding(GM); break;
	}
	DrawToast(GM);
}

void AAbyssHUD::DrawToast(AAbyssGameMode* GM)
{
	if (GM->ToastTime <= 0.f || GM->Toast.IsEmpty()) return;
	const float A = FMath::Clamp(GM->ToastTime * 2.f, 0.f, 1.f);
	const float W = TextW(GM->Toast, 19.f) + 48.f * S;
	const float X = Canvas->ClipX * 0.5f - W * 0.5f;
	const float Y = 118.f * S;
	Tex(UITex(TEXT("H_Toast")), X - 30.f * S, Y - 6.f * S, W + 60.f * S, 50.f * S, FLinearColor(1.f, 1.f, 1.f, A));
	FLinearColor C = GM->ToastColor; C.A = A;
	Text(GM->Toast, Canvas->ClipX * 0.5f, Y + 7.f * S, 19.f, C, true);
}

// ---------------------------------------------------------------- explore

void AAbyssHUD::DrawExplore(AAbyssGameMode* GM, ASubmarinePawn* Sub)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float Heading = Sub->GetHeading();
	const FVector SubLoc = Sub->GetActorLocation();

	DrawObjectives(GM);
	DrawCompass(GM, SubLoc, Heading, false);
	DrawCargo(GM, 12.f * S);
	{
		const float X = W - 24.f * S;
		float Y = 150.f * S;
		const FString Hints[] = { TEXT("Claw        [Hold LMB]"), TEXT("Dive out   [E] (hover)"), TEXT("Light            [F]"), FString::Printf(TEXT("Bait  x%d        [B]"), GM->Bait), TEXT("Boost        [Shift]"), TEXT("Up / Down  [Space/Ctrl]"), TEXT("Language     [Tab]") };
		Tex(UITex(TEXT("H_Hints")), X - 214.f * S, Y - 14.f * S, 214.f * S, 7 * 24.f * S + 28.f * S, FLinearColor(1.f, 1.f, 1.f, 0.85f));
		for (const FString& Hn : Hints)
		{
			Text(Hn, X - 192.f * S, Y + 1.f * S, 13.f, FLinearColor(0.85f, 0.95f, 1.f));
			Y += 24.f * S;
		}
		if (!Sub->bLightOn) Text(TEXT("RUNNING DARK - claw range reduced"), W * 0.5f, 150.f * S, 16.f, FLinearColor(0.6f, 0.8f, 1.f, 0.9f), true);
	}

	// --- Depth gauge (bottom-left)
	{
		const float Depth = Sub->GetDepthMeters();
		const float GS = 120.f * S, GX = 14.f * S, GY = H - 228.f * S;
		Tex(UITex(TEXT("H_DepthGauge")), GX, GY, GS, GS);
		Text(TEXT("DEPTH"), GX + GS + 8.f * S, GY + 30.f * S, 15.f, Muted);
		Text(FString::Printf(TEXT("%.1f m"), Depth), GX + GS + 8.f * S, GY + 50.f * S, 30.f, FLinearColor::White);
		if (Depth > 90.f)
		{
			const bool bBlink = FMath::Frac(Time * 1.5f) < 0.6f;
			Text(TEXT("HULL PRESSURE HIGH"), GX + GS + 8.f * S, GY + 88.f * S, 14.f, bBlink ? Red : FLinearColor(1.f, 0.3f, 0.25f, 0.3f));
		}
	}

	// --- Battery (bottom-left)
	{
		const float B = Sub->Battery01;
		const FLinearColor C = B > 0.4f ? Green : B > 0.2f ? Gold : (FMath::Frac(Time * 2.f) < 0.5f ? Red : FLinearColor(0.5f, 0.1f, 0.1f));
		const float X = 14.f * S, Y = H - 82.f * S, BW = 260.f * S, BH = 74.f * S;
		Tex(UITex(TEXT("H_BatteryBar")), X, Y, BW, BH);
		// inner track of the picture: ~9%..86% wide, 33%..67% high
		const float TX = X + BW * 0.1f, TY = Y + BH * 0.36f, TW = BW * 0.74f, TH = BH * 0.28f;
		const int32 Seg = 10;
		for (int32 k = 0; k < Seg; ++k)
		{
			const bool bLit = k < FMath::CeilToInt(B * Seg - 0.001f);
			Panel(TX + TW * k / Seg + 2.f * S, TY, TW / Seg - 4.f * S, TH, bLit ? C : FLinearColor(1.f, 1.f, 1.f, 0.05f));
		}
		Text(TEXT("BATTERY"), X + 16.f * S, Y - 16.f * S, 13.f, Muted);
		Text(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(B * 100.f)), X + BW - 10.f * S, Y - 18.f * S, 16.f, FLinearColor::White, false, true);
		if (B < 0.2f)
		{
			Tex(UITex(TEXT("H_Danger")), 0.f, 0.f, W, H, FLinearColor(1.f, 1.f, 1.f, 0.45f + 0.35f * FMath::Sin(Time * 6.f)));
			Text(TEXT("LOW BATTERY - RETURN TO DOCK"), W * 0.5f, H * 0.28f, 30.f, C, true);
		}
	}

	DrawSonar(GM, SubLoc, Heading, 5000.f);
	DrawFishHints(GM, SubLoc, false);
	if (ACatchableFish* F = Sub->Target.Get())
	{
		DrawFishTag(F, SubLoc, false, Sub->CatchProgress, Sub->bCatching);
	}

	// --- Dock / dive prompt
	if (!Sub->IsInDockZone() && Sub->CanDiveOut() && GM->CargoCount() < AAbyssGameMode::CargoCapacity)
	{
		Text(TEXT("[E] Dive out"), W * 0.5f, H * 0.78f, 22.f, FLinearColor(0.6f, 0.95f, 1.f, 0.85f), true);
	}
	if (Sub->IsInDockZone())
	{
		const FString P = TEXT("[E]  Dock & open the restaurant");
		const float PW = TextW(P, 28.f) + 60.f * S;
		Panel(W * 0.5f - PW * 0.5f, H * 0.72f, PW, 56.f * S, FLinearColor(0.05f, 0.04f, 0.01f, 0.8f));
		Border(W * 0.5f - PW * 0.5f, H * 0.72f, PW, 56.f * S, Gold, 2.f);
		Text(P, W * 0.5f, H * 0.72f + 12.f * S, 28.f, Gold, true);
	}
}

// ---------------------------------------------------------------- restaurant

void AAbyssHUD::DrawTopBar(AAbyssGameMode* GM)
{
	const float W = Canvas->ClipX;
	const float U = S;
	const int32 Lv = 1 + GM->LifetimeServed / 4;
	Tex(UITex(TEXT("S_LvPill")), 84.f * U, 20.f * U, 216.f * U, 71.f * U);
	Text(FString::Printf(TEXT("Lv.%d"), Lv), 128.f * U, 26.f * U, 26.f, FLinearColor::White);
	Panel(128.f * U, 63.f * U, 150.f * U * ((GM->LifetimeServed % 4) / 4.f), 10.f * U, FLinearColor(0.35f, 0.85f, 1.f));
	const float AS = 104.f * U;
	Tex(UITex(TEXT("S_AvatarRing")), 8.f * U, 4.f * U, AS * 93.f / 103.f, AS);
	Tex(UITex(TEXT("AvatarRing")), 22.f * U, 17.f * U, AS * 0.66f, AS * 0.66f);

	const float JW = 380.f * U, JH = JW * 106.f / 374.f, JX = W * 0.5f - JW * 0.5f;
	Tex(UITex(TEXT("S_JellyPanel")), JX, 6.f * U, JW, JH);
	FString DayLine = FString::Printf(TEXT("Day %d"), GM->Day);
	if (ARestaurantDome* Dome = GM->GetDome())
	{
		const int32 Mins = 18 * 60 + FMath::FloorToInt(Dome->NightClock * 2.f);
		DayLine += FString::Printf(TEXT("   %02d:%02d"), (Mins / 60) % 24, Mins % 60);
		Text(FString::Printf(TEXT("Guests tonight: %d left"), Dome->GetRemaining()), W * 0.5f, 6.f * U + JH + 2.f * U, 17.f, FLinearColor(0.85f, 0.92f, 1.f), true);
	}
	Text(DayLine, JX + JW * 0.56f, 6.f * U + JH * 0.5f - 15.f * U, 24.f, FLinearColor(0.2f, 0.16f, 0.3f), true);

	const float GS = 64.f * U, GX = W - GS - 12.f * U;
	Tex(UITex(TEXT("S_Gear")), GX, 10.f * U, GS, GS * 63.f / 60.f);
	const float SPW = 200.f * U, SPH = SPW * 54.f / 170.f, SX = GX - SPW - 12.f * U;
	Tex(UITex(TEXT("S_ShellPill")), SX, 14.f * U, SPW, SPH);
	Text(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(GM->Satisfaction * 100.f)), SX + SPW * 0.5f, 14.f * U + SPH * 0.5f - 13.f * U, 22.f, FLinearColor::White, true);
	const float CPW = 220.f * U, CPH = CPW * 48.f / 185.f, CX = SX - CPW - 12.f * U;
	Tex(UITex(TEXT("S_CoinPill")), CX, 16.f * U, CPW, CPH);
	Text(FString::Printf(TEXT("%d"), GM->Money), CX + CPW * 0.5f, 16.f * U + CPH * 0.5f - 13.f * U, 22.f, FLinearColor::White, true);
}

void AAbyssHUD::DrawRestaurant(AAbyssGameMode* GM)
{
	ARestaurantDome* Dome = GM->GetDome();
	if (!Dome) return;
	if (Dome->Counter.bOpen)
	{
		DrawCounter(GM, Dome);
		DrawToast(GM);
		return;
	}
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	DrawTopBar(GM);

	TArray<ARestaurantCustomer*> Queue;
	Dome->GetQueue(Queue);

	// --- Orders panel
	{
		const float X = 14.f * S, Y = 120.f * S, PW = 400.f * S;
		const int32 Rows = FMath::Min(Queue.Num(), 4);
		const float PH = (96.f + FMath::Max(Rows, 1) * 74.f) * S;
		Tex(UITex(TEXT("H_OrderList")), X, Y, PW, PH);
		Text(FString::Printf(TEXT("Orders (%d)"), Queue.Num()), X + PW * 0.5f, Y + 42.f * S, 24.f, Gold, true);
		if (Rows == 0)
		{
			Text(Dome->GetRemaining() > 0 ? TEXT("Waiting for guests...") : TEXT("All guests served"), X + PW * 0.5f, Y + 98.f * S, 20.f, Muted, true);
		}
		for (int32 i = 0; i < Rows; ++i)
		{
			ARestaurantCustomer* C = Queue[i];
			const float RY = Y + (78.f + i * 74.f) * S;
			const bool bCooking = C->State == ECustomerState::Cooking;
			const int32 DFish = AbyssData::DishFish(C->Dish);
			const bool bNoStock = DFish >= 0 && GM->Stock[DFish] <= 0;
			Tex(UITex(TEXT("H_OrderRow")), X + 18.f * S, RY, PW - 36.f * S, 66.f * S, bCooking ? FLinearColor(0.75f, 1.1f, 0.75f, 1.f) : FLinearColor::White);
			Panel(X + 20.f * S, RY + 10.f * S, 46.f * S, 46.f * S, DFish >= 0 ? FishColor(DFish) * 0.7f : FLinearColor(0.2f, 0.5f, 0.15f));
			Text(FString::Printf(TEXT("%d"), i + 1), X + 43.f * S, RY + 16.f * S, 26.f, FLinearColor::White, true);
			const FLinearColor Ink(0.28f, 0.17f, 0.07f, 1.f);
			Text(AbyssData::DishName(C->Dish), X + 78.f * S, RY + 8.f * S, 20.f, Ink);
			const int32 Secs = FMath::CeilToInt(C->Patience);
			Text(FString::Printf(TEXT("%d:%02d"), Secs / 60, Secs % 60), X + PW - 34.f * S, RY + 8.f * S, 19.f, C->Patience01() < 0.3f ? Red : Ink, false, true);
			const float P = C->Patience01();
			Bar(X + 78.f * S, RY + 40.f * S, PW - 210.f * S, 10.f * S, P, P > 0.5f ? Green : P > 0.25f ? Gold : Red);
			Text(bCooking ? TEXT("COOKING") : bNoStock ? TEXT("NO STOCK") : FString::Printf(TEXT("$%d"), GM->DishFinalPrice(C->Dish, false, GM->SeasonCount(GM->MenuMain[C->Dish]) > 0 ? GM->MenuMain[C->Dish] : 0, GM->SeasonCount(GM->MenuSide[C->Dish]) > 0 ? GM->MenuSide[C->Dish] : 0)),
				X + PW - 34.f * S, RY + 36.f * S, 16.f, bCooking ? FLinearColor(0.1f, 0.5f, 0.15f) : bNoStock ? Red : FLinearColor(0.6f, 0.4f, 0.05f), false, true);
			if (C->Taste != ETaste::None)
			{
				// what this guest fancies
				const FLinearColor TC = AbyssData::TasteColor(C->Taste);
				Panel(X + 78.f * S, RY + 53.f * S, 9.f * S, 9.f * S, TC * 0.75f);
				Text(FString::Printf(TEXT("likes %s"), AbyssData::TasteName(C->Taste)), X + 92.f * S, RY + 49.f * S, 12.f, FLinearColor(0.35f, 0.22f, 0.1f));
			}
		}
	}

	// --- Bubbles over guests
	{
		TArray<ARestaurantCustomer*> All;
		Dome->GetCustomers(All);
		for (ARestaurantCustomer* C : All)
		{
			FVector2D SP;
			if (!GetOwningPlayerController()->ProjectWorldLocationToScreen(C->GetBubbleLocation(), SP)) continue;
			if (C->IsWaitingForFood())
			{
				const int32 QI = Queue.IndexOfByKey(C);
				FString Label = QI != INDEX_NONE && QI < 4 ? FString::Printf(TEXT("[%d] %s"), QI + 1, *AbyssData::DishName(C->Dish)) : AbyssData::DishName(C->Dish);
				if (C->Taste != ETaste::None) Label = AbyssLoc::Translate(Label) + TEXT("  ~ ") + AbyssLoc::Translate(AbyssData::TasteName(C->Taste));
				const float BW = TextW(Label, 17.f) + 44.f * S, BH = 62.f * S;
				Tex(UITex(TEXT("H_Bubble")), SP.X - BW * 0.5f, SP.Y - BH, BW, BH);
				const float P = C->Patience01();
				Panel(SP.X - BW * 0.32f, SP.Y - BH * 0.36f, BW * 0.64f * P, 4.f * S, P > 0.5f ? Green : P > 0.25f ? Gold : Red);
				FCanvasTextItem T(FVector2D(SP.X - BW * 0.5f + 22.f * S, SP.Y - BH + 13.f * S), FText::FromString(AbyssLoc::Translate(Label)), Font, FLinearColor(0.2f, 0.12f, 0.05f));
				T.Scale = FVector2D(17.f * S / FontBaseH);
				Canvas->DrawItem(T);
			}
			else if (C->WantsToOrder())
			{
				const APawn* Me = GetOwningPawn();
				const bool bNear = Me && FVector::Dist2D(Me->GetActorLocation(), C->GetActorLocation()) < GM->GetServeReach();
				const float Bob = FMath::Sin(Time * 4.f + C->Seat) * 4.f * S;
				Disc(SP.X, SP.Y - 30.f * S + Bob, 22.f * S, FLinearColor(1.f, 0.85f, 0.35f, 0.95f), 24);
				Text(TEXT("?"), SP.X, SP.Y - 46.f * S + Bob, 28.f, FLinearColor(0.15f, 0.08f, 0.02f), true);
				const float P = C->Patience01();
				Circle(SP.X, SP.Y - 30.f * S + Bob, 26.f * S, P > 0.5f ? Green : P > 0.25f ? Gold : Red, 3.f, 32, -PI * 0.5f, -PI * 0.5f + 2.f * PI * P);
				if (bNear) Text(TEXT("[E] Take order"), SP.X, SP.Y + 2.f * S, 18.f, FLinearColor::White, true);
			}
			else if (C->State == ECustomerState::Ready && Dome->GetCarryTarget() == C)
			{
				const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * 6.f);
				const FString Lb = FString::Printf(TEXT("Serve: %s"), *AbyssData::DishName(C->Dish));
				const float BW = TextW(Lb, 18.f) + 28.f * S;
				Panel(SP.X - BW * 0.5f, SP.Y - 44.f * S, BW, 40.f * S, FLinearColor(0.1f, 0.5f + 0.2f * Pulse, 0.2f, 0.92f));
				Border(SP.X - BW * 0.5f, SP.Y - 44.f * S, BW, 40.f * S, Green, 2.f);
				Text(Lb, SP.X, SP.Y - 37.f * S, 18.f, FLinearColor::White, true);
				Line(SP.X - 8 * S, SP.Y - 4.f * S, SP.X, SP.Y + 8 * S, Green, 3.f);
				Line(SP.X + 8 * S, SP.Y - 4.f * S, SP.X, SP.Y + 8 * S, Green, 3.f);
			}
			else if (C->State == ECustomerState::Eating)
			{
				const FLinearColor Cl = C->Quality > 0.9f ? Green : C->Quality > 0.6f ? Gold : FLinearColor(1.f, 0.6f, 0.4f);
				const float FS = 44.f * S * (1.f + 0.08f * FMath::Sin(Time * 5.f));
				Tex(UITex(C->Quality > 0.6f ? TEXT("H_Happy") : TEXT("H_Angry")), SP.X - FS * 0.5f, SP.Y - 52.f * S - FS * 0.5f, FS, FS);
				Text(C->bTasteHit && C->Quality > 0.6f ? TEXT("Just my taste!") : C->Quality > 0.9f ? TEXT("Delicious!") : C->Quality > 0.6f ? TEXT("Yum") : TEXT("Hmm..."), SP.X, SP.Y - 26.f * S, 18.f, Cl, true);
			}
			else if (C->State == ECustomerState::Leaving && C->bAngry)
			{
				Tex(UITex(TEXT("H_Angry")), SP.X - 30.f * S, SP.Y - 70.f * S, 60.f * S, 60.f * S * 77.f / 105.f);
			}
		}
	}

	// --- Stock (bottom-left)
	{
		const float X = 14.f * S, Y = H - 84.f * S;
		Tex(UITex(TEXT("H_Fridge")), X, Y, 600.f * S, 72.f * S);
		Text(TEXT("FRIDGE"), X + 118.f * S, Y + 24.f * S, 18.f, FLinearColor(0.85f, 0.95f, 1.f));
		for (int32 i = 0; i < AbyssData::NumFish; ++i)
		{
			const float IX = X + 190.f * S + i * 132.f * S;
			Panel(IX, Y + 30.f * S, 12.f * S, 12.f * S, FishColor(i));
			const FString FT = FString::Printf(TEXT("%s x%d"), i == 0 ? LOCS("Yellowtail", "黄尾鲷") : i == 1 ? LOCS("Tuna", "金枪鱼") : LOCS("Grouper", "石斑鱼"), GM->Stock[i]);
			Text(FT, IX + 18.f * S, Y + 16.f * S, 17.f, FLinearColor::White);
			if (GM->Prime[i] > 0)
			{
				Star(IX + 26.f * S, Y + 50.f * S, 14.f * S);
				Text(FString::Printf(TEXT("%d prime"), GM->Prime[i]), IX + 37.f * S, Y + 41.f * S, 13.f, Gold);
			}
		}
	}

	// --- carrying a dish: hint + arrow toward the guest when the marker is off-screen
	if (ARestaurantCustomer* T = Dome->GetCarryTarget())
	{
		const FString Hint = FString::Printf(TEXT("Carrying %s - bring it to the guest and press [E]"), *AbyssData::DishName(Dome->CarryDish));
		const float HW = TextW(Hint, 20.f) + 40.f * S;
		Panel(W * 0.5f - HW * 0.5f, H - 132.f * S, HW, 40.f * S, FLinearColor(0.05f, 0.25f, 0.1f, 0.85f));
		Border(W * 0.5f - HW * 0.5f, H - 132.f * S, HW, 40.f * S, Green, 1.5f);
		Text(Hint, W * 0.5f, H - 124.f * S, 20.f, FLinearColor::White, true);
		FVector2D SP;
		const bool bOn = GetOwningPlayerController()->ProjectWorldLocationToScreen(T->GetBubbleLocation(), SP) && SP.X > 0.f && SP.Y > 0.f && SP.X < W && SP.Y < H;
		if (!bOn)
		{
			const APawn* Me = GetOwningPawn();
			const APlayerController* PC = GetOwningPlayerController();
			if (Me && PC)
			{
				const FVector To = (T->GetActorLocation() - Me->GetActorLocation()).GetSafeNormal2D();
				const float Rel = FMath::DegreesToRadians(To.Rotation().Yaw - PC->GetControlRotation().Yaw);
				const FVector2D Dir(FMath::Sin(Rel), -FMath::Cos(Rel));
				const FVector2D C0(W * 0.5f, H * 0.5f);
				const FVector2D Tip = C0 + Dir * (FMath::Min(W, H) * 0.42f);
				const FVector2D Side(-Dir.Y, Dir.X);
				Line(Tip.X, Tip.Y, Tip.X - Dir.X * 30.f * S + Side.X * 16.f * S, Tip.Y - Dir.Y * 30.f * S + Side.Y * 16.f * S, Green, 5.f);
				Line(Tip.X, Tip.Y, Tip.X - Dir.X * 30.f * S - Side.X * 16.f * S, Tip.Y - Dir.Y * 30.f * S - Side.Y * 16.f * S, Green, 5.f);
			}
		}
	}
	Text(TEXT("[H] Recipe book"), W - 24.f * S, H - 40.f * S, 16.f, Muted, false, true);

	// --- 3-step cooking minigame: walk to the station, then play that step
	if (Dome->Kitchen.bActive && Dome->Kitchen.bAtStation)
	{
		DrawKitchen(GM, Dome);
	}
	else if (Dome->Kitchen.bActive)
	{
		const EKitchenStep St = Dome->Kitchen.Step;
		const FString Where = FString::Printf(TEXT("Go to the %s"), Dome->GetStationName(St));
		Text(Where, W * 0.5f, H - 110.f * S, 28.f, Gold, true);
		Text(AbyssData::DishName(Dome->Kitchen.Dish), W * 0.5f, H - 72.f * S, 20.f, FLinearColor::White, true);
		FVector2D KP;
		if (GetOwningPlayerController()->ProjectWorldLocationToScreen(Dome->GetStationLocation(St) + FVector(0.f, 0.f, 230.f), KP))
		{
			KP.X = FMath::Clamp(KP.X, 60.f * S, W - 60.f * S);
			KP.Y = FMath::Clamp(KP.Y, 120.f * S, H - 160.f * S);
			const float Bob = FMath::Sin(Time * 4.f) * 6.f * S;
			const FString L = Dome->GetStationName(St);
			const float LW = TextW(L, 22.f) + 28.f * S;
			Panel(KP.X - LW * 0.5f, KP.Y - 44.f * S + Bob, LW, 34.f * S, FLinearColor(0.08f, 0.05f, 0.01f, 0.85f));
			Border(KP.X - LW * 0.5f, KP.Y - 44.f * S + Bob, LW, 34.f * S, Gold, 2.f);
			Text(L, KP.X, KP.Y - 40.f * S + Bob, 22.f, Gold, true);
			Line(KP.X - 10 * S, KP.Y - 6 * S + Bob, KP.X, KP.Y + 6 * S + Bob, Gold, 3.f);
			Line(KP.X + 10 * S, KP.Y - 6 * S + Bob, KP.X, KP.Y + 6 * S + Bob, Gold, 3.f);
		}
	}
	else if (Dome->IsNightActive() && Queue.Num() > 0)
	{
		if (GM->IsNearKitchen())
			Text(TEXT("[E] Cook the next order   (or 1-4 to pick one)"), W * 0.5f, H - 70.f * S, 24.f, FLinearColor(1.f, 0.9f, 0.7f), true);
		else
			Text(TEXT("Walk to the kitchen counter to cook  (WASD)"), W * 0.5f, H - 70.f * S, 24.f, Gold, true);
	}

	// Kitchen marker (shown while not at the counter)
	if (Dome->IsNightActive() && !GM->IsNearKitchen())
	{
		FVector2D KP;
		const FVector K = Dome->GetKitchenLocation() + FVector(0.f, 0.f, 260.f);
		if (GetOwningPlayerController()->ProjectWorldLocationToScreen(K, KP))
		{
			KP.X = FMath::Clamp(KP.X, 60.f * S, W - 60.f * S);
			KP.Y = FMath::Clamp(KP.Y, 120.f * S, H - 160.f * S);
			const float Bob = FMath::Sin(Time * 4.f) * 6.f * S;
			const FString L = TEXT("KITCHEN");
			const float LW = TextW(L, 22.f) + 28.f * S;
			Panel(KP.X - LW * 0.5f, KP.Y - 44.f * S + Bob, LW, 34.f * S, FLinearColor(0.08f, 0.05f, 0.01f, 0.85f));
			Border(KP.X - LW * 0.5f, KP.Y - 44.f * S + Bob, LW, 34.f * S, Gold, 2.f);
			Text(L, KP.X, KP.Y - 40.f * S + Bob, 22.f, Gold, true);
			Line(KP.X - 10 * S, KP.Y - 6 * S + Bob, KP.X, KP.Y + 6 * S + Bob, Gold, 3.f);
			Line(KP.X + 10 * S, KP.Y - 6 * S + Bob, KP.X, KP.Y + 6 * S + Bob, Gold, 3.f);
		}
	}

	if (Dome->ResultTime > 0.f)
	{
		FLinearColor C = Dome->ResultColor;
		C.A = FMath::Clamp(Dome->ResultTime * 1.5f, 0.f, 1.f);
		const float Pop = 1.f + 0.25f * FMath::Clamp(Dome->ResultTime - 1.1f, 0.f, 1.f) * 3.f;
		Text(Dome->ResultText, W * 0.5f, H * 0.42f, 64.f * Pop, C, true);
	}
	if (bRecipeBook) DrawRecipeBook(GM);
}

void AAbyssHUD::DrawSummary(AAbyssGameMode* GM)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	Panel(0.f, 0.f, W, H, FLinearColor(0.f, 0.01f, 0.03f, 0.55f));
	const float PW = 920.f * S, PH = PW * 131.f / 251.f, X = W * 0.5f - PW * 0.5f, Y = 70.f * S;
	Tex(UITex(TEXT("H_Summary")), X, Y, PW, PH);
	Text(FString::Printf(TEXT("Night %d Complete"), GM->Day), W * 0.5f, Y - 6.f * S, 34.f, Gold, true);

	// stars: served ratio, perfect dishes, satisfaction
	const float Ratio = GM->NightCustomers > 0 ? (float)GM->NightServed / GM->NightCustomers : 0.f;
	const int32 NStar = (Ratio >= 0.5f ? 1 : 0) + (Ratio >= 0.85f && GM->NightLeft == 0 ? 1 : 0) + (GM->Satisfaction >= 0.85f ? 1 : 0);
	for (int32 k = 0; k < 3; ++k)
	{
		const float SS = (k == 1 ? 86.f : 70.f) * S;
		const float SX = W * 0.5f + (k - 1) * 96.f * S - SS * 0.5f;
		const float SY = Y + PH * 0.27f - (k == 1 ? 10.f * S : 0.f);
		Tex(UITex(k < NStar ? TEXT("H_Star") : TEXT("H_StarOff")), SX, SY, SS, SS);
	}

	float RY = Y + PH * 0.27f + 84.f * S;
	const float LX = X + PW * 0.28f, RX = X + PW * 0.72f;
	auto Row = [&](const FString& L, const FString& V, const FLinearColor& C)
	{
		Text(L, LX, RY, 18.f, FLinearColor(0.82f, 0.9f, 1.f));
		Text(V, RX, RY, 18.f, C, false, true);
		RY += 23.f * S;
	};
	Row(TEXT("Guests served"), FString::Printf(TEXT("%d / %d"), GM->NightServed, GM->NightCustomers), FLinearColor::White);
	Row(TEXT("Perfect dishes"), FString::FromInt(GM->NightPerfect), Green);
	Row(TEXT("Prime-catch dishes"), FString::FromInt(GM->NightPrimeServed), Gold);
	Row(TEXT("Taste matches (double tip)"), FString::FromInt(GM->NightTasteHits), FLinearColor(0.6f, 1.f, 0.6f));
	Row(TEXT("Guests who left angry"), FString::FromInt(GM->NightLeft), GM->NightLeft > 0 ? Red : FLinearColor::White);
	Row(TEXT("Earned tonight"), FString::Printf(TEXT("$%d"), GM->NightEarned), Gold);
	Row(TEXT("Satisfaction"), FString::Printf(TEXT("%d%%"), FMath::RoundToInt(GM->Satisfaction * 100.f)), FLinearColor::White);


	// upgrade cards
	struct FUp { const TCHAR* Key; const TCHAR* Name; const TCHAR* Img; int32 Lvl; int32 Cost; int32 Max; };
	const FUp Ups[4] = {
		{ TEXT("B"), TEXT("Battery capacity"), TEXT("H_UpgBattery"), GM->BatteryLevel, GM->UpgradeCost(0), AAbyssGameMode::MaxUpgrade },
		{ TEXT("L"), TEXT("Searchlight & claw range"), TEXT("H_UpgLight"), GM->LightLevel, GM->UpgradeCost(1), AAbyssGameMode::MaxUpgrade },
		{ TEXT("O"), TEXT("Diver oxygen tank"), TEXT("H_UpgOxygen"), GM->OxygenLevel, GM->UpgradeCost(2), AAbyssGameMode::MaxUpgrade },
		{ TEXT("R"), TEXT("Restaurant expansion"), TEXT("H_UpgDome"), GM->RestaurantTier, GM->UpgradeCost(3), 2 } };
	const float CW = 180.f * S, CH = CW * 156.f / 119.f, Gap = 24.f * S;
	const float CX0 = W * 0.5f - (4 * CW + 3 * Gap) * 0.5f, CY = Y + PH + 44.f * S;
	Text(FString::Printf(TEXT("UPGRADES   (money: $%d)"), GM->Money), W * 0.5f, CY - 34.f * S, 22.f, Gold, true);
	for (int32 k = 0; k < 4; ++k)
	{
		const FUp& U = Ups[k];
		const float CX = CX0 + k * (CW + Gap);
		const bool bMax = U.Lvl >= U.Max;
		const bool bAfford = GM->Money >= U.Cost;
		Tex(UITex(TEXT("H_UpgCard")), CX, CY, CW, CH, bMax || bAfford ? FLinearColor::White : FLinearColor(0.7f, 0.7f, 0.75f, 1.f));
		const float IS = CW * 0.5f;
		Tex(UITex(U.Img), CX + CW * 0.5f - IS * 0.5f, CY + CH * 0.13f, IS, IS);
		Text(U.Name, CX + CW * 0.5f, CY + CH * 0.6f, 15.f, FLinearColor(0.3f, 0.18f, 0.08f), true);
		Text(FString::Printf(TEXT("[%s]  Lv.%d"), U.Key, U.Lvl), CX + CW * 0.5f, CY + CH * 0.68f, 15.f, FLinearColor(0.3f, 0.18f, 0.08f), true);
		Text(bMax ? FString(TEXT("MAX")) : FString::Printf(TEXT("$%d"), U.Cost), CX + CW * 0.5f, CY + CH * 0.8f, 22.f, bMax ? Muted : bAfford ? Green : Red, true);
	}

	const float BW = 340.f * S, BH = BW * 90.f / 281.f;
	const float BX = W * 0.5f - BW * 0.5f, BY = H - BH - 8.f * S;
	const float Pu = 1.f + 0.02f * FMath::Sin(Time * 4.f);
	Tex(UITex(TEXT("H_MenuBtn")), BX - BW * (Pu - 1.f) * 0.5f, BY, BW * Pu, BH);
	Text(TEXT("[ENTER]  Dive again"), BX + BW * 0.58f, BY + BH * 0.5f - 13.f * S, 22.f, FLinearColor::White, true);
}

// ---------------------------------------------------------------- shared explore widgets

void AAbyssHUD::DrawObjectives(AAbyssGameMode* GM)
{
	// --- Objectives (top-left)
	{
		const float X = 12.f * S, Y = 8.f * S;
		const FLinearColor Ink(0.25f, 0.15f, 0.06f, 1.f), InkDone(0.1f, 0.45f, 0.15f, 1.f);
		Tex(UITex(TEXT("H_Objective")), X, Y, 430.f * S, 224.f * S);
		Text(FString::Printf(TEXT("Day %d  -  Reef Survey"), GM->Day), X + 215.f * S, Y + 84.f * S, 16.f, Ink, true);

		auto Objective = [&](int32 Row, const FString& Str, bool bDone)
		{
			const float OY = Y + (110.f + Row * 22.f) * S;
			Tex(UITex(bDone ? TEXT("H_CheckOn") : TEXT("H_Check")), X + 48.f * S, OY + 1.f * S, 17.f * S, 17.f * S);
			Text(Str, X + 72.f * S, OY, 14.f, bDone ? InkDone : Ink);
		};
		const int32 Cargo = GM->CargoCount();
		Objective(0, FString::Printf(TEXT("Catch fish for tonight  (%d/%d)"), Cargo, AAbyssGameMode::CargoCapacity), Cargo >= 6);
		Objective(1, FString::Printf(TEXT("Find a Rare Grouper near the trench  (%d/1)"), GM->bGrouperFoundToday ? 1 : 0), GM->bGrouperFoundToday);
		Objective(2, TEXT("Dive out [E] and use the net on yellowtail"), GM->Stock[0] > 0);
		Objective(3, TEXT("Return to the docking bay  [E]"), false);
	}

}

void AAbyssHUD::DrawCompass(AAbyssGameMode* GM, const FVector& Origin, float Heading, bool bSubMarker)
{
	// --- Compass (top-center)
	{
		const float CX = Canvas->ClipX * 0.5f, Y = 24.f * S, HalfW = 240.f * S, Span = 75.f;
		Tex(UITex(TEXT("H_Compass")), CX - HalfW - 44.f * S, Y - 16.f * S, 2.f * HalfW + 88.f * S, 64.f * S);
		static const TCHAR* Labels[] = { TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"), TEXT("S"), TEXT("SW"), TEXT("W"), TEXT("NW") };
		for (int32 B = 0; B < 360; B += 15)
		{
			const float D = FMath::FindDeltaAngleDegrees(Heading, (float)B);
			if (FMath::Abs(D) > Span) continue;
			const float X = CX + D / Span * HalfW;
			const float Fade = 1.f - FMath::Abs(D) / Span;
			const bool bMajor = B % 45 == 0;
			if (bMajor)
			{
				Text(Labels[B / 45], X, Y + 2.f * S, B == 0 ? 18.f : 14.f, B == 0 ? FLinearColor(1.f, 0.85f, 0.4f, 0.4f + 0.6f * Fade) : FLinearColor(1.f, 1.f, 1.f, 0.35f + 0.65f * Fade), true);
			}
		}

		auto Marker = [&](const FVector& WorldP, const FLinearColor& C, const FString& Label)
		{
			const float Bearing = (WorldP - Origin).Rotation().Yaw;
			float D = FMath::FindDeltaAngleDegrees(Heading, Bearing);
			D = FMath::Clamp(D, -Span, Span);
			const float X = CX + D / Span * HalfW;
			Tex(UITex(TEXT("H_Marker")), X - 9.f * S, Y + 50.f * S, 18.f * S, 26.f * S, C);
			Text(FString::Printf(TEXT("%s %dm"), *Label, FMath::RoundToInt(FVector::Dist(WorldP, Origin) / 100.f)), X, Y + 76 * S, 13.f, C, true);
		};
		if (ARestaurantDome* Dome = GM->GetDockDome())
		{
			Marker(Dome->GetDockLocation(), Gold, TEXT("DOCK"));
		}
		if (bSubMarker && GM->GetSub())
		{
			Marker(GM->GetSub()->GetActorLocation(), Cyan, TEXT("SUB"));
		}
	}

}

void AAbyssHUD::DrawSonar(AAbyssGameMode* GM, const FVector& Origin, float Heading, float Range)
{
	// --- Sonar (bottom-right)
	{
		const float R = 82.f * S, CX = Canvas->ClipX - 26.f * S - R * 1.32f, CY = Canvas->ClipY - 22.f * S - R * 1.32f;
		const float PR = R * 1.32f;   // the picture's brass rim sits outside the radar area
		Tex(UITex(TEXT("H_Sonar")), CX - PR, CY - PR, 2.f * PR, 2.f * PR);
		const float Sweep = Time * 2.2f;
		for (int32 k = 0; k < 10; ++k)
		{
			const float A = Sweep - k * 0.05f;
			Line(CX, CY, CX + FMath::Cos(A) * R, CY + FMath::Sin(A) * R, FLinearColor(0.35f, 0.95f, 1.f, 0.35f * (1.f - k / 10.f)), 2.f);
		}

		const FRotator YawRot(0.f, Heading, 0.f);
		const FVector Fwd = YawRot.Vector();
		const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		auto ToRadar = [&](const FVector& P, FVector2D& Out) -> bool
		{
			const FVector Rel = P - Origin;
			const float F = FVector::DotProduct(Rel, Fwd), Rt = FVector::DotProduct(Rel, Right);
			if (FMath::Sqrt(F * F + Rt * Rt) > Range) return false;
			Out = FVector2D(CX + Rt / Range * R, CY - F / Range * R);
			return true;
		};

		for (TActorIterator<ACatchableFish> It(GetWorld()); It; ++It)
		{
			if (!It->IsCatchable()) continue;
			FVector2D P;
			if (!ToRadar(It->GetActorLocation(), P)) continue;
			const FLinearColor C = FishColor((int32)It->Type);
			if (It->Type == EFishType::Grouper)
			{
				const float D = 8.f * S;
				Line(P.X - D, P.Y, P.X, P.Y - D, C, 2.5f); Line(P.X, P.Y - D, P.X + D, P.Y, C, 2.5f);
				Line(P.X + D, P.Y, P.X, P.Y + D, C, 2.5f); Line(P.X, P.Y + D, P.X - D, P.Y, C, 2.5f);
			}
			else
			{
				Panel(P.X - 3 * S, P.Y - 3 * S, 6 * S, 6 * S, C);
			}
		}
		if (ARestaurantDome* Dome = GM->GetDockDome())
		{
			FVector2D P;
			if (ToRadar(Dome->GetDockLocation(), P))
			{
				Border(P.X - 6 * S, P.Y - 6 * S, 12 * S, 12 * S, Gold, 2.f);
			}
		}
		if (GM->IsDiving() && GM->GetSub())
		{
			FVector2D P;
			if (ToRadar(GM->GetSub()->GetActorLocation(), P))
			{
				Circle(P.X, P.Y, 7.f * S, Cyan, 2.5f, 12);
			}
		}
		// player arrow
		Line(CX, CY - 10 * S, CX - 7 * S, CY + 7 * S, Cyan, 2.5f);
		Line(CX, CY - 10 * S, CX + 7 * S, CY + 7 * S, Cyan, 2.5f);
		Text(FString::Printf(TEXT("SONAR  %d m"), FMath::RoundToInt(Range / 100.f)), CX, CY - R * 1.32f - 20.f * S, 13.f, Muted, true);
	}

}

void AAbyssHUD::DrawCargo(AAbyssGameMode* GM, float Y)
{
	const float X = Canvas->ClipX - 24.f * S;
	const float PW = 250.f * S, PH = 106.f * S, PX = X - PW;
	Tex(UITex(TEXT("H_Cargo")), PX, Y, PW, PH);
	const float IX = PX + PW * 0.37f;
	Text(FString::Printf(TEXT("CARGO  %d / %d"), GM->CargoCount(), AAbyssGameMode::CargoCapacity), IX, Y + 22.f * S, 16.f, Gold);
	for (int32 i = 0; i < AbyssData::NumFish; ++i)
	{
		Panel(IX + i * 50.f * S, Y + 54.f * S, 8.f * S, 8.f * S, FishColor(i));
		Text(FString::Printf(TEXT("x%d"), GM->Stock[i]), IX + 11.f * S + i * 50.f * S, Y + 47.f * S, 15.f, FLinearColor::White);
		if (GM->Prime[i] > 0)
		{
			Star(IX + 4.f * S + i * 50.f * S, Y + 76.f * S, 13.f * S);
			Text(FString::FromInt(GM->Prime[i]), IX + 13.f * S + i * 50.f * S, Y + 67.f * S, 13.f, Gold);
		}
	}
}

void AAbyssHUD::Star(float CX, float CY, float Size, float Alpha)
{
	Tex(UITex(TEXT("H_Star")), CX - Size * 0.5f, CY - Size * 0.5f, Size, Size, FLinearColor(1.f, 1.f, 1.f, Alpha));
}

// World-space help for the three fish behaviours: the tuna's route ahead, groupers hiding in their dens, the bait
void AAbyssHUD::DrawFishHints(AAbyssGameMode* GM, const FVector& From, bool bDiver)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	auto OnScreen = [&](const FVector& P, FVector2D& SP) { return PC->ProjectWorldLocationToScreen(P, SP) && SP.X > 0.f && SP.Y > 0.f && SP.X < W && SP.Y < H; };
	for (TActorIterator<ACatchableFish> It(GetWorld()); It; ++It)
	{
		ACatchableFish* F = *It;
		const float Dist = FVector::Dist(F->GetActorLocation(), From);
		if (F->Type == EFishType::Tuna && F->IsCatchable() && F->IsCalm() && Dist < 5200.f && !bDiver)
		{
			// where it is heading: park ahead of it instead of chasing
			FVector2D Prev;
			bool bPrev = OnScreen(F->GetActorLocation(), Prev);
			for (int32 k = 1; k <= 8; ++k)
			{
				FVector2D SP;
				const bool bOk = OnScreen(F->PredictRoute(k * 0.7f), SP);
				const float A = 0.55f * (1.f - k / 9.f);
				if (bOk) Disc(SP.X, SP.Y, (4.5f - k * 0.3f) * S, FLinearColor(0.45f, 0.75f, 1.f, A), 8);
				if (bOk && bPrev) Line(Prev.X, Prev.Y, SP.X, SP.Y, FLinearColor(0.45f, 0.75f, 1.f, A * 0.5f), 1.f);
				Prev = SP; bPrev = bOk;
			}
		}
		else if (F->Type == EFishType::Grouper && F->IsHiding() && Dist < 4200.f)
		{
			FVector2D SP;
			if (!OnScreen(F->GetDen() + FVector(0.f, 0.f, 160.f), SP)) continue;
			const float Pulse = 0.55f + 0.35f * FMath::Sin(Time * 3.f);
			Circle(SP.X, SP.Y, 20.f * S, FLinearColor(0.75f, 0.4f, 1.f, Pulse), 2.f, 20);
			Text(TEXT("Grouper hiding in its den"), SP.X, SP.Y + 26.f * S, 15.f, FLinearColor(0.85f, 0.65f, 1.f, 0.95f), true);
			Text(TEXT("Lights off [F], stay still - or drop bait [B]"), SP.X, SP.Y + 46.f * S, 13.f, Muted, true);
		}
	}
	FVector Bait;
	FVector2D SP;
	if (GM->GetBait(Bait) && OnScreen(Bait, SP))
	{
		Circle(SP.X, SP.Y, 14.f * S, FLinearColor(1.f, 0.6f, 0.3f, 0.8f), 2.f, 16);
		Text(TEXT("BAIT"), SP.X, SP.Y - 34.f * S, 13.f, FLinearColor(1.f, 0.7f, 0.4f), true);
	}
}

void AAbyssHUD::DrawFishTag(ACatchableFish* F, const FVector& From, bool bDiver, float Progress, bool bActive)
{
	FVector2D SP;
	if (!GetOwningPlayerController()->ProjectWorldLocationToScreen(F->GetActorLocation(), SP)) return;
	const float Dist = FVector::Dist(F->GetActorLocation(), From);
	const float Half = FMath::Clamp(F->GetRadius() * 900.f / FMath::Max(Dist, 1.f), 26.f, 120.f) * S;
	const FFishDef& D = F->Def();
	const bool bUsable = bDiver ? (D.bHarpoon || D.bNet) : D.bSubClaw;
	const FLinearColor C = bActive ? Cyan : bUsable ? Green : FLinearColor(1.f, 0.6f, 0.3f);
	const float X0 = SP.X - Half, X1 = SP.X + Half, Y0 = SP.Y - Half, Y1 = SP.Y + Half;
	Tex(UITex(TEXT("H_Reticle")), X0, Y0, 2.f * Half, 2.f * Half, FLinearColor(C.R * 1.2f, C.G * 1.2f, C.B * 1.2f, 1.f));

	const FLinearColor TagC = FishColor((int32)F->Type);
	const float TW = 290.f * S, TH = 81.f * S;
	const float TX = X1 + 10 * S, TY = Y0 - TH - 6 * S;
	Line(SP.X + Half * 0.4f, Y0, TX + 20.f * S, TY + TH - 10.f * S, TagC, 1.5f);
	Tex(UITex(TEXT("H_FishTag")), TX, TY, TW, TH);
	const float TL = TX + TW * 0.27f;
	Text(D.Name, TL, TY + 10 * S, 16.f, FLinearColor::White);
	Text(FString::Printf(TEXT("Lv.%s   %dm   $%d"), *Stars(D.Stars), FMath::RoundToInt(Dist / 100.f), D.Price), TL, TY + 30 * S, 12.f, Gold);
	// catch methods: OK / NO for each
	float MX = TL;
	for (int32 m = 0; m < 3; ++m)
	{
		const ECatchMethod Method = (ECatchMethod)m;
		const bool bOk = AbyssData::CanCatch(F->Type, Method);
		const FString Label = FString::Printf(TEXT("%s %s"), *AbyssLoc::Translate(AbyssData::MethodName(Method)), bOk ? LOCS("OK", "可用") : LOCS("NO", "不行"));
		Text(Label, MX, TY + 47 * S, 11.f, bOk ? Green : FLinearColor(1.f, 0.4f, 0.3f, 0.8f));
		MX += TextW(Label, 11.f) + 10.f * S;
	}

	float LY = Y1 + 10 * S;
	if (!bDiver)
	{
		if (bActive || Progress > 0.f)
		{
			Bar(X0, LY, X1 - X0, 10 * S, Progress, Cyan);
			Text(TEXT("CLAW..."), SP.X, LY + 14 * S, 18.f, Cyan, true);
			LY += 40 * S;
		}
		else
		{
			Text(D.bSubClaw ? TEXT("[Hold LMB] Claw") : TEXT("Dive out [E] to catch"), SP.X, LY, 18.f, C, true);
			LY += 26 * S;
		}
	}
	else if (F->HarpoonHitsTaken > 0)
	{
		Text(FString::Printf(TEXT("HIT %d/%d"), F->HarpoonHitsTaken, D.HarpoonHits), SP.X, LY, 18.f, Gold, true);
		LY += 26 * S;
	}
	// nerves: a calm fish is a prime catch, a spooked one only an ordinary one
	{
		const float BW2 = FMath::Max(X1 - X0, 120.f * S);
		const FLinearColor AC = F->Alarm < 0.45f ? Green : F->Alarm < 0.8f ? Gold : Red;
		Panel(SP.X - BW2 * 0.5f, LY, BW2, 6.f * S, FLinearColor(0.f, 0.f, 0.f, 0.55f));
		Panel(SP.X - BW2 * 0.5f, LY, BW2 * FMath::Clamp(F->Alarm, 0.f, 1.f), 6.f * S, AC);
		Panel(SP.X - BW2 * 0.5f + BW2 * 0.45f, LY - 2.f * S, 2.f * S, 10.f * S, FLinearColor(1.f, 1.f, 1.f, 0.7f));
		if (F->IsCalm())
		{
			Star(SP.X - TextW(TEXT("CALM - prime catch"), 14.f) * 0.5f - 12.f * S, LY + 19.f * S, 15.f * S);
			Text(F->IsFeeding() ? TEXT("FEEDING - prime catch") : TEXT("CALM - prime catch"), SP.X, LY + 10.f * S, 14.f, Gold, true);
		}
		else
		{
			Text(TEXT("SPOOKED - back off, let it settle"), SP.X, LY + 10.f * S, 14.f, AC, true);
		}
		Text(AbyssData::PrimeTip(F->Type), SP.X, LY + 30.f * S, 12.f, Muted, true);
	}
}

void AAbyssHUD::DrawDive(AAbyssGameMode* GM, ADiverPawn* Dv)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const FVector Me = Dv->GetActorLocation();
	DrawObjectives(GM);
	DrawCompass(GM, Me, Dv->GetHeading(), true);
	DrawCargo(GM, 12.f * S);
	DrawSonar(GM, Me, Dv->GetHeading(), 3000.f);
	DrawFishHints(GM, Me, true);
	if (Dv->GetVelocity().Size() > 300.f) Text(TEXT("Swimming fast scares the school - ease off to drift in"), W * 0.5f, H * 0.5f + 96.f * S, 14.f, FLinearColor(1.f, 0.8f, 0.5f, 0.8f), true);

	// --- Oxygen + depth (bottom-left)
	{
		const float O = Dv->Oxygen01;
		const FLinearColor C = O > 0.4f ? Cyan : O > 0.2f ? Gold : (FMath::Frac(Time * 2.f) < 0.5f ? Red : FLinearColor(0.5f, 0.1f, 0.1f));
		const float CX = 84.f * S, CY = H - 92.f * S, R = 48.f * S;
		Tex(UITex(TEXT("H_Oxygen")), CX - 72.f * S, CY - 72.f * S, 144.f * S, 144.f * S);
		Circle(CX, CY, R, C, 7.f, 48, -PI * 0.5f, -PI * 0.5f + 2.f * PI * O);
		if (O < 0.25f)
		{
			const float Pa = 0.45f + 0.35f * FMath::Sin(Time * 6.f);
			Tex(UITex(TEXT("H_Danger")), 0.f, 0.f, W, H, FLinearColor(1.f, 1.f, 1.f, Pa));
		}
		Text(TEXT("O2"), CX, CY - 26.f * S, 14.f, Muted, true);
		Text(FString::Printf(TEXT("%ds"), FMath::CeilToInt(O * Dv->OxygenSeconds())), CX, CY - 8.f * S, 24.f, FLinearColor::White, true);
		Text(TEXT("DEPTH"), CX + 84.f * S, CY - 26.f * S, 14.f, Muted);
		Text(FString::Printf(TEXT("%.1f m"), Dv->GetDepthMeters()), CX + 84.f * S, CY - 8.f * S, 26.f, Dv->bTooDeep ? Red : FLinearColor::White);
		Text(FString::Printf(TEXT("limit %d m"), FMath::RoundToInt(AbyssData::DiverMaxDepth / 100.f)), CX + 84.f * S, CY + 24.f * S, 12.f, Muted);
		if (O < 0.25f) Text(TEXT("LOW OXYGEN - RETURN TO THE SUB"), W * 0.5f, H * 0.28f, 30.f, C, true);
	}

	// --- Tool selector (bottom-center): round buttons, golden ring on the selected one
	{
		const float BS = 84.f * S, Y = H - BS - 30.f * S;
		const TCHAR* Imgs[] = { TEXT("H_ToolHarpoon"), TEXT("H_ToolNet") };
		const TCHAR* Names[] = { TEXT("[1] Speargun"), TEXT("[2] Hand Net") };
		for (int32 i = 0; i < 2; ++i)
		{
			const bool bSel = (int32)Dv->Tool == i;
			const float X = W * 0.5f - BS - 16.f * S + i * (BS + 32.f * S);
			Tex(UITex(Imgs[i]), X, Y, BS, BS, bSel ? FLinearColor::White : FLinearColor(0.6f, 0.6f, 0.65f, 0.9f));
			if (bSel)
			{
				const float Pu = 1.f + 0.03f * FMath::Sin(Time * 4.f);
				const float RS = BS * 1.22f * Pu;
				Tex(UITex(TEXT("H_SelRing")), X + BS * 0.5f - RS * 0.5f, Y + BS * 0.5f - RS * 0.5f, RS, RS);
			}
			Text(Names[i], X + BS * 0.5f, Y + BS + 4.f * S, 13.f, bSel ? FLinearColor::White : Muted, true);
		}
		if (Dv->Cooldown > 0.f)
		{
			const float RS = 80.f * S;
			const float CX = W * 0.5f, CY = Y + BS * 0.5f;
			Tex(UITex(TEXT("H_ProgRing")), CX - RS * 0.5f, CY - RS * 0.5f, RS, RS, FLinearColor(1.f, 1.f, 1.f, 0.85f));
		}
	}

	// --- Crosshair
	{
		const float CX = W * 0.5f, CY = H * 0.5f;
		const FLinearColor C = Dv->Aimed.IsValid() ? Green : FLinearColor(1.f, 1.f, 1.f, 0.8f);
		if (Dv->Tool == EDiverTool::Harpoon)
		{
			const float RS = 70.f * S;
			Tex(UITex(TEXT("H_Reticle")), CX - RS * 0.5f, CY - RS * 0.5f, RS, RS, Dv->Aimed.IsValid() ? FLinearColor(0.6f, 1.4f, 0.6f, 1.f) : FLinearColor(1.f, 1.f, 1.f, 0.75f));
		}
		else
		{
			Circle(CX, CY, 46.f * S, FLinearColor(1.f, 1.f, 1.f, 0.5f), 1.5f, 40);
		}
	}

	if (ACatchableFish* F = Dv->Aimed.Get())
	{
		DrawFishTag(F, Me, true, 0.f, false);
	}
	for (const TWeakObjectPtr<ACatchableFish>& WF : Dv->NetTargets)
	{
		ACatchableFish* F = WF.Get();
		FVector2D SP;
		if (!F || !GetOwningPlayerController()->ProjectWorldLocationToScreen(F->GetActorLocation(), SP)) continue;
		const FLinearColor C = F->Def().bNet ? Green : FLinearColor(1.f, 0.5f, 0.3f);
		Circle(SP.X, SP.Y, 22.f * S, C, 2.5f, 20);
		if (!F->Def().bNet) Text(TEXT("too strong"), SP.X, SP.Y + 24.f * S, 15.f, C, true);
	}
	if (Dv->Tool == EDiverTool::Net && Dv->NetTargets.Num() > 0)
	{
		Text(TEXT("[LMB] Swing net"), W * 0.5f, H * 0.5f + 60.f * S, 20.f, Green, true);
	}

	// --- Tether / depth warnings + hatch prompt
	if (Dv->bTooFar)
	{
		Text(FString::Printf(TEXT("TOO FAR FROM THE SUB  (%dm)"), FMath::RoundToInt(Dv->DistToSub / 100.f)), W * 0.5f, H * 0.2f, 26.f, Gold, true);
	}
	if (Dv->bTooDeep)
	{
		Text(TEXT("DIVER DEPTH LIMIT - use the sub claw for deep fish"), W * 0.5f, H * 0.24f, 24.f, Red, true);
	}
	if (Dv->bNearHatch)
	{
		const FString P = TEXT("[E]  Board the submarine");
		const float PW = TextW(P, 26.f) + 60.f * S;
		Panel(W * 0.5f - PW * 0.5f, H * 0.7f, PW, 52.f * S, FLinearColor(0.01f, 0.06f, 0.08f, 0.8f));
		Border(W * 0.5f - PW * 0.5f, H * 0.7f, PW, 52.f * S, Cyan, 2.f);
		Text(P, W * 0.5f, H * 0.7f + 11.f * S, 26.f, Cyan, true);
	}
}

// ---------------------------------------------------------------- kitchen minigame

void AAbyssHUD::Disc(float CX, float CY, float R, const FLinearColor& C, int32 Seg)
{
	for (int32 i = 0; i < Seg; ++i)
	{
		const float A0 = 2.f * PI * i / Seg, A1 = 2.f * PI * (i + 1) / Seg;
		FCanvasTriangleItem T(FVector2D(CX, CY), FVector2D(CX + FMath::Cos(A0) * R, CY + FMath::Sin(A0) * R),
			FVector2D(CX + FMath::Cos(A1) * R, CY + FMath::Sin(A1) * R), GWhiteTexture);
		T.SetColor(C);
		T.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(T);
	}
}

UTexture2D* AAbyssHUD::UITex(const FString& Name)
{
	const FName Key(*Name);
	if (TObjectPtr<UTexture2D>* Found = UITextures.Find(Key)) return Found->Get();
	const FString P = FString::Printf(TEXT("/Game/Abyss/UI/T_%s.T_%s"), *Name, *Name);
	UTexture2D* T = LoadObject<UTexture2D>(nullptr, *P, nullptr, LOAD_NoWarn | LOAD_Quiet);
	UITextures.Add(Key, T);
	return T;
}

void AAbyssHUD::Tex(UTexture2D* T, float X, float Y, float W, float H, const FLinearColor& C)
{
	if (!T || !T->GetResource()) return;
	FCanvasTileItem I(FVector2D(X, Y), T->GetResource(), FVector2D(W, H), C);
	I.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(I);
}

void AAbyssHUD::DrawKitchen(AAbyssGameMode* GM, ARestaurantDome* Dome)
{
	const FKitchenGame& K = Dome->Kitchen;
	const FRecipe& R = AbyssRecipes::Get(K.Dish);
	const FKitchenLayout L = FKitchenLayout::Make(Canvas->ClipX, Canvas->ClipY);
	const float U = L.S; // panel units -> pixels
	const float PX = L.PanelPos.X, PY = L.PanelPos.Y, PW = L.PanelSize.X, PH = L.PanelSize.Y;
	auto At = [&](float X, float Y) { return FVector2D(PX + X * U, PY + Y * U); };
	const FLinearColor Cream(1.f, 0.95f, 0.82f, 1.f);
	const FLinearColor Ink(0.25f, 0.16f, 0.1f, 1.f);

	Panel(0.f, 0.f, Canvas->ClipX, Canvas->ClipY, FLinearColor(0.f, 0.01f, 0.03f, 0.45f));
	if (UTexture2D* PT = UITex(TEXT("Panel"))) Tex(PT, PX, PY, PW, PH);
	else { Panel(PX, PY, PW, PH, FLinearColor(0.02f, 0.06f, 0.1f, 0.95f)); Border(PX, PY, PW, PH, Gold, 2.f); }

	// ---- header: dish picture, name, price, step chips
	{
		const FVector2D DP = At(46.f, 8.f);
		Tex(UITex(FString::Printf(TEXT("Dish_%d"), FMath::Clamp(K.Dish, 0, 3))), DP.X, DP.Y, 78.f * U, 78.f * U);
		Text(R.Name, PX + 136.f * U, PY + 18.f * U, 32.f, Cream);
		Text(FString::Printf(TEXT("$%d"), R.Price), PX + 136.f * U, PY + 56.f * U, 18.f, Gold);
		struct FChip { const TCHAR* Label; EKitchenStep Step; bool bUsed; };
		const FChip Chips[] = { { TEXT("1 Slice"), EKitchenStep::Slice, R.bSlice }, { TEXT("2 Cook"), EKitchenStep::Cook, R.bCook }, { TEXT("3 Plate"), EKitchenStep::Plate, true } };
		const float CW = 180.f * U, CH = 44.f * U, Gap = 12.f * U;
		float CX = PX + PW - 70.f * U - 3.f * CW - 2.f * Gap;
		for (int32 c = 0; c < 3; ++c)
		{
			const FChip& Ch = Chips[c];
			const bool bNow = K.Step == Ch.Step;
			const bool bDone = (int32)K.Step > (int32)Ch.Step && Ch.bUsed;
			const float CY = PY + 24.f * U;
			if (bNow)
			{
				// the "on" chip art has a 20px glow margin around a 260x64 pill
				const float PadX = 20.f * CW / 260.f, PadY = 20.f * CH / 64.f;
				Tex(UITex(TEXT("Chip_On")), CX - PadX, CY - PadY, CW + 2.f * PadX, CH + 2.f * PadY);
			}
			else
			{
				Tex(UITex(bDone ? TEXT("Chip_Done") : TEXT("Chip_Off")), CX, CY, CW, CH, Ch.bUsed ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.35f));
			}
			FString Lb = Ch.Label;
			Text(Lb, CX + CW * 0.5f, CY + 9.f * U, 19.f, bNow ? Cream : bDone ? Green : FLinearColor(0.7f, 0.82f, 0.9f, Ch.bUsed ? 1.f : 0.4f), true);
			CX += CW + Gap;
		}
	}

	if (K.Step == EKitchenStep::Slice)
	{
		// wooden board + the real fillet + gold cut guides + a chef knife that chops
		const FVector2D B = At(100.f, 128.f);
		const float BW = 1000.f * U, BH = 333.f * U;
		Tex(UITex(TEXT("Board")), B.X, B.Y, BW, BH);
		const float FX = B.X + 70.f * U, FY = B.Y + 32.f * U, FW = 860.f * U, FH = 269.f * U;
		Tex(UITex(FString::Printf(TEXT("Fillet_%d"), FMath::Clamp(R.Fish, 0, 2))), FX, FY, FW, FH);
		for (int32 t = 0; t < 3; ++t)
		{
			const float TX = FX + FW * K.CutTargets[t];
			const float Pulse = 0.65f + 0.35f * FMath::Sin(Time * 6.f + t);
			const FLinearColor TC = K.CutUsed[t] ? FLinearColor(1.f, 0.78f, 0.32f, 0.2f) : FLinearColor(1.f, 0.82f, 0.35f, Pulse);
			for (float Yd = FY + 10.f * U; Yd < FY + FH - 10.f * U; Yd += 18.f * U) Line(TX, Yd, TX, Yd + 10.f * U, TC, 3.f);
		}
		for (int32 c = 0; c < K.Cuts.Num(); ++c)
		{
			const float CXp = FX + FW * K.Cuts[c];
			Line(CXp, FY + 30.f * U, CXp, FY + FH - 30.f * U, FLinearColor(0.25f, 0.05f, 0.05f, 0.85f), 4.f);
			const float Sc = K.CutScores[c];
			Text(FString::FromInt(FMath::RoundToInt(Sc)), CXp, B.Y + BH + 8.f * U, 22.f, Sc >= 90.f ? Green : Sc >= 70.f ? Gold : Red, true);
		}
		if (K.Cuts.Num() != LastCuts) { LastCuts = K.Cuts.Num(); ChopT = 0.f; }
		ChopT += GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
		const float Drop = ChopT < 0.18f ? FMath::Sin(ChopT / 0.18f * PI) * 70.f * U : 0.f;
		const float KX = FX + FW * K.Knife;
		const float KW = 72.f * U, KH = 230.f * U;
		Tex(UITex(TEXT("Knife")), KX - KW * 0.5f, FY - 120.f * U + Drop, KW, KH);
		Text(FString::Printf(TEXT("Cut on the gold lines  (%d/3)"), K.Cuts.Num()), PX + PW * 0.5f, PY + 520.f * U, 26.f, Cream, true);
		Text(TEXT("[Click] / [Space] / [E]"), PX + PW * 0.5f, PY + 556.f * U, 18.f, Muted, true);
	}
	else if (K.Step == EKitchenStep::Cook)
	{
		const FVector2D G = At(200.f, 110.f);
		const float GW = 800.f * U, GH = 364.f * U;
		Tex(UITex(TEXT("Teppan")), G.X, G.Y, GW, GH, FLinearColor(0.42f, 0.43f, 0.47f, 1.f));
		const float Heat = 0.5f + 0.5f * FMath::Sin(Time * 13.f);
		Panel(G.X + 20.f * U, G.Y + GH - 14.f * U, GW - 40.f * U, 6.f * U, FLinearColor(1.f, 0.4f, 0.08f, 0.35f + 0.3f * Heat));
		const float D = FMath::Clamp(K.Doneness, 0.f, 1.f);
		const float SW = 360.f * U, SH = 180.f * U;
		const float SX = G.X + GW * 0.5f - SW * 0.5f, SY = G.Y + GH * 0.5f - SH * 0.5f;
		const float CookA = FMath::Clamp(D / 0.86f, 0.f, 1.f);
		const float Burn = FMath::Clamp((D - 0.86f) / 0.14f, 0.f, 1.f);
		// sizzling oil around the steak
		for (int32 sp = 0; sp < 18; ++sp)
		{
			const float A = sp * 2.4f + Time * 0.3f;
			const float Fl = FMath::Frac(Time * (1.3f + sp * 0.07f) + sp * 0.37f);
			const FVector2D C(SX + SW * 0.5f + FMath::Cos(A) * (SW * 0.55f + 20.f * U * Fl), SY + SH * 0.5f + FMath::Sin(A) * (SH * 0.6f + 14.f * U * Fl));
			Disc(C.X, C.Y, (2.f + 4.f * (1.f - Fl)) * U, FLinearColor(1.f, 0.85f, 0.5f, 0.55f * (1.f - Fl)), 8);
		}
		Tex(UITex(TEXT("Steak_Raw")), SX, SY, SW, SH, FLinearColor(1.f, 1.f, 1.f, 1.f - CookA));
		Tex(UITex(TEXT("Steak_Cooked")), SX, SY, SW, SH, FLinearColor(FMath::Lerp(1.f, 0.28f, Burn), FMath::Lerp(1.f, 0.24f, Burn), FMath::Lerp(1.f, 0.22f, Burn), CookA));
		for (int32 st = 0; st < 7; ++st)
		{
			const float T = FMath::Frac(Time * 0.55f + st * 0.15f);
			Disc(SX + SW * (0.1f + st * 0.13f) + FMath::Sin(Time * 2.f + st) * 12.f * U, SY - T * 110.f * U, (10.f + 18.f * T) * U, FLinearColor(1.f, 1.f, 1.f, (0.14f + 0.1f * Burn) * (1.f - T)), 14);
		}
		// doneness bar: raw -> golden -> burnt, two green windows, marker
		const float W = R.CookWindow;
		const float BX = PX + 160.f * U, BW = 880.f * U, BY = PY + 500.f * U, BH = 26.f * U;
		Panel(BX - 3.f * U, BY - 3.f * U, BW + 6.f * U, BH + 6.f * U, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		const int32 NSeg = 44;
		for (int32 k = 0; k < NSeg; ++k)
		{
			const float t = (k + 0.5f) / NSeg;
			const FLinearColor C = t < 0.86f ? FMath::Lerp(FLinearColor(0.85f, 0.35f, 0.32f), FLinearColor(0.9f, 0.62f, 0.25f), t / 0.86f)
			                                 : FMath::Lerp(FLinearColor(0.55f, 0.3f, 0.12f), FLinearColor(0.1f, 0.06f, 0.04f), (t - 0.86f) / 0.14f);
			Panel(BX + BW * k / NSeg, BY, BW / NSeg + 0.5f, BH, C);
		}
		for (int32 z = 0; z < 2; ++z)
		{
			const float Ctr = z == 0 ? K.FlipAt : K.FinishAt;
			const bool bActiveZone = K.CookStage == z;
			const float ZX = BX + BW * (Ctr - W), ZW = BW * 2.f * W;
			Panel(ZX, BY - 4.f * U, ZW, BH + 8.f * U, FLinearColor(0.35f, 1.f, 0.45f, bActiveZone ? 0.45f + 0.2f * Heat : 0.15f));
			Border(ZX, BY - 4.f * U, ZW, BH + 8.f * U, bActiveZone ? Green : FLinearColor(0.35f, 1.f, 0.45f, 0.35f), bActiveZone ? 2.5f : 1.f);
			Text(z == 0 ? TEXT("FLIP") : TEXT("DONE"), ZX + ZW * 0.5f, BY - 32.f * U, 18.f, bActiveZone ? Green : Muted, true);
		}
		Border(BX, BY, BW, BH, FLinearColor(1.f, 0.9f, 0.7f, 0.6f), 1.5f);
		const float MX = BX + BW * D;
		Panel(MX - 3.f * U, BY - 10.f * U, 6.f * U, BH + 20.f * U, FLinearColor::White);
		Line(MX - 10.f * U, BY - 18.f * U, MX, BY - 8.f * U, FLinearColor::White, 3.f);
		Line(MX + 10.f * U, BY - 18.f * U, MX, BY - 8.f * U, FLinearColor::White, 3.f);
		Text(K.CookStage == 0 ? TEXT("Flip it in the first green zone!") : TEXT("Take it off in the second green zone!"), PX + PW * 0.5f, PY + 550.f * U, 26.f, Cream, true);
		Text(TEXT("[Click] / [Space] / [E]"), PX + PW * 0.5f, PY + 586.f * U, 18.f, Muted, true);
	}
	else if (K.Step == EKitchenStep::Plate)
	{
		// ---- order ticket with what still has to go on the plate
		TMap<int32, int32> Need, Have;
		for (EIngredient I : R.Slots) Need.FindOrAdd((int32)I)++;
		for (int32 v : K.Placed) if (v >= 0) Have.FindOrAdd(v)++;
		{
			const FVector2D T = At(36.f, 116.f);
			const float TW = 260.f * U, TH = 375.f * U;
			Tex(UITex(TEXT("Ticket")), T.X, T.Y, TW, TH);
			Text(TEXT("ORDER"), T.X + 22.f * U, T.Y + 6.f * U, 20.f, Cream);
			Tex(UITex(FString::Printf(TEXT("Dish_%d"), FMath::Clamp(K.Dish, 0, 3))), T.X + TW * 0.5f - 52.f * U, T.Y + 42.f * U, 104.f * U, 104.f * U);
			float RY = T.Y + 156.f * U;
			for (const TPair<int32, int32>& P : Need)
			{
				const int32 Got = Have.FindRef(P.Key);
				const bool bOk = Got >= P.Value;
				Tex(UITex(FString::Printf(TEXT("Ing_%d"), P.Key)), T.X + 20.f * U, RY - 4.f * U, 40.f * U, 40.f * U);
				Text(AbyssRecipes::IngredientName((EIngredient)P.Key), T.X + 66.f * U, RY + 4.f * U, 18.f, Ink);
				Text(FString::Printf(TEXT("%d/%d"), FMath::Min(Got, P.Value), P.Value), T.X + TW - 22.f * U, RY + 4.f * U, 18.f, bOk ? FLinearColor(0.1f, 0.55f, 0.2f) : Ink, false, true);
				if (bOk) Line(T.X + 18.f * U, RY + 16.f * U, T.X + TW - 18.f * U, RY + 16.f * U, FLinearColor(0.1f, 0.55f, 0.2f, 0.6f), 2.f);
				RY += 46.f * U;
			}
		}
		// ---- ingredient cards (drag sources)
		for (int32 i = 0; i < K.Tray.Num(); ++i)
		{
			const FVector2D TP = L.TrayItemPos(i);
			const EIngredient I = K.Tray[i];
			const bool bHover = K.DragPos.X >= TP.X && K.DragPos.X <= TP.X + L.TrayItemSize.X && K.DragPos.Y >= TP.Y && K.DragPos.Y <= TP.Y + L.TrayItemSize.Y;
			Tex(UITex(bHover ? TEXT("Card_Sel") : TEXT("Card")), TP.X, TP.Y, L.TrayItemSize.X, L.TrayItemSize.Y);
			const float IS = 84.f * U;
			Tex(UITex(FString::Printf(TEXT("Ing_%d"), (int32)I)), TP.X + L.TrayItemSize.X * 0.5f - IS * 0.5f, TP.Y + 6.f * U - (bHover ? 4.f * U : 0.f), IS, IS);
			Text(AbyssRecipes::IngredientName(I), TP.X + L.TrayItemSize.X * 0.5f, TP.Y + L.TrayItemSize.Y - 30.f * U, 17.f, Cream, true);
		}
		Text(TEXT("Drag ingredients onto the plate"), L.TrayOrigin.X, L.TrayItemPos(5).Y + L.TrayItemSize.Y + 14.f * U, 18.f, Muted);

		// ---- the plate (top view) with drop slots
		const float PR = L.PlateR * 1.12f;
		Tex(UITex(TEXT("Plate")), L.PlateCenter.X - PR, L.PlateCenter.Y - PR, PR * 2.f, PR * 2.f);
		const int32 N = K.Placed.Num();
		for (int32 sl = 0; sl < N; ++sl)
		{
			const FVector2D SP = L.SlotPos(sl, N);
			const float SS = L.SlotR * 2.1f;
			if (K.Placed[sl] >= 0)
			{
				Tex(UITex(FString::Printf(TEXT("Ing_%d"), K.Placed[sl])), SP.X - SS * 0.6f, SP.Y - SS * 0.6f, SS * 1.2f, SS * 1.2f);
			}
			else
			{
				const float Pulse = K.Dragging >= 0 ? 0.6f + 0.4f * FMath::Sin(Time * 6.f) : 0.8f;
				Tex(UITex(TEXT("Slot")), SP.X - SS * 0.5f, SP.Y - SS * 0.5f, SS, SS, FLinearColor(1.f, 1.f, 1.f, Pulse));
			}
		}
		if (K.Dragging >= 0)
		{
			const float DS = 110.f * U;
			Tex(UITex(FString::Printf(TEXT("Ing_%d"), K.Dragging)), K.DragPos.X - DS * 0.5f, K.DragPos.Y - DS * 0.5f, DS, DS, FLinearColor(1.f, 1.f, 1.f, 0.9f));
		}
		// ---- serve button (art has a 20px glow margin around a 420x112 pill)
		const bool bHover = L.InServe(K.DragPos);
		const float PadX = 20.f * L.ServeSize.X / 420.f, PadY = 20.f * L.ServeSize.Y / 112.f;
		Tex(UITex(TEXT("Button")), L.ServePos.X - PadX, L.ServePos.Y - PadY, L.ServeSize.X + 2.f * PadX, L.ServeSize.Y + 2.f * PadY,
			bHover ? FLinearColor(1.25f, 1.25f, 1.25f, 1.f) : FLinearColor::White);
		Text(TEXT("SERVE"), L.ServePos.X + L.ServeSize.X * 0.5f, L.ServePos.Y + 16.f * U, 28.f, FLinearColor::White, true);
	}

	// scores so far
	{
		FString Sc;
		if (R.bSlice && (int32)K.Step > (int32)EKitchenStep::Slice) Sc += FString::Printf(TEXT("Prep %d   "), FMath::RoundToInt(K.PrepScore));
		if (R.bCook && (int32)K.Step > (int32)EKitchenStep::Cook) Sc += FString::Printf(TEXT("Cook %d"), FMath::RoundToInt(K.CookScore));
		if (!Sc.IsEmpty()) Text(Sc, PX + 120.f * U, PY + PH - 62.f * U, 19.f, Gold);
	}
	if (K.FeedbackTime > 0.f)
	{
		FLinearColor C = K.FeedbackColor;
		C.A = FMath::Clamp(K.FeedbackTime * 2.f, 0.f, 1.f);
		const float FWd = TextW(K.Feedback, 32.f) + 60.f * U;
		Panel(PX + PW * 0.5f - FWd * 0.5f, PY + 96.f * U, FWd, 50.f * U, FLinearColor(0.f, 0.f, 0.f, 0.55f * C.A));
		Text(K.Feedback, PX + PW * 0.5f, PY + 102.f * U, 32.f, C, true);
	}
}

// ---------------------------------------------------------------- ending (day 2 cliffhanger)

void AAbyssHUD::DrawEnding(AAbyssGameMode* GM)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float T = GM->EndingTime;
	Panel(0.f, 0.f, W, H, FLinearColor(0.f, 0.005f, 0.01f, FMath::Clamp(T * 0.8f, 0.f, 1.f)));

	// sonar display
	const float CX = W * 0.5f, CY = H * 0.42f, R = 230.f * S;
	const float A = FMath::Clamp((T - 0.5f) * 1.5f, 0.f, 1.f);
	Circle(CX, CY, R, FLinearColor(0.35f, 0.95f, 1.f, 0.8f * A), 2.f, 64);
	Circle(CX, CY, R * 0.66f, FLinearColor(0.35f, 0.95f, 1.f, 0.3f * A), 1.f, 48);
	Circle(CX, CY, R * 0.33f, FLinearColor(0.35f, 0.95f, 1.f, 0.3f * A), 1.f, 32);
	const float Sweep = T * 1.6f;
	for (int32 k = 0; k < 16; ++k)
	{
		const float Ang = Sweep - k * 0.04f;
		Line(CX, CY, CX + FMath::Cos(Ang) * R, CY + FMath::Sin(Ang) * R, FLinearColor(0.35f, 0.95f, 1.f, 0.5f * A * (1.f - k / 16.f)), 2.f);
	}
	// the contact: huge and getting bigger
	if (T > 1.5f)
	{
		const float Grow = FMath::Clamp((T - 1.5f) * 0.4f, 0.f, 1.f);
		const float Pulse = 0.6f + 0.4f * FMath::Sin(T * 5.f);
		const FVector2D B(CX + R * 0.35f, CY - R * 0.4f);
		Disc(B.X, B.Y, (30.f + 80.f * Grow) * S, FLinearColor(1.f, 0.15f, 0.1f, 0.35f * Pulse), 40);
		Disc(B.X, B.Y, (14.f + 30.f * Grow) * S, FLinearColor(1.f, 0.25f, 0.15f, 0.9f), 24);
		Text(TEXT("SONAR CONTACT: UNKNOWN"), CX, CY + R + 30.f * S, 26.f, FLinearColor(1.f, 0.35f, 0.3f, Pulse), true);
		Text(TEXT("Massive signal  -  312 m  -  rising"), CX, CY + R + 66.f * S, 20.f, FLinearColor(1.f, 0.6f, 0.5f, 0.9f), true);
	}
	Tex(UITex(TEXT("H_Frame")), -W * 0.06f, -H * 0.08f, W * 1.12f, H * 1.16f, FLinearColor(1.f, 1.f, 1.f, 0.75f * A));
	if (T > 3.5f)
	{
		const float B = FMath::Clamp((T - 3.5f) * 0.8f, 0.f, 1.f);
		const float LW = 520.f * S, LH = LW * 162.f / 256.f;
		Tex(UITex(TEXT("H_Logo")), CX - LW * 0.5f, H * 0.8f - LH * 0.78f, LW, LH, FLinearColor(1.f, 1.f, 1.f, B));
		Text(TEXT("ABYSS DINING"), CX, H * 0.8f, 44.f, FLinearColor(0.3f, 0.18f, 0.06f, B), true);
		Text(TEXT("End of demo  -  thanks for playing"), CX, H * 0.8f + 70.f * S, 22.f, FLinearColor(0.8f, 0.9f, 0.95f, B), true);
		if (T > 4.f) Text(TEXT("[ENTER]  Keep running the restaurant"), CX, H - 60.f * S, 20.f, FLinearColor(0.6f, 0.95f, 1.f, 0.6f + 0.4f * FMath::Sin(T * 3.f)), true);
	}
}


// ---------------------------------------------------------------- recipe book (H)

void AAbyssHUD::DrawRecipeBook(AAbyssGameMode* GM)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float U = H / 1080.f;
	const float PW = 1200.f * U, PH = 672.f * U;
	const float PX = W * 0.5f - PW * 0.5f, PY = H * 0.5f - PH * 0.5f + 20.f * U;
	const FLinearColor Cream(1.f, 0.95f, 0.82f, 1.f);
	Panel(0.f, 0.f, W, H, FLinearColor(0.f, 0.01f, 0.03f, 0.5f));
	if (UTexture2D* PT = UITex(TEXT("Panel"))) Tex(PT, PX, PY, PW, PH);
	Text(TEXT("RECIPE BOOK"), PX + PW * 0.5f, PY + 26.f * U, 34.f, Gold, true);
	Text(TEXT("[H] close"), PX + PW - 80.f * U, PY + 36.f * U, 16.f, Muted, false, true);
	const TArray<FRecipe>& All = AbyssRecipes::All();
	for (int32 d = 0; d < All.Num() && d < 5; ++d)
	{
		const FRecipe& R = All[d];
		const float RY = PY + (96.f + d * 100.f) * U;
		const float RX = PX + 70.f * U;
		Panel(RX, RY, PW - 140.f * U, 92.f * U, FLinearColor(1.f, 1.f, 1.f, 0.05f));
		Tex(UITex(FString::Printf(TEXT("Dish_%d"), d)), RX + 8.f * U, RY + 4.f * U, 84.f * U, 84.f * U);
		Text(R.Name, RX + 110.f * U, RY + 8.f * U, 23.f, Cream);
		Text(FString::Printf(TEXT("$%d"), R.Price), RX + 110.f * U, RY + 38.f * U, 18.f, Gold);
		if (R.Fish >= 0) Text(FString::Printf(TEXT("Needs: %s"), AbyssData::Fish((EFishType)R.Fish).Name), RX + 110.f * U, RY + 64.f * U, 15.f, Muted);
		Text(R.Focus, RX + PW - 160.f * U, RY + 60.f * U, 15.f, Gold, false, true);
		// steps
		float SX = RX + 430.f * U;
		auto Step = [&](const TCHAR* Label, bool bOn)
		{
			const float CW = 130.f * U;
			Tex(UITex(bOn ? TEXT("Chip_Done") : TEXT("Chip_Off")), SX, RY + 6.f * U, CW, 32.f * U, bOn ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.3f));
			Text(Label, SX + CW * 0.5f, RY + 11.f * U, 15.f, bOn ? FLinearColor::White : FLinearColor(0.7f, 0.8f, 0.9f, 0.4f), true);
			SX += CW + 10.f * U;
		};
		Step(TEXT("1 Slice"), R.bSlice);
		Step(TEXT("2 Cook"), R.bCook);
		Step(TEXT("3 Plate"), true);
		// plate contents
		float IX = RX + 430.f * U;
		for (EIngredient I : R.Slots)
		{
			Tex(UITex(FString::Printf(TEXT("Ing_%d"), (int32)I)), IX, RY + 44.f * U, 44.f * U, 44.f * U);
			IX += 50.f * U;
		}
	}
	Text(TEXT("Take orders at the tables [E]  ->  cook at the stations  ->  carry the plate back to the guest [E]"), PX + PW * 0.5f, PY + PH - 62.f * U, 16.f, Cream, true);
}

// ---------------------------------------------------------------- full-screen kitchen counter (Papa's style)

namespace
{
	FString CounterItemTex(const FCItem& I)
	{
		switch (I.Kind)
		{
		case ECItem::Fillet:  return FString::Printf(TEXT("Fillet_%d"), FMath::Clamp((int32)I.Fish, 0, 2));
		case ECItem::Sashimi: return TEXT("Ing_1");
		case ECItem::Steak:   return TEXT("Steak_Raw");
		case ECItem::Cooked:  return I.bBurnt ? TEXT("Steak_Burnt") : TEXT("Steak_Cooked");
		case ECItem::Rice:    return TEXT("Ing_0");
		case ECItem::Lemon:   return TEXT("Ing_3");
		case ECItem::Greens:  return TEXT("Ing_4");
		case ECItem::Kelp:    return TEXT("Ing_5");
		case ECItem::Wasabi:  return TEXT("Ing_6");
		case ECItem::Sesame:  return TEXT("Ing_7");
		case ECItem::Stew:    return TEXT("Ing_8");
		default:              return TEXT("");
		}
	}
	float CounterItemAspect(const FCItem& I)
	{
		return I.Kind == ECItem::Fillet ? 3.2f : (I.Kind == ECItem::Steak || I.Kind == ECItem::Cooked) ? 2.f : 1.f;
	}
	FLinearColor CounterItemTint(const FCItem& I)
	{
		// grouper flesh is white
		if (I.Fish == 2 && (I.Kind == ECItem::Steak || I.Kind == ECItem::Sashimi)) return FLinearColor(1.15f, 1.05f, 1.f, 1.f);
		return FLinearColor::White;
	}
}

void AAbyssHUD::DrawCounterItem(const FCItem& I, const FVector2D& C, float Size, float Alpha, float MaxW)
{
	if (!I.IsValid()) return;
	const float A = CounterItemAspect(I);
	float Wd = A >= 1.f ? Size * FMath::Min(A, 1.8f) : Size;
	if (MaxW > 0.f) Wd = FMath::Min(Wd, MaxW);
	const float Ht = Wd / A;
	FLinearColor T = CounterItemTint(I);
	T.A = Alpha;
	Tex(UITex(CounterItemTex(I)), C.X - Wd * 0.5f, C.Y - Ht * 0.5f, Wd, Ht, T);
	// cut from a prime catch
	if (I.bPrime && I.IsFish()) Star(C.X + Wd * 0.38f, C.Y - Ht * 0.5f + 2.f, FMath::Clamp(Size * 0.3f, 12.f, 30.f), Alpha);
}

void AAbyssHUD::DrawCounter(AAbyssGameMode* GM, ARestaurantDome* Dome)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	FKitchenCounter& K = Dome->Counter;
	TArray<ARestaurantCustomer*> Queue;
	Dome->GetQueue(Queue);
	const FCounterLayout L = FCounterLayout::Make(W, H, Queue.Num());
	const float U = L.U;
	const FLinearColor Cream(1.f, 0.95f, 0.82f, 1.f);
	const FLinearColor Ink(0.3f, 0.18f, 0.08f, 1.f);
	const FVector2D M = K.Mouse;
	auto Img = [&](float X, float Y) { return L.Img(X, Y); };

	// --- the dreamy dining room stays visible behind; a soft violet-blue veil over it
	for (int32 b = 0; b < 10; ++b)
	{
		const float T = b / 9.f;
		Panel(0.f, b * H * 0.1f, W, H * 0.1f + 1.f, FLinearColor(FMath::Lerp(0.05f, 0.02f, T), FMath::Lerp(0.03f, 0.05f, T), FMath::Lerp(0.16f, 0.12f, T), FMath::Lerp(0.25f, 0.6f, T)));
	}
	if (L.Origin.X > 0.f) Panel(0.f, L.Origin.Y + 420.f * U, W, H, FLinearColor(0.03f, 0.05f, 0.12f, 1.f));

	// --- the counter (Blender render) + ingredient bins on ice
	Tex(UITex(TEXT("Counter")), L.Origin.X, L.Origin.Y, L.CounterSize.X, L.CounterSize.Y);
	for (int32 i = 0; i < 5; ++i)
	{
		const FCItem BI = FKitchenCounter::BinItem(i);
		for (int32 k = 0; k < 3; ++k) DrawCounterItem(BI, L.Bins[i] + FVector2D((-26.f + k * 26.f) * U, (k % 2 ? -6.f : 4.f) * U), 48.f * U);
	}
	for (int32 i = 0; i < FKitchenCounter::NumPans; ++i)
	{
		const float Fl = 0.6f + 0.4f * FMath::Sin(Time * 17.f + i * 2.f) * FMath::Sin(Time * 5.3f);
		if (K.Pans[i].Busy()) Disc(L.Pan[i].X, L.Pan[i].Y + 8.f * U, L.PanR * 0.8f, FLinearColor(1.f, 0.55f, 0.2f, (0.07f + 0.06f * K.Heat[i]) * Fl), 24);
	}
	const FRecipe* ActiveR = K.Active.IsValid() ? &AbyssRecipes::Get(K.Active->Dish) : nullptr;

	// --- cutting board (on the prep table)
	{
		const FBox2D& B = L.Board;
		const FVector2D BC = B.GetCenter();
		const float BW = B.Max.X - B.Min.X;
		if (K.Board.Kind == ECItem::Fillet)
		{
			DrawCounterItem(K.Board, BC, BW * 0.52f);
			const bool bFine = K.NumCuts >= 4;   // sashimi-grade: thin lines, tight tolerance
			for (int32 t = 0; t < K.NumCuts; ++t)
			{
				const float X = B.Min.X + BW * K.CutTargets[t];
				bool bUsed = false;
				for (float C : K.Cuts) if (FMath::Abs(C - K.CutTargets[t]) < K.CutGap() * 0.45f) bUsed = true;
				const float Pulse = 0.6f + 0.4f * FMath::Sin(Time * 6.f + t);
				for (float Y = B.Min.Y; Y < B.Max.Y; Y += 10.f * U) Line(X, Y, X, Y + 6.f * U, FLinearColor(1.f, 0.85f, 0.3f, bUsed ? 0.15f : Pulse), bFine ? 1.5f : 3.f);
			}
			Text(FString::Printf(TEXT("%s  %d / %d"), bFine ? TEXT("THIN SLICES") : TEXT("CUT"), K.Cuts.Num(), K.NumCuts), BC.X, B.Max.Y + 6.f * U, 15.f, FLinearColor(1.f, 0.92f, 0.7f), true);
			if (K.bCutting)
			{
				// the stroke so far: straight down is best
				const float Slant = FMath::Abs(M.X - K.CutStart.X) / BW;
				Line(K.CutStart.X, K.CutStart.Y, M.X, M.Y, Slant < 0.012f ? FLinearColor(0.5f, 1.f, 0.6f) : Slant < 0.03f ? Gold : Red, 3.f);
			}
			for (int32 c = 0; c < K.Cuts.Num(); ++c)
			{
				const float X = B.Min.X + BW * K.Cuts[c];
				Line(X, B.Min.Y + 6.f * U, X, B.Max.Y - 6.f * U, FLinearColor(0.3f, 0.05f, 0.05f, 0.9f), 3.f);
				const float S2 = K.CutScores[c];
				Text(FString::FromInt(FMath::RoundToInt(S2)), X, B.Min.Y - 28.f * U, 19.f, S2 >= 90.f ? Green : S2 >= 70.f ? Gold : Red, true);
			}
			const float KX = FMath::Clamp(M.X, B.Min.X, B.Max.X);
			// the knife follows the hand: above the board until the button goes down, then through the fish
			const float KY = K.bCutting ? FMath::Clamp(M.Y, B.Min.Y, B.Max.Y) - 122.f * U : B.Min.Y - 128.f * U;
			if (M.Y > B.Min.Y - 160.f * U && M.Y < B.Max.Y + 60.f * U) Tex(UITex(TEXT("Knife")), KX - 20.f * U, KY, 40.f * U, 130.f * U);
		}
		else if (K.Board.Kind == ECItem::Sashimi)
		{
			const int32 NS = FMath::Max(1, (int32)K.Board.Count) * 2;
			for (int32 k = 0; k < NS; ++k) DrawCounterItem(K.Board, BC + FVector2D((-26.f * (NS - 1) + k * 52.f) * U, 0.f), 62.f * U);
			Text(FString::Printf(TEXT("Knife work %d"), FMath::RoundToInt(K.Board.Prep)), BC.X, B.Max.Y + 6.f * U, 15.f, K.Board.Prep >= 90.f ? Green : K.Board.Prep >= 70.f ? Gold : Red, true);
		}
		else if (K.Board.IsValid())
		{
			DrawCounterItem(K.Board, BC, 120.f * U);
		}
		else if (K.Drag.Kind == ECItem::Fillet)
		{
			Panel(B.Min.X, B.Min.Y, BW, B.Max.Y - B.Min.Y, FLinearColor(1.f, 0.85f, 0.4f, 0.18f + 0.1f * FMath::Sin(Time * 6.f)));
		}
	}

	// --- pans on the stove
	for (int32 i = 0; i < FKitchenCounter::NumPans; ++i)
	{
		const FCPan& P = K.Pans[i];
		const FVector2D C = L.Pan[i];
		if (!P.Busy())
		{
			if (K.Drag.Kind == ECItem::Steak) Circle(C.X, C.Y, L.PanR * 0.75f, FLinearColor(0.4f, 1.f, 0.5f, 0.5f + 0.3f * FMath::Sin(Time * 6.f)), 3.f, 40);
			continue;
		}
		const float D = P.Done;
		const float Cooked = FMath::Clamp(D / 0.86f, 0.f, 1.f);
		const float Burn = FMath::Clamp((D - 0.9f) / 0.25f, 0.f, 1.f);
		for (int32 s = 0; s < 14; ++s)
		{
			const float A = s * 2.3f + Time * 0.5f;
			const float F = FMath::Frac(Time * (1.6f + s * 0.09f) + s * 0.41f);
			Disc(C.X + FMath::Cos(A) * L.PanR * (0.55f + 0.25f * F), C.Y + FMath::Sin(A) * L.PanR * 0.32f * (0.6f + 0.3f * F), (2.f + 3.f * (1.f - F)) * U, FLinearColor(1.f, 0.85f, 0.5f, 0.6f * (1.f - F)), 8);
		}
		if (K.DragFromPan == i && FVector2D::Distance(M, K.DownAt) >= 8.f * U) continue;
		const float SW = L.PanR * 1.2f;
		DrawCounterItem(P.Item, C, SW, 1.f - Cooked);
		const FLinearColor BT(FMath::Lerp(1.f, 0.25f, Burn), FMath::Lerp(1.f, 0.2f, Burn), FMath::Lerp(1.f, 0.18f, Burn), Cooked);
		Tex(UITex(TEXT("Steak_Cooked")), C.X - SW * 0.5f, C.Y - SW * 0.25f, SW, SW * 0.5f, BT);
		for (int32 s = 0; s < 6; ++s)
		{
			const float T = FMath::Frac(Time * 0.6f + s * 0.17f);
			const FLinearColor SC = Burn > 0.f ? FLinearColor(0.15f, 0.15f, 0.15f, 0.5f * (1.f - T)) : FLinearColor(1.f, 1.f, 1.f, 0.18f * (1.f - T));
			Disc(C.X + (s - 2.5f) * 16.f * U + FMath::Sin(Time * 2.f + s) * 8.f * U, C.Y - 20.f * U - T * 140.f * U, (10.f + 20.f * T) * U, SC, 14);
		}
		if (D > 1.05f)
		{
			for (int32 f = 0; f < 7; ++f)
			{
				const float Fl = FMath::Abs(FMath::Sin(Time * 9.f + f * 1.7f));
				Disc(C.X + (f - 3) * 15.f * U, C.Y - (10.f + 40.f * Fl) * U, (9.f + 8.f * Fl) * U, FLinearColor(1.f, 0.45f + 0.3f * Fl, 0.05f, 0.75f), 10);
			}
		}
		// doneness gauge
		const float GX = C.X - 70.f * U, GY = C.Y - L.PanR * 0.5f - 46.f * U, GW = 140.f * U, GH = 14.f * U;
		Tex(UITex(TEXT("S_BarBlue")), GX - 10.f * U, GY - 7.f * U, GW + 20.f * U, GH + 14.f * U, FLinearColor(0.3f, 0.3f, 0.3f, 1.f));
		const int32 NSeg = 20;
		for (int32 k = 0; k < NSeg; ++k)
		{
			const float t = (k + 0.5f) / NSeg;
			const FLinearColor SC = t < 0.86f ? FMath::Lerp(FLinearColor(0.85f, 0.35f, 0.32f), FLinearColor(0.95f, 0.65f, 0.2f), t / 0.86f) : FLinearColor(0.15f, 0.08f, 0.04f);
			Panel(GX + GW * k / NSeg, GY, GW / NSeg + 0.5f, GH, SC);
		}
		Panel(GX + GW * 0.37f, GY - 3.f * U, GW * 0.16f, GH + 6.f * U, FLinearColor(0.4f, 0.9f, 1.f, P.bFlipped ? 0.15f : 0.45f));
		Panel(GX + GW * 0.79f, GY - 3.f * U, GW * 0.14f, GH + 6.f * U, FLinearColor(0.35f, 1.f, 0.45f, 0.45f));
		const float MX = GX + GW * FMath::Clamp(D / 1.1f, 0.f, 1.f);
		Panel(MX - 2.f * U, GY - 6.f * U, 4.f * U, GH + 12.f * U, FLinearColor::White);
		const float Pulse = 0.6f + 0.4f * FMath::Sin(Time * 8.f);
		const float LY = GY - 34.f * U;
		if (D >= 1.f) Text(TEXT("BURNT!"), C.X, LY, 22.f, Red, true);
		else if (D > 0.94f) Text(TEXT("BURNING!"), C.X, LY, 22.f, FLinearColor(1.f, 0.4f, 0.2f, Pulse), true);
		else if (!P.bFlipped && D > 0.3f) Text(TEXT("Click to FLIP"), C.X, LY, 20.f, FLinearColor(0.5f, 0.9f, 1.f, Pulse), true);
		else if (D > 0.78f) Text(TEXT("READY - drag to plate"), C.X, LY, 19.f, FLinearColor(0.4f, 1.f, 0.5f, Pulse), true);
		else if (D < 0.3f && P.Sear < 0.9f && K.Heat[i] < 2) Text(TEXT("Turn the knob to HIGH to sear"), C.X, LY, 17.f, FLinearColor(1.f, 0.7f, 0.35f, Pulse), true);
		else if (D > 0.5f && K.Heat[i] == 2) Text(TEXT("Too hot - turn it down!"), C.X, LY, 18.f, FLinearColor(1.f, 0.45f, 0.25f, Pulse), true);
		// crust: built on high heat at the start, ruined by high heat at the end
		{
			const float SY = GY + GH + 6.f * U;
			Panel(GX, SY, GW, 7.f * U, FLinearColor(0.f, 0.f, 0.f, 0.6f));
			Panel(GX, SY, GW * FMath::Clamp(P.Sear - 0.4f * P.Scorch, 0.f, 1.f), 7.f * U, P.Scorch > 0.5f ? Red : FLinearColor(1.f, 0.62f, 0.22f));
			Text(P.Scorch > 0.5f ? TEXT("SCORCHING") : TEXT("SEAR"), GX - 8.f * U, SY - 6.f * U, 12.f, FLinearColor(1.f, 0.85f, 0.6f), false, true);
		}
	}

	// --- the stew pot on the second burner: kelp first, then the fish, greens last - each at its moment
	{
		const FCPot& Po = K.Pot;
		const FVector2D C = L.Pot;
		const float PS = L.PotR * 2.75f;
		Tex(UITex(TEXT("Pot")), C.X - PS * 0.5f, C.Y - PS * 0.56f, PS, PS);
		const bool bDragLadle = K.bDragFromPot && FVector2D::Distance(M, K.DownAt) >= 8.f * U;
		const FVector2D Mouth = C + FVector2D(0.f, -L.PotR * 0.3f);
		if (Po.Busy())
		{
			// what is in there so far
			int32 n = 0;
			for (ECItem It : Po.Added)
			{
				FCItem Shown; Shown.Kind = It; Shown.Fish = 0;
				const float A = Time * 0.5f + n * 2.1f;
				DrawCounterItem(Shown, Mouth + FVector2D(FMath::Cos(A) * L.PotR * 0.36f, FMath::Sin(A) * L.PotR * 0.12f), 44.f * U, bDragLadle ? 0.5f : 0.95f);
				++n;
			}
			// simmer: lazy bubbles on low, a rolling boil on high
			const int32 NB = 5 + K.Heat[1] * 5;
			for (int32 b = 0; b < NB; ++b)
			{
				const float T = FMath::Frac(Time * (0.5f + 0.35f * K.Heat[1]) + b * 0.37f);
				const float A = b * 2.4f;
				Disc(Mouth.X + FMath::Cos(A) * L.PotR * 0.5f, Mouth.Y + FMath::Sin(A) * L.PotR * 0.16f - T * 8.f * U, (2.f + 4.f * T) * U, FLinearColor(1.f, 1.f, 1.f, 0.35f * (1.f - T)), 8);
			}
			for (int32 s = 0; s < 5; ++s)
			{
				const float T = FMath::Frac(Time * 0.5f + s * 0.21f);
				Disc(Mouth.X + (s - 2) * 18.f * U + FMath::Sin(Time * 2.f + s) * 8.f * U, Mouth.Y - 20.f * U - T * 120.f * U, (10.f + 18.f * T) * U, FLinearColor(1.f, 1.f, 1.f, 0.14f * (1.f - T)), 14);
			}
		}
		else if (K.Drag.Kind == ECItem::Kelp || K.Drag.Kind == ECItem::Sashimi || K.Drag.Kind == ECItem::Greens)
		{
			Circle(Mouth.X, Mouth.Y, L.PotR * 0.6f, FLinearColor(0.4f, 1.f, 0.5f, 0.5f + 0.3f * FMath::Sin(Time * 6.f)), 3.f, 40);
		}
		// timeline: when each thing should go in, and when to ladle it out
		const bool bStewTicket = ActiveR && ActiveR->bStew;
		if (Po.Busy() || bStewTicket)
		{
			const float GW = 230.f * U, GH = 14.f * U, GX = C.X - GW * 0.5f, GY = C.Y - L.PotR - 78.f * U;
			const float Span = FCPot::MushyAt;
			auto TX = [&](float T) { return GX + GW * FMath::Clamp(T / Span, 0.f, 1.f); };
			Panel(GX - 6.f * U, GY - 6.f * U, GW + 12.f * U, GH + 12.f * U, FLinearColor(0.02f, 0.04f, 0.1f, 0.85f));
			Panel(GX, GY, GW, GH, FLinearColor(0.25f, 0.35f, 0.5f, 0.9f));
			Panel(TX(FCPot::ReadyAt - 0.8f), GY, TX(FCPot::ReadyAt + 0.8f) - TX(FCPot::ReadyAt - 0.8f), GH, FLinearColor(0.35f, 1.f, 0.45f, 0.8f));
			Panel(TX(FCPot::MushyAt - 1.5f), GY, GX + GW - TX(FCPot::MushyAt - 1.5f), GH, FLinearColor(0.5f, 0.2f, 0.1f, 0.9f));
			struct FMark { ECItem Kind; float At; };
			const FMark Marks[] = { { ECItem::Kelp, 0.f }, { ECItem::Sashimi, FCPot::FishAt }, { ECItem::Greens, FCPot::GreensAt } };
			const ECItem Next = K.PotNext();
			for (const FMark& Mk : Marks)
			{
				const bool bIn = Po.Has(Mk.Kind);
				const bool bNext = Mk.Kind == Next;
				const float X = TX(Mk.At);
				Panel(X - 1.5f * U, GY - 4.f * U, 3.f * U, GH + 8.f * U, bIn ? FLinearColor(0.4f, 1.f, 0.5f) : bNext ? Gold : FLinearColor(1.f, 1.f, 1.f, 0.6f));
				FCItem Shown; Shown.Kind = Mk.Kind; Shown.Fish = 0;
				const float Bob = bNext ? FMath::Sin(Time * 6.f) * 3.f * U : 0.f;
				DrawCounterItem(Shown, FVector2D(X, GY - 24.f * U + Bob), 34.f * U, bIn ? 0.35f : 1.f);
			}
			if (Po.Busy()) Panel(TX(Po.T) - 2.f * U, GY - 7.f * U, 4.f * U, GH + 14.f * U, FLinearColor::White);
			const float Pulse = 0.6f + 0.4f * FMath::Sin(Time * 8.f);
			const float LY = GY - 68.f * U;
			if (!Po.Busy()) Text(TEXT("Stew: drag KELP into the pot to start"), C.X, LY, 17.f, FLinearColor(1.f, 0.9f, 0.6f, Pulse), true);
			else if (Next == ECItem::Sashimi) Text(Po.T > FCPot::FishAt - 0.6f ? TEXT("Add the FISH now!") : TEXT("Stock is building... fish next"), C.X, LY, 17.f, Po.T > FCPot::FishAt - 0.6f ? FLinearColor(0.5f, 1.f, 0.6f, Pulse) : FLinearColor(1.f, 0.9f, 0.6f), true);
			else if (Next == ECItem::Greens) Text(Po.T > FCPot::GreensAt - 0.6f ? TEXT("Add the GREENS now!") : TEXT("Simmering... greens go in last"), C.X, LY, 17.f, Po.T > FCPot::GreensAt - 0.6f ? FLinearColor(0.5f, 1.f, 0.6f, Pulse) : FLinearColor(1.f, 0.9f, 0.6f), true);
			else if (Next == ECItem::Kelp) Text(TEXT("It needs KELP for the stock!"), C.X, LY, 17.f, FLinearColor(1.f, 0.5f, 0.3f, Pulse), true);
			else if (Po.T >= FCPot::MushyAt - 1.5f) Text(TEXT("OVERCOOKING!"), C.X, LY, 19.f, FLinearColor(1.f, 0.4f, 0.2f, Pulse), true);
			else if (Po.T >= FCPot::ReadyAt - 0.8f) Text(TEXT("READY - drag a ladle to the plate"), C.X, LY, 17.f, FLinearColor(0.4f, 1.f, 0.5f, Pulse), true);
			else Text(TEXT("Simmering..."), C.X, LY, 17.f, FLinearColor(1.f, 0.9f, 0.6f), true);
			if (Po.Busy() && K.Heat[1] == 2) Text(TEXT("Rolling boil - it is breaking up the fish"), C.X, LY - 22.f * U, 14.f, FLinearColor(1.f, 0.5f, 0.3f), true);
		}
	}

	// --- gas knobs: click to step low -> medium -> high
	for (int32 i = 0; i < 2; ++i)
	{
		const FVector2D KP = L.Knob[i];
		const int32 Lv = K.Heat[i];
		const bool bHover = L.KnobAt(M) == i;
		if (bHover) Circle(KP.X, KP.Y, L.KnobR, FLinearColor(1.f, 0.85f, 0.4f, 0.8f), 2.5f, 28);
		for (int32 f = 0; f < 3; ++f)
		{
			// three flames over the knob
			const FVector2D FP = KP + FVector2D((f - 1) * 26.f * U, -L.KnobR - 10.f * U - (f == 1 ? 6.f * U : 0.f));
			const bool bLit = f <= Lv;
			const float Fl = bLit ? 1.f + 0.15f * FMath::Sin(Time * 14.f + f * 2.f + i) : 1.f;
			const FLinearColor FC = !bLit ? FLinearColor(0.1f, 0.12f, 0.2f, 0.8f) : f == 0 ? FLinearColor(0.4f, 0.75f, 1.f) : f == 1 ? FLinearColor(1.f, 0.78f, 0.25f) : FLinearColor(1.f, 0.4f, 0.15f);
			Disc(FP.X, FP.Y, 9.f * U * Fl, FC, 12);
			if (bLit) Disc(FP.X, FP.Y - 7.f * U * Fl, 5.f * U * Fl, FC, 10);
		}
		Panel(KP.X - 34.f * U, KP.Y + L.KnobR - 2.f * U, 68.f * U, 24.f * U, FLinearColor(0.02f, 0.04f, 0.1f, 0.85f));
		Text(Lv == 0 ? TEXT("LOW") : Lv == 1 ? TEXT("MED") : TEXT("HIGH"), KP.X, KP.Y + L.KnobR, 17.f, Lv == 0 ? FLinearColor(0.6f, 0.85f, 1.f) : Lv == 1 ? Gold : FLinearColor(1.f, 0.5f, 0.25f), true);
	}

	// --- plate on the slate mat + the bell
	{
		const float PR = L.PlateR;
		Tex(UITex(TEXT("Plate")), L.Plate.X - PR, L.Plate.Y - PR * 0.56f, PR * 2.f, PR * 1.12f);
		if (K.Drag.IsValid()) Circle(L.Plate.X, L.Plate.Y, PR * 0.9f, FLinearColor(1.f, 0.85f, 0.4f, 0.35f + 0.2f * FMath::Sin(Time * 6.f)), 2.f, 40);
		for (const FCPlateItem& Pi : K.Plate) DrawCounterItem(Pi.Item, L.Plate + FVector2D(Pi.Off.X * PR, Pi.Off.Y * PR * 0.56f), 66.f * U);
		if (K.Plate.Num() == 0) Text(TEXT("Plate"), L.Plate.X, L.Plate.Y - 10.f * U, 18.f, FLinearColor(0.45f, 0.45f, 0.5f, 0.8f), true);
		const bool bHover = L.InBell(M);
		if (bHover || K.BellRing > 0.f) Circle(L.Bell.X, L.Bell.Y, L.BellR * (1.f + 0.4f * K.BellRing), FLinearColor(1.f, 0.85f, 0.4f, 0.7f), 3.f, 32);
		const float TW = 120.f * U, TH = 46.f * U;
		Tex(UITex(TEXT("S_BtnOk")), L.Bell.X - TW * 0.5f, L.Bell.Y - L.BellR - TH - 6.f * U, TW, TH);
		Text(K.BellRing > 0.f ? TEXT("DING!") : TEXT("Serve"), L.Bell.X, L.Bell.Y - L.BellR - TH + 4.f * U, 20.f, FLinearColor::White, true);
	}

	// --- ingredient tiles
	{
		Panel(L.BarPos.X, L.BarPos.Y, L.BarSize.X, L.BarSize.Y, FLinearColor(0.04f, 0.08f, 0.16f, 0.85f));
		Border(L.BarPos.X, L.BarPos.Y, L.BarSize.X, L.BarSize.Y, FLinearColor(0.9f, 0.68f, 0.3f, 1.f), 3.f);
	}
	for (int32 i = 0; i < 9; ++i)
	{
		const FVector2D SP = L.SlotPos[i];
		const bool bHover = L.SlotAt(M) == i;
		const FCItem BI = FKitchenCounter::BarItem(i);
		const bool bEmpty = BI.Kind == ECItem::Fillet && GM->Stock[BI.Fish] <= 0;
		const float Lift = bHover ? 6.f * U : 0.f;
		const float TS = L.SlotSize;
		const TCHAR* Tile = BI.Kind == ECItem::Lemon ? TEXT("S_Tile_Lemon") : BI.Kind == ECItem::Greens ? TEXT("S_Tile_Parsley") : TEXT("S_TileEmpty");
		Tex(UITex(Tile), SP.X, SP.Y - Lift, TS, TS * 204.f / 216.f, bEmpty ? FLinearColor(0.5f, 0.5f, 0.5f, 1.f) : FLinearColor::White);
		if (BI.Kind != ECItem::Lemon && BI.Kind != ECItem::Greens)
			DrawCounterItem(BI, SP + FVector2D(TS * 0.5f, TS * 0.45f - Lift), TS * 0.62f, bEmpty ? 0.35f : 1.f, TS * 0.84f);
		if (BI.Kind == ECItem::Fillet)
		{
			const int32 Pr = GM->Prime[BI.Fish], No = GM->Stock[BI.Fish] - Pr;
			Disc(SP.X + TS - 16.f * U, SP.Y + TS * 0.86f - Lift, 15.f * U, No <= 0 ? FLinearColor(0.7f, 0.1f, 0.1f) : FLinearColor(0.1f, 0.35f, 0.7f), 16);
			Text(FString::FromInt(No), SP.X + TS - 16.f * U, SP.Y + TS * 0.86f - 11.f * U - Lift, 17.f, FLinearColor::White, true);
			if (Pr > 0)
			{
				// the gold strip hands out the prime catch
				const bool bTop = bHover && M.Y < SP.Y + TS * 0.4f;
				Panel(SP.X + 8.f * U, SP.Y + 6.f * U - Lift, TS - 16.f * U, 26.f * U, FLinearColor(0.95f, 0.7f, 0.15f, bTop ? 1.f : 0.82f));
				Star(SP.X + 24.f * U, SP.Y + 19.f * U - Lift, 20.f * U);
				Text(FString::Printf(TEXT("PRIME x%d"), Pr), SP.X + 38.f * U, SP.Y + 8.f * U - Lift, 14.f, FLinearColor(0.25f, 0.13f, 0.02f));
			}
		}
		if (bHover)
		{
			FString Tip = FKitchenCounter::ItemName(BI);
			if (BI.Kind == ECItem::Fillet && GM->Prime[BI.Fish] > 0) Tip = AbyssLoc::Translate(Tip) + (M.Y < SP.Y + TS * 0.4f ? LOCS("  (PRIME)", "（优质）") : LOCS("  (ordinary)", "（普通）"));
			Text(Tip, SP.X + TS * 0.5f, SP.Y - 30.f * U, 17.f, Cream, true);
		}
	}

	// --- clipboard tickets
	static const TCHAR* Cards[] = { TEXT("S_Card_Sashimi"), TEXT("S_Card_Grill"), TEXT("S_Card_Platter"), TEXT("Dish_3"), TEXT("Dish_4") };
	for (int32 t = 0; t < Queue.Num() && t < 4; ++t)
	{
		ARestaurantCustomer* C = Queue[t];
		const FRecipe& R = AbyssRecipes::Get(C->Dish);
		const bool bSel = K.Active.Get() == C;
		const float Sway = FMath::Sin(Time * 1.3f + t) * 2.f * U;
		const FVector2D TP = L.TicketPos[t] + FVector2D(Sway, bSel ? 6.f * U : 0.f);
		const FVector2D TS = L.TicketSize;
		Line(TP.X + TS.X * 0.5f, 0.f, TP.X + TS.X * 0.5f, TP.Y + 10.f * U, FLinearColor(0.75f, 0.6f, 0.35f, 0.9f), 3.f);
		if (bSel) Disc(TP.X + TS.X * 0.5f, TP.Y + TS.Y * 0.5f, TS.X * 0.66f, FLinearColor(1.f, 0.85f, 0.4f, 0.16f), 32);
		Tex(UITex(TEXT("S_Clipboard")), TP.X, TP.Y, TS.X, TS.Y, bSel ? FLinearColor(1.1f, 1.05f, 0.95f, 1.f) : FLinearColor(0.85f, 0.85f, 0.85f, 1.f));
		Text(FString::Printf(TEXT("%d"), t + 1), TP.X + 22.f * U, TP.Y + 30.f * U, 20.f, Ink);
		const float CS = 112.f * U;
		const float CS2 = 100.f * U;
		(void)CS;
		Tex(UITex(Cards[FMath::Clamp(C->Dish, 0, 4)]), TP.X + TS.X * 0.5f - CS2 * 0.5f, TP.Y + 36.f * U, CS2, C->Dish >= 3 ? CS2 : CS2 * 106.f / 102.f);
		Text(R.Name, TP.X + TS.X * 0.5f, TP.Y + 140.f * U, 15.f, Ink, true);
		{
			// tonight's seasoning for this dish, and what the guest fancies
			const int32 Mn = GM->SeasonCount(GM->MenuMain[C->Dish]) > 0 ? GM->MenuMain[C->Dish] : 0;
			const int32 Sd = GM->SeasonCount(GM->MenuSide[C->Dish]) > 0 ? GM->MenuSide[C->Dish] : 0;
			FString Sea = AbyssLoc::Translate(AbyssData::Season(Mn).Name);
			if (Sd > 0) Sea += TEXT(" + ") + AbyssLoc::Translate(AbyssData::Season(Sd).Name);
			const bool bHit = C->Taste != ETaste::None && (AbyssData::Season(Mn).Taste == C->Taste || AbyssData::Season(Sd).Taste == C->Taste);
			Text(Sea, TP.X + TS.X * 0.5f, TP.Y + 160.f * U, 12.f, FLinearColor(0.42f, 0.3f, 0.16f), true);
			if (C->Taste != ETaste::None)
			{
				const FString TL = FString::Printf(TEXT("likes %s"), AbyssData::TasteName(C->Taste));
				const float TW2 = TextW(TL, 12.f) + 14.f * U;
				const FLinearColor TC = AbyssData::TasteColor(C->Taste);
				Panel(TP.X + TS.X - TW2 - 14.f * U, TP.Y + 22.f * U, TW2, 20.f * U, FLinearColor(TC.R * 0.35f, TC.G * 0.35f, TC.B * 0.35f, 0.95f));
				if (bHit) Border(TP.X + TS.X - TW2 - 14.f * U, TP.Y + 22.f * U, TW2, 20.f * U, Green, 2.f);
				Text(TL, TP.X + TS.X - TW2 * 0.5f - 14.f * U, TP.Y + 24.f * U, 12.f, bHit ? FLinearColor(0.7f, 1.f, 0.7f) : FLinearColor::White, true);
			}
		}
		TMap<int32, int32> Need, Have;
		for (EIngredient G : R.Slots) Need.FindOrAdd((int32)G)++;
		if (bSel) for (const FCPlateItem& Pi : K.Plate) { const EIngredient G = Pi.Item.AsIngredient(); if (G != EIngredient::Count) Have.FindOrAdd((int32)G)++; }
		const float IS = 30.f * U;
		float IX = TP.X + TS.X * 0.5f - (Need.Num() * (IS + 6.f * U)) * 0.5f;
		for (const TPair<int32, int32>& N : Need)
		{
			FCItem Shown;
			switch ((EIngredient)N.Key)
			{
			case EIngredient::FishSlice: Shown.Kind = ECItem::Sashimi; Shown.Fish = (int8)R.Fish; break;
			case EIngredient::Steak:     Shown.Kind = ECItem::Cooked; break;
			case EIngredient::Rice:      Shown.Kind = ECItem::Rice; break;
			case EIngredient::Lemon:     Shown.Kind = ECItem::Lemon; break;
			case EIngredient::Greens:    Shown.Kind = ECItem::Greens; break;
			case EIngredient::Kelp:      Shown.Kind = ECItem::Kelp; break;
			case EIngredient::Wasabi:    Shown.Kind = ECItem::Wasabi; break;
			case EIngredient::Stew:      Shown.Kind = ECItem::Stew; break;
			default:                     Shown.Kind = ECItem::Sesame; break;
			}
			DrawCounterItem(Shown, FVector2D(IX + IS * 0.5f, TP.Y + 194.f * U), IS);
			const bool bOk = Have.FindRef(N.Key) >= N.Value;
			Text(FString::FromInt(N.Value), IX + IS - 2.f * U, TP.Y + 196.f * U, 15.f, bOk ? FLinearColor(0.1f, 0.55f, 0.2f) : Ink);
			IX += IS + 6.f * U;
		}
		const float P = C->Patience01();
		const float BX = TP.X + 22.f * U, BY = TP.Y + TS.Y - 34.f * U, BW = TS.X - 44.f * U;
		Panel(BX, BY, BW, 11.f * U, FLinearColor(0.2f, 0.15f, 0.1f, 0.6f));
		Panel(BX, BY, BW * P, 11.f * U, P > 0.5f ? Green : P > 0.25f ? Gold : Red);
	}
	if (Queue.Num() == 0) Text(TEXT("No orders yet - take orders at the tables [E]"), W * 0.5f, 210.f * U, 24.f, Cream, true);

	// --- top bar
	{
		const int32 Lv = 1 + GM->LifetimeServed / 4;
		Tex(UITex(TEXT("S_LvPill")), 84.f * U, 20.f * U, 216.f * U, 71.f * U);
		Text(FString::Printf(TEXT("Lv.%d"), Lv), 128.f * U, 26.f * U, 26.f, FLinearColor::White);
		Panel(128.f * U, 63.f * U, 150.f * U * ((GM->LifetimeServed % 4) / 4.f), 10.f * U, FLinearColor(0.35f, 0.85f, 1.f));
		const float AS = 104.f * U;
		Tex(UITex(TEXT("S_AvatarRing")), 8.f * U, 4.f * U, AS * 93.f / 103.f, AS);
		Tex(UITex(TEXT("AvatarRing")), 22.f * U, 17.f * U, AS * 0.66f, AS * 0.66f);

		const float JW = 330.f * U, JH = JW * 106.f / 374.f;
		const float JX = W * 0.5f - JW * 0.5f;
		Tex(UITex(TEXT("S_JellyPanel")), JX, 6.f * U, JW, JH);
		const int32 Mins = 18 * 60 + FMath::FloorToInt(Dome->NightClock * 2.f);
		Text(FString::Printf(TEXT("Day %d   %02d:%02d"), GM->Day, (Mins / 60) % 24, Mins % 60), JX + JW * 0.56f, 6.f * U + JH * 0.5f - 15.f * U, 24.f, FLinearColor(0.2f, 0.16f, 0.3f), true);

		const float GS = 64.f * U;
		const float GX = W - GS - 12.f * U;
		Tex(UITex(TEXT("S_Gear")), GX, 10.f * U, GS, GS * 63.f / 60.f);
		const float SPW = 200.f * U, SPH = SPW * 54.f / 170.f;
		const float SX = GX - SPW - 12.f * U;
		Tex(UITex(TEXT("S_ShellPill")), SX, 14.f * U, SPW, SPH);
		Text(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(GM->Satisfaction * 100.f)), SX + SPW * 0.5f, 14.f * U + SPH * 0.5f - 13.f * U, 22.f, FLinearColor::White, true);
		const float CPW = 220.f * U, CPH = CPW * 48.f / 185.f;
		const float CX = SX - CPW - 12.f * U;
		Tex(UITex(TEXT("S_CoinPill")), CX, 16.f * U, CPW, CPH);
		Text(FString::Printf(TEXT("%d"), GM->Money), CX + CPW * 0.5f, 16.f * U + CPH * 0.5f - 13.f * U, 22.f, FLinearColor::White, true);
	}

	// --- left menu
	{
		static const TCHAR* Imgs[] = { TEXT("S_Side_Menu"), TEXT("S_Side_Shop"), TEXT("S_Side_Decor"), TEXT("S_Side_Tasks"), TEXT("S_Side_Codex"), TEXT("S_Side_Upgrade") };
		static const TCHAR* Labels[] = { TEXT("Menu"), TEXT("Shop"), TEXT("Decor"), TEXT("Tasks"), TEXT("Codex"), TEXT("Upgrade") };
		bool bTask = false;
		{
			TArray<ARestaurantCustomer*> All;
			Dome->GetCustomers(All);
			for (ARestaurantCustomer* C : All) if (C->WantsToOrder()) bTask = true;
		}
		for (int32 i = 0; i < 6; ++i)
		{
			const FVector2D BP = L.SideBtn[i];
			const bool bHover = L.SideAt(M) == i || (K.SidePanel == 0 && i == 0) || (K.SidePanel == 2 && i == 3) || (K.SidePanel == 3 && i == 4);
			Tex(UITex(Imgs[i]), BP.X, BP.Y, L.SideBtnSize.X, L.SideBtnSize.Y, bHover ? FLinearColor(1.25f, 1.2f, 1.1f, 1.f) : FLinearColor::White);
			Text(Labels[i], BP.X + L.SideBtnSize.X * 0.6f, BP.Y + L.SideBtnSize.Y * 0.5f - 14.f * U, 24.f, FLinearColor::White, true);
			if (i == 3 && bTask) Tex(UITex(TEXT("S_Badge")), BP.X + L.SideBtnSize.X - 30.f * U, BP.Y - 10.f * U, 36.f * U, 36.f * U);
		}
	}
	// back button, submarine, sign
	Tex(UITex(TEXT("S_BtnBack")), L.CloseBtn.X, L.CloseBtn.Y, L.CloseSize.X, L.CloseSize.Y, L.InClose(M) ? FLinearColor(1.25f, 1.25f, 1.25f, 1.f) : FLinearColor::White);
	Text(TEXT("< Dining room [E]"), L.CloseBtn.X + L.CloseSize.X * 0.5f, L.CloseBtn.Y + L.CloseSize.Y * 0.5f - 13.f * U, 20.f, FLinearColor::White, true);
	Tex(UITex(TEXT("S_SubBtn")), L.SubBtn.X - L.SubSize * 0.5f, L.SubBtn.Y - L.SubSize * 0.5f, L.SubSize, L.SubSize * 154.f / 157.f);
	Text(TEXT("Go fishing >"), L.SubBtn.X, L.SubBtn.Y + L.SubSize * 0.24f, 17.f, FLinearColor::White, true);
	{
		const bool bOpenNow = Dome->IsNightActive();
		Tex(UITex(bOpenNow ? TEXT("S_SignOpen") : TEXT("S_SignClosed")), L.SignPos.X, L.SignPos.Y, L.SignSize.X, bOpenNow ? L.SignSize.Y : L.SignSize.X * 90.f / 209.f);
		Text(bOpenNow ? TEXT("OPEN") : TEXT("CLOSED"), L.SignPos.X + L.SignSize.X * 0.5f, L.SignPos.Y + (bOpenNow ? L.SignSize.Y * 0.56f : 24.f * U) - 20.f * U, 34.f, FLinearColor(1.f, 0.9f, 0.62f), true);
	}

	Panel(W * 0.5f - 420.f * U, L.BarPos.Y - 31.f * U, 840.f * U, 26.f * U, FLinearColor(0.02f, 0.04f, 0.1f, 0.8f));
	Text(ActiveR && *ActiveR->Focus ? FString::Printf(TEXT("%s  -  %s"), *AbyssLoc::Translate(ActiveR->Name), *AbyssLoc::Translate(ActiveR->Focus)) : FString(TEXT("Drag ingredients   |   drag the knife down the gold lines   |   knobs set the heat   |   ring the bell to serve")),
		W * 0.5f, L.BarPos.Y - 29.f * U, 17.f, FLinearColor(1.f, 0.92f, 0.7f, 1.f), true);

	if (K.Drag.IsValid() && !((K.DragFromPanActive() || K.bDragFromPot) && FVector2D::Distance(M, K.DownAt) < 8.f * U)) DrawCounterItem(K.Drag, M, 104.f * U, 0.95f);

	if (K.FeedbackTime > 0.f)
	{
		FLinearColor C = K.FeedbackColor;
		C.A = FMath::Clamp(K.FeedbackTime * 2.f, 0.f, 1.f);
		const FVector2D At = K.FeedbackAt.IsZero() ? FVector2D(W * 0.5f, H * 0.5f) : K.FeedbackAt;
		Text(K.Feedback, At.X, At.Y - 60.f * U - (1.4f - K.FeedbackTime) * 30.f * U, 26.f, C, true);
	}

	// side panels
	if (K.SidePanel == 0) DrawRecipeBook(GM);
	else if (K.SidePanel == 3 || K.SidePanel == 4)
	{
		const float PW = 760.f * U, PH = 460.f * U, PX = W * 0.5f - PW * 0.5f, PY = H * 0.5f - PH * 0.5f;
		Panel(0.f, 0.f, W, H, FLinearColor(0.f, 0.01f, 0.03f, 0.45f));
		Tex(UITex(TEXT("Panel")), PX, PY, PW, PH);
		if (K.SidePanel == 3)
		{
			Text(TEXT("TONIGHT"), PX + PW * 0.5f, PY + 22.f * U, 30.f, Gold, true);
			int32 Ordering = 0;
			TArray<ARestaurantCustomer*> All;
			Dome->GetCustomers(All);
			for (ARestaurantCustomer* C : All) if (C->WantsToOrder()) ++Ordering;
			const FString Lines[] = {
				FString::Printf(TEXT("Guests waiting to order: %d"), Ordering),
				FString::Printf(TEXT("Orders to cook: %d"), Queue.Num()),
				FString::Printf(TEXT("Served tonight: %d   (perfect %d)"), GM->NightServed, GM->NightPerfect),
				FString::Printf(TEXT("Guests still coming: %d"), Dome->GetRemaining()),
				FString::Printf(TEXT("Earned tonight: $%d"), GM->NightEarned) };
			for (int32 i = 0; i < 5; ++i) Text(Lines[i], PX + 90.f * U, PY + (110.f + i * 56.f) * U, 24.f, Cream);
		}
		else
		{
			Text(TEXT("FISH CODEX"), PX + PW * 0.5f, PY + 22.f * U, 30.f, Gold, true);
			for (int32 f = 0; f < AbyssData::NumFish; ++f)
			{
				const FFishDef& F = AbyssData::Fish((EFishType)f);
				const float RY = PY + (100.f + f * 112.f) * U;
				Tex(UITex(FString::Printf(TEXT("Fillet_%d"), f)), PX + 60.f * U, RY + 10.f * U, 220.f * U, 69.f * U);
				Text(F.Name, PX + 310.f * U, RY + 8.f * U, 24.f, Cream);
				Text(FString::Printf(TEXT("$%d   |   in stock: %d  (%d prime)"), AbyssData::DishPrice(f), GM->Stock[f], GM->Prime[f]), PX + 310.f * U, RY + 40.f * U, 18.f, Gold);
				Text(AbyssData::PrimeTip((EFishType)f), PX + 310.f * U, RY + 68.f * U, 14.f, Muted);
			}
		}
		Text(TEXT("click anywhere to close"), PX + PW * 0.5f, PY + PH - 54.f * U, 16.f, Muted, true);
	}
}
