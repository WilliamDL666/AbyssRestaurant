#include "KitchenCounter.h"
#include "RestaurantDome.h"
#include "RestaurantCustomer.h"
#include "AbyssGameMode.h"
#include "AbyssTypes.h"

namespace
{
	// counter picture (UI_Counter.png, 1920x700) anchors from Scripts/blender_make_kitchen_ui.py -> counter_anchors.json
	// anchors in UI_Counter.png (1920x900, the user's cook counter rendered by Scripts/blender_render_counter.py)
	const FVector2D APan[2] = { FVector2D(392.f, 440.f), FVector2D(615.f, 440.f) };
	const float APanR = 86.f;
	const FVector2D ABoardMin(800.f, 468.f), ABoardMax(1252.f, 612.f);
	const FVector2D APlate(1450.f, 505.f);
	const float APlateR = 122.f;
	const FVector2D ABell(1650.f, 510.f);
	const FVector2D ABins[5] = { FVector2D(825.f, 402.f), FVector2D(965.f, 402.f), FVector2D(1110.f, 402.f), FVector2D(1232.f, 402.f), FVector2D(-9999.f, -9999.f) };
	constexpr float PlateSquash = 0.56f;   // the plate is seen at an angle
	constexpr float CookRate = 1.f / 10.5f; // pan doneness per second (perfect at 0.86 -> ~9 s)
}

// ---------------------------------------------------------------- items

EIngredient FCItem::AsIngredient() const
{
	switch (Kind)
	{
	case ECItem::Sashimi: return EIngredient::FishSlice;
	case ECItem::Cooked:  return EIngredient::Steak;
	case ECItem::Rice:    return EIngredient::Rice;
	case ECItem::Lemon:   return EIngredient::Lemon;
	case ECItem::Greens:  return EIngredient::Greens;
	case ECItem::Kelp:    return EIngredient::Kelp;
	case ECItem::Wasabi:  return EIngredient::Wasabi;
	case ECItem::Sesame:  return EIngredient::Sesame;
	default:              return EIngredient::Count;
	}
}

const TCHAR* FKitchenCounter::ItemName(const FCItem& I)
{
	switch (I.Kind)
	{
	case ECItem::Fillet:  return I.Fish == 0 ? TEXT("Yellowtail fillet") : I.Fish == 1 ? TEXT("Tuna fillet") : TEXT("Grouper fillet");
	case ECItem::Sashimi: return TEXT("Fish Slice");
	case ECItem::Steak:   return TEXT("Raw steak");
	case ECItem::Cooked:  return I.bBurnt ? TEXT("Burnt steak") : TEXT("Seared steak");
	case ECItem::Rice:    return TEXT("Rice");
	case ECItem::Lemon:   return TEXT("Lemon");
	case ECItem::Greens:  return TEXT("Greens");
	case ECItem::Kelp:    return TEXT("Kelp");
	case ECItem::Wasabi:  return TEXT("Wasabi");
	case ECItem::Sesame:  return TEXT("Sesame");
	default:              return TEXT("");
	}
}

FCItem FKitchenCounter::BarItem(int32 Slot)
{
	FCItem I;
	if (Slot >= 0 && Slot < 3) { I.Kind = ECItem::Fillet; I.Fish = (int8)Slot; return I; }
	static const ECItem Rest[] = { ECItem::Rice, ECItem::Lemon, ECItem::Greens, ECItem::Kelp, ECItem::Wasabi, ECItem::Sesame };
	if (Slot >= 3 && Slot < 9) I.Kind = Rest[Slot - 3];
	return I;
}

FCItem FKitchenCounter::BinItem(int32 Bin)
{
	static const ECItem B[] = { ECItem::Rice, ECItem::Lemon, ECItem::Greens, ECItem::Kelp, ECItem::Wasabi };
	FCItem I;
	if (Bin >= 0 && Bin < 5) I.Kind = B[Bin];
	return I;
}

// ---------------------------------------------------------------- layout

