#include "AquariumTank.h"
#include "AbyssAssets.h"
#include "CatchableFish.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

AAquariumTank::AAquariumTank()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AAquariumTank::BeginPlay()
{
	Super::BeginPlay();
	using namespace AbyssAssets;
	USceneComponent* Root = GetRootComponent();
	UMaterialInterface* Navy = Lit(this, FLinearColor(0.03f, 0.07f, 0.22f), 0.3f, 0.2f);
	UMaterialInterface* Gold = Lit(this, FLinearColor(0.9f, 0.62f, 0.2f), 0.3f, 1.f);
	UMaterialInterface* Sand = Lit(this, FLinearColor(0.75f, 0.68f, 0.5f), 0.9f);
	UMaterialInterface* Coral = Lit(this, FLinearColor(0.85f, 0.3f, 0.45f), 0.6f, 0.f, 0.3f);
	UMaterialInterface* Glow = Lit(this, FLinearColor(0.3f, 0.9f, 1.f), 0.4f, 0.f, 2.5f);
	UMaterialInterface* GlassM = Glass(this, FLinearColor(0.55f, 0.8f, 0.9f), 0.05f);

	float BlockR = 170.f, BlockH = 300.f;
	if (UStaticMesh* Model = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Abyss/Aquarium/SM_TankModel.SM_TankModel"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		// The user's tank (Tripo). Its painted glass wall was cut away in Blender (Scripts/blender_cut_tank_glass.py):
		// base + corals + lid stay, and a real glass cylinder goes where the wall was. Proportions of the model's height:
		// glass radius 0.1829, glass from 0.212 to 0.756.
		const FBox B = Model->GetBoundingBox();
		const float Sc = ModelHeight / FMath::Max(B.GetSize().Z, 1.f);
		const FVector Cn(B.GetCenter().X * Sc, B.GetCenter().Y * Sc, B.Min.Z * Sc);
		AddPart(this, Root, Model, -Cn, FRotator::ZeroRotator, FVector(Sc), nullptr, false, false);
		const float GR = ModelHeight * 0.1829f, Lo = ModelHeight * 0.212f, Hi = ModelHeight * 0.756f;
		AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, (Lo + Hi) * 0.5f), FRotator::ZeroRotator, FVector(GR / 50.f, GR / 50.f, (Hi - Lo) / 100.f), GlassM, false, false);
		SwimRadius = GR - 34.f;
		WaterBottom = Lo + 50.f;
		WaterTop = Hi - 10.f;
		BlockR = ModelHeight * 0.24f;
		BlockH = ModelHeight;
	}
	else if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Abyss/Aquarium/SM_AquariumTank.SM_AquariumTank"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		UStaticMeshComponent* C = AddPart(this, Root, Mesh, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.f), nullptr, false, false);
		const TArray<FStaticMaterial>& Slots = Mesh->GetStaticMaterials();
		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			const FString N = Slots[i].MaterialSlotName.ToString();
			C->SetMaterial(i, N.Contains(TEXT("Glass")) ? GlassM : N.Contains(TEXT("Gold")) ? Gold : N.Contains(TEXT("Sand")) ? Sand
				: N.Contains(TEXT("Coral")) ? Coral : N.Contains(TEXT("Glow")) ? Glow : Navy);
		}
	}
	else
	{
		AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 30.f), FRotator::ZeroRotator, FVector(2.9f, 2.9f, 0.6f), Navy, false, false);
		AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 62.f), FRotator::ZeroRotator, FVector(3.f, 3.f, 0.06f), Gold, false, false);
		AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 68.f), FRotator::ZeroRotator, FVector(2.5f, 2.5f, 0.08f), Sand, false, false);
		AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 175.f), FRotator::ZeroRotator, FVector(2.6f, 2.6f, 2.2f), GlassM, false, false);
		AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 300.f), FRotator::ZeroRotator, FVector(2.9f, 2.9f, 0.35f), Navy, false, false);
		AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 282.f), FRotator::ZeroRotator, FVector(3.f, 3.f, 0.06f), Gold, false, false);
	}
	// the water itself: a faint blue volume inside the glass
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, (WaterBottom + WaterTop) * 0.5f), FRotator::ZeroRotator,
		FVector((SwimRadius + 30.f) / 50.f, (SwimRadius + 30.f) / 50.f, (WaterTop - WaterBottom) / 100.f), Glass(this, FLinearColor(0.05f, 0.4f, 0.65f), 0.12f), false, false);
	// guests and the chef walk around it
	UStaticMeshComponent* Block = AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, BlockH * 0.5f), FRotator::ZeroRotator, FVector(BlockR / 50.f, BlockR / 50.f, BlockH / 100.f), nullptr, true, false);
	Block->SetVisibility(false);
	Light = AddPointLight(this, Root, FVector(0.f, 0.f, (WaterBottom + WaterTop) * 0.5f), FLinearColor(0.35f, 0.85f, 1.f), 14.f, 650.f, false, false);
}

