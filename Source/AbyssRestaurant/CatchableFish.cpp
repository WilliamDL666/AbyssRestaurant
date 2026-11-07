#include "CatchableFish.h"
#include "AbyssAssets.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

ACatchableFish::ACatchableFish()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Root);
	TailPivot = CreateDefaultSubobject<USceneComponent>(TEXT("TailPivot"));
	TailPivot->SetupAttachment(Visual);
}

void ACatchableFish::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	using namespace AbyssAssets;
	const FFishDef& D = Def();

	Visual->SetRelativeScale3D(FVector(D.Scale));
	TailPivot->SetRelativeLocation(FVector(-48.f, 0.f, 0.f));

	UMaterialInterface* BodyM = Lit(this, D.Body, 0.35f, 0.25f, D.Glow);
	UMaterialInterface* FinM  = Lit(this, D.Fin, 0.5f, 0.f, D.Glow * 1.5f);
	UMaterialInterface* EyeM  = Lit(this, FLinearColor(0.01f, 0.01f, 0.01f), 0.1f);
	UMaterialInterface* BellyM = Lit(this, D.Body * 0.4f + FLinearColor(0.5f, 0.5f, 0.5f), 0.35f, 0.3f, D.Glow * 0.5f);

	// Body: elongated sphere, lighter belly, dorsal/pectoral fins, eyes, forked tail
	AddPart(this, Visual, Sphere(), FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.1f, 0.32f, 0.45f), BodyM);
	AddPart(this, Visual, Sphere(), FVector(4.f, 0.f, -6.f), FRotator::ZeroRotator, FVector(0.9f, 0.27f, 0.3f), BellyM);
	AddPart(this, Visual, Cone(), FVector(-6.f, 0.f, 24.f), FRotator(35.f, 0.f, 0.f), FVector(0.3f, 0.03f, 0.3f), FinM);
	AddPart(this, Visual, Cone(), FVector(8.f, 15.f, -8.f), FRotator(0.f, 0.f, -70.f), FVector(0.15f, 0.02f, 0.22f), FinM);
	AddPart(this, Visual, Cone(), FVector(8.f, -15.f, -8.f), FRotator(0.f, 0.f, 70.f), FVector(0.15f, 0.02f, 0.22f), FinM);
	AddPart(this, Visual, Sphere(), FVector(36.f, 11.f, 5.f), FRotator::ZeroRotator, FVector(0.07f), EyeM);
	AddPart(this, Visual, Sphere(), FVector(36.f, -11.f, 5.f), FRotator::ZeroRotator, FVector(0.07f), EyeM);
	AddPart(this, TailPivot, Cone(), FVector(-12.f, 0.f, 9.f), FRotator(60.f, 0.f, 0.f), FVector(0.22f, 0.03f, 0.35f), FinM);
	AddPart(this, TailPivot, Cone(), FVector(-12.f, 0.f, -9.f), FRotator(120.f, 0.f, 0.f), FVector(0.22f, 0.03f, 0.35f), FinM);

	if (D.Glow > 0.f)
	{
		Glow = AddPointLight(this, Visual, FVector::ZeroVector, D.Body, 60.f, 700.f);
	}
}

void ACatchableFish::BeginPlay()
{
	Super::BeginPlay();
	Home = GetActorLocation();
	Phase = FMath::FRand() * 10.f;
	FloorZ = TraceFloor(Home);
	PickGoal();
}

float ACatchableFish::TraceFloor(const FVector& At) const
{
	FHitResult Hit;
	FCollisionQueryParams P(SCENE_QUERY_STAT(FishFloor), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, At + FVector(0, 0, 3000.f), At - FVector(0, 0, 20000.f), ECC_WorldStatic, P))
	{
		return Hit.ImpactPoint.Z;
	}
	return At.Z - 1500.f;
}

void ACatchableFish::PickGoal()
{
	const FVector2D R = FMath::RandPointInCircle(WanderRadius);
	Goal = Home + FVector(R.X, R.Y, 0.f);
	const float Floor = TraceFloor(Goal);
	Goal.Z = Floor + FMath::FRandRange(150.f, 700.f);
}

