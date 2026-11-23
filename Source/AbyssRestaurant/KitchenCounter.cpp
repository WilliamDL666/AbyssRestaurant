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
	constexpr float CookRate = 1.f / 10.5f; // pan doneness per second on medium heat (perfect at 0.86 -> ~9 s)
	const FVector2D APot(622.f, 418.f);     // the stew pot stands on the second burner
	const float APotR = 104.f;
	const FVector2D AKnob[2] = { FVector2D(403.f, 640.f), FVector2D(598.f, 640.f) };
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
	case ECItem::Stew:    return EIngredient::Stew;
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
	case ECItem::Stew:    return TEXT("Kelp stew");
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
	L.Pot = L.Img(APot.X, APot.Y);
	L.PotR = APotR * U;
	for (int32 i = 0; i < 2; ++i) L.Knob[i] = L.Img(AKnob[i].X, AKnob[i].Y);
	L.KnobR = 40.f * U;
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

bool FCounterLayout::InPot(const FVector2D& P) const
{
	const FVector2D D = P - Pot;
	return FMath::Square(D.X / PotR) + FMath::Square(D.Y / (PotR * 0.8f)) < 1.15f;
}

int32 FCounterLayout::KnobAt(const FVector2D& P) const
{
	for (int32 i = 0; i < 2; ++i) if (FVector2D::Distance(P, Knob[i]) < KnobR) return i;
	return INDEX_NONE;
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
	bCutting = false;
	for (FCPan& P : Pans) P = FCPan();
	Pot = FCPot();
	Heat[0] = Heat[1] = 1;
	Plate.Reset();
	Drag = FCItem();
	DragFromPan = -1;
	bDragFromBoard = false;
	bDragFromPot = false;
	SidePanel = -1;
}

void FKitchenCounter::Say(const FString& S, const FLinearColor& C, const FVector2D& At)
{
	Feedback = S;
	FeedbackColor = C;
	FeedbackTime = 1.6f;
	FeedbackAt = At;
}

ECItem FKitchenCounter::PotNext() const
{
	if (!Pot.Has(ECItem::Kelp)) return ECItem::Kelp;
	if (!Pot.Has(ECItem::Sashimi)) return ECItem::Sashimi;
	if (!Pot.Has(ECItem::Greens)) return ECItem::Greens;
	return ECItem::None;
}

