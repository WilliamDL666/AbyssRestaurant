#include "PrepMenu.h"
#include "AbyssGameMode.h"
#include "AbyssHUD.h"
#include "AbyssLoc.h"
#include "KitchenGame.h"
#include "AbyssSfx.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"

const int32 FPrepLayout::DishOrder[AbyssData::NumDishes] = { 0, 4, 1, 2, 3 };

FPrepLayout FPrepLayout::Make(float W, float H)
{
	FPrepLayout L;
	L.U = FMath::Min(H / 1080.f, W / 1920.f);
	L.Size = FVector2D(1760.f, 960.f) * L.U;
	L.Pos = FVector2D(W * 0.5f - L.Size.X * 0.5f, H * 0.5f - L.Size.Y * 0.5f);
	return L;
}

FBox2D FPrepLayout::Arrow(int32 Row, int32 Slot, int32 Dir) const
{
	const FBox2D S = Selector(Row, Slot);
	const float AW = 42.f * U;
	return Dir < 0 ? FBox2D(S.Min, FVector2D(S.Min.X + AW, S.Max.Y)) : FBox2D(FVector2D(S.Max.X - AW, S.Min.Y), S.Max);
}

bool FPrepLayout::Click(AAbyssGameMode* GM, const FVector2D& P) const
{
	if (!GM) return false;
	if (OpenBtn().IsInside(P))
	{
		GM->OpenRestaurant();
		return true;
	}
	for (int32 i = 0; i < 2; ++i)
	{
		if (Tab(i).IsInside(P)) { GM->PrepTab = i; AbyssSfx::Play(GM, TEXT("Click")); return true; }
	}
	if (GM->PrepTab == 1)
	{
		for (int32 f = 0; f < AbyssData::NumFish; ++f)
		{
			if (DisplayBtn(f).IsInside(P)) { GM->TankAdd(f); return true; }
			if (SellBtn(f).IsInside(P)) { GM->SellLive(f); return true; }
		}
		for (int32 i = 0; i < AAbyssGameMode::MaxTankSlots; ++i)
		{
			if (!SlotBtn(i).IsInside(P)) continue;
			if (i < GM->Tank.Num()) GM->TankRemove(i);
			else if (i == GM->TankSlots) GM->ExpandTank();
			return true;
		}
		return false;
	}
	for (int32 r = 0; r < AbyssData::NumDishes; ++r)
	{
		for (int32 s = 0; s < 2; ++s)
		{
			if (!Selector(r, s).IsInside(P)) continue;
			// the arrows step back / forward; clicking the name steps forward too
			GM->CycleMenu(DishOrder[r], s, Arrow(r, s, -1).IsInside(P) ? -1 : 1);
			AbyssSfx::Play(GM, TEXT("Click"));
			return true;
		}
	}
	for (int32 r = 0; r < MarketRows; ++r)
	{
		const bool bBait = r == MarketRows - 1;
		if (Plus(r).IsInside(P))
		{
			if (bBait) GM->BuyBait(); else GM->BuySeason(r + 1);
			return true;
		}
		if (Minus(r).IsInside(P))
		{
			if (bBait) GM->ReturnBait(); else GM->ReturnSeason(r + 1);
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------- drawing (part of the HUD)

void AAbyssHUD::DrawPrep(AAbyssGameMode* GM)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const FPrepLayout L = FPrepLayout::Make(W, H);
	const float U = L.U;
	const FVector2D M = GM->PrepMouse;
	const FLinearColor Gold(1.f, 0.78f, 0.32f), Cream(1.f, 0.95f, 0.82f), Muted(0.7f, 0.82f, 0.85f), Green(0.35f, 1.f, 0.45f), Red(1.f, 0.4f, 0.3f);
	const FLinearColor PanelC(0.03f, 0.07f, 0.14f, 0.88f), RowC(0.08f, 0.14f, 0.26f, 0.9f), Edge(0.9f, 0.68f, 0.3f, 0.9f);
	// text sizes below are in 1080p pixels of this panel's own scale
	const float TS = U / FMath::Max(S, 0.001f);
	auto T = [&](const FString& Str, float X, float Y, float Px, const FLinearColor& C, bool bCenter = false, bool bRight = false) { Text(Str, X, Y, Px * TS, C, bCenter, bRight); };
	auto Fill = [&](const FBox2D& B, const FLinearColor& C) { Panel(B.Min.X, B.Min.Y, B.Max.X - B.Min.X, B.Max.Y - B.Min.Y, C); };
	auto Frame = [&](const FBox2D& B, const FLinearColor& C, float Th = 1.5f) { Border(B.Min.X, B.Min.Y, B.Max.X - B.Min.X, B.Max.Y - B.Min.Y, C, Th * TS); };
	auto RowBg = [&](float X, float Y, float Wd, float Ht, const FLinearColor& Tint = FLinearColor::White)
	{
		// a row plate: deep navy with a thin gold rim (rare / finished rows get a warmer or greener rim through Tint)
		Panel(X, Y, Wd, Ht, FLinearColor(0.035f, 0.09f, 0.22f, 0.9f));
		Panel(X, Y, Wd, 3.f * U, FLinearColor(0.3f, 0.5f, 0.8f, 0.25f));
		Border(X, Y, Wd, Ht, FLinearColor(0.95f * Tint.R, 0.72f * Tint.G, 0.32f * Tint.B, 0.8f), 1.5f * U / FMath::Max(S, 0.001f));
	};

	// the dining room stays visible behind a deep-blue veil
	for (int32 b = 0; b < 10; ++b)
	{
		const float K = b / 9.f;
		Panel(0.f, b * H * 0.1f, W, H * 0.1f + 1.f, FLinearColor(FMath::Lerp(0.04f, 0.01f, K), FMath::Lerp(0.03f, 0.04f, K), FMath::Lerp(0.14f, 0.1f, K), 0.78f));
	}
	{
		// GPT panel art: its gold frame (x 120..1640, y 151..862 of the 1760x960 picture) is fitted around the panel
		const float Mg = 22.f * U;
		const float DW = (L.Size.X + 2.f * Mg) * 1760.f / (1640.f - 120.f), DH = (L.Size.Y + 2.f * Mg) * 960.f / (862.f - 151.f);
		Fill(FBox2D(L.Pos, L.Pos + L.Size), FLinearColor(PanelC.R, PanelC.G, PanelC.B, 0.6f));
		Tex(UITex(TEXT("A_Panel")), L.Pos.X - Mg - 120.f * DW / 1760.f, L.Pos.Y - Mg - 151.f * DH / 960.f, DW, DH);
	}

	// ---- header
	T(FString::Printf(TEXT("Day %d  -  BEFORE OPENING"), GM->Day), L.Pos.X + L.Size.X * 0.5f - 150.f * U, L.Pos.Y + 24.f * U, 30.f, Gold, false, true);
	T(GM->PrepTab == 0 ? TEXT("Set tonight's menu and stock the pantry. House style is always free.") : TEXT("Live fish on show bring ticket money and guests every night - but cost upkeep and cannot be cooked."),
		L.Pos.X + L.Size.X * 0.5f, L.Pos.Y + 72.f * U, 14.f, Muted, true);
	{
		const float CW = 230.f * U, CH = CW * 48.f / 185.f;
		const float CX = L.Pos.X + L.Size.X - CW - 24.f * U, CY = L.Pos.Y + 22.f * U;
		Tex(UITex(TEXT("S_CoinPill")), CX, CY, CW, CH);
		T(FString::Printf(TEXT("%d"), GM->Money), CX + CW * 0.55f, CY + CH * 0.5f - 14.f * U, 24.f, FLinearColor::White, true);
		T(FString::Printf(TEXT("Pantry %d / %d"), GM->PantryCount(), AAbyssGameMode::PantryCapacity), CX - 16.f * U, CY + 16.f * U, 17.f,
			GM->PantryCount() >= AAbyssGameMode::PantryCapacity ? Red : Cream, false, true);
	}

	// ---- tabs
	const bool bMenu = GM->PrepTab == 0;
	for (int32 i = 0; i < 2; ++i)
	{
		const FBox2D B = L.Tab(i);
		const bool bOn = GM->PrepTab == i;
		Tex(UITex(bOn ? TEXT("A_Tab_On") : TEXT("A_Tab_Off")), B.Min.X, B.Min.Y - 6.f * U, B.Max.X - B.Min.X, B.Max.Y - B.Min.Y + 8.f * U,
			!bOn && B.IsInside(M) ? FLinearColor(1.35f, 1.35f, 1.35f, 1.f) : FLinearColor::White);
		T(i == 0 ? TEXT("Menu & Market") : TEXT("Aquarium"), (B.Min.X + B.Max.X) * 0.5f, B.Min.Y + 15.f * U, 19.f, bOn ? Gold : Cream, true);
		if (i == 1 && GM->Tank.Num() > 0) T(FString::Printf(TEXT("%d"), GM->Tank.Num()), B.Max.X - 14.f * U, B.Min.Y + 4.f * U, 14.f, Green, false, true);
	}

	// ---- left: the fridge + tonight's crowd
	if (bMenu)
	{
		const FBox2D C = L.FridgeCol();
		Fill(C, FLinearColor(0.02f, 0.05f, 0.1f, 0.45f));
		const float X = C.Min.X + 18.f * U;
		float Y = C.Min.Y + 14.f * U;
		T(TEXT("TODAY'S CATCH"), X, Y, 22.f, Gold);
		Y += 40.f * U;
		for (int32 f = 0; f < AbyssData::NumFish; ++f)
		{
			const FFishDef& F = AbyssData::Fish((EFishType)f);
			const int32 Pr = GM->Prime[f], No = GM->Stock[f] - Pr;
			RowBg(X - 6.f * U, Y, C.Max.X - C.Min.X - 24.f * U, 100.f * U);
			Tex(UITex(FString::Printf(TEXT("Fillet_%d"), f)), X + 2.f * U, Y + 8.f * U, 150.f * U, 47.f * U, GM->Stock[f] > 0 ? FLinearColor::White : FLinearColor(0.4f, 0.4f, 0.45f, 1.f));
			T(F.Name, X + 164.f * U, Y + 8.f * U, 19.f, Cream);
			Star(X + 176.f * U, Y + 48.f * U, 20.f * U, Pr > 0 ? 1.f : 0.3f);
			T(FString::Printf(TEXT("PRIME x%d"), Pr), X + 192.f * U, Y + 36.f * U, 17.f, Pr > 0 ? Gold : FLinearColor(0.6f, 0.55f, 0.4f, 0.7f));
			T(FString::Printf(TEXT("ordinary x%d"), No), X + 164.f * U, Y + 64.f * U, 16.f, No > 0 ? FLinearColor::White : FLinearColor(0.6f, 0.6f, 0.65f, 0.7f));
			T(FString::Printf(TEXT("+%d%%"), FMath::RoundToInt(AbyssData::PrimeBonus * 100.f)), X + 316.f * U, Y + 36.f * U, 15.f, Pr > 0 ? Gold : FLinearColor(0.6f, 0.55f, 0.4f, 0.5f));
			Y += 108.f * U;
		}
		Y += 8.f * U;
		T(TEXT("TONIGHT'S GUESTS"), X, Y, 22.f, Gold);
		Y += 38.f * U;
		T(FString::Printf(TEXT("Expected: %d guests"), GM->NightCustomers), X, Y, 18.f, Cream);
		Y += 30.f * U;
		const FLinearColor TC = AbyssData::TasteColor(GM->TonightTaste);
		T(TEXT("Most are craving:"), X, Y, 18.f, Cream);
		Y += 30.f * U;
		Panel(X, Y, 220.f * U, 46.f * U, FLinearColor(TC.R * 0.3f, TC.G * 0.3f, TC.B * 0.3f, 0.95f));
		Border(X, Y, 220.f * U, 46.f * U, TC, 2.f * TS);
		TasteIcon(GM->TonightTaste, X + 30.f * U, Y + 23.f * U, 44.f * U);
		T(AbyssData::TasteName(GM->TonightTaste), X + 128.f * U, Y + 8.f * U, 26.f, TC, true);
		Y += 60.f * U;
		T(TEXT("A seasoning or garnish that matches"), X, Y, 15.f, Muted);
		T(TEXT("a guest's taste doubles the tip."), X, Y + 22.f * U, 15.f, Muted);
		Y += 62.f * U;
		T(TEXT("Fresh garnishes spoil - buy what"), X, Y, 15.f, Muted);
		T(TEXT("you can sell. Rare saffron only pays"), X, Y + 22.f * U, 15.f, Muted);
		T(TEXT("off on a PRIME catch."), X, Y + 44.f * U, 15.f, Muted);
	}

	// ---- middle: tonight's menu
	bool bMissing = false;
	for (int32 r = 0; bMenu && r < AbyssData::NumDishes; ++r)
	{
		const int32 Dish = FPrepLayout::DishOrder[r];
		const FRecipe& R = AbyssRecipes::Get(Dish);
		const FBox2D B = L.DishRow(r);
		const int32 Fish = AbyssData::DishFish(Dish);
		const int32 Stock = Fish >= 0 ? GM->Stock[Fish] : 99;
		const bool bPrimeFish = Fish >= 0 && GM->Prime[Fish] > 0;
		RowBg(B.Min.X, B.Min.Y, B.Max.X - B.Min.X, B.Max.Y - B.Min.Y);
		const float X = B.Min.X, Y = B.Min.Y;
		Tex(UITex(FString::Printf(TEXT("Dish_%d"), Dish)), X + 10.f * U, Y + 12.f * U, 118.f * U, 118.f * U, Stock > 0 ? FLinearColor::White : FLinearColor(0.45f, 0.45f, 0.5f, 1.f));
		T(R.Name, X + 140.f * U, Y + 8.f * U, 22.f, Cream);
		T(R.Focus, X + 140.f * U, Y + 38.f * U, 14.f, Muted);
		if (Fish >= 0)
		{
			if (Stock > 0) T(FString::Printf(TEXT("%s x%d"), AbyssData::Fish((EFishType)Fish).Name, Stock), B.Max.X - 14.f * U, Y + 10.f * U, 15.f, Cream, false, true);
			else T(TEXT("No fish - guests cannot order it"), B.Max.X - 14.f * U, Y + 10.f * U, 15.f, Red, false, true);
		}
		const int32 Mn = GM->MenuMain[Dish], Sd = GM->MenuSide[Dish];
		// what it sells for tonight
		{
			const int32 P0 = AbyssData::DishPrice(Dish);
			const int32 P1 = GM->DishFinalPrice(Dish, false, Mn, Sd);
			FString Pr = P1 != P0 ? FString::Printf(TEXT("$%d > $%d"), P0, P1) : FString::Printf(TEXT("$%d"), P0);
			T(Pr, B.Max.X - 14.f * U, Y + 34.f * U, 20.f, P1 > P0 ? Green : Gold, false, true);
			if (Fish >= 0)
			{
				const FString PP = FString::Printf(TEXT("PRIME $%d"), GM->DishFinalPrice(Dish, true, Mn, Sd));
				T(PP, B.Max.X - 14.f * U, Y + 60.f * U, 16.f, bPrimeFish ? Gold : FLinearColor(0.7f, 0.6f, 0.4f, 0.6f), false, true);
				Star(B.Max.X - 24.f * U - TextW(PP, 16.f * TS), Y + 71.f * U, 16.f * U, bPrimeFish ? 1.f : 0.4f);
			}
		}
		for (int32 s = 0; s < 2; ++s)
		{
			const FBox2D Sel = L.Selector(r, s);
			const int32 Id = s == 0 ? Mn : Sd;
			const FSeasonDef& D = AbyssData::Season(Id);
			const int32 Have = GM->SeasonCount(Id);
			const bool bHover = Sel.IsInside(M);
			if (Id > 0 && Have <= 0) bMissing = true;
			T(s == 0 ? TEXT("Seasoning") : TEXT("Garnish (optional)"), Sel.Min.X + 4.f * U, Sel.Min.Y - 20.f * U, 13.f, Muted);
			Fill(Sel, bHover ? FLinearColor(0.16f, 0.26f, 0.44f, 1.f) : FLinearColor(0.05f, 0.1f, 0.2f, 1.f));
			Frame(Sel, Id > 0 && Have <= 0 ? Red : Id > 0 ? AbyssData::TasteColor(D.Taste) : FLinearColor(0.5f, 0.6f, 0.75f, 0.7f), 1.5f);
			const float AS = 30.f * U;
			Tex(UITex(TEXT("S_ArrowL")), Sel.Min.X + 6.f * U, Sel.Min.Y + 6.f * U, AS, AS, L.Arrow(r, s, -1).IsInside(M) ? FLinearColor(1.3f, 1.3f, 1.3f, 1.f) : FLinearColor::White);
			Tex(UITex(TEXT("S_ArrowR")), Sel.Max.X - 6.f * U - AS, Sel.Min.Y + 6.f * U, AS, AS, L.Arrow(r, s, 1).IsInside(M) ? FLinearColor(1.3f, 1.3f, 1.3f, 1.f) : FLinearColor::White);
			const float CX = (Sel.Min.X + Sel.Max.X) * 0.5f;
			if (Id == 0)
			{
				T(s == 0 ? TEXT("House Style") : TEXT("None"), CX, Sel.Min.Y + 9.f * U, 17.f, FLinearColor(0.8f, 0.85f, 0.9f), true);
			}
			else
			{
				const FString Nm = FString::Printf(TEXT("%s  x%d"), *AbyssLoc::Translate(D.Name), Have);
				T(Nm, CX + 12.f * U, Sel.Min.Y + 9.f * U, 17.f, Have > 0 ? FLinearColor::White : Red, true);
				TasteIcon(D.Taste, CX - TextW(Nm, 17.f * TS) * 0.5f - 8.f * U, Sel.Min.Y + 21.f * U, 26.f * U);
			}
		}
	}

	// ---- right: the market
	if (bMenu) T(TEXT("MARKET"), L.MarketRow(0).Min.X + 4.f * U, L.Pos.Y + 74.f * U, 22.f, Gold);
	for (int32 r = 0; bMenu && r < FPrepLayout::MarketRows; ++r)
	{
		const bool bBait = r == FPrepLayout::MarketRows - 1;
		const int32 Id = r + 1;
		const FBox2D B = L.MarketRow(r);
		const float X = B.Min.X, Y = B.Min.Y;
		const FSeasonDef* D = bBait ? nullptr : &AbyssData::Season(Id);
		const int32 Tier = D ? D->Tier : 0;
		RowBg(B.Min.X, B.Min.Y, B.Max.X - B.Min.X, B.Max.Y - B.Min.Y, Tier == 2 ? FLinearColor(1.25f, 1.1f, 0.8f, 1.f) : FLinearColor::White);
		Tex(UITex(bBait ? FString(TEXT("C_Bait")) : FString::Printf(TEXT("C_Sea_%d"), Id)), X + 10.f * U, Y + 5.f * U, 76.f * U, 76.f * U);
		const int32 Have = bBait ? GM->Bait : GM->SeasonCount(Id);
		const int32 Cost = bBait ? AAbyssGameMode::BaitCost : D->Cost;
		const int32 Bought = bBait ? GM->BaitBoughtNow : GM->BoughtNow[Id];
		T(bBait ? TEXT("Grouper Bait") : D->Name, X + 90.f * U, Y + 6.f * U, 19.f, Cream);
		if (bBait)
		{
			T(TEXT("Drop it at sea [B] to lure a grouper out"), X + 90.f * U, Y + 34.f * U, 13.f, Muted);
			T(TEXT("Tool"), X + 90.f * U, Y + 56.f * U, 13.f, FLinearColor(1.f, 0.7f, 0.4f));
		}
		else
		{
			const FString Kind = D->Slot == 0 ? TEXT("Seasoning") : TEXT("Garnish");
			const FString Tag = Tier == 2 ? TEXT("Rare") : Tier == 1 ? TEXT("Perishable") : TEXT("Basic");
			T(FString::Printf(TEXT("%s  -  %s"), *AbyssLoc::Translate(Tag), *AbyssLoc::Translate(Kind)), X + 90.f * U, Y + 34.f * U, 13.f, Tier == 2 ? Gold : Tier == 1 ? FLinearColor(0.6f, 1.f, 0.6f) : Muted);
			const FLinearColor TC = AbyssData::TasteColor(D->Taste);
			TasteIcon(D->Taste, X + 96.f * U, Y + 65.f * U, 20.f * U);
			FString Eff = FString::Printf(TEXT("%s   +%d%%"), *AbyssLoc::Translate(AbyssData::TasteName(D->Taste)), FMath::RoundToInt((D->Mult - 1.f) * 100.f));
			if (D->PrimeMult > D->Mult) Eff += FString::Printf(TEXT("   (%s +%d%%)"), LOCS("PRIME", "优质"), FMath::RoundToInt((D->PrimeMult - 1.f) * 100.f));
			if (D->Shelf > 0) Eff += FString::Printf(TEXT("   %s"), *AbyssLoc::Translate(D->Shelf == 1 ? FString(TEXT("spoils tonight")) : FString::Printf(TEXT("keeps %d nights"), D->Shelf)));
			T(Eff, X + 106.f * U, Y + 54.f * U, 13.f, TC);
		}
		T(FString::Printf(TEXT("$%d"), Cost), X + 436.f * U, Y + 8.f * U, 19.f, GM->Money >= Cost ? Gold : Red, false, true);
		T(FString::Printf(TEXT("x%d"), Have), X + 436.f * U, Y + 34.f * U, 19.f, Have > 0 ? FLinearColor::White : FLinearColor(0.6f, 0.6f, 0.65f), false, true);
		if (!bBait && GM->SeasonExpiring(Id) > 0) T(FString::Printf(TEXT("%d spoil tonight"), GM->SeasonExpiring(Id)), X + 436.f * U, Y + 60.f * U, 12.f, FLinearColor(1.f, 0.65f, 0.3f), false, true);
		const FBox2D Mi = L.Minus(r), Pl = L.Plus(r);
		Tex(UITex(TEXT("S_Minus")), Mi.Min.X, Mi.Min.Y, Mi.Max.X - Mi.Min.X, Mi.Max.Y - Mi.Min.Y,
			Bought <= 0 ? FLinearColor(0.4f, 0.4f, 0.45f, 0.6f) : Mi.IsInside(M) ? FLinearColor(1.3f, 1.3f, 1.3f, 1.f) : FLinearColor::White);
		Tex(UITex(TEXT("S_Plus")), Pl.Min.X, Pl.Min.Y, Pl.Max.X - Pl.Min.X, Pl.Max.Y - Pl.Min.Y,
			GM->Money < Cost ? FLinearColor(0.4f, 0.4f, 0.45f, 0.6f) : Pl.IsInside(M) ? FLinearColor(1.3f, 1.3f, 1.3f, 1.f) : FLinearColor::White);
	}

	if (!bMenu) DrawPrepAquarium(GM);

	// ---- open
	{
		const FBox2D B = L.OpenBtn();
		const bool bHover = B.IsInside(M);
		const float Pu = bHover ? 1.04f : 1.f + 0.015f * FMath::Sin(Time * 4.f);
		const FVector2D C = B.GetCenter(), Sz = (B.Max - B.Min) * Pu;
		Tex(UITex(TEXT("S_BtnOk")), C.X - Sz.X * 0.5f, C.Y - Sz.Y * 0.5f, Sz.X, Sz.Y, bHover ? FLinearColor(1.2f, 1.2f, 1.2f, 1.f) : FLinearColor::White);
		T(TEXT("OPEN THE RESTAURANT"), C.X, C.Y - 16.f * U, 26.f, FLinearColor::White, true);
		if (bMissing) T(TEXT("Some picks are out of stock - those dishes go out house style"), B.Max.X + 18.f * U, B.Min.Y + 26.f * U, 13.f, FLinearColor(1.f, 0.65f, 0.3f), false, false);
	}
}

// ---------------------------------------------------------------- aquarium tab

void AAbyssHUD::DrawPrepAquarium(AAbyssGameMode* GM)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const FPrepLayout L = FPrepLayout::Make(W, H);
	const float U = L.U;
	const FVector2D M = GM->PrepMouse;
	const FLinearColor Gold(1.f, 0.78f, 0.32f), Cream(1.f, 0.95f, 0.82f), Muted(0.7f, 0.82f, 0.85f), Green(0.35f, 1.f, 0.45f), Red(1.f, 0.4f, 0.3f);
	const FLinearColor RowC(0.08f, 0.14f, 0.26f, 0.9f);
	const float TS = U / FMath::Max(S, 0.001f);
	auto T = [&](const FString& Str, float X, float Y, float Px, const FLinearColor& C, bool bCenter = false, bool bRight = false) { Text(Str, X, Y, Px * TS, C, bCenter, bRight); };
	auto Fill = [&](const FBox2D& B, const FLinearColor& C) { Panel(B.Min.X, B.Min.Y, B.Max.X - B.Min.X, B.Max.Y - B.Min.Y, C); };
	auto Frame = [&](const FBox2D& B, const FLinearColor& C, float Th = 1.5f) { Border(B.Min.X, B.Min.Y, B.Max.X - B.Min.X, B.Max.Y - B.Min.Y, C, Th * TS); };
	auto RowBg = [&](float X, float Y, float Wd, float Ht, const FLinearColor& Tint = FLinearColor::White)
	{
		// a row plate: deep navy with a thin gold rim (rare / finished rows get a warmer or greener rim through Tint)
		Panel(X, Y, Wd, Ht, FLinearColor(0.035f, 0.09f, 0.22f, 0.9f));
		Panel(X, Y, Wd, 3.f * U, FLinearColor(0.3f, 0.5f, 0.8f, 0.25f));
		Border(X, Y, Wd, Ht, FLinearColor(0.95f * Tint.R, 0.72f * Tint.G, 0.32f * Tint.B, 0.8f), 1.5f * U / FMath::Max(S, 0.001f));
	};
	auto Button = [&](const FBox2D& B, const FString& Label, const FLinearColor& C, bool bEnabled)
	{
		const bool bHover = bEnabled && B.IsInside(M);
		// gold = put on show / expand, teal = sell, red = take out
		const TCHAR* Art = C.R > 0.55f && C.G > 0.4f ? TEXT("A_Btn_Gold") : C.R > 0.5f ? TEXT("A_Btn_Red") : C.G > 0.45f ? TEXT("A_Btn_Teal") : TEXT("A_Btn_Gold");
		const float Gx = 6.f * U, Gy = 5.f * U;
		Tex(UITex(Art), B.Min.X - Gx, B.Min.Y - Gy, B.Max.X - B.Min.X + 2.f * Gx, B.Max.Y - B.Min.Y + 2.f * Gy,
			!bEnabled ? FLinearColor(0.35f, 0.36f, 0.4f, 0.75f) : bHover ? FLinearColor(1.25f, 1.25f, 1.25f, 1.f) : FLinearColor::White);
		T(Label, (B.Min.X + B.Max.X) * 0.5f, B.Min.Y + 10.f * U, 16.f, bEnabled ? FLinearColor::White : FLinearColor(0.7f, 0.7f, 0.75f), true);
	};
	auto Portrait = [&](int32 f, float CX, float CY, float Sz, float Alpha)
	{
		static const TCHAR* Pn[] = { TEXT("B_Fish_Yellowtail"), TEXT("B_Fish_Tuna"), TEXT("B_Fish_Grouper") };
		Tex(UITex(Pn[FMath::Clamp(f, 0, 2)]), CX - Sz * 0.5f, CY - Sz * 0.5f, Sz, Sz, FLinearColor(1.f, 1.f, 1.f, Alpha));
	};
	auto FishPic = [&](int32 f, float X, float Y, float Wd, float Alpha)
	{
		// side-view render of the fish (Scripts/blender_make_fish.py), 2:1
		Tex(UITex(FString::Printf(TEXT("Fish_%d"), f)), X, Y, Wd, Wd * 0.5f, FLinearColor(1.f, 1.f, 1.f, Alpha));
	};

	// ---- left: live catch -> on show, or sold
	T(TEXT("LIVE CATCH"), L.Pos.X + 42.f * U, L.Pos.Y + 108.f * U, 22.f, Gold);
	for (int32 f = 0; f < AbyssData::NumFish; ++f)
	{
		const FFishDef& D = AbyssData::Fish((EFishType)f);
		const FBox2D B = L.LiveRow(f);
		const int32 Lv = GM->Live[f];
		RowBg(B.Min.X, B.Min.Y, B.Max.X - B.Min.X, B.Max.Y - B.Min.Y);
		Portrait(f, B.Min.X + 84.f * U, B.Min.Y + 46.f * U, 78.f * U, Lv > 0 ? 1.f : 0.4f);
		T(D.Name, B.Min.X + 168.f * U, B.Min.Y + 8.f * U, 19.f, Cream);
		T(FString::Printf(TEXT("alive x%d"), Lv), B.Min.X + 168.f * U, B.Min.Y + 36.f * U, 17.f, Lv > 0 ? Green : FLinearColor(0.6f, 0.6f, 0.65f));
		T(FString::Printf(TEXT("in fridge x%d"), GM->Stock[f]), B.Min.X + 168.f * U, B.Min.Y + 60.f * U, 13.f, Muted);
		Button(L.DisplayBtn(f), TEXT("Put on show"), FLinearColor(0.75f, 0.5f, 0.12f), Lv > 0 && GM->Tank.Num() < GM->TankSlots);
		Button(L.SellBtn(f), FString::Printf(TEXT("Sell  $%d"), AbyssData::LiveSalePrice((EFishType)f, false)), FLinearColor(0.1f, 0.5f, 0.45f), Lv > 0);
	}
	{
		const float X = L.Pos.X + 42.f * U;
		float Y = L.Pos.Y + 612.f * U;
		T(TEXT("Only fish caught alive count:"), X, Y, 15.f, Muted);
		T(TEXT("the net and the sub claw. A speared"), X, Y + 22.f * U, 15.f, Muted);
		T(TEXT("fish can only be cooked."), X, Y + 44.f * U, 15.f, Muted);
		T(TEXT("A fish on show can be taken back"), X, Y + 80.f * U, 15.f, Muted);
		T(TEXT("out on a later day. No breeding."), X, Y + 102.f * U, 15.f, Muted);
	}

	// ---- middle: the display tank
	for (int32 i = 0; i < AAbyssGameMode::MaxTankSlots; ++i)
	{
		const FBox2D B = L.TankSlot(i);
		const FVector2D C = B.GetCenter();
		const bool bOpen = i < GM->TankSlots;
		// the tank card: glass on top, name plate underneath (440 x 570 art)
		const float AW = B.Max.X - B.Min.X, AH = AW * 570.f / 440.f;
		Tex(UITex(bOpen ? TEXT("A_TankSlot") : TEXT("A_TankSlot_Locked")), B.Min.X, B.Min.Y, AW, AH);
		if (i < GM->Tank.Num())
		{
			const FTankFish& F = GM->Tank[i];
			const FFishDef& D = AbyssData::Fish(F.Type);
			for (int32 b = 0; b < 6; ++b)
			{
				const float Tb = FMath::Frac(Time * 0.25f + b * 0.17f + i * 0.31f);
				Disc(B.Min.X + (30.f + b * 32.f) * U + FMath::Sin(Time * 2.f + b) * 4.f * U, B.Min.Y + (190.f - Tb * 170.f) * U, (2.f + 3.f * Tb) * U, FLinearColor(1.f, 1.f, 1.f, 0.25f * (1.f - Tb)), 8);
			}
			const float Sway = FMath::Sin(Time * 1.2f + i) * 8.f * U;
			FishPic((int32)F.Type, C.X - 78.f * U + Sway, B.Min.Y + 92.f * U + FMath::Sin(Time * 0.8f + i * 2.f) * 6.f * U, 156.f * U, 1.f);
			if (F.bPrime) Star(B.Max.X - 34.f * U, B.Min.Y + 70.f * U, 26.f * U);
			T(D.Name, C.X, B.Min.Y + 236.f * U, 16.f, Cream, true);
			Tex(UITex(TEXT("B_Ticket")), B.Min.X + 14.f * U, B.Min.Y + 284.f * U, 24.f * U, 24.f * U);
			T(FString::Printf(TEXT("+$%d"), AbyssData::TankTicket(F.Type, F.bPrime)), B.Min.X + 42.f * U, B.Min.Y + 286.f * U, 15.f, Green);
			Tex(UITex(TEXT("B_Upkeep")), C.X + 8.f * U, B.Min.Y + 284.f * U, 24.f * U, 24.f * U);
			T(FString::Printf(TEXT("-$%d"), AbyssData::TankUpkeep(F.Type)), C.X + 36.f * U, B.Min.Y + 286.f * U, 15.f, FLinearColor(1.f, 0.7f, 0.45f));
			Button(L.SlotBtn(i), TEXT("Take out"), FLinearColor(0.6f, 0.2f, 0.18f), true);
		}
		else if (bOpen)
		{
			T(TEXT("Empty"), C.X, B.Min.Y + 236.f * U, 16.f, FLinearColor(0.8f, 0.9f, 1.f, 0.75f), true);
			T(TEXT("put a live fish on show"), C.X, B.Min.Y + 290.f * U, 13.f, FLinearColor(0.7f, 0.85f, 1.f, 0.6f), true);
		}
		else
		{
			if (i == GM->TankSlots) Button(L.SlotBtn(i), FString::Printf(TEXT("Expand  $%d"), GM->ExpandCost()), FLinearColor(0.2f, 0.35f, 0.6f), GM->Money >= GM->ExpandCost());
		}
	}

	// ---- right: tonight + research
	{
		const float X = L.Pos.X + 1158.f * U, Wd = 578.f * U;
		float Y = L.Pos.Y + 104.f * U;
		const int32 Tk = GM->TankTicketsPerNight(), Up = GM->TankUpkeepPerNight(), Gs = GM->TankExtraGuests();
		RowBg(X, Y, Wd, 196.f * U);
		T(TEXT("TONIGHT'S AQUARIUM"), X + 22.f * U, Y + 12.f * U, 20.f, Gold);
		auto Row = [&](const TCHAR* Icon, const FString& A, const FString& V, const FLinearColor& C, float RY)
		{
			Tex(UITex(Icon), X + 20.f * U, RY - 3.f * U, 26.f * U, 26.f * U);
			T(A, X + 54.f * U, RY, 17.f, Cream);
			T(V, X + Wd - 24.f * U, RY, 17.f, C, false, true);
		};
		Row(TEXT("B_Ticket"), TEXT("Ticket money"), FString::Printf(TEXT("+$%d"), Tk), Green, Y + 48.f * U);
		Row(TEXT("B_Upkeep"), TEXT("Upkeep"), FString::Printf(TEXT("-$%d"), Up), FLinearColor(1.f, 0.7f, 0.45f), Y + 76.f * U);
		Row(TEXT("Coin"), TEXT("Net per night"), FString::Printf(TEXT("%s$%d"), Tk - Up >= 0 ? TEXT("+") : TEXT("-"), FMath::Abs(Tk - Up)), Tk - Up >= 0 ? Gold : Red, Y + 104.f * U);
		Row(TEXT("B_Guests"), TEXT("Extra guests (1 per species)"), FString::Printf(TEXT("+%d"), Gs), Green, Y + 132.f * U);
		Row(TEXT("B_Expand"), TEXT("Tank space"), FString::Printf(TEXT("%d / %d"), GM->Tank.Num(), GM->TankSlots), Cream, Y + 160.f * U);
		Y += 212.f * U;
		Tex(UITex(TEXT("B_Research")), X + 2.f * U, Y - 4.f * U, 30.f * U, 30.f * U);
		T(TEXT("FISH RESEARCH"), X + 38.f * U, Y, 20.f, Gold);
		Y += 34.f * U;
		for (int32 f = 0; f < AbyssData::NumFish; ++f)
		{
			const FFishDef& D = AbyssData::Fish((EFishType)f);
			const int32 N = FMath::Min(GM->Studied[f], AbyssData::ResearchNights);
			const bool bDone = GM->IsStudied((EFishType)f);
			RowBg(X, Y, Wd, 136.f * U, bDone ? FLinearColor(0.9f, 1.25f, 1.f, 1.f) : FLinearColor::White);
			Portrait(f, X + 70.f * U, Y + 38.f * U, 62.f * U, GM->Studied[f] > 0 ? 1.f : 0.35f);
			T(D.Name, X + 140.f * U, Y + 8.f * U, 18.f, Cream);
			// progress
			Panel(X + 140.f * U, Y + 38.f * U, 200.f * U, 10.f * U, FLinearColor(0.f, 0.f, 0.f, 0.6f));
			Panel(X + 140.f * U, Y + 38.f * U, 200.f * U * N / AbyssData::ResearchNights, 10.f * U, bDone ? Green : Gold);
			T(bDone ? FString(TEXT("Research complete")) : FString::Printf(TEXT("%d / %d nights on show"), N, AbyssData::ResearchNights), X + 352.f * U, Y + 30.f * U, 14.f, bDone ? Green : Muted);
			if (GM->Studied[f] > 0) T(D.Hint, X + 22.f * U, Y + 76.f * U, 12.f, Cream);
			else T(TEXT("Put one on show to start studying it"), X + 22.f * U, Y + 76.f * U, 13.f, Muted);
			if (bDone) T(AbyssData::ResearchNote((EFishType)f), X + 22.f * U, Y + 102.f * U, 12.f, Green);
			else T(TEXT("???"), X + 22.f * U, Y + 102.f * U, 13.f, FLinearColor(0.6f, 0.6f, 0.65f, 0.7f));
			Y += 146.f * U;
		}
	}
}