bool ACatchableFish::OnHarpoonHit()
{
	if (bCaught) return false;
	++HarpoonHitsTaken;
	if (HarpoonHitsTaken >= FMath::Max(Def().HarpoonHits, 1))
	{
		return true;
	}
	StunTime = 1.6f;
	return false;
}

void ACatchableFish::Escape(const FVector& From)
{
	BoltDir = (GetActorLocation() - From).GetSafeNormal();
	BoltTime = 1.5f;
	StunTime = 0.f;
	Goal = GetActorLocation() + BoltDir * 2500.f;
}

void ACatchableFish::OnCaught()
{
	HarpoonHitsTaken = 0;
	StunTime = 0.f;
	BoltTime = 0.f;
	bCaught = true;
	bBeingCaught = false;
	RespawnTimer = RespawnSeconds;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ACatchableFish::Tick(float Dt)
{
	Super::Tick(Dt);
	Phase += Dt;

	if (bCaught)
	{
		RespawnTimer -= Dt;
		if (RespawnTimer <= 0.f)
		{
			bCaught = false;
			SetActorLocation(Home);
			Velocity = FVector::ZeroVector;
			SetActorHiddenInGame(false);
			PickGoal();
		}
		return;
	}

	const FFishDef& D = Def();
	const FVector P = GetActorLocation();
	FVector Desired = (Goal - P);
	if (Desired.Size() < 200.f)
	{
		PickGoal();
	}
	float Speed = D.Speed;
	Desired = Desired.GetSafeNormal() * Speed;

	// Flee from the submarine
	if (APawn* Sub = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const FVector Away = P - Sub->GetActorLocation();
		const float Dist = Away.Size();
		const float FleeRadius = (Type == EFishType::Grouper ? 1500.f : 900.f);
		if (Dist < FleeRadius)
		{
			const float K = 1.f - Dist / FleeRadius;
			Desired += Away.GetSafeNormal() * D.Speed * 2.2f * K;
		}
	}

	if (BoltTime > 0.f)
	{
		BoltTime -= Dt;
		Desired = BoltDir * D.Speed * 3.f;
	}
	if (StunTime > 0.f)
	{
		StunTime -= Dt;
		Desired = Desired * 0.1f + FVector(FMath::Sin(Phase * 31.f), FMath::Cos(Phase * 27.f), 0.f) * 120.f;
		if (StunTime <= 0.f) HarpoonHitsTaken = 0; // recovered
	}
	if (bBeingCaught)
	{
		// Struggle: slowed + jittering
		Desired = Desired * 0.25f + FVector(FMath::Sin(Phase * 23.f), FMath::Cos(Phase * 19.f), FMath::Sin(Phase * 17.f)) * 220.f;
	}

	Velocity = FMath::VInterpTo(Velocity, Desired, Dt, 1.8f);
	FVector NewP = P + Velocity * Dt;
	NewP.Z = FMath::Clamp(NewP.Z, FloorZ + 120.f, -250.f);
	if (FMath::Abs(NewP.Z - P.Z) > 1.f || FMath::Fmod(Phase, 1.f) < Dt)
	{
		FloorZ = FMath::Lerp(FloorZ, TraceFloor(NewP), 0.5f);
	}
	SetActorLocation(NewP);

	if (!Velocity.IsNearlyZero(5.f))
	{
		const FRotator Want = Velocity.Rotation();
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(FMath::Clamp(Want.Pitch, -35.f, 35.f), Want.Yaw, 0.f), Dt, 4.f));
	}

	const float Wag = FMath::Sin(Phase * (bBeingCaught ? 28.f : 9.f + Velocity.Size() * 0.01f));
	TailPivot->SetRelativeRotation(FRotator(0.f, Wag * 28.f, 0.f));
	Visual->SetRelativeRotation(FRotator(0.f, -Wag * 5.f, 0.f));

	if (Glow)
	{
		Glow->SetIntensity(45.f + 25.f * FMath::Sin(Phase * 3.f));
	}
}
