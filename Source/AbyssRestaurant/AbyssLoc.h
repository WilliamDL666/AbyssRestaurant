#pragma once

#include "CoreMinimal.h"

// Tiny runtime localization: all UI text is authored in English and translated at draw time.
// Whole-string matches first, then phrase replacement (longest first) so dynamic text
// like "Caught: Bluefin Tuna (+1 Tuna Sashimi Set)" also comes out in Chinese.
// Toggle in game with [Tab].
namespace AbyssLoc
{
	ABYSSRESTAURANT_API bool IsChinese();
	ABYSSRESTAURANT_API void SetChinese(bool bChinese);
	ABYSSRESTAURANT_API void Toggle();
	ABYSSRESTAURANT_API const FString& Translate(const FString& English);
}

// Pick a literal directly when a string is too short for safe phrase replacement
#define LOCS(En, Zh) (AbyssLoc::IsChinese() ? TEXT(Zh) : TEXT(En))
