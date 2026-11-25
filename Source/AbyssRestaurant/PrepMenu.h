#pragma once

#include "CoreMinimal.h"
#include "AbyssTypes.h"

class AAbyssGameMode;

// Before-opening screen: the fridge, tonight's menu (one seasoning + one optional garnish per dish) and the market.
// Layout shared by the HUD (drawing) and the player controller (clicks).
struct ABYSSRESTAURANT_API FPrepLayout
{
	float U = 1.f;            // 1080p scale
	FVector2D Pos, Size;      // the whole panel

	// menu rows show the dishes in this order (nigiri, stew, tuna, grouper, salad)
	static const int32 DishOrder[AbyssData::NumDishes];
	// market rows: seasonings 1..NumSeason-1, then the bait
	static constexpr int32 MarketRows = AbyssData::NumSeason;

	static FPrepLayout Make(float W, float H);
	FBox2D Box(float X, float Y, float W, float H) const { return FBox2D(Pos + FVector2D(X, Y) * U, Pos + FVector2D(X + W, Y + H) * U); }
	FBox2D FridgeCol() const { return Box(24.f, 104.f, 404.f, 748.f); }
	FBox2D DishRow(int32 Row) const { return Box(446.f, 104.f + Row * 150.f, 694.f, 142.f); }
	FBox2D Selector(int32 Row, int32 Slot) const { return Box(446.f + 140.f + Slot * 274.f, 104.f + Row * 150.f + 90.f, 262.f, 42.f); }
	FBox2D Arrow(int32 Row, int32 Slot, int32 Dir) const;
	FBox2D MarketRow(int32 Row) const { return Box(1158.f, 104.f + Row * 94.f, 578.f, 86.f); }
	FBox2D Minus(int32 Row) const { return Box(1158.f + 448.f, 104.f + Row * 94.f + 22.f, 44.f, 44.f); }
	FBox2D Plus(int32 Row) const { return Box(1158.f + 518.f, 104.f + Row * 94.f + 22.f, 44.f, 44.f); }
	FBox2D OpenBtn() const { return Box(640.f, 872.f, 480.f, 72.f); }
	// tabs: 0 = menu + market, 1 = aquarium
	FBox2D Tab(int32 i) const { return Box(24.f + i * 200.f, 20.f, 190.f, 56.f); }
	// aquarium tab
	FBox2D LiveRow(int32 Fish) const { return Box(24.f, 150.f + Fish * 150.f, 404.f, 140.f); }
	FBox2D DisplayBtn(int32 Fish) const { return Box(38.f, 150.f + Fish * 150.f + 88.f, 184.f, 42.f); }
	FBox2D SellBtn(int32 Fish) const { return Box(232.f, 150.f + Fish * 150.f + 88.f, 184.f, 42.f); }
	FBox2D TankSlot(int32 i) const { return Box(446.f + (i % 3) * 232.f, 104.f + (i / 3) * 376.f, 220.f, 366.f); }
	FBox2D SlotBtn(int32 i) const { return Box(446.f + (i % 3) * 232.f + 20.f, 104.f + (i / 3) * 376.f + 312.f, 180.f, 40.f); }

	// handles a left click; returns true when something was hit
	bool Click(AAbyssGameMode* GM, const FVector2D& P) const;
};
