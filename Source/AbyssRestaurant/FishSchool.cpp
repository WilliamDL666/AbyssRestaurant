#include "FishSchool.h"
#include "AbyssAssets.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

AFishSchool::AFishSchool()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	auto MakeISM = [this, Root](const TCHAR* Name)
	{
		UInstancedStaticMeshComponent* C = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		C->SetupAttachment(Root);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		C->SetCastShadow(true);
		C->SetCullDistances(0, 16000);
		return C;
	};
	Bodies = MakeISM(TEXT("Bodies"));
	Backs = MakeISM(TEXT("Backs"));
	Tails = MakeISM(TEXT("Tails"));
}

void AFishSchool::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	using namespace AbyssAssets;
	Bodies->SetStaticMesh(Sphere());
	Backs->SetStaticMesh(Sphere());
	Tails->SetStaticMesh(Cone());
	Bodies->SetMaterial(0, Lit(this, BodyColor, 0.3f, 0.45f));
	Backs->SetMaterial(0, Lit(this, BackColor, 0.3f, 0.5f));
	Tails->SetMaterial(0, Lit(this, BodyColor * 0.9f, 0.4f, 0.2f));

	FRandomStream R(GetTypeHash(GetName()) ^ NumFish);
	Members.SetNum(FMath::Max(NumFish, 1));
	for (FMember& M : Members)
	{
		M.Offset = FMath::VRand() * FMath::Pow(R.FRand(), 0.5f) * SchoolRadius;
		M.Offset.Z *= 0.45f;
		M.Phase = R.FRandRange(0.f, 10.f);
		M.Push = FVector::ZeroVector;
	}

	Bodies->ClearInstances();
	Backs->ClearInstances();
	Tails->ClearInstances();
	BodyT.SetNum(Members.Num());
	BackT.SetNum(Members.Num());
	TailT.SetNum(Members.Num());
	Layout(0.f, FVector(1e9f), 0.f);
	Bodies->AddInstances(BodyT, false, true);
	Backs->AddInstances(BackT, false, true);
	Tails->AddInstances(TailT, false, true);
}

FVector AFishSchool::CenterAt(float T) const
{
	const float W = Speed;
	return GetActorLocation() + FVector(
		FMath::Sin(T * W) * WanderRange,
		FMath::Sin(T * W * 0.73f + 1.3f) * WanderRange * 0.8f,
		FMath::Sin(T * W * 1.31f + 0.4f) * 180.f);
}

void AFishSchool::Layout(float T, const FVector& Avoid, float Dt)
{
	const FVector C0 = CenterAt(T);
	const FVector Vel = (CenterAt(T + 0.1f) - C0).GetSafeNormal();
	const FRotator Heading = Vel.Rotation();
	const FQuat HQ = FQuat(FRotator(0.f, Heading.Yaw, 0.f));

	for (int32 i = 0; i < Members.Num(); ++i)
	{
		FMember& M = Members[i];
		const float Ph = M.Phase;
		FVector P = C0 + HQ.RotateVector(M.Offset)
			+ FVector(FMath::Sin(T * 1.3f + Ph) * 45.f, FMath::Cos(T * 1.1f + Ph) * 45.f, FMath::Sin(T * 0.9f + Ph) * 25.f);

		// Scatter away from the submarine
		const FVector Away = P - Avoid;
		const float D = Away.Size();
		FVector WantPush = FVector::ZeroVector;
		if (D < ScatterRadius)
		{
			WantPush = Away.GetSafeNormal() * (ScatterRadius - D) * 1.4f;
		}
		M.Push = Dt > 0.f ? FMath::VInterpTo(M.Push, WantPush, Dt, 3.f) : WantPush;
		P += M.Push;

		const float Wag = FMath::Sin(T * 14.f + Ph * 3.f);
		const FVector Dir = (Vel * 300.f + M.Push * 0.8f).GetSafeNormal();
		FRotator Rot = Dir.Rotation();
		Rot.Yaw += Wag * 6.f;
		Rot.Pitch = FMath::Clamp(Rot.Pitch, -30.f, 30.f);
		const FQuat Q = Rot.Quaternion();
		const float S = FishScale * (0.85f + 0.3f * FMath::Frac(Ph * 7.31f));

		BodyT[i] = FTransform(Q, P, FVector(0.9f, 0.22f, 0.3f) * S);
		BackT[i] = FTransform(Q, P + Q.RotateVector(FVector(0.f, 0.f, 4.f) * S), FVector(0.75f, 0.17f, 0.2f) * S);
		const FQuat TailQ = Q * FRotator(-90.f, Wag * 25.f, 0.f).Quaternion();
		TailT[i] = FTransform(TailQ, P - Q.RotateVector(FVector(52.f, 0.f, 0.f) * S), FVector(0.25f, 0.04f, 0.3f) * S);
	}
}

void AFishSchool::Tick(float Dt)
{
	Super::Tick(Dt);
	Time += Dt;
	FVector Avoid(1e9f);
	if (APawn* Sub = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Avoid = Sub->GetActorLocation();
	}
	if ((CenterAt(Time) - Avoid).Size() > 25000.f)
	{
		return; // too far to matter
	}
	Layout(Time, Avoid, Dt);
	Bodies->BatchUpdateInstancesTransforms(0, BodyT, true, true, true);
	Backs->BatchUpdateInstancesTransforms(0, BackT, true, true, true);
	Tails->BatchUpdateInstancesTransforms(0, TailT, true, true, true);
}
