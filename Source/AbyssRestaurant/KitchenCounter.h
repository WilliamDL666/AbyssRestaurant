#pragma once

#include "CoreMinimal.h"
#include "KitchenGame.h"

class ARestaurantCustomer;
class ARestaurantDome;

// Papa's-style full-screen kitchen counter: tickets on top, stove / cutting board / plate / bell on the counter,
// a drag-and-drop ingredient bar at the bottom. The pan and the stew pot keep cooking while the chef is away.
// Each dish leans on one skill: nigiri on knife work, teppanyaki on heat control, the stew on what goes in when.

enum class ECItem : uint8
{
	None,
	Fillet,   // raw fillet from the fridge (has Fish) -> cutting board
	Sashimi,  // cut yellowtail -> plate, or into the stew pot
	Steak,    // cut tuna / grouper steak, raw -> pan
	Cooked,   // seared steak -> plate
	Rice,
	Lemon,
	Greens,
	Kelp,
	Wasabi,
	Sesame,
	Stew      // a ladle from the pot -> plate
};

struct FCItem
{
	ECItem Kind = ECItem::None;
	int8 Fish = -1;
	float Prep = 100.f;   // knife score
	float Cook = 100.f;   // pan / pot score
	bool bBurnt = false;
	bool bPrime = false;  // cut from a prime catch
	int8 Count = 1;       // portions left (a sliced fillet gives several)
	bool IsValid() const { return Kind != ECItem::None; }
	// what this item counts as on a plate (EIngredient::Count = not plateable)
	EIngredient AsIngredient() const;
	bool IsFish() const { return Kind == ECItem::Fillet || Kind == ECItem::Sashimi || Kind == ECItem::Steak || Kind == ECItem::Cooked || Kind == ECItem::Stew; }
};

struct FCPan
{
	FCItem Item;          // the steak in the pan
	float Done = 0.f;     // 0 raw .. 0.86 perfect .. 1 burnt
	bool bFlipped = false;
	float FlipScore = 0.f;
	float Sear = 0.f;     // crust from high heat early on (0..1)
	float Scorch = 0.f;   // seconds of high heat after it should have come down
	bool Busy() const { return Item.IsValid(); }
};

// The stew pot on the second burner. Kelp starts the stock; the fish and the greens each have their moment.
struct FCPot
{
	TArray<ECItem> Added;
	TArray<float> Scores;
	float T = 0.f;        // simmer clock (seconds at medium heat)
	float Boil = 0.f;     // seconds at a rolling boil (bad)
	FCItem Fish;
	bool Busy() const { return Added.Num() > 0; }
	bool Has(ECItem K) const { return Added.Contains(K); }
	bool Complete() const { return Added.Num() >= 3; }

	static constexpr float FishAt = 3.2f, GreensAt = 7.f, ReadyAt = 10.5f, MushyAt = 14.f;
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
	FVector2D Pot;                 // the stew pot sits on the second burner
	float PotR = 0.f;
	FVector2D Knob[2];             // gas knobs: 0 = pan, 1 = pot
	float KnobR = 0.f;
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
	bool InPot(const FVector2D& P) const;
	int32 KnobAt(const FVector2D& P) const;
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
	// one frying pan (burner 0); burner 1 holds the stew pot
	static constexpr int32 NumPans = 1;

	bool bOpen = false;
	TWeakObjectPtr<ARestaurantCustomer> Active;   // ticket being plated
	FCItem Board;
	TArray<float> Cuts, CutScores;
	int32 NumCuts = 3;
	float CutTight = 900.f;
	float CutTargets[6] = { 0.28f, 0.5f, 0.72f, 0.f, 0.f, 0.f };
	float CutGap() const { return NumCuts > 1 ? CutTargets[1] - CutTargets[0] : 0.2f; }
	bool bCutting = false;        // the knife is down: drag it through the fillet
	FVector2D CutStart = FVector2D::ZeroVector;
	FCPan Pans[2];
	FCPot Pot;
	int32 Heat[2] = { 1, 1 };     // 0 low, 1 medium, 2 high
	TArray<FCPlateItem> Plate;
	// drag
	FCItem Drag;
	FVector2D Mouse = FVector2D::ZeroVector;
	FVector2D DownAt = FVector2D::ZeroVector;
	int32 DragFromPan = -1;
	bool bDragFromBoard = false;
	bool bDragFromPot = false;
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
	static float HeatRate(int32 Level) { return Level <= 0 ? 0.55f : Level == 1 ? 1.f : 1.7f; }
	// what the pot wants next (ECItem::None when it has everything)
	ECItem PotNext() const;

	// AutoTest: run the whole dish for the active ticket with good timing
	void DebugPrepare(ARestaurantDome* Dome, float PanDone);
	void DebugPlate(ARestaurantDome* Dome);
	void Serve(ARestaurantDome* Dome);

private:
	void Say(const FString& S, const FLinearColor& C, const FVector2D& At);
	bool PlaceOnPlate(const FCItem& I, const FVector2D& P, const FCounterLayout& L);
	void SetCuts(int32 N, float Tight);
	void Cut(float X01, float Slant);
	bool AddToPot(const FCItem& I, const FVector2D& P);
	FCItem Ladle() const;
	FCItem TakeFromPan(int32 Pi, const FVector2D& P);
	float ScorePlate(const FRecipe& R, int32 Dish, bool& bOutPrime) const;
};