FCounterLayout FCounterLayout::Make(float W, float H, int32 NumTickets)
{
	// The counter picture (UI_Counter.png, 1920x700) sits at the bottom; widgets from the user's UI sheet around it.
	FCounterLayout L;
	L.U = FMath::Min(H / 1080.f, W / 1920.f);
	const float U = L.U;
	L.CounterSize = FVector2D(1920.f, 900.f) * U;
	L.Origin = FVector2D(W * 0.5f - L.CounterSize.X * 0.5f, H - L.CounterSize.Y);
	for (int32 i = 0; i < 2; ++i) L.Pan[i] = L.Img(APan[i].X, APan[i].Y);
	L.PanR = APanR * U;
	L.Board = FBox2D(L.Img(ABoardMin.X, ABoardMin.Y), L.Img(ABoardMax.X, ABoardMax.Y));
	L.Plate = L.Img(APlate.X, APlate.Y);
	L.PlateR = APlateR * U;
	L.Bell = L.Img(ABell.X, ABell.Y);
	L.BellR = 50.f * U;
	L.ServeBtn = L.Bell;
	L.ServeR = L.BellR;
	for (int32 i = 0; i < 5; ++i) L.Bins[i] = L.Img(ABins[i].X, ABins[i].Y);
	// ingredient tiles on the counter front
	L.SlotSize = 112.f * U;
	L.BarSize = FVector2D(9 * 124.f + 16.f, 136.f) * U;
	L.BarPos = FVector2D(W * 0.5f - L.BarSize.X * 0.5f, L.Img(0.f, 712.f).Y);
	for (int32 i = 0; i < 9; ++i) L.SlotPos[i] = L.BarPos + FVector2D((14.f + i * 124.f) * U, 12.f * U);
	// clipboard tickets hang along the top
	L.TicketSize = FVector2D(200.f, 248.f) * U;
	const int32 N = FMath::Clamp(NumTickets, 1, 4);
	const float Gap = 26.f * U;
	const float X0 = W * 0.5f - (N * L.TicketSize.X + (N - 1) * Gap) * 0.5f + 40.f * U;
	for (int32 i = 0; i < 4; ++i) L.TicketPos[i] = FVector2D(X0 + i * (L.TicketSize.X + Gap), 112.f * U);
	L.SideBtnSize = FVector2D(232.f, 75.f) * U;
	for (int32 i = 0; i < 6; ++i) L.SideBtn[i] = FVector2D(8.f * U, (130.f + i * 80.f) * U);
	L.CloseSize = FVector2D(228.f, 90.f) * U;
	L.CloseBtn = FVector2D(W - L.CloseSize.X - 14.f * U, 100.f * U);
	L.SubSize = 176.f * U;
	L.SubBtn = FVector2D(L.SubSize * 0.5f + 8.f * U, H - L.SubSize * 0.5f - 8.f * U);
	L.SignSize = FVector2D(270.f, 178.f) * U;
	L.SignPos = FVector2D(W - L.SignSize.X - 8.f * U, H - L.SignSize.Y - 6.f * U);
	return L;
}

bool FCounterLayout::InPan(int32 i, const FVector2D& P) const
{
	const FVector2D D = P - Pan[i];
	return FMath::Square(D.X / PanR) + FMath::Square(D.Y / (PanR * 0.5f)) < 1.3f;
}

bool FCounterLayout::InPlate(const FVector2D& P) const
{
	const FVector2D D = P - Plate;
	return FMath::Square(D.X / PlateR) + FMath::Square(D.Y / (PlateR * PlateSquash)) < 1.f;
}

int32 FCounterLayout::SlotAt(const FVector2D& P) const
{
	for (int32 i = 0; i < 9; ++i)
		if (P.X >= SlotPos[i].X && P.X <= SlotPos[i].X + SlotSize && P.Y >= SlotPos[i].Y && P.Y <= SlotPos[i].Y + SlotSize) return i;
	return INDEX_NONE;
}

int32 FCounterLayout::BinAt(const FVector2D& P) const
{
	for (int32 i = 0; i < 5; ++i)
		if (FMath::Abs(P.X - Bins[i].X) < 60.f * U && FMath::Abs(P.Y - Bins[i].Y) < 34.f * U) return i;
	return INDEX_NONE;
}

int32 FCounterLayout::TicketAt(const FVector2D& P, int32 Num) const
{
	for (int32 i = 0; i < Num && i < 4; ++i)
		if (P.X >= TicketPos[i].X && P.X <= TicketPos[i].X + TicketSize.X && P.Y >= TicketPos[i].Y && P.Y <= TicketPos[i].Y + TicketSize.Y) return i;
	return INDEX_NONE;
}

