#include "CatchableFish.h"
#include "AbyssAssets.h"
#include "AbyssGameMode.h"
#include "SubmarinePawn.h"
#include "DiverPawn.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	const TCHAR* FishKey(EFishType T)
	{
		return T == EFishType::Yellowtail ? TEXT("Yellowtail") : T == EFishType::Tuna ? TEXT("Tuna") : TEXT("Grouper");
	}

	// Blender fish (Scripts/blender_make_fish.py): slots Back / Belly / Fin / Eye / Stripe get the species colors
	void PaintFish(AActor* Owner, UStaticMeshComponent* C, const FFishDef& D)
	{
		using namespace AbyssAssets;
		const UStaticMesh* M = C->GetStaticMesh();
		if (!M) return;
		const TArray<FStaticMaterial>& Slots = M->GetStaticMaterials();
		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			const FString N = Slots[i].MaterialSlotName.ToString();
			UMaterialInterface* Mat;
			if (N.Contains(TEXT("Belly")))       Mat = Lit(Owner, D.Body * 0.25f + FLinearColor(0.72f, 0.74f, 0.76f), 0.3f, 0.35f, D.Glow * 0.4f);
			else if (N.Contains(TEXT("Fin")))    Mat = Lit(Owner, D.Fin, 0.45f, 0.1f, D.Glow * 1.5f);
			else if (N.Contains(TEXT("Eye")))    Mat = Lit(Owner, FLinearColor(0.01f, 0.01f, 0.012f), 0.08f, 0.2f);
			else if (N.Contains(TEXT("Stripe"))) Mat = Lit(Owner, D.Fin * 0.6f + D.Body * 0.6f, 0.35f, 0.3f, D.Glow * 2.2f);
			else                                 Mat = Lit(Owner, D.Body, 0.32f, 0.35f, D.Glow);
			C->SetMaterial(i, Mat);
		}
	}
}

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
	BuildVisual();
}

void ACatchableFish::BuildVisual()
{
	using namespace AbyssAssets;
	const FFishDef& D = Def();
	Visual->SetRelativeScale3D(FVector(D.Scale));

	UStaticMesh* BodyMesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Abyss/Fish/SM_Fish_%s.SM_Fish_%s"), FishKey(Type), FishKey(Type)), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UStaticMesh* TailMesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Abyss/Fish/SM_FishTail_%s.SM_FishTail_%s"), FishKey(Type), FishKey(Type)), nullptr, LOAD_NoWarn | LOAD_Quiet);
	bModel = BodyMesh && TailMesh;
	if (bModel)
	{
		// the model is ~110 cm long, head on +X; the tail piece was exported with its origin on the tail joint
		const FBoxSphereBounds B = BodyMesh->GetBounds();
		TailPivot->SetRelativeLocation(FVector(B.Origin.X - B.BoxExtent.X + 3.f, 0.f, 0.f));
		PaintFish(this, AddPart(this, Visual, BodyMesh, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.f), nullptr), D);
		PaintFish(this, AddPart(this, TailPivot, TailMesh, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.f), nullptr), D);
	}
	else
	{
		TailPivot->SetRelativeLocation(FVector(-48.f, 0.f, 0.f));
		UMaterialInterface* BodyM = Lit(this, D.Body, 0.35f, 0.25f, D.Glow);
		UMaterialInterface* FinM  = Lit(this, D.Fin, 0.5f, 0.f, D.Glow * 1.5f);
		UMaterialInterface* EyeM  = Lit(this, FLinearColor(0.01f, 0.01f, 0.01f), 0.1f);
		UMaterialInterface* BellyM = Lit(this, D.Body * 0.4f + FLinearColor(0.5f, 0.5f, 0.5f), 0.35f, 0.3f, D.Glow * 0.5f);
		AddPart(this, Visual, Sphere(), FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.1f, 0.32f, 0.45f), BodyM);
		AddPart(this, Visual, Sphere(), FVector(4.f, 0.f, -6.f), FRotator::ZeroRotator, FVector(0.9f, 0.27f, 0.3f), BellyM);
		AddPart(this, Visual, Cone(), FVector(-6.f, 0.f, 24.f), FRotator(35.f, 0.f, 0.f), FVector(0.3f, 0.03f, 0.3f), FinM);
		AddPart(this, Visual, Cone(), FVector(8.f, 15.f, -8.f), FRotator(0.f, 0.f, -70.f), FVector(0.15f, 0.02f, 0.22f), FinM);
		AddPart(this, Visual, Cone(), FVector(8.f, -15.f, -8.f), FRotator(0.f, 0.f, 70.f), FVector(0.15f, 0.02f, 0.22f), FinM);
		AddPart(this, Visual, Sphere(), FVector(36.f, 11.f, 5.f), FRotator::ZeroRotator, FVector(0.07f), EyeM);
		AddPart(this, Visual, Sphere(), FVector(36.f, -11.f, 5.f), FRotator::ZeroRotator, FVector(0.07f), EyeM);
		AddPart(this, TailPivot, Cone(), FVector(-12.f, 0.f, 9.f), FRotator(60.f, 0.f, 0.f), FVector(0.22f, 0.03f, 0.35f), FinM);
		AddPart(this, TailPivot, Cone(), FVector(-12.f, 0.f, -9.f), FRotator(120.f, 0.f, 0.f), FVector(0.22f, 0.03f, 0.35f), FinM);
	}

	if (D.Glow > 0.f)
	{
		// on the root, so the den keeps a faint glow while the fish itself is hidden inside
		Glow = AddPointLight(this, GetRootComponent(), FVector::ZeroVector, D.Body, 60.f, 700.f);
	}
}

