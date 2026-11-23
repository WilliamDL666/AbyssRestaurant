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

	int32 NextSchool = 0;
	auto SpawnAt = [&](EFishType Type, const FVector& P, float Wander, int32 School, ACatchableFish* Lead, const FVector& Offset) -> ACatchableFish*
	{
		const FTransform T(FRotator(0.f, R.FRandRange(0.f, 360.f), 0.f), P);
		ACatchableFish* F = W->SpawnActorDeferred<ACatchableFish>(ACatchableFish::StaticClass(), T, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (F)
		{
			F->Type = Type;
			F->WanderRadius = Wander;
			F->SchoolId = School;
			F->Leader = Lead;
			F->SchoolOffset = Offset;
			F->FinishSpawning(T);
		}
		return F;
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
			if (Count > 1)
			{
				// a school: the first fish leads, the others hold a slot around the shared gathering point
				const int32 School = NextSchool++;
				ACatchableFish* Lead = nullptr;
				for (int32 i = 0; i < Count; ++i)
				{
					const float A2 = 2.f * PI * i / Count;
					const FVector Off(FMath::Cos(A2) * R.FRandRange(90.f, 190.f), FMath::Sin(A2) * R.FRandRange(90.f, 190.f), R.FRandRange(-70.f, 70.f));
					ACatchableFish* F = SpawnAt(Type, P + Off, 1300.f, School, Lead, Off);
					if (!Lead) Lead = F;
				}
			}
			else
			{
				SpawnAt(Type, P, Type == EFishType::Grouper ? 520.f : 2000.f, -1, nullptr, FVector::ZeroVector);
			}
			return;
		}
	};

	for (int32 i = 0; i < NumYellowtailSchools; ++i) SpawnOne(EFishType::Yellowtail, 2500.f, Radius * 0.6f, -1e9f, YellowtailPerSchool);
	for (int32 i = 0; i < NumTuna; ++i)    SpawnOne(EFishType::Tuna, 6000.f, Radius, -1e9f, 1);
	for (int32 i = 0; i < NumGrouper; ++i) SpawnOne(EFishType::Grouper, 12000.f, Radius * 1.4f, 13000.f, 1);
}
