#include "KitchenGame.h"
#include "AbyssTypes.h"
#include "RestaurantCustomer.h"

// ---------------------------------------------------------------- recipes

namespace AbyssRecipes
{
	const TArray<FRecipe>& All()
	{
		using E = EIngredient;
		static const TArray<FRecipe> R = {
			// 0: Yellowtail nigiri - slice + plate (rice x2, fish x2, wasabi)
			{ TEXT("Yellowtail Nigiri"), 0, true, false, { E::Rice, E::Rice, E::FishSlice, E::FishSlice, E::Wasabi }, 0.4f, 0.f, 0.6f, 80, 0.07f },
			// 1: Tuna teppanyaki - slice + flip/finish + plate (steak, lemon, greens)
			{ TEXT("Tuna Teppanyaki"), 1, true, true, { E::Steak, E::Lemon, E::Greens }, 0.3f, 0.4f, 0.3f, 160, 0.06f },
			// 2: Grouper teppanyaki (signature) - narrower cooking window
			{ TEXT("Grouper Teppanyaki"), 2, true, true, { E::Steak, E::Lemon, E::Greens, E::Sesame }, 0.25f, 0.5f, 0.25f, 520, 0.04f },
			// 3: Kelp salad - no fish, plate only
			{ TEXT("Kelp Salad"), -1, false, false, { E::Kelp, E::Kelp, E::Sesame }, 0.f, 0.f, 1.f, 30, 0.07f },
		};
		return R;
	}

	const FRecipe& Get(int32 Dish)
	{
		const TArray<FRecipe>& R = All();
		return R[FMath::Clamp(Dish, 0, R.Num() - 1)];
	}

	const TCHAR* IngredientName(EIngredient I)
	{
		switch (I)
		{
		case EIngredient::Rice:      return TEXT("Rice");
		case EIngredient::FishSlice: return TEXT("Fish Slice");
		case EIngredient::Steak:     return TEXT("Fish Steak");
		case EIngredient::Lemon:     return TEXT("Lemon");
		case EIngredient::Greens:    return TEXT("Greens");
		case EIngredient::Kelp:      return TEXT("Seaweed");
		case EIngredient::Wasabi:    return TEXT("Wasabi");
		case EIngredient::Sesame:    return TEXT("Sesame");
		default: return TEXT("?");
		}
	}

	FLinearColor IngredientColor(EIngredient I)
	{
		switch (I)
		{
		case EIngredient::Rice:      return FLinearColor(0.95f, 0.94f, 0.9f);
		case EIngredient::FishSlice: return FLinearColor(0.98f, 0.55f, 0.45f);
		case EIngredient::Steak:     return FLinearColor(0.6f, 0.32f, 0.15f);
		case EIngredient::Lemon:     return FLinearColor(1.f, 0.88f, 0.2f);
		case EIngredient::Greens:    return FLinearColor(0.3f, 0.75f, 0.25f);
		case EIngredient::Kelp:      return FLinearColor(0.12f, 0.4f, 0.18f);
		case EIngredient::Wasabi:    return FLinearColor(0.55f, 0.8f, 0.3f);
		case EIngredient::Sesame:    return FLinearColor(0.85f, 0.75f, 0.55f);
		default: return FLinearColor::White;
		}
	}
}

// ---------------------------------------------------------------- layout

FKitchenLayout FKitchenLayout::Make(float W, float H)
{
	FKitchenLayout L;
	L.S = H / 1080.f;
	// panel art is 2000x1120 -> 1200x672 at 1080p
	L.PanelSize = FVector2D(1200.f, 672.f) * L.S;
	L.PanelPos = FVector2D(W * 0.5f - L.PanelSize.X * 0.5f, H * 0.5f - L.PanelSize.Y * 0.5f + 30.f * L.S);
	L.TrayItemSize = FVector2D(150.f, 125.f) * L.S;
	L.TrayGap = 16.f * L.S;
	L.TrayOrigin = L.PanelPos + FVector2D(330.f, 130.f) * L.S;
	L.PlateCenter = L.PanelPos + FVector2D(1000.f, 330.f) * L.S;
	L.PlateR = 165.f * L.S;
	L.SlotR = 46.f * L.S;
	L.ServeSize = FVector2D(240.f, 70.f) * L.S;
	L.ServePos = L.PanelPos + L.PanelSize - L.ServeSize - FVector2D(40.f, 34.f) * L.S;
	return L;
}

FVector2D FKitchenLayout::TrayItemPos(int32 i) const
{
	const int32 Col = i % 3, Row = i / 3;
	return TrayOrigin + FVector2D(Col * (TrayItemSize.X + TrayGap), Row * (TrayItemSize.Y + TrayGap));
}

FVector2D FKitchenLayout::SlotPos(int32 i, int32 Num) const
{
	if (Num <= 1) return PlateCenter;
	const float A = -PI * 0.5f + 2.f * PI * i / Num;
	return PlateCenter + FVector2D(FMath::Cos(A), FMath::Sin(A)) * PlateR * 0.55f;
}