void ACatchableFish::BeginPlay()
{
	Super::BeginPlay();
	Home = GetActorLocation();
	Phase = FMath::FRand() * 10.f;
	FloorZ = TraceFloor(Home);
	Anchor = AnchorGoal = Home;

	if (Type == EFishType::Tuna)
	{
		RouteA = FMath::FRandRange(2200.f, 3200.f);
		RouteB = FMath::FRandRange(1100.f, 1700.f);
		RouteYaw = FMath::FRandRange(0.f, 2.f * PI);
		RouteT = FMath::FRandRange(0.f, 2.f * PI);
		RouteDir = FMath::RandBool() ? 1.f : -1.f;
		SetActorLocation(RoutePoint(RouteT));
	}
	else if (Type == EFishType::Grouper)
	{
		// its den sits on the seabed under the spawn point
		Den = FVector(Home.X, Home.Y, FloorZ + 150.f);
		Home = Den;
		WanderRadius = 520.f;
		SetActorLocation(Den + FVector(120.f, 0.f, 60.f));
		if (UStaticMesh* DenMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Abyss/Fish/SM_GrouperDen.SM_GrouperDen"), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			FActorSpawnParameters SP;
			SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(Den.X, Den.Y, FloorZ - 20.f), FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), SP))
			{
				UStaticMeshComponent* C = A->GetStaticMeshComponent();
				C->SetMobility(EComponentMobility::Movable);
				C->SetStaticMesh(DenMesh);
				C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				const TArray<FStaticMaterial>& Slots = DenMesh->GetStaticMaterials();
				for (int32 i = 0; i < Slots.Num(); ++i)
				{
					const FString N = Slots[i].MaterialSlotName.ToString();
					UMaterialInterface* Mat = nullptr;
					if (N.Contains(TEXT("Glow")))       Mat = AbyssAssets::Lit(A, FLinearColor(0.15f, 0.9f, 0.8f), 0.4f, 0.f, 3.f);
					else if (N.Contains(TEXT("Coral"))) Mat = AbyssAssets::Lit(A, FLinearColor(0.75f, 0.2f, 0.5f), 0.6f, 0.f, 0.25f);
					else                                Mat = AbyssAssets::Lit(A, FLinearColor(0.07f, 0.09f, 0.12f), 0.85f);
					C->SetMaterial(i, Mat);
				}
			}
		}
	}
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
	if (Type == EFishType::Grouper)
	{
		Goal.Z = Den.Z + FMath::FRandRange(20.f, 260.f);
		return;
	}
	const float Floor = TraceFloor(Goal);
	Goal.Z = Floor + FMath::FRandRange(150.f, 700.f);
}