void AAquariumTank::SetFish(const TArray<FTankFish>& Fish)
{
	for (USceneComponent* R : FishRoots)
	{
		if (!R) continue;
		TArray<USceneComponent*> Kids;
		R->GetChildrenComponents(true, Kids);
		for (USceneComponent* K : Kids) K->DestroyComponent();
		R->DestroyComponent();
	}
	FishRoots.Reset();
	Swimmers.Reset();
	FRandomStream Rs(Fish.Num() * 31 + 7);
	for (int32 i = 0; i < Fish.Num(); ++i)
	{
		const FFishDef& D = AbyssData::Fish(Fish[i].Type);
		// shown at a size that fits the tank: the big ones are juveniles
		const float Scale = FMath::Clamp(0.62f / D.Scale, 0.3f, 0.7f) * D.Scale * (Fish[i].bPrime ? 1.12f : 1.f) * FMath::Min(1.f, SwimRadius / 105.f);
		USceneComponent* Tail = nullptr;
		USceneComponent* Root = ACatchableFish::MakeDisplayFish(this, GetRootComponent(), Fish[i].Type, Scale, Tail);
		FishRoots.Add(Root);
		FSwimmer S;
		S.Root = Root;
		S.Tail = Tail;
		S.Radius = SwimRadius * Rs.FRandRange(0.45f, 0.8f);
		S.Height = FMath::Lerp(WaterBottom + 30.f, WaterTop - 30.f, Fish.Num() > 1 ? (float)i / (Fish.Num() - 1) : 0.5f) + Rs.FRandRange(-10.f, 10.f);
		S.Speed = Rs.FRandRange(0.35f, 0.6f) * (Fish[i].Type == EFishType::Tuna ? 1.5f : Fish[i].Type == EFishType::Grouper ? 0.6f : 1.f);
		S.Phase = Rs.FRandRange(0.f, 2.f * PI);
		S.Dir = (i % 2) ? -1.f : 1.f;
		Swimmers.Add(S);
	}
}

void AAquariumTank::Tick(float Dt)
{
	Super::Tick(Dt);
	Time += Dt;
	for (const FSwimmer& S : Swimmers)
	{
		USceneComponent* R = S.Root.Get();
		if (!R) continue;
		const float A = S.Phase + Time * S.Speed * S.Dir;
		const float Bob = FMath::Sin(Time * 0.7f + S.Phase) * 12.f;
		R->SetRelativeLocation(FVector(FMath::Cos(A) * S.Radius, FMath::Sin(A) * S.Radius, S.Height + Bob));
		// nose along the circle
		R->SetRelativeRotation(FRotator(0.f, FMath::RadiansToDegrees(A) + 90.f * S.Dir, 0.f));
		if (USceneComponent* T = S.Tail.Get()) T->SetRelativeRotation(FRotator(0.f, FMath::Sin(Time * 6.f + S.Phase) * 22.f, 0.f));
	}
	if (Light) Light->SetIntensity(12.f + 3.f * FMath::Sin(Time * 1.3f));
}
