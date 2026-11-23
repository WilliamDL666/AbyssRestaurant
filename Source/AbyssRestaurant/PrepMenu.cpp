#include "PrepMenu.h"
#include "AbyssGameMode.h"
#include "AbyssHUD.h"
#include "AbyssLoc.h"
#include "KitchenGame.h"
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
	for (int32 r = 0; r < AbyssData::NumDishes; ++r)
	{
		for (int32 s = 0; s < 2; ++s)
		{
			if (!Selector(r, s).IsInside(P)) continue;
			// the arrows step back / forward; clicking the name steps forward too
			GM->CycleMenu(DishOrder[r], s, Arrow(r, s, -1).IsInside(P) ? -1 : 1);
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

	// the dining room stays visible behind a deep-blue veil
	for (int32 b = 0; b < 10; ++b)
	{
		const float K = b / 9.f;
		Panel(0.f, b * H * 0.1f, W, H * 0.1f + 1.f, FLinearColor(FMath::Lerp(0.04f, 0.01f, K), FMath::Lerp(0.03f, 0.04f, K), FMath::Lerp(0.14f, 0.1f, K), 0.78f));
	}
	Fill(FBox2D(L.Pos, L.Pos + L.Size), PanelC);
	Frame(FBox2D(L.Pos, L.Pos + L.Size), Edge, 3.f);

	// ---- header
	T(FString::Printf(TEXT("Day %d  -  BEFORE OPENING"), GM->Day), L.Pos.X + L.Size.X * 0.5f, L.Pos.Y + 22.f * U, 36.f, Gold, true);
	T(TEXT("Set tonight's menu and stock the pantry. House style is always free."), L.Pos.X + L.Size.X * 0.5f, L.Pos.Y + 68.f * U, 16.f, Muted, true);
	{
		const float CW = 230.f * U, CH = CW * 48.f / 185.f;
		const float CX = L.Pos.X + L.Size.X - CW - 24.f * U, CY = L.Pos.Y + 22.f * U;
		Tex(UITex(TEXT("S_CoinPill")), CX, CY, CW, CH);
		T(FString::Printf(TEXT("%d"), GM->Money), CX + CW * 0.55f, CY + CH * 0.5f - 14.f * U, 24.f, FLinearColor::White, true);
		T(FString::Printf(TEXT("Pantry %d / %d"), GM->PantryCount(), AAbyssGameMode::PantryCapacity), CX - 16.f * U, CY + 16.f * U, 17.f,
			GM->PantryCount() >= AAbyssGameMode::PantryCapacity ? Red : Cream, false, true);
	}

	// ---- left: the fridge + tonight's crowd
	{
		const FBox2D C = L.FridgeCol();
		Fill(C, FLinearColor(0.02f, 0.05f, 0.1f, 0.7f));
		Frame(C, FLinearColor(0.4f, 0.6f, 0.8f, 0.5f));
		const float X = C.Min.X + 18.f * U;
		float Y = C.Min.Y + 14.f * U;
		T(TEXT("TODAY'S CATCH"), X, Y, 22.f, Gold);
		Y += 40.f * U;
		for (int32 f = 0; f < AbyssData::NumFish; ++f)
		{
			const FFishDef& F = AbyssData::Fish((EFishType)f);
			const int32 Pr = GM->Prime[f], No = GM->Stock[f] - Pr;
			Panel(X - 6.f * U, Y, C.Max.X - C.Min.X - 24.f * U, 100.f * U, RowC);
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
		T(AbyssData::TasteName(GM->TonightTaste), X + 110.f * U, Y + 8.f * U, 26.f, TC, true);
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
	for (int32 r = 0; r < AbyssData::NumDishes; ++r)
	{
		const int32 Dish = FPrepLayout::DishOrder[r];
		const FRecipe& R = AbyssRecipes::Get(Dish);
		const FBox2D B = L.DishRow(r);
		const int32 Fish = AbyssData::DishFish(Dish);
		const int32 Stock = Fish >= 0 ? GM->Stock[Fish] : 99;
		const bool bPrimeFish = Fish >= 0 && GM->Prime[Fish] > 0;
		Fill(B, RowC);
		Frame(B, FLinearColor(0.5f, 0.65f, 0.85f, 0.45f));
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
				T(Nm, CX, Sel.Min.Y + 9.f * U, 17.f, Have > 0 ? FLinearColor::White : Red, true);
			}
		}
	}

	// ---- right: the market
	T(TEXT("MARKET"), L.MarketRow(0).Min.X + 4.f * U, L.Pos.Y + 74.f * U, 22.f, Gold);
	for (int32 r = 0; r < FPrepLayout::MarketRows; ++r)
	{
		const bool bBait = r == FPrepLayout::MarketRows - 1;
		const int32 Id = r + 1;
		const FBox2D B = L.MarketRow(r);
		Fill(B, RowC);
		const float X = B.Min.X, Y = B.Min.Y;
		const FSeasonDef* D = bBait ? nullptr : &AbyssData::Season(Id);
		const int32 Tier = D ? D->Tier : 0;
		Frame(B, Tier == 2 ? FLinearColor(1.f, 0.75f, 0.25f, 0.9f) : Tier == 1 ? FLinearColor(0.45f, 0.9f, 0.5f, 0.6f) : FLinearColor(0.5f, 0.65f, 0.85f, 0.45f));
		Tex(UITex(bBait ? FString(TEXT("Bait")) : FString::Printf(TEXT("Sea_%d"), Id)), X + 8.f * U, Y + 7.f * U, 72.f * U, 72.f * U);
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
			Panel(X + 90.f * U, Y + 60.f * U, 10.f * U, 10.f * U, TC);
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