int32 FCounterLayout::SideAt(const FVector2D& P) const
{
	for (int32 i = 0; i < 6; ++i)
		if (P.X >= SideBtn[i].X && P.X <= SideBtn[i].X + SideBtnSize.X && P.Y >= SideBtn[i].Y && P.Y <= SideBtn[i].Y + SideBtnSize.Y) return i;
	return INDEX_NONE;
}

bool FCounterLayout::InClose(const FVector2D& P) const
{
	return P.X >= CloseBtn.X && P.X <= CloseBtn.X + CloseSize.X && P.Y >= CloseBtn.Y && P.Y <= CloseBtn.Y + CloseSize.Y;
}

// ---------------------------------------------------------------- rules

void FKitchenCounter::Reset()
{
	bOpen = false;
	Active = nullptr;
	Board = FCItem();
	Cuts.Reset();
	CutScores.Reset();
	for (FCPan& P : Pans) P = FCPan();
	Plate.Reset();
	Drag = FCItem();
	DragFromPan = -1;
	bDragFromBoard = false;
	SidePanel = -1;
}

void FKitchenCounter::Say(const FString& S, const FLinearColor& C, const FVector2D& At)
{
	Feedback = S;
	FeedbackColor = C;
	FeedbackTime = 1.4f;
	FeedbackAt = At;
}

void FKitchenCounter::Tick(float Dt, ARestaurantDome* Dome)
{
	Time += Dt;
	FeedbackTime = FMath::Max(0.f, FeedbackTime - Dt);
	BellRing = FMath::Max(0.f, BellRing - Dt);
	// pans keep cooking whether the counter is open or not
	for (int32 i = 0; i < 2; ++i)
	{
		FCPan& P = Pans[i];
		if (!P.Busy() || (DragFromPan == i)) continue;
		const float Before = P.Done;
		P.Done = FMath::Min(P.Done + Dt * CookRate * (P.Done > 0.86f ? 0.4f : 1.f), 1.25f);
		if (Before < 1.f && P.Done >= 1.f && Dome)
		{
			if (AAbyssGameMode* GM = AAbyssGameMode::Get(Dome)) GM->ShowToast(TEXT("Something is burning on the stove!"), FLinearColor(1.f, 0.35f, 0.2f), 2.f);
		}
	}
	// keep the active ticket valid
	if (Dome && (!Active.IsValid() || !Active->IsWaitingForFood()))
	{
		TArray<ARestaurantCustomer*> Q;
		Dome->GetQueue(Q);
		Active = Q.Num() > 0 ? Q[0] : nullptr;
	}
}

void FKitchenCounter::SelectTicket(int32 Index, ARestaurantDome* Dome)
{
	TArray<ARestaurantCustomer*> Q;
	Dome->GetQueue(Q);
	if (Q.IsValidIndex(Index)) Active = Q[Index];
}

void FKitchenCounter::Cut(float X01)
{
	int32 Best = INDEX_NONE;
	float BestD = 1.f;
	for (int32 t = 0; t < 3; ++t)
	{
		const float D = FMath::Abs(X01 - CutTargets[t]);
		bool bUsed = false;
		for (float C : Cuts) if (FMath::Abs(C - CutTargets[t]) < 0.1f) bUsed = true;
		if (!bUsed && D < BestD) { BestD = D; Best = t; }
	}
	const float Score = Best == INDEX_NONE ? 40.f : FMath::Clamp(100.f - BestD * 900.f, 20.f, 100.f);
	Cuts.Add(X01);
	CutScores.Add(Score);
	if (Cuts.Num() >= 3)
	{
		float Avg = 0.f;
		for (float S : CutScores) Avg += S;
		Avg /= CutScores.Num();
		Board.Kind = Board.Fish == 0 ? ECItem::Sashimi : ECItem::Steak;
		Board.Prep = Avg;
		Cuts.Reset();
		CutScores.Reset();
	}
}

bool FKitchenCounter::PlaceOnPlate(const FCItem& I, const FVector2D& P, const FCounterLayout& L)
{
	if (I.AsIngredient() == EIngredient::Count)
	{
		Say(I.Kind == ECItem::Fillet ? TEXT("Cut it on the board first!") : TEXT("Cook it in a pan first!"), FLinearColor(1.f, 0.6f, 0.3f), P);
		return false;
	}
	if (Plate.Num() >= 8)
	{
		Say(TEXT("The plate is full"), FLinearColor(1.f, 0.6f, 0.3f), P);
		return false;
	}
	FCPlateItem Pit;
	Pit.Item = I;
	const FVector2D D = P - L.Plate;
	Pit.Off = FVector2D(FMath::Clamp(D.X / L.PlateR, -0.75f, 0.75f), FMath::Clamp(D.Y / (L.PlateR * PlateSquash), -0.7f, 0.7f));
	Plate.Add(Pit);
	return true;
}

