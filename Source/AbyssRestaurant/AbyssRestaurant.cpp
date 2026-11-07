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
		  TEXT("Small & fast - too small for the claw. Best: diver NET") },
		// Tuna: big, fast, strong -> SPEARGUN (2 hits), or a long sub claw hold. Breaks nets.
		{ TEXT("Bluefin Tuna"), TEXT("Tuna Teppanyaki"), 160, 2,
		  FLinearColor(0.10f, 0.16f, 0.30f), FLinearColor(0.55f, 0.58f, 0.62f), 1.8f, 380.f, 2.6f, 0.0f,
		  true, true, false, 2,
		  TEXT("Strong - breaks nets. Best: SPEARGUN (2 hits) or sub claw") },
		// Grouper: rare, lives below diver depth limit -> SUB CLAW.
		{ TEXT("Rare Grouper"), TEXT("Grouper Teppanyaki"), 520, 3,
		  FLinearColor(0.42f, 0.12f, 0.80f), FLinearColor(0.10f, 0.85f, 0.75f), 1.6f, 320.f, 2.0f, 1.2f,
		  true, true, false, 1,
		  TEXT("Lives deep near the trench. Best: SUB CLAW") },
	};

	const FFishDef& Fish(EFishType Type)
	{
		return GFish[FMath::Clamp((int32)Type, 0, NumFish - 1)];
	}

	FString DishName(int32 Dish)
	{
		return Dish >= 0 && Dish < NumFish ? FString(GFish[Dish].Dish) : FString(TEXT("Kelp Salad"));
	}

	int32 DishPrice(int32 Dish)
	{
		return Dish >= 0 && Dish < NumFish ? GFish[Dish].Price : SaladPrice;
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