void FKitchenCounter::Tick(float Dt, ARestaurantDome* Dome)
{
	Time += Dt;
	FeedbackTime = FMath::Max(0.f, FeedbackTime - Dt);
	BellRing = FMath::Max(0.f, BellRing - Dt);
	AAbyssGameMode* GM = Dome ? AAbyssGameMode::Get(Dome) : nullptr;
	// the pan keeps cooking whether the counter is open or not; the knob sets how fast
	for (int32 i = 0; i < NumPans; ++i)
	{
		FCPan& P = Pans[i];
		if (!P.Busy() || (DragFromPan == i)) continue;
		const float Before = P.Done;
		P.Done = FMath::Min(P.Done + Dt * CookRate * HeatRate(Heat[i]) * (P.Done > 0.86f ? 0.4f : 1.f), 1.25f);
		// high heat early builds the crust; high heat late scorches it
		if (Heat[i] == 2 && P.Done < 0.4f) P.Sear = FMath::Min(1.f, P.Sear + Dt / 2.f);
		if (Heat[i] == 2 && P.Done > 0.55f) P.Scorch += Dt;
		if (Before < 1.f && P.Done >= 1.f && GM)
		{
			GM->ShowToast(TEXT("Something is burning on the stove!"), FLinearColor(1.f, 0.35f, 0.2f), 2.f);
		}
	}
	if (Pot.Busy() && !bDragFromPot)
	{
		const float Before = Pot.T;
		Pot.T += Dt * HeatRate(Heat[1]);
		if (Heat[1] == 2) Pot.Boil += Dt;
		if (Pot.Complete() && Before < FCPot::MushyAt && Pot.T >= FCPot::MushyAt && GM)
		{
			GM->ShowToast(TEXT("The stew is overcooking!"), FLinearColor(1.f, 0.5f, 0.2f), 2.f);
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

void FKitchenCounter::SetCuts(int32 N, float Tight)
{
	NumCuts = FMath::Clamp(N, 1, 6);
	CutTight = Tight;
	// the fillet covers the middle half of the board
	for (int32 k = 0; k < NumCuts; ++k) CutTargets[k] = 0.26f + 0.48f * (k + 0.5f) / NumCuts;
	Cuts.Reset();
	CutScores.Reset();
}

void FKitchenCounter::Cut(float X01, float Slant)
{
	int32 Best = INDEX_NONE;
	float BestD = 1.f;
	const float Near = CutGap() * 0.45f;
	for (int32 t = 0; t < NumCuts; ++t)
	{
		const float D = FMath::Abs(X01 - CutTargets[t]);
		bool bUsed = false;
		for (float C : Cuts) if (FMath::Abs(C - CutTargets[t]) < Near) bUsed = true;
		if (!bUsed && D < BestD) { BestD = D; Best = t; }
	}
	// off the mark and a crooked stroke both cost points
	const float Score = Best == INDEX_NONE ? 30.f : FMath::Clamp(100.f - BestD * CutTight - Slant * 260.f, 15.f, 100.f);
	Cuts.Add(X01);
	CutScores.Add(Score);
	if (Cuts.Num() >= NumCuts)
	{
		float Avg = 0.f;
		for (float S : CutScores) Avg += S;
		Avg /= CutScores.Num();
		Board.Kind = Board.Fish == 0 ? ECItem::Sashimi : ECItem::Steak;
		Board.Count = Board.Fish == 0 ? 2 : 1;
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
	Pit.Item.Count = 1;
	const FVector2D D = P - L.Plate;
	Pit.Off = FVector2D(FMath::Clamp(D.X / L.PlateR, -0.75f, 0.75f), FMath::Clamp(D.Y / (L.PlateR * PlateSquash), -0.7f, 0.7f));
	Plate.Add(Pit);
	return true;
}

// Something goes into the stew pot. The order (kelp -> fish -> greens) and the moment both count.
bool FKitchenCounter::AddToPot(const FCItem& I, const FVector2D& P)
{
	const FLinearColor Bad(1.f, 0.6f, 0.3f), Good(0.4f, 1.f, 0.5f), Ok(1.f, 0.85f, 0.3f);
	if (I.Kind != ECItem::Kelp && I.Kind != ECItem::Sashimi && I.Kind != ECItem::Greens)
	{
		Say(I.Kind == ECItem::Steak || I.Kind == ECItem::Cooked ? TEXT("Steaks go in the frying pan") : I.Kind == ECItem::Fillet ? TEXT("Cut it on the board first!") : TEXT("That does not go in the stew"), Bad, P);
		return false;
	}
	if (Pot.Has(I.Kind))
	{
		Say(TEXT("Already in the pot"), Bad, P);
		return false;
	}
	const ECItem Want = PotNext();
	float Score;
	FString Msg;
	FLinearColor Col = Good;
	if (I.Kind != Want)
	{
		Score = 25.f;
		Msg = TEXT("Wrong order! Kelp, then fish, then greens");
		Col = Bad;
	}
	else if (I.Kind == ECItem::Kelp)
	{
		Score = 100.f;
		Msg = TEXT("Stock started");
	}
	else
	{
		const float At = I.Kind == ECItem::Sashimi ? FCPot::FishAt : FCPot::GreensAt;
		const float D = Pot.T - At;
		Score = FMath::Clamp(100.f - FMath::Max(0.f, FMath::Abs(D) - 0.45f) * 40.f, 30.f, 100.f);
		if (Score >= 90.f) Msg = TEXT("Right on time!");
		else { Msg = D < 0.f ? TEXT("A bit early") : TEXT("A bit late"); Col = Ok; }
	}
	if (!Pot.Busy()) { Pot.T = 0.f; Pot.Boil = 0.f; }
	Pot.Added.Add(I.Kind);
	Pot.Scores.Add(Score);
	if (I.Kind == ECItem::Sashimi) Pot.Fish = I;
	Say(Msg, Col, P);
	return true;
}

FCItem FKitchenCounter::Ladle() const
{
	FCItem S;
	S.Kind = ECItem::Stew;
	S.Fish = 0;
	S.Prep = Pot.Fish.IsValid() ? Pot.Fish.Prep : 60.f;
	S.bPrime = Pot.Fish.bPrime;
	float Avg = 0.f;
	for (float V : Pot.Scores) Avg += V;
	Avg = Pot.Scores.Num() > 0 ? Avg / Pot.Scores.Num() : 0.f;
	const float Finish = FMath::Clamp(100.f - FMath::Max(0.f, FMath::Abs(Pot.T - FCPot::ReadyAt) - 0.8f) * 28.f, 10.f, 100.f);
	S.bBurnt = Pot.T >= FCPot::MushyAt + 2.f;
	S.Cook = FMath::Clamp(0.7f * Avg + 0.3f * Finish - Pot.Boil * 6.f, 5.f, 100.f);
	return S;
}

// The steak leaves the pan: finish point + flip + how the heat was handled
FCItem FKitchenCounter::TakeFromPan(int32 Pi, const FVector2D& P)
{
	const FCPan& Pan = Pans[Pi];
	FCItem C = Pan.Item;
	const float Done = Pan.Done;
	// the signature grouper has a narrower sweet spot
	const float K = C.Fish == 2 ? 640.f : 420.f;
	const float Finish = FMath::Clamp(100.f - FMath::Abs(Done - 0.86f) * K, 0.f, 100.f);
	const float HeatS = FMath::Clamp(100.f * Pan.Sear - 45.f * Pan.Scorch, 0.f, 100.f);
	C.Kind = ECItem::Cooked;
	C.bBurnt = Done >= 1.f;
	C.Cook = C.bBurnt ? 5.f : 0.4f * Finish + 0.25f * (Pan.bFlipped ? Pan.FlipScore : 15.f) + 0.35f * HeatS;
	if (C.bBurnt) Say(TEXT("Burnt!"), FLinearColor(1.f, 0.35f, 0.25f), P);
	else if (Pan.Sear < 0.5f) Say(TEXT("No crust - start it on HIGH heat"), FLinearColor(1.f, 0.8f, 0.3f), P);
	else if (Pan.Scorch > 0.8f) Say(TEXT("Scorched - turn the heat down after the sear"), FLinearColor(1.f, 0.8f, 0.3f), P);
	else if (C.Cook >= 90.f) Say(TEXT("Perfect sear!"), FLinearColor(0.4f, 1.f, 0.5f), P);
	return C;
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
		else if (Side == 1) GM->ShowToast(TEXT("The market is open before service - stock up on seasonings then"), FLinearColor(0.6f, 0.9f, 1.f), 2.4f);
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

	// gas knobs: low -> medium -> high
	const int32 Knob = L.KnobAt(P);
	if (Knob != INDEX_NONE)
	{
		Heat[Knob] = (Heat[Knob] + 1) % 3;
		Say(Heat[Knob] == 0 ? TEXT("Heat: LOW") : Heat[Knob] == 1 ? TEXT("Heat: MEDIUM") : TEXT("Heat: HIGH"),
			Heat[Knob] == 0 ? FLinearColor(0.5f, 0.8f, 1.f) : Heat[Knob] == 1 ? FLinearColor(1.f, 0.85f, 0.4f) : FLinearColor(1.f, 0.45f, 0.25f), L.Knob[Knob]);
		return;
	}

	// cutting board: a fillet on it -> the knife goes down here; drag it through the fish (see MouseUp)
	if (L.InBoard(P) && Board.Kind == ECItem::Fillet)
	{
		bCutting = true;
		CutStart = P;
		return;
	}
	// pick things up
	if (L.InBoard(P) && Board.IsValid())
	{
		Drag = Board; bDragFromBoard = true;
		return;
	}
	for (int32 i = 0; i < NumPans; ++i)
	{
		if (L.InPan(i, P) && Pans[i].Busy())
		{
			Drag = Pans[i].Item; DragFromPan = i;  // a click (no drag) flips, see MouseUp
			return;
		}
	}
	if (L.InPot(P) && Pot.Busy())
	{
		Drag = Ladle(); bDragFromPot = true;       // drag a ladle to the plate
		return;
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
		FCItem I = BarItem(Slot);
		if (I.Kind == ECItem::Fillet)
		{
			if (GM->Stock[I.Fish] <= 0)
			{
				Say(TEXT("Out of stock - catch more at sea"), FLinearColor(1.f, 0.4f, 0.3f), P);
				return;
			}
			// the gold strip at the top of the tile hands out the prime catch, the rest an ordinary one
			const int32 Pr = GM->Prime[I.Fish], No = GM->Stock[I.Fish] - Pr;
			const bool bTop = P.Y < L.SlotPos[Slot].Y + L.SlotSize * 0.4f;
			I.bPrime = (bTop && Pr > 0) || No <= 0;
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
	if (bCutting)
	{
		bCutting = false;
		if (Board.Kind == ECItem::Fillet)
		{
			const float BW = L.Board.Max.X - L.Board.Min.X;
			const float Pull = P.Y - CutStart.Y;
			if (Pull < 28.f * L.U)
			{
				Say(TEXT("Pull the knife down through the fillet"), FLinearColor(1.f, 0.9f, 0.6f), P);
			}
			else
			{
				const float X01 = FMath::Clamp((0.5f * (P.X + CutStart.X) - L.Board.Min.X) / BW, 0.f, 1.f);
				const int32 Before = CutScores.Num();
				Cut(X01, FMath::Abs(P.X - CutStart.X) / BW);
				if (Board.Kind != ECItem::Fillet)
				{
					Say(Board.Prep >= 90.f ? TEXT("Beautiful knife work!") : Board.Prep >= 70.f ? TEXT("Decent cuts") : TEXT("Rough cuts..."),
						Board.Prep >= 90.f ? FLinearColor(0.4f, 1.f, 0.5f) : Board.Prep >= 70.f ? FLinearColor(1.f, 0.85f, 0.3f) : FLinearColor(1.f, 0.5f, 0.35f), P);
				}
				(void)Before;
			}
		}
		return;
	}
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
		if (L.InPlate(P))
		{
			const FCItem C = TakeFromPan(Pi, P);
			if (PlaceOnPlate(C, P, L)) Pans[Pi] = FCPan();
		}
		else
		{
			Pans[Pi] = FCPan();
			Say(TEXT("Thrown away"), FLinearColor(1.f, 0.5f, 0.4f), P);
		}
		return;
	}
	if (bDragFromPot)
	{
		bDragFromPot = false;
		if (bClick || L.InPot(P)) return;
		if (L.InPlate(P))
		{
			if (!Pot.Complete())
			{
				const ECItem Next = PotNext();
				Say(Next == ECItem::Kelp ? TEXT("The stew still needs kelp") : Next == ECItem::Sashimi ? TEXT("The stew still needs the fish") : TEXT("The stew still needs greens"), FLinearColor(1.f, 0.6f, 0.3f), P);
				return;
			}
			const FCItem S = Ladle();
			if (PlaceOnPlate(S, P, L))
			{
				Say(S.Cook >= 90.f ? TEXT("A perfect stew!") : Pot.T < FCPot::ReadyAt - 0.8f ? TEXT("Ladled a bit early") : Pot.T > FCPot::ReadyAt + 0.8f ? TEXT("Simmered a bit long") : TEXT("Stew is up"),
					S.Cook >= 90.f ? FLinearColor(0.4f, 1.f, 0.5f) : FLinearColor(1.f, 0.85f, 0.3f), P);
				Pot = FCPot();
			}
			return;
		}
		Pot = FCPot();
		Say(TEXT("Poured away"), FLinearColor(1.f, 0.5f, 0.4f), P);
		return;
	}
	if (bDragFromBoard)
	{
		bDragFromBoard = false;
		bool bUsed = false, bAll = false;
		if (I.Kind == ECItem::Steak)
		{
			for (int32 i = 0; i < NumPans && !bUsed; ++i)
			{
				if (L.InPan(i, P) && !Pans[i].Busy())
				{
					Pans[i] = FCPan();
					Pans[i].Item = I;
					bUsed = bAll = true;
				}
			}
			if (!bUsed && L.InPot(P)) Say(TEXT("Steaks go in the frying pan"), FLinearColor(1.f, 0.6f, 0.3f), P);
			else if (!bUsed && L.InPan(0, P)) Say(TEXT("The pan is busy"), FLinearColor(1.f, 0.6f, 0.3f), P);
		}
		else if (L.InPot(P)) bUsed = bAll = AddToPot(I, P);     // the whole sliced fillet goes in
		else if (L.InPlate(P)) bUsed = PlaceOnPlate(I, P, L);
		if (bUsed)
		{
			if (bAll || --Board.Count <= 0) Board = FCItem();
		}
		else if (!L.InBoard(P) && !L.InPot(P) && !L.InPan(0, P)) { Board = FCItem(); Say(TEXT("Thrown away"), FLinearColor(1.f, 0.5f, 0.4f), P); }
		return;
	}

	// dropped from the bar / bins / plate
	if (I.Kind == ECItem::Fillet)
	{
		if (L.InBoard(P))
		{
			if (Board.IsValid()) { Say(TEXT("The board is busy"), FLinearColor(1.f, 0.6f, 0.3f), P); return; }
			Board = I;
			if (GM) Board.bPrime = GM->TakeFish(I.Fish, I.bPrime);   // a fillet comes out of the fridge
			// how it should be cut depends on what it is for
			const FRecipe* R = Active.IsValid() ? &AbyssRecipes::Get(Active->Dish) : nullptr;
			if (R && R->Fish == I.Fish) SetCuts(R->Cuts, R->CutTight);
			else SetCuts(I.Fish == 0 ? 5 : 2, I.Fish == 0 ? 1500.f : 600.f);
			Say(NumCuts >= 4 ? TEXT("Thin, even slices: drag the knife down each gold line") : TEXT("Drag the knife down the gold lines"), FLinearColor(1.f, 0.9f, 0.6f), P);
		}
		return;
	}
	if (L.InPot(P)) AddToPot(I, P);
	else if (L.InPlate(P)) PlaceOnPlate(I, P, L);
	else if (I.Kind == ECItem::Steak) { for (int32 i = 0; i < NumPans; ++i) if (L.InPan(i, P) && !Pans[i].Busy()) { Pans[i].Item = I; break; } }
}

float FKitchenCounter::ScorePlate(const FRecipe& R, int32 Dish, bool& bOutPrime) const
{
	// multiset match of plate contents vs the recipe; fish items must be the ordered fish
	TArray<EIngredient> Need = R.Slots;
	int32 Matched = 0, Extra = 0;
	float Prep = 0.f, Cook = 0.f;
	int32 NPrep = 0, NCook = 0;
	bOutPrime = false;
	for (const FCPlateItem& Pit : Plate)
	{
		const EIngredient G = Pit.Item.AsIngredient();
		const bool bFishItem = Pit.Item.IsFish();
		const bool bFishOk = !bFishItem || Pit.Item.Fish == R.Fish;
		const int32 Idx = bFishOk ? Need.Find(G) : INDEX_NONE;
		if (Idx != INDEX_NONE)
		{
			Need.RemoveAt(Idx);
			++Matched;
			if (bFishItem) { Prep += Pit.Item.Prep; ++NPrep; bOutPrime |= Pit.Item.bPrime; }
			if (Pit.Item.Kind == ECItem::Cooked || Pit.Item.Kind == ECItem::Stew) { Cook += Pit.Item.Cook; ++NCook; }
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
	bool bPrime = false;
	const float Score = ScorePlate(R, C->Dish, bPrime);
	BellRing = 0.6f;
	Plate.Reset();
	Dome->CompleteDish(C, Score, false, bPrime);
	bOpen = false;   // the chef carries the plate to the table; the pan and the pot keep going
}

// ---------------------------------------------------------------- autotest helpers

void FKitchenCounter::DebugPrepare(ARestaurantDome* Dome, float PanDone)
{
	ARestaurantCustomer* C = Active.Get();
	if (!C) return;
	const FRecipe& R = AbyssRecipes::Get(C->Dish);
	if (R.Fish < 0) return;
	Board = FCItem();
	Board.Kind = ECItem::Fillet;
	Board.Fish = (int8)R.Fish;
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(Dome)) Board.bPrime = GM->TakeFish(R.Fish, true);
	SetCuts(R.Cuts, R.CutTight);
	const int32 N = NumCuts;
	for (int32 t = 0; t < N; ++t) Cut(CutTargets[t] + 0.004f, 0.01f);
	if (R.bStew)
	{
		Pot = FCPot();
		Pot.Added = { ECItem::Kelp, ECItem::Sashimi };
		Pot.Scores = { 100.f, 95.f };
		Pot.Fish = Board;
		Pot.T = 5.5f;
		Board = FCItem();
	}
	else if (R.bCook)
	{
		Pans[0] = FCPan();
		Pans[0].Item = Board;
		Pans[0].Done = PanDone;
		Pans[0].bFlipped = PanDone > 0.5f;
		Pans[0].FlipScore = 95.f;
		Pans[0].Sear = 1.f;
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
		case EIngredient::FishSlice: Pit.Item = Board; Pit.Item.Kind = ECItem::Sashimi; Pit.Item.Fish = (int8)R.Fish; Pit.Item.Prep = Board.IsValid() ? Board.Prep : 95.f; break;
		case EIngredient::Steak:
			Pit.Item = Pans[0].Busy() ? Pans[0].Item : FCItem();
			Pit.Item.Kind = ECItem::Cooked; Pit.Item.Fish = (int8)R.Fish; Pit.Item.Cook = 92.f;
			Pans[0] = FCPan();
			break;
		case EIngredient::Stew:
			Pot.Added.AddUnique(ECItem::Kelp); Pot.Added.AddUnique(ECItem::Sashimi); Pot.Added.AddUnique(ECItem::Greens);
			while (Pot.Scores.Num() < 3) Pot.Scores.Add(95.f);
			Pot.T = FCPot::ReadyAt;
			Pit.Item = Ladle();
			Pot = FCPot();
			break;
		case EIngredient::Rice:   Pit.Item.Kind = ECItem::Rice; break;
		case EIngredient::Lemon:  Pit.Item.Kind = ECItem::Lemon; break;
		case EIngredient::Greens: Pit.Item.Kind = ECItem::Greens; break;
		case EIngredient::Kelp:   Pit.Item.Kind = ECItem::Kelp; break;
		case EIngredient::Wasabi: Pit.Item.Kind = ECItem::Wasabi; break;
		case EIngredient::Sesame: Pit.Item.Kind = ECItem::Sesame; break;
		default: break;
		}
		Pit.Item.Count = 1;
		Plate.Add(Pit);
	}
	Board = FCItem();
}
