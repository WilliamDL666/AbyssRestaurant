#include "AbyssRestaurant.h"
#include "AbyssTypes.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogAbyss);

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, AbyssRestaurant, "AbyssRestaurant");

namespace AbyssData
{
	static const FFishDef GFish[NumFish] =
	{
		// Yellowtail: small, schools in the shallow reef -> diver NET catches several at once.
		{ TEXT("Yellowtail"), TEXT("Yellowtail Nigiri"), 80, 1,
		  FLinearColor(0.80f, 0.62f, 0.10f), FLinearColor(0.15f, 0.30f, 0.55f), 0.9f, 260.f, 0.9f, 0.0f,
		  /*Sub*/ false, /*Harpoon*/ true, /*Net*/ true, 1,
		  TEXT("Schools scatter together when startled. Swim in slowly and net 3 at once") },
		// Tuna: big, fast, strong -> SPEARGUN (2 hits), or a long sub claw hold. Breaks nets.
		{ TEXT("Bluefin Tuna"), TEXT("Tuna Teppanyaki"), 160, 2,
		  FLinearColor(0.10f, 0.16f, 0.30f), FLinearColor(0.55f, 0.58f, 0.62f), 1.8f, 640.f, 2.6f, 0.0f,
		  true, true, false, 2,
		  TEXT("Patrols a fixed loop - chasing burns battery. Park ahead on its route and claw it as it passes") },
		// Grouper: rare, lives below diver depth limit -> SUB CLAW.
		{ TEXT("Rare Grouper"), TEXT("Grouper Teppanyaki"), 520, 3,
		  FLinearColor(0.42f, 0.12f, 0.80f), FLinearColor(0.10f, 0.85f, 0.75f), 1.6f, 300.f, 2.0f, 1.2f,
		  true, true, false, 1,
		  TEXT("Hides in its den from light and noise. Lights off [F], creep in, or drop bait [B]") },
	};

	const FFishDef& Fish(EFishType Type)
	{
		return GFish[FMath::Clamp((int32)Type, 0, NumFish - 1)];
	}

	FString DishName(int32 Dish)
	{
		if (Dish == StewDish) return FString(TEXT("Yellowtail Kelp Stew"));
		return Dish >= 0 && Dish < NumFish ? FString(GFish[Dish].Dish) : FString(TEXT("Kelp Salad"));
	}

	int32 DishPrice(int32 Dish)
	{
		if (Dish == StewDish) return 120;
		return Dish >= 0 && Dish < NumFish ? GFish[Dish].Price : SaladPrice;
	}

	int32 DishFish(int32 Dish)
	{
		if (Dish == StewDish) return (int32)EFishType::Yellowtail;
		return Dish >= 0 && Dish < NumFish ? Dish : -1;
	}

	static const FSeasonDef GSeason[NumSeason] =
	{
		{ TEXT("House Style"),   0, 0,  0, 0, 1.00f, 1.00f, ETaste::None,  TEXT("No seasoning. Always available.") },
		{ TEXT("Sea Salt"),      0, 0,  4, 0, 1.08f, 1.08f, ETaste::Salty, TEXT("Cheap and steady. Never spoils.") },
		{ TEXT("Soy Sauce"),     0, 0,  5, 0, 1.10f, 1.10f, ETaste::Rich,  TEXT("Cheap and steady. Never spoils.") },
		{ TEXT("Chili Oil"),     0, 0,  6, 0, 1.10f, 1.10f, ETaste::Spicy, TEXT("Cheap and steady. Never spoils.") },
		{ TEXT("Abyss Saffron"), 0, 2, 40, 0, 1.12f, 1.60f, ETaste::Rich,  TEXT("Rare. Wasted on ordinary fish - makes a PRIME catch a signature dish.") },
		{ TEXT("Fresh Yuzu"),    1, 1,  9, 2, 1.15f, 1.15f, ETaste::Fresh, TEXT("Fresh garnish. Keeps 2 nights.") },
		{ TEXT("Shiso Leaf"),    1, 1,  6, 3, 1.09f, 1.09f, ETaste::Fresh, TEXT("Fresh garnish. Keeps 3 nights.") },
		{ TEXT("Sea Urchin"),    1, 1, 30, 1, 1.32f, 1.32f, ETaste::Rich,  TEXT("Luxury garnish. Spoils after tonight!") },
	};

	const FSeasonDef& Season(int32 Id)
	{
		return GSeason[FMath::Clamp(Id, 0, NumSeason - 1)];
	}

	const TCHAR* TasteName(ETaste T)
	{
		switch (T)
		{
		case ETaste::Salty: return TEXT("Salty");
		case ETaste::Rich:  return TEXT("Rich");
		case ETaste::Spicy: return TEXT("Spicy");
		case ETaste::Fresh: return TEXT("Fresh");
		default:            return TEXT("Any");
		}
	}

	FLinearColor TasteColor(ETaste T)
	{
		switch (T)
		{
		case ETaste::Salty: return FLinearColor(0.75f, 0.9f, 1.f);
		case ETaste::Rich:  return FLinearColor(1.f, 0.72f, 0.3f);
		case ETaste::Spicy: return FLinearColor(1.f, 0.4f, 0.3f);
		case ETaste::Fresh: return FLinearColor(0.55f, 1.f, 0.5f);
		default:            return FLinearColor(0.8f, 0.8f, 0.8f);
		}
	}

	const TCHAR* PrimeTip(EFishType Type)
	{
		switch (Type)
		{
		case EFishType::Yellowtail: return TEXT("PRIME: net 3 at once while the school is calm");
		case EFishType::Tuna:       return TEXT("PRIME: claw it while calm (intercept, don't chase)");
		default:                    return TEXT("PRIME: catch it before it gets nervous (lights off / bait)");
		}
	}

	bool CanCatch(EFishType Type, ECatchMethod Method)
	{
		const FFishDef& D = Fish(Type);
		switch (Method)
		{
		case ECatchMethod::SubClaw: return D.bSubClaw;
		case ECatchMethod::Harpoon: return D.bHarpoon;
		case ECatchMethod::Net:     return D.bNet;
		}
		return false;
	}

	const TCHAR* MethodName(ECatchMethod Method)
	{
		switch (Method)
		{
		case ECatchMethod::SubClaw: return TEXT("Sub Claw");
		case ECatchMethod::Harpoon: return TEXT("Speargun");
		case ECatchMethod::Net:     return TEXT("Net");
		}
		return TEXT("");
	}
}
