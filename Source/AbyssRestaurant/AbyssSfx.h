#pragma once

#include "CoreMinimal.h"

// One-shot UI / gameplay sounds: /Game/Abyss/Sfx/S_<Name> (synthesised by Scripts/make_sfx.py).
// Names: Bell Chop Sizzle Coin Pop Plop Click Chime Sparkle Whoosh Thunk Bubble Error Flip
namespace AbyssSfx
{
	ABYSSRESTAURANT_API void Play(const UObject* WorldContext, const TCHAR* Name, float Volume = 1.f, float Pitch = 1.f);
}
