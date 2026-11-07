#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FishSpawner.generated.h"

// Spawns catchable fish at play start. Common fish on the reef shelf,
// tuna further out, rare groupers near the trench edge.
UCLASS()
class ABYSSRESTAURANT_API AFishSpawner : public AActor
{
	GENERATED_BODY()

public:
	AFishSpawner();
	virtual void BeginPlay() override;

	// Yellowtail spawn in tight schools so the diver net can catch several at once
	UPROPERTY(EditAnywhere, Category = "Spawn") int32 NumYellowtailSchools = 6;
	UPROPERTY(EditAnywhere, Category = "Spawn") int32 YellowtailPerSchool = 6;
	UPROPERTY(EditAnywhere, Category = "Spawn") int32 NumTuna = 12;
	UPROPERTY(EditAnywhere, Category = "Spawn") int32 NumGrouper = 4;
	UPROPERTY(EditAnywhere, Category = "Spawn") float Radius = 15000.f;
	UPROPERTY(EditAnywhere, Category = "Spawn") int32 Seed = 3;
};
