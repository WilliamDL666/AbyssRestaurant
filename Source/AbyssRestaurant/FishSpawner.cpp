#include "FishSpawner.h"
#include "CatchableFish.h"
#include "Engine/World.h"

AFishSpawner::AFishSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AFishSpawner::BeginPlay()
{
	Super::BeginPlay();
	FRandomStream R(Seed);
	UWorld* W = GetWorld();
	const FVector C = GetActorLocation();

	auto SpawnAt = [&](EFishType Type, const FVector& P, float Wander)
	{
		const FTransform T(FRotator(0.f, R.FRandRange(0.f, 360.f), 0.f), P);
		ACatchableFish* F = W->SpawnActorDeferred<ACatchableFish>(ACatchableFish::StaticClass(), T, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (F)
		{
			F->Type = Type;
			F->WanderRadius = Wander;
			F->FinishSpawning(T);
		}
	};

	auto SpawnOne = [&](EFishType Type, float MinDist, float MaxDist, float MinX, int32 Count)
	{
		for (int32 Try = 0; Try < 20; ++Try)
		{
			const float A = R.FRandRange(0.f, 2.f * PI);
			const float D = R.FRandRange(MinDist, MaxDist);
			FVector P = C + FVector(FMath::Cos(A) * D, FMath::Sin(A) * D, 0.f);
			if (P.X < MinX) continue;

			FHitResult Hit;
			if (!W->LineTraceSingleByChannel(Hit, FVector(P.X, P.Y, -200.f), FVector(P.X, P.Y, -30000.f), ECC_WorldStatic))
			{
				continue;
			}
			P.Z = Hit.ImpactPoint.Z + R.FRandRange(250.f, 650.f);
			for (int32 i = 0; i < Count; ++i)
			{
				const FVector Off = Count > 1 ? FVector(R.FRandRange(-250.f, 250.f), R.FRandRange(-250.f, 250.f), R.FRandRange(-80.f, 80.f)) : FVector::ZeroVector;
				SpawnAt(Type, P + Off, Count > 1 ? 450.f : (Type == EFishType::Grouper ? 1200.f : 2000.f));
			}
			return;
		}
	};

	for (int32 i = 0; i < NumYellowtailSchools; ++i) SpawnOne(EFishType::Yellowtail, 2500.f, Radius * 0.6f, -1e9f, YellowtailPerSchool);
	for (int32 i = 0; i < NumTuna; ++i)    SpawnOne(EFishType::Tuna, 6000.f, Radius, -1e9f, 1);
	for (int32 i = 0; i < NumGrouper; ++i) SpawnOne(EFishType::Grouper, 12000.f, Radius * 1.4f, 13000.f, 1);
}