bool FKitchenLayout::InServe(const FVector2D& P) const
{
	return P.X >= ServePos.X && P.Y >= ServePos.Y && P.X <= ServePos.X + ServeSize.X && P.Y <= ServePos.Y + ServeSize.Y;
}

// ---------------------------------------------------------------- game

float FKitchenGame::WindowScore(float V, float C, float W)
{
	const float D = FMath::Abs(V - C);
	if (D <= W) return 100.f - 30.f * D / W;
	return FMath::Max(0.f, 70.f - 35.f * (D - W) / W);
}

void FKitchenGame::Say(const FString& Msg, const FLinearColor& C)
{
	Feedback = Msg;
	FeedbackColor = C;
	FeedbackTime = 1.1f;
}

void FKitchenGame::Start(int32 InDish, ARestaurantCustomer* InFor)
{
	*this = FKitchenGame();
	bActive = true;
	Dish = InDish;
	For = InFor;

	const FRecipe& R = AbyssRecipes::Get(Dish);
	// tray: everything the recipe needs + a few decoys
	TSet<EIngredient> Need(R.Slots);
	for (EIngredient I : Need) Tray.Add(I);
	const EIngredient Decoys[] = { EIngredient::Rice, EIngredient::Lemon, EIngredient::Kelp, EIngredient::Greens, EIngredient::Wasabi, EIngredient::Sesame, EIngredient::FishSlice, EIngredient::Steak };
	for (EIngredient I : Decoys)
	{
		if (Tray.Num() >= 6) break;
		if (!Need.Contains(I)) Tray.Add(I);
	}
	Placed.Init(-1, R.Slots.Num());

	Step = EKitchenStep::None;
	NextStep();
}

void FKitchenGame::Cancel()
{
	*this = FKitchenGame();
}

void FKitchenGame::NextStep()
{
	const FRecipe& R = AbyssRecipes::Get(Dish);
	StepTime = 0.f;
	switch (Step)
	{
	case EKitchenStep::None:  Step = R.bSlice ? EKitchenStep::Slice : R.bCook ? EKitchenStep::Cook : EKitchenStep::Plate; break;
	case EKitchenStep::Slice: Step = R.bCook ? EKitchenStep::Cook : EKitchenStep::Plate; break;
	case EKitchenStep::Cook:  Step = EKitchenStep::Plate; break;
	case EKitchenStep::Plate: Step = EKitchenStep::Done; break;
	default: break;
	}
	if (Step == EKitchenStep::Cook)
	{
		Doneness = 0.f;
		CookStage = 0;
		bFlipped = false;
	}
}

void FKitchenGame::Tick(float Dt)
{
	if (!bActive || !bAtStation) return;
	StepTime += Dt;
	FeedbackTime = FMath::Max(0.f, FeedbackTime - Dt);

	if (Step == EKitchenStep::Slice)
	{
		Knife += KnifeDir * 0.75f * Dt;
		if (Knife >= 1.f) { Knife = 1.f; KnifeDir = -1.f; }
		if (Knife <= 0.f) { Knife = 0.f; KnifeDir = 1.f; }
	}
	else if (Step == EKitchenStep::Cook)
	{
		Doneness += 0.2f * Dt;
		const float W = AbyssRecipes::Get(Dish).CookWindow;
		if (CookStage == 0 && Doneness > FlipAt + 3.f * W)
		{
			// never flipped: one side burns
			FlipScore = 0.f;
			CookStage = 1;
			Say(TEXT("Forgot to flip - one side burnt!"), FLinearColor(1.f, 0.35f, 0.25f));
		}
		if (Doneness >= 1.f)
		{
			FinishScore = 0.f;
			CookScore = (FlipScore + FinishScore) * 0.5f;
			Say(TEXT("BURNT..."), FLinearColor(1.f, 0.35f, 0.25f));
			NextStep();
		}
	}
}