void FKitchenCounter::MouseDown(const FVector2D& P, const FCounterLayout& L, ARestaurantDome* Dome)
{
	Mouse = DownAt = P;
	AAbyssGameMode* GM = AAbyssGameMode::Get(Dome);
	if (!GM) return;

	// buttons
	if (L.InClose(P)) { bOpen = false; return; }
	const int32 Side = L.SideAt(P);
	if (Side != INDEX_NONE)
	{
		// 0 menu (recipe book), 1 shop, 2 decor, 3 tasks, 4 codex, 5 upgrade (restaurant expansion)
		if (Side == 5) GM->TryUpgrade(3);
		else if (Side == 1) GM->ShowToast(TEXT("Restock by catching fish at sea after closing"), FLinearColor(0.6f, 0.9f, 1.f), 2.f);
		else if (Side == 2) GM->ShowToast(TEXT("Decor shop coming soon"), FLinearColor(0.6f, 0.9f, 1.f), 1.6f);
		else SidePanel = SidePanel == Side ? -1 : Side;
		return;
	}
	if (SidePanel >= 0) { SidePanel = -1; return; }
	if (L.InSub(P))
	{
		GM->ShowToast(TEXT("Finish tonight's service before heading out"), FLinearColor(0.6f, 0.9f, 1.f), 1.8f);
		return;
	}
	{
		TArray<ARestaurantCustomer*> Q;
		Dome->GetQueue(Q);
		const int32 T = L.TicketAt(P, Q.Num());
		if (T != INDEX_NONE) { Active = Q[T]; return; }
	}
	if (L.InBell(P)) { Serve(Dome); return; }

	// cutting board: a fillet on it -> this click is a cut
	if (L.InBoard(P) && Board.Kind == ECItem::Fillet)
	{
		Cut(FMath::Clamp((P.X - L.Board.Min.X) / (L.Board.Max.X - L.Board.Min.X), 0.f, 1.f));
		return;
	}
	// pick things up
	if (L.InBoard(P) && Board.IsValid())
	{
		Drag = Board; bDragFromBoard = true;
		return;
	}
	for (int32 i = 0; i < 2; ++i)
	{
		if (L.InPan(i, P) && Pans[i].Busy())
		{
			Drag = Pans[i].Item; DragFromPan = i;  // a click (no drag) flips, see MouseUp
			return;
		}
	}
	if (L.InPlate(P))
	{
		// click a plated item to take it off again (dropped somewhere else = thrown away)
		int32 Best = INDEX_NONE;
		float BestD = 1e9f;
		for (int32 k = 0; k < Plate.Num(); ++k)
		{
			const FVector2D IP = L.Plate + FVector2D(Plate[k].Off.X * L.PlateR, Plate[k].Off.Y * L.PlateR * PlateSquash);
			const float D = FVector2D::Distance(IP, P);
			if (D < BestD) { BestD = D; Best = k; }
		}
		if (Best != INDEX_NONE && BestD < 50.f * L.U)
		{
			Drag = Plate[Best].Item;
			Plate.RemoveAt(Best);
		}
		return;
	}
	const int32 Slot = L.SlotAt(P);
	if (Slot != INDEX_NONE)
	{
		const FCItem I = BarItem(Slot);
		if (I.Kind == ECItem::Fillet && GM->Stock[I.Fish] <= 0)
		{
			Say(TEXT("Out of stock - catch more at sea"), FLinearColor(1.f, 0.4f, 0.3f), P);
			return;
		}
		Drag = I;
		return;
	}
	const int32 Bin = L.BinAt(P);
	if (Bin != INDEX_NONE) Drag = BinItem(Bin);
}

