#include "AbyssSfx.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

namespace AbyssSfx
{
	void Play(const UObject* Ctx, const TCHAR* Name, float Volume, float Pitch)
	{
		if (!Ctx || !Ctx->GetWorld()) return;
		// kept alive by the map: looked up by name each time, null while the asset is missing
		static TMap<FName, TWeakObjectPtr<USoundBase>> Cache;
		const FName Key(Name);
		USoundBase* S = Cache.FindRef(Key).Get();
		if (!S)
		{
			S = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Abyss/Sfx/S_%s.S_%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
			if (!S) return;
			S->AddToRoot();
			Cache.Add(Key, S);
		}
		UGameplayStatics::PlaySound2D(Ctx, S, Volume, Pitch);
	}
}