void FKitchenGame::PressAction()
{
	if (!bActive || !bAtStation) return;
	if (Step == EKitchenStep::Slice)
	{
		// score the cut against the nearest unused gold line
		int32 Best = -1;
		float BestD = 1e9f;
		for (int32 i = 0; i < 3; ++i)
		{
			if (CutUsed[i]) continue;
			const float D = FMath::Abs(Knife - CutTargets[i]);
			if (D < BestD) { BestD = D; Best = i; }
		}
		if (Best < 0) return;
		CutUsed[Best] = true;
		const float Sc = WindowScore(Knife, CutTargets[Best], 0.035f);
		Cuts.Add(Knife);
		CutScores.Add(Sc);
		Say(Sc >= 90.f ? TEXT("Clean cut!") : Sc >= 70.f ? TEXT("Good cut") : TEXT("Messy cut..."),
			Sc >= 90.f ? FLinearColor(0.35f, 1.f, 0.45f) : Sc >= 70.f ? FLinearColor(1.f, 0.85f, 0.25f) : FLinearColor(1.f, 0.45f, 0.3f));
		if (Cuts.Num() >= 3)
		{
			PrepScore = (CutScores[0] + CutScores[1] + CutScores[2]) / 3.f;
			NextStep();
		}
	}
	else if (Step == EKitchenStep::Cook)
	{
		const float W = AbyssRecipes::Get(Dish).CookWindow;
		if (CookStage == 0)
		{
			FlipScore = WindowScore(Doneness, FlipAt, W);
			bFlipped = true;
			CookStage = 1;
			Say(FlipScore >= 90.f ? TEXT("Perfect flip!") : FlipScore >= 70.f ? TEXT("Flipped") : TEXT("Bad timing..."),
				FlipScore >= 70.f ? FLinearColor(0.35f, 1.f, 0.45f) : FLinearColor(1.f, 0.45f, 0.3f));
		}
		else
		{
			FinishScore = WindowScore(Doneness, FinishAt, W);
			CookScore = (FlipScore + FinishScore) * 0.5f;
			Say(FinishScore >= 90.f ? TEXT("Perfectly cooked!") : FinishScore >= 70.f ? TEXT("Nicely done") : (Doneness < FinishAt ? TEXT("UNDERCOOKED...") : TEXT("Overcooked...")),
				FinishScore >= 70.f ? FLinearColor(0.35f, 1.f, 0.45f) : FLinearColor(1.f, 0.45f, 0.3f));
			NextStep();
		}
	}
}

void FKitchenGame::MouseDown(const FVector2D& P, const FKitchenLayout& L)
{
	if (!bActive || !bAtStation) return;
	DragPos = P;
	if (Step == EKitchenStep::Slice || Step == EKitchenStep::Cook)
	{
		PressAction();
		return;
	}
	if (Step != EKitchenStep::Plate) return;

	if (L.InServe(P))
	{
		FinishPlate();
		return;
	}
	for (int32 i = 0; i < Tray.Num(); ++i)
	{
		const FVector2D TP = L.TrayItemPos(i);
		if (P.X >= TP.X && P.Y >= TP.Y && P.X <= TP.X + L.TrayItemSize.X && P.Y <= TP.Y + L.TrayItemSize.Y)
		{
			Dragging = (int32)Tray[i];
			return;
		}
	}
	// pick an already placed ingredient back up
	for (int32 s = 0; s < Placed.Num(); ++s)
	{
		if (Placed[s] >= 0 && FVector2D::Distance(P, L.SlotPos(s, Placed.Num())) < L.SlotR)
		{
			Dragging = Placed[s];
			Placed[s] = -1;
			return;
		}
	}
}

void FKitchenGame::MouseUp(const FVector2D& P, const FKitchenLayout& L)
{
	if (!bActive || Step != EKitchenStep::Plate || Dragging < 0) return;
	int32 Best = -1;
	float BestD = L.SlotR * 1.5f;
	for (int32 s = 0; s < Placed.Num(); ++s)
	{
		const float D = FVector2D::Distance(P, L.SlotPos(s, Placed.Num()));
		if (D < BestD) { BestD = D; Best = s; }
	}
	if (Best >= 0)
	{
		Placed[Best] = Dragging;
	}
	Dragging = -1;
}

void FKitchenGame::FinishPlate()
{
	const FRecipe& R = AbyssRecipes::Get(Dish);
	float Sum = 0.f;
	TArray<EIngredient> Want = R.Slots;
	// order-independent matching: each placed item that is still wanted counts as correct
	for (int32 s = 0; s < Placed.Num(); ++s)
	{
		if (Placed[s] < 0) continue;
		const EIngredient I = (EIngredient)Placed[s];
		const int32 Idx = Want.Find(I);
		if (Idx != INDEX_NONE)
		{
			Sum += 100.f;
			Want.RemoveAt(Idx);
		}
		else
		{
			Sum += 20.f; // wrong ingredient
		}
	}
	PlateScore = R.Slots.Num() > 0 ? Sum / R.Slots.Num() : 100.f;
	NextStep();
}

void FKitchenGame::DebugFillPlate()
{
	const FRecipe& R = AbyssRecipes::Get(Dish);
	for (int32 s = 0; s < Placed.Num() && s < R.Slots.Num(); ++s) Placed[s] = (int32)R.Slots[s];
}

float FKitchenGame::TotalScore() const
{
	const FRecipe& R = AbyssRecipes::Get(Dish);
	const float WP = R.bSlice ? R.WPrep : 0.f;
	const float WC = R.bCook ? R.WCook : 0.f;
	const float WL = R.WPlate;
	const float WSum = FMath::Max(WP + WC + WL, 0.001f);
	return (PrepScore * WP + CookScore * WC + PlateScore * WL) / WSum;
}
