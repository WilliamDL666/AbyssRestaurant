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
	// Divers can't go deeper than this (cm). Deeper fish need the submarine claw.
	constexpr float DiverMaxDepth = 6000.f;

	ABYSSRESTAURANT_API const FFishDef& Fish(EFishType Type);
	ABYSSRESTAURANT_API FString DishName(int32 Dish);
	ABYSSRESTAURANT_API int32 DishPrice(int32 Dish);
	ABYSSRESTAURANT_API bool CanCatch(EFishType Type, ECatchMethod Method);
	ABYSSRESTAURANT_API const TCHAR* MethodName(ECatchMethod Method);
}