FVector ACatchableFish::RoutePoint(float T) const
{
	const float X = FMath::Cos(T) * RouteA, Y = FMath::Sin(T) * RouteB;
	const float C = FMath::Cos(RouteYaw), Sn = FMath::Sin(RouteYaw);
	return Home + FVector(X * C - Y * Sn, X * Sn + Y * C, FMath::Sin(T * 2.f) * 180.f);
}

FVector ACatchableFish::PredictRoute(float Seconds) const
{
	if (Type != EFishType::Tuna || !IsCalm()) return GetActorLocation() + Velocity * Seconds;
	const float R = 0.5f * (RouteA + RouteB);
	return RoutePoint(RouteT + RouteDir * Def().Speed * Seconds / R);
}

bool ACatchableFish::PrimeIfCaught(ECatchMethod Method, int32 Haul) const
{
	if (Type == EFishType::Yellowtail)
	{
		// a clean haul: several fish in one throw at a school that was not already fleeing. A spear tears the small fish up.
		return Method == ECatchMethod::Net && Haul >= 3 && Alarm < 0.7f;
	}
	// tuna: intercepted, not chased down. grouper: taken before it got nervous.
	// A spear only counts when the first hit was on a calm fish.
	if (Method == ECatchMethod::SubClaw) return IsCalm();
	return Method == ECatchMethod::Harpoon && bCalmAtFirstHit;
}

void ACatchableFish::Spook(const FVector& From, bool bWholeSchool)
{
	if (bCaught) return;
	Alarm = 1.f;
	FVector Away = (GetActorLocation() - From).GetSafeNormal();
	if (Away.IsNearlyZero()) Away = FMath::VRand();
	// each fish breaks away at its own angle so a school bursts apart
	BoltDir = (Away + FMath::VRand() * 0.55f).GetSafeNormal();
	BoltDir.Z *= 0.4f;
	BoltTime = Type == EFishType::Tuna ? 2.6f : 1.8f;
	StunTime = 0.f;
	if (Type == EFishType::Grouper) bRetreating = true;
	if (bWholeSchool && SchoolId >= 0)
	{
		for (TActorIterator<ACatchableFish> It(GetWorld()); It; ++It)
		{
			if (*It != this && It->SchoolId == SchoolId) It->Spook(From, false);
		}
	}
}

bool ACatchableFish::OnHarpoonHit()
{
	if (bCaught) return false;
	if (HarpoonHitsTaken == 0) bCalmAtFirstHit = IsCalm();
	++HarpoonHitsTaken;
	if (HarpoonHitsTaken >= FMath::Max(Def().HarpoonHits, 1))
	{
		return true;
	}
	Alarm = 1.f;
	StunTime = 1.6f;
	return false;
}

void ACatchableFish::Escape(const FVector& From)
{
	Spook(From, true);
	Goal = GetActorLocation() + BoltDir * 2500.f;
}

void ACatchableFish::SetHiding(bool bHide)
{
	if (bHidden == bHide) return;
	bHidden = bHide;
	Visual->SetVisibility(!bHide, true);
	bBeingCaught = false;
}

