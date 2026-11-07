#pragma once

#include "CoreMinimal.h"
#include "KitchenGame.h"

class ARestaurantCustomer;
class ARestaurantDome;

// Papa's-style full-screen kitchen counter: tickets on top, stove / cutting board / plate / bell on the counter,
// a drag-and-drop ingredient bar at the bottom. Pans keep cooking (and burning) while the chef is away.

enum class ECItem : uint8
{
	None,
	Fillet,   // raw fillet from the fridge (has Fish) -> cutting board
	Sashimi,  // cut yellowtail -> plate
	Steak,    // cut tuna / grouper steak, raw -> pan
	Cooked,   // seared steak -> plate
	Rice,
	Lemon,
	Greens,
	Kelp,
	Wasabi,
	Sesame
};

struct FCItem
{
	ECItem Kind = ECItem::None;
	int8 Fish = -1;
	float Prep = 100.f;   // knife score
	float Cook = 100.f;   // pan score
	bool bBurnt = false;
	bool IsValid() const { return Kind != ECItem::None; }
	// what this item counts as on a plate (EIngredient::Count = not plateable)
	EIngredient AsIngredient() const;
};

struct FCPan
{
	FCItem Item;          // the steak in the pan
	float Done = 0.f;     // 0 raw .. 0.86 perfect .. 1 burnt
	bool bFlipped = false;
	float FlipScore = 0.f;
	bool Busy() const { return Item.IsValid(); }
};

struct FCPlateItem
{
	FCItem Item;
	FVector2D Off;        // position on the plate, -1..1
};

// Screen layout (pixels) shared by the HUD (drawing) and the controller (hit tests).
struct ABYSSRESTAURANT_API FCounterLayout
{
	float U = 1.f;                 // 1080p scale (fits 1920x1080 inside the window)
	FVector2D StovePos, StoveSize, PrepPos, PrepSize, DisplayPos, DisplaySize;
	FVector2D ServeBtn; float ServeR = 0.f;
	FVector2D SignPos, SignSize;
	FVector2D Origin;              // top-left of the counter picture
	FVector2D CounterSize;
	FVector2D Pan[2];
	float PanR = 0.f;
	FBox2D Board;
	FVector2D Plate;
	float PlateR = 0.f;
	FVector2D Bell;
	float BellR = 0.f;
	FVector2D Bins[5];
	FVector2D BarPos, BarSize;
	FVector2D SlotPos[9];
	float SlotSize = 0.f;
	FVector2D TicketPos[4];
	FVector2D TicketSize;
	FVector2D SideBtn[6];
	FVector2D SideBtnSize;
	FVector2D CloseBtn, CloseSize;
	FVector2D SubBtn;
	float SubSize = 0.f;

	static FCounterLayout Make(float W, float H, int32 NumTickets);
	FVector2D Img(float X, float Y) const { return Origin + FVector2D(X, Y) * U; } // counter picture px -> screen
	bool InPan(int32 i, const FVector2D& P) const;
	bool InPlate(const FVector2D& P) const;
	bool InBell(const FVector2D& P) const { return FVector2D::Distance(P, Bell) < BellR; }
	bool InBoard(const FVector2D& P) const { return Board.IsInside(P); }
	int32 SlotAt(const FVector2D& P) const;
	int32 BinAt(const FVector2D& P) const;
	int32 TicketAt(const FVector2D& P, int32 Num) const;
	int32 SideAt(const FVector2D& P) const;
	bool InClose(const FVector2D& P) const;
	bool InSub(const FVector2D& P) const { return FVector2D::Distance(P, SubBtn) < SubSize * 0.5f; }
};

struct ABYSSRESTAURANT_API FKitchenCounter
{
	bool bOpen = false;
	TWeakObjectPtr<ARestaurantCustomer> Active;   // ticket being plated
	FCItem Board;
	TArray<float> Cuts, CutScores;
	float CutTargets[3] = { 0.28f, 0.5f, 0.72f };
	FCPan Pans[2];
	TArray<FCPlateItem> Plate;
	// drag
	FCItem Drag;
	FVector2D Mouse = FVector2D::ZeroVector;
	FVector2D DownAt = FVector2D::ZeroVector;
	int32 DragFromPan = -1;
	bool bDragFromBoard = false;
	// feedback
	FString Feedback;
	FLinearColor FeedbackColor = FLinearColor::White;
	float FeedbackTime = 0.f;
	FVector2D FeedbackAt = FVector2D::ZeroVector;
	float BellRing = 0.f;
	float Time = 0.f;
	// side panels
	int32 SidePanel = -1;  // 0 menu, 1 decor, 2 tasks, 3 codex

	bool DragFromPanActive() const { return DragFromPan >= 0; }
	void Reset();
	void Tick(float Dt, ARestaurantDome* Dome);
	void MouseDown(const FVector2D& P, const FCounterLayout& L, ARestaurantDome* Dome);
	void MouseUp(const FVector2D& P, const FCounterLayout& L, ARestaurantDome* Dome);
	void SelectTicket(int32 Index, ARestaurantDome* Dome);

	static const TCHAR* ItemName(const FCItem& I);
	static FCItem BarItem(int32 Slot);           // what slot i of the ingredient bar hands out
	static FCItem BinItem(int32 Bin);

	// AutoTest: run the whole dish for the active ticket with good timing
	void DebugPrepare(ARestaurantDome* Dome, float PanDone);
	void DebugPlate(ARestaurantDome* Dome);
	void Serve(ARestaurantDome* Dome);

private:
	void Say(const FString& S, const FLinearColor& C, const FVector2D& At);
	bool PlaceOnPlate(const FCItem& I, const FVector2D& P, const FCounterLayout& L);
	void Cut(float X01);
	float ScorePlate(const FRecipe& R, int32 Dish) const;
};