void FKitchenCounter::MouseUp(const FVector2D& P, const FCounterLayout& L, ARestaurantDome* Dome)
{
	Mouse = P;
	if (!Drag.IsValid()) return;
	AAbyssGameMode* GM = AAbyssGameMode::Get(Dome);
	const bool bClick = FVector2D::Distance(P, DownAt) < 8.f * L.U;
	const FCItem I = Drag;
	Drag = FCItem();

	// a click on a pan flips the steak
	if (DragFromPan >= 0)
	{
		const int32 Pi = DragFromPan;
		DragFromPan = -1;
		FCPan& Pan = Pans[Pi];
		if (bClick || L.InPan(Pi, P))
		{
			if (!Pan.bFlipped)
			{
				Pan.bFlipped = true;
				Pan.FlipScore = FMath::Clamp(100.f - FMath::Abs(Pan.Done - 0.45f) * 380.f, 10.f, 100.f);
				Say(Pan.FlipScore >= 85.f ? TEXT("Nice flip!") : Pan.Done < 0.45f ? TEXT("Flipped a bit early") : TEXT("Flipped a bit late"),
					Pan.FlipScore >= 85.f ? FLinearColor(0.4f, 1.f, 0.5f) : FLinearColor(1.f, 0.8f, 0.3f), L.Pan[Pi]);
			}
			return;
		}
		// dragged out of the pan: it is done cooking
		FCItem C = Pan.Item;
		const float Done = Pan.Done;
		const float Finish = FMath::Clamp(100.f - FMath::Abs(Done - 0.86f) * 420.f, 0.f, 100.f);
		C.Kind = ECItem::Cooked;
		C.bBurnt = Done >= 1.f;
		C.Cook = C.bBurnt ? 5.f : (Pan.bFlipped ? 0.6f * Finish + 0.4f * Pan.FlipScore : 0.5f * Finish);
		const bool bOnPlate = L.InPlate(P) && PlaceOnPlate(C, P, L);
		if (bOnPlate || !L.InPan(1 - Pi, P))
		{
			Pans[Pi] = FCPan();
			if (!bOnPlate) Say(TEXT("Thrown away"), FLinearColor(1.f, 0.5f, 0.4f), P);
		}
		return;
	}
	if (bDragFromBoard)
	{
		bDragFromBoard = false;
		bool bUsed = false;
		if (I.Kind == ECItem::Steak)
		{
			for (int32 i = 0; i < 2 && !bUsed; ++i)
			{
				if (L.InPan(i, P) && !Pans[i].Busy())
				{
					Pans[i] = FCPan();
					Pans[i].Item = I;
					bUsed = true;
				}
			}
		}
		else if (L.InPlate(P)) bUsed = PlaceOnPlate(I, P, L);
		if (bUsed) Board = FCItem();
		else if (!L.InBoard(P)) { Board = FCItem(); Say(TEXT("Thrown away"), FLinearColor(1.f, 0.5f, 0.4f), P); }
		return;
	}

	// dropped from the bar / bins / plate
	if (I.Kind == ECItem::Fillet)
	{
		if (L.InBoard(P))
		{
			if (Board.IsValid()) { Say(TEXT("The board is busy"), FLinearColor(1.f, 0.6f, 0.3f), P); return; }
			if (GM) GM->TakeFish(I.Fish);   // a fillet comes out of the fridge
			Board = I;
			Cuts.Reset();
			CutScores.Reset();
			Say(TEXT("Click on the gold lines to cut"), FLinearColor(1.f, 0.9f, 0.6f), P);
		}
		return;
	}
	if (L.InPlate(P)) PlaceOnPlate(I, P, L);
	else if (I.Kind == ECItem::Steak) { for (int32 i = 0; i < 2; ++i) if (L.InPan(i, P) && !Pans[i].Busy()) { Pans[i].Item = I; break; } }
}

float FKitchenCounter::ScorePlate(const FRecipe& R, int32 Dish) const
{
	// multiset match of plate contents vs the recipe; fish items must be the ordered fish
	TArray<EIngredient> Need = R.Slots;
	int32 Matched = 0, Extra = 0;
	float Prep = 0.f, Cook = 0.f;
	int32 NPrep = 0, NCook = 0;
	for (const FCPlateItem& Pit : Plate)
	{
		const EIngredient G = Pit.Item.AsIngredient();
		const bool bFishOk = (Pit.Item.Kind != ECItem::Sashimi && Pit.Item.Kind != ECItem::Cooked) || Pit.Item.Fish == R.Fish;
		const int32 Idx = bFishOk ? Need.Find(G) : INDEX_NONE;
		if (Idx != INDEX_NONE)
		{
			Need.RemoveAt(Idx);
			++Matched;
			if (Pit.Item.Kind == ECItem::Sashimi || Pit.Item.Kind == ECItem::Cooked) { Prep += Pit.Item.Prep; ++NPrep; }
			if (Pit.Item.Kind == ECItem::Cooked) { Cook += Pit.Item.Cook; ++NCook; }
		}
		else ++Extra;
	}
	const float PlateS = 100.f * Matched / FMath::Max(1, R.Slots.Num() + Extra);
	const float PrepS = R.bSlice ? (NPrep > 0 ? Prep / NPrep : 0.f) : 100.f;
	const float CookS = R.bCook ? (NCook > 0 ? Cook / NCook : 0.f) : 100.f;
	const float WSum = FMath::Max(R.WPrep + R.WCook + R.WPlate, 0.01f);
	return (R.WPrep * PrepS + R.WCook * CookS + R.WPlate * PlateS) / WSum;
}

