#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssTypes.h"
#include "AquariumTank.generated.h"

class UPointLightComponent;

// The display tank in the middle of the dining room. Live fish put on show swim around inside it.
// Uses /Game/Abyss/Aquarium/SM_AquariumTank when it exists (slots Base / Gold / Glass / Sand / Coral / Glow),
// otherwise a tank built from basic shapes.
UCLASS()
class ABYSSRESTAURANT_API AAquariumTank : public AActor
{
	GENERATED_BODY()

public:
	AAquariumTank();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetFish(const TArray<FTankFish>& Fish);

	// inside of the glass (cm): where the fish may swim
	UPROPERTY(EditAnywhere, Category = "Tank") float SwimRadius = 105.f;
	UPROPERTY(EditAnywhere, Category = "Tank") float WaterBottom = 75.f;
	UPROPERTY(EditAnywhere, Category = "Tank") float WaterTop = 275.f;
	// Height the Tripo tank model (/Game/Abyss/Aquarium/SM_TankModel) is scaled to
	UPROPERTY(EditAnywhere, Category = "Tank") float ModelHeight = 700.f;

private:
	struct FSwimmer
	{
		TWeakObjectPtr<USceneComponent> Root, Tail;
		float Radius = 80.f, Height = 150.f, Speed = 0.5f, Phase = 0.f, Dir = 1.f;
	};
	TArray<FSwimmer> Swimmers;
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> FishRoots;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> Light;
	float Time = 0.f;
};
