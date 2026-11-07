#pragma once

#include "CoreMinimal.h"

class ARestaurantCustomer;

// ---------------------------------------------------------------- recipes (data-driven)

enum class EIngredient : uint8
{
	Rice,
	FishSlice,
	Steak,
	Lemon,
	Greens,
	Kelp,
	Wasabi,
	Sesame,
	Count
};

struct FRecipe
{
	const TCHAR* Name;      // English (translated at draw time)
	int32 Fish;             // EFishType index, -1 = no fish
	bool bSlice;            // Prep: cut the fillet
	bool bCook;             // Cook: flip + finish on the teppan
	TArray<EIngredient> Slots; // Plate: what goes on the plate
	float WPrep, WCook, WPlate; // score weights
	int32 Price;
	float CookWindow;       // half-width of the flip/finish sweet spot (0..1 of the bar)
};

namespace AbyssRecipes
{
	// Recipe index == dish index used by orders (0..2 = fish dishes, 3 = kelp salad)
	ABYSSRESTAURANT_API const TArray<FRecipe>& All();
	ABYSSRESTAURANT_API const FRecipe& Get(int32 Dish);
	ABYSSRESTAURANT_API const TCHAR* IngredientName(EIngredient I);
	ABYSSRESTAURANT_API FLinearColor IngredientColor(EIngredient I);
}

// ---------------------------------------------------------------- screen layout shared by HUD (draw) + controller (mouse)

struct FKitchenLayout
{
	float S = 1.f;
	FVector2D PanelPos, PanelSize;
	FVector2D TrayOrigin, TrayItemSize;
	float TrayGap = 0.f;
	FVector2D PlateCenter;
	float PlateR = 0.f, SlotR = 0.f;
	FVector2D ServePos, ServeSize;

	static FKitchenLayout Make(float ScreenW, float ScreenH);
	FVector2D TrayItemPos(int32 i) const;        // top-left of tray item i
	FVector2D SlotPos(int32 i, int32 Num) const; // center of plate slot i
	bool InServe(const FVector2D& P) const;
};

// ---------------------------------------------------------------- the 3-step cooking minigame

enum class EKitchenStep : uint8
{
	None,
	Slice,
	Cook,
	Plate,
	Done
};

struct ABYSSRESTAURANT_API FKitchenGame
{
	bool bActive = false;
	// The player must stand at the station for the current step (prep / stove / serving counter)
	bool bAtStation = true;
	int32 Dish = -1;
	TWeakObjectPtr<ARestaurantCustomer> For;
	EKitchenStep Step = EKitchenStep::None;

	// Slice
	float Knife = 0.f, KnifeDir = 1.f;
	float CutTargets[3] = { 0.28f, 0.5f, 0.72f };
	bool CutUsed[3] = { false, false, false };
	TArray<float> Cuts;        // where the player cut
	TArray<float> CutScores;

	// Cook
	float Doneness = 0.f;
	int32 CookStage = 0;       // 0 = waiting for flip, 1 = waiting for finish
	float FlipAt = 0.45f, FinishAt = 0.86f;
	float FlipScore = 0.f, FinishScore = 0.f;
	bool bFlipped = false;

	// Plate
	TArray<EIngredient> Tray;
	TArray<int32> Placed;      // per slot: ingredient or -1
	int32 Dragging = -1;       // ingredient being dragged
	FVector2D DragPos = FVector2D::ZeroVector;

	float PrepScore = 100.f, CookScore = 100.f, PlateScore = 0.f;
	FString Feedback;
	FLinearColor FeedbackColor = FLinearColor::White;
	float FeedbackTime = 0.f;
	float StepTime = 0.f;

	void Start(int32 InDish, ARestaurantCustomer* InFor);
	void Cancel();
	void Tick(float Dt);
	void PressAction();                                  // Space / E / click during Slice + Cook
	void MouseDown(const FVector2D& P, const FKitchenLayout& L);
	void MouseUp(const FVector2D& P, const FKitchenLayout& L);
	void MouseMove(const FVector2D& P) { DragPos = P; }
	void FinishPlate();
	float TotalScore() const;                            // 0..100
	bool IsDone() const { return bActive && Step == EKitchenStep::Done; }

	// AutoTest helper: place the right ingredients
	void DebugFillPlate();

private:
	void NextStep();
	void Say(const FString& Msg, const FLinearColor& C);
	static float WindowScore(float Value, float Center, float HalfWidth);
};