void ACatchableFish::OnCaught()
{
	HarpoonHitsTaken = 0;
	StunTime = 0.f;
	BoltTime = 0.f;
	bCaught = true;
	bBeingCaught = false;
	bRetreating = bFeeding = false;
	RespawnTimer = RespawnSeconds;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

// How much the player disturbs this fish this frame
void ACatchableFish::TickSenses(float Dt, const FVector& P)
{
	APawn* Pl = UGameplayStatics::GetPlayerPawn(this, 0);
	float Gain = 0.f;
	if (Pl)
	{
		const FVector To = P - Pl->GetActorLocation();
		const float Dist = To.Size();
		const float Speed = Pl->GetVelocity().Size();
		const ASubmarinePawn* Sub = Cast<ASubmarinePawn>(Pl);
		bool bLit = false;
		if (Sub && Sub->bLightOn && Dist < 3200.f && Dist > 1.f)
		{
			bLit = FVector::DotProduct(Sub->GetLightDirection(), To / Dist) > 0.9f;
		}
		switch (Type)
		{
		case EFishType::Yellowtail:
			// a diver drifting in slowly is tolerated; anything fast or right on top of them is not
			if (Dist < 1100.f)
			{
				const float Near = 1.f - Dist / 1100.f;
				Gain += (Sub ? (Speed > 120.f ? 2.2f : 0.5f) : (Speed > 300.f ? 1.9f : Speed > 190.f ? 0.4f : 0.f)) * Near;
			}
			if (Dist < 150.f) Gain += 1.4f;
			break;
		case EFishType::Tuna:
			// ignores a parked sub; bolts from one bearing down on it
			if (Dist < 1700.f && Speed > 420.f)
			{
				const float Closing = FVector::DotProduct(Pl->GetVelocity().GetSafeNormal(), To.GetSafeNormal());
				if (Closing > 0.3f) Gain += 1.5f * (1.f - Dist / 1700.f) * Closing;
			}
			if (Dist < 380.f) Gain += 1.6f;
			if (bLit && Dist < 900.f) Gain += 0.25f;
			break;
		default:
			if (bLit) Gain += 0.6f * (1.f - Dist / 3200.f) + 0.25f;
			if (Dist < 2400.f && Speed > 280.f) Gain += 0.75f * (1.f - Dist / 2400.f);
			if (Dist < 320.f) Gain += 0.5f;
			if (bFeeding) Gain *= 0.3f;
			break;
		}
	}
	if (Gain > 0.f)
	{
		const bool bWasBelow = Alarm < 1.f;
		Alarm = FMath::Min(1.f, Alarm + Gain * Dt);
		if (bWasBelow && Alarm >= 1.f && Pl) Spook(Pl->GetActorLocation(), true);
	}
	else
	{
		Alarm = FMath::Max(0.f, Alarm - Dt * (Type == EFishType::Tuna ? 0.2f : Type == EFishType::Grouper ? 0.14f : 0.16f));
	}
}

void ACatchableFish::Tick(float Dt)
{
	Super::Tick(Dt);
	Phase += Dt;

	// the school's gathering point keeps drifting even while the lead fish is in somebody's net
	if (SchoolId >= 0 && !Leader.IsValid())
	{
		if (FVector::Dist(Anchor, AnchorGoal) < 150.f)
		{
			const FVector2D R = FMath::RandPointInCircle(WanderRadius);
			AnchorGoal = Home + FVector(R.X, R.Y, 0.f);
			AnchorGoal.Z = TraceFloor(AnchorGoal) + FMath::FRandRange(250.f, 600.f);
		}
		Anchor += (AnchorGoal - Anchor).GetSafeNormal() * 110.f * Dt;
	}

	if (bCaught)
	{
		RespawnTimer -= Dt;
		if (RespawnTimer <= 0.f)
		{
			bCaught = false;
			Alarm = 0.f;
			SetActorLocation(Type == EFishType::Tuna ? RoutePoint(RouteT) : Home);
			Velocity = FVector::ZeroVector;
			SetActorHiddenInGame(false);
			SetHiding(false);
			PickGoal();
		}
		return;
	}

	const FFishDef& D = Def();
	const FVector P = GetActorLocation();
	TickSenses(Dt, P);

	float Speed = D.Speed;
	FVector Desired = FVector::ZeroVector;
	float Turn = 1.8f;
	APawn* Pl = UGameplayStatics::GetPlayerPawn(this, 0);

	if (Type == EFishType::Yellowtail && SchoolId >= 0)
	{
		const ACatchableFish* L = Leader.IsValid() ? Leader.Get() : this;
		// circle the gathering point in formation
		const float A = Phase * 0.35f;
		const FVector Off(SchoolOffset.X * FMath::Cos(A) - SchoolOffset.Y * FMath::Sin(A), SchoolOffset.X * FMath::Sin(A) + SchoolOffset.Y * FMath::Cos(A), SchoolOffset.Z);
		Goal = L->GetSchoolAnchor() + Off;
		const FVector To = Goal - P;
		Speed = D.Speed * FMath::Clamp(To.Size() / 260.f, 0.25f, IsCalm() ? 1.f : 1.5f);
		Desired = To.GetSafeNormal() * Speed;
		Turn = 2.6f;
		if (!IsCalm() && Pl)
		{
			// keep away from whatever startled them until they settle
			const FVector Away = P - Pl->GetActorLocation();
			if (Away.Size() < 1300.f) Desired = Desired * 0.3f + Away.GetSafeNormal() * D.Speed * 1.6f * Alarm;
		}
	}
	else if (Type == EFishType::Tuna)
	{
		const float R = 0.5f * (RouteA + RouteB);
		Turn = 2.4f;
		if (IsCalm())
		{
			// stay on the loop: the target runs ahead of the fish along the route
			RouteT += RouteDir * D.Speed / R * Dt;
			const FVector To = RoutePoint(RouteT) - P;
			Speed = D.Speed * FMath::Clamp(To.Size() / 500.f, 0.6f, 1.25f);
			Desired = To.GetSafeNormal() * Speed;
		}
		else
		{
			FVector Away = BoltDir;
			if (Pl) Away = (P - Pl->GetActorLocation()).GetSafeNormal();
			Away.Z *= 0.3f;
			Desired = Away.GetSafeNormal() * D.Speed * 1.75f;
			// rejoin the loop at the nearest point once it has calmed down
			float BestD = 1e12f;
			for (int32 k = 0; k < 16; ++k)
			{
				const float T = k * PI / 8.f;
				const float Dd = FVector::DistSquared(RoutePoint(T), P);
				if (Dd < BestD) { BestD = Dd; RouteT = T; }
			}
		}
	}
	else if (Type == EFishType::Grouper)
	{
		AAbyssGameMode* GM = AAbyssGameMode::Get(this);
		FVector Bait;
		const bool bBait = GM && GM->GetBait(Bait) && FVector::Dist(Bait, Den) < 3600.f;
		bFeeding = false;
		if (bHidden)
		{
			HideTime -= Dt;
			// peeks out again when things are quiet, or sooner when it smells bait
			if (HideTime <= 0.f && (Alarm < 0.35f || bBait))
			{
				SetHiding(false);
				bRetreating = false;
				Alarm = FMath::Min(Alarm, 0.3f);
				PickGoal();
			}
			if (Glow) Glow->SetIntensity(10.f + 6.f * FMath::Sin(Phase * 2.f));
			return;
		}
		if (bRetreating)
		{
			const FVector To = Den - P;
			Desired = To.GetSafeNormal() * D.Speed * 1.9f;
			Turn = 4.f;
			if (To.Size() < 110.f)
			{
				SetHiding(true);
				HideTime = 7.f;
				Velocity = FVector::ZeroVector;
				return;
			}
		}
		else if (bBait)
		{
			const FVector To = Bait - P;
			bFeeding = To.Size() < 260.f;
			Desired = bFeeding ? FVector(FMath::Sin(Phase * 1.3f), FMath::Cos(Phase * 1.1f), 0.f) * 40.f : To.GetSafeNormal() * D.Speed * 0.9f;
		}
		else
		{
			if (FVector::Dist(Goal, P) < 120.f) PickGoal();
			Desired = (Goal - P).GetSafeNormal() * D.Speed * 0.4f;
		}
	}
	else
	{
		Desired = Goal - P;
		if (Desired.Size() < 200.f) PickGoal();
		Desired = Desired.GetSafeNormal() * Speed;
		if (Pl)
		{
			const FVector Away = P - Pl->GetActorLocation();
			const float Dist = Away.Size();
			if (Dist < 900.f) Desired += Away.GetSafeNormal() * D.Speed * 2.2f * (1.f - Dist / 900.f);
		}
	}

	if (BoltTime > 0.f && !bRetreating)
	{
		BoltTime -= Dt;
		Desired = BoltDir * D.Speed * (Type == EFishType::Tuna ? 1.9f : 3.f);
	}
	if (StunTime > 0.f)
	{
		StunTime -= Dt;
		Desired = Desired * 0.1f + FVector(FMath::Sin(Phase * 31.f), FMath::Cos(Phase * 27.f), 0.f) * 120.f;
		if (StunTime <= 0.f) HarpoonHitsTaken = 0; // recovered
	}
	if (bBeingCaught)
	{
		// Struggle: slowed + jittering. A calm fish barely notices the claw closing in.
		const float Fight = IsCalm() ? 0.35f : 1.f;
		Desired = Desired * (IsCalm() ? 0.12f : 0.3f) + FVector(FMath::Sin(Phase * 23.f), FMath::Cos(Phase * 19.f), FMath::Sin(Phase * 17.f)) * 220.f * Fight;
	}

	Velocity = FMath::VInterpTo(Velocity, Desired, Dt, Turn);
	FVector NewP = P + Velocity * Dt;
	NewP.Z = FMath::Clamp(NewP.Z, FloorZ + (Type == EFishType::Grouper ? 60.f : 120.f), -250.f);
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

	const float Wag = FMath::Sin(Phase * (bBeingCaught ? 28.f : 7.f + Velocity.Size() * 0.014f));
	TailPivot->SetRelativeRotation(FRotator(0.f, Wag * (bModel ? 22.f : 28.f), 0.f));
	Visual->SetRelativeRotation(FRotator(0.f, -Wag * 5.f, Wag * 2.f));

	if (Glow)
	{
		Glow->SetIntensity(45.f + 25.f * FMath::Sin(Phase * 3.f));
	}
}

// ---------------------------------------------------------------- bait

AAbyssBait::AAbyssBait()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AAbyssBait::BeginPlay()
{
	Super::BeginPlay();
	using namespace AbyssAssets;
	UMaterialInterface* Chum = Lit(this, FLinearColor(1.f, 0.42f, 0.22f), 0.5f, 0.f, 4.f);
	UMaterialInterface* Bits = Lit(this, FLinearColor(1.f, 0.8f, 0.5f), 0.5f, 0.f, 2.f);
	AddPart(this, GetRootComponent(), Sphere(), FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.28f, 0.24f, 0.2f), Chum, false, false);
	for (int32 i = 0; i < 7; ++i)
	{
		AddPart(this, GetRootComponent(), Sphere(), FMath::VRand() * FMath::FRandRange(18.f, 42.f), FRotator::ZeroRotator, FVector(FMath::FRandRange(0.05f, 0.1f)), Bits, false, false);
	}
	Light = AddPointLight(this, GetRootComponent(), FVector::ZeroVector, FLinearColor(1.f, 0.5f, 0.25f), 40.f, 600.f, false, false);
	FHitResult Hit;
	const FVector At = GetActorLocation();
	FloorZ = GetWorld()->LineTraceSingleByChannel(Hit, At, At - FVector(0, 0, 20000.f), ECC_WorldStatic) ? Hit.ImpactPoint.Z : At.Z - 800.f;
}

void AAbyssBait::Tick(float Dt)
{
	Super::Tick(Dt);
	Time += Dt;
	Life -= Dt;
	FVector P = GetActorLocation();
	P.Z = FMath::Max(P.Z - 140.f * Dt, FloorZ + 90.f);
	SetActorLocation(P);
	AddActorLocalRotation(FRotator(0.f, 40.f * Dt, 0.f));
	if (Light) Light->SetIntensity((30.f + 14.f * FMath::Sin(Time * 5.f)) * FMath::Clamp(Life / 4.f, 0.f, 1.f));
	if (Life <= 0.f) Destroy();
}
