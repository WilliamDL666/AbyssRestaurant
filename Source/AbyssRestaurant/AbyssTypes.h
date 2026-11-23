#pragma once

#include "CoreMinimal.h"
#include "AbyssTypes.generated.h"

UENUM(BlueprintType)
enum class EFishType : uint8
{
	Yellowtail,
	Tuna,
	Grouper,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EAbyssPhase : uint8
{
	Explore,
	Prep,       // before opening: tonight's menu, seasonings and garnishes
	Restaurant,
	Summary,
	Ending      // day-2 cliffhanger: giant sonar contact + title card
};

UENUM(BlueprintType)
enum class ECatchMethod : uint8
{
	SubClaw,   // submarine capture beam / claw
	Harpoon,   // diver speargun
	Net        // diver hand net
};

// How well a fish was caught: decides what its dish sells for
enum class EFishQuality : uint8
{
	Normal,
	Prime
};

// What a guest likes; a seasoning / garnish with the same taste earns a bigger tip
enum class ETaste : uint8
{
	None,
	Salty,
	Rich,
	Spicy,
	Fresh,
	Count
};

// Seasonings (one per dish) and garnishes (optional, one per dish) bought before opening
enum class ESeason : uint8
{
	None,      // house style: always available, free
	Salt,
	Soy,
	Chili,
	Saffron,   // rare: shines on prime fish
	Yuzu,      // fresh garnishes spoil
	Shiso,
	Urchin,
	Count
};

struct FSeasonDef
{
	const TCHAR* Name;
	int32 Slot;        // 0 = main seasoning, 1 = garnish
	int32 Tier;        // 0 basic, 1 fresh, 2 rare
	int32 Cost;        // per portion
	int32 Shelf;       // nights it keeps (0 = never spoils)
	float Mult;        // price multiplier
	float PrimeMult;   // price multiplier on a prime fish (signature dish)
	ETaste Taste;
	const TCHAR* Hint;
};

UENUM(BlueprintType)
enum class EDiverTool : uint8
{
	Harpoon,
	Net
};

struct FFishDef
{
	const TCHAR* Name;
	const TCHAR* Dish;
	int32 Price;
	int32 Stars;
	FLinearColor Body;
	FLinearColor Fin;
	float Scale;      // 1 = 60cm fish
	float Speed;      // cm/s cruise
	float CatchTime;  // seconds of sub claw beam to catch
	float Glow;       // emissive strength
	// Which methods work on this fish (different fish -> different methods)
	bool bSubClaw;
	bool bHarpoon;
	bool bNet;
	int32 HarpoonHits;
	const TCHAR* Hint;
};

namespace AbyssData
{
	constexpr int32 NumFish = (int32)EFishType::Count;
	// Dish index == NumFish means "Kelp Salad" (needs no fish)
	constexpr int32 SaladDish = NumFish;
	constexpr int32 SaladPrice = 30;
	// Dishes: 0 nigiri (yellowtail), 1 tuna teppanyaki, 2 grouper teppanyaki, 3 kelp salad, 4 yellowtail kelp stew
	constexpr int32 StewDish = 4;
	constexpr int32 NumDishes = 5;
	constexpr int32 NumSeason = (int32)ESeason::Count;
	// A prime catch sells for this much more
	constexpr float PrimeBonus = 0.35f;
	constexpr int32 MaxShelf = 3;
	// Divers can't go deeper than this (cm). Deeper fish need the submarine claw.
	constexpr float DiverMaxDepth = 6000.f;

	ABYSSRESTAURANT_API const FFishDef& Fish(EFishType Type);
	ABYSSRESTAURANT_API FString DishName(int32 Dish);
	ABYSSRESTAURANT_API int32 DishPrice(int32 Dish);
	ABYSSRESTAURANT_API bool CanCatch(EFishType Type, ECatchMethod Method);
	ABYSSRESTAURANT_API const TCHAR* MethodName(ECatchMethod Method);
	// Which fish a dish uses (-1 = none)
	ABYSSRESTAURANT_API int32 DishFish(int32 Dish);
	ABYSSRESTAURANT_API const FSeasonDef& Season(int32 Id);
	ABYSSRESTAURANT_API const TCHAR* TasteName(ETaste T);
	ABYSSRESTAURANT_API FLinearColor TasteColor(ETaste T);
	// How to land this fish as a prime catch
	ABYSSRESTAURANT_API const TCHAR* PrimeTip(EFishType Type);
}