void FKitchenCounter::Serve(ARestaurantDome* Dome)
{
	ARestaurantCustomer* C = Active.Get();
	if (!C) { Say(TEXT("No order selected"), FLinearColor(1.f, 0.6f, 0.3f), FVector2D::ZeroVector); return; }
	if (Plate.Num() == 0) { Say(TEXT("The plate is empty"), FLinearColor(1.f, 0.6f, 0.3f), FVector2D::ZeroVector); return; }
	if (Dome->IsCarrying()) { Say(TEXT("Serve the dish in your hands first"), FLinearColor(1.f, 0.6f, 0.3f), FVector2D::ZeroVector); return; }
	const FRecipe& R = AbyssRecipes::Get(C->Dish);
	const float Score = ScorePlate(R, C->Dish);
	BellRing = 0.6f;
	Plate.Reset();
	Dome->CompleteDish(C, Score, false);
	bOpen = false;   // the chef carries the plate to the table; the pans keep going
}

// ---------------------------------------------------------------- autotest helpers

void FKitchenCounter::DebugPrepare(ARestaurantDome* Dome, float PanDone)
{
	ARestaurantCustomer* C = Active.Get();
	if (!C) return;
	const FRecipe& R = AbyssRecipes::Get(C->Dish);
	if (R.Fish < 0) return;
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(Dome)) GM->TakeFish(R.Fish);
	Board = FCItem();
	Board.Kind = ECItem::Fillet;
	Board.Fish = (int8)R.Fish;
	Cuts.Reset();
	CutScores.Reset();
	for (int32 t = 0; t < 3; ++t) Cut(CutTargets[t] + 0.01f);
	if (R.bCook)
	{
		Pans[0] = FCPan();
		Pans[0].Item = Board;
		Pans[0].Done = PanDone;
		Pans[0].bFlipped = PanDone > 0.5f;
		Pans[0].FlipScore = 95.f;
		Board = FCItem();
	}
}

void FKitchenCounter::DebugPlate(ARestaurantDome* Dome)
{
	ARestaurantCustomer* C = Active.Get();
	if (!C) return;
	const FRecipe& R = AbyssRecipes::Get(C->Dish);
	Plate.Reset();
	for (int32 k = 0; k < R.Slots.Num(); ++k)
	{
		FCPlateItem Pit;
		const float A = 2.f * UE_PI * k / R.Slots.Num();
		Pit.Off = FVector2D(FMath::Cos(A) * 0.45f, FMath::Sin(A) * 0.4f);
		switch (R.Slots[k])
		{
		case EIngredient::FishSlice: Pit.Item.Kind = ECItem::Sashimi; Pit.Item.Fish = (int8)R.Fish; Pit.Item.Prep = Board.IsValid() ? Board.Prep : 95.f; break;
		case EIngredient::Steak:
			Pit.Item = Pans[0].Busy() ? Pans[0].Item : FCItem();
			Pit.Item.Kind = ECItem::Cooked; Pit.Item.Fish = (int8)R.Fish; Pit.Item.Cook = 92.f;
			Pans[0] = FCPan();
			break;
		case EIngredient::Rice:   Pit.Item.Kind = ECItem::Rice; break;
		case EIngredient::Lemon:  Pit.Item.Kind = ECItem::Lemon; break;
		case EIngredient::Greens: Pit.Item.Kind = ECItem::Greens; break;
		case EIngredient::Kelp:   Pit.Item.Kind = ECItem::Kelp; break;
		case EIngredient::Wasabi: Pit.Item.Kind = ECItem::Wasabi; break;
		case EIngredient::Sesame: Pit.Item.Kind = ECItem::Sesame; break;
		default: break;
		}
		Plate.Add(Pit);
	}
	Board = FCItem();
}
