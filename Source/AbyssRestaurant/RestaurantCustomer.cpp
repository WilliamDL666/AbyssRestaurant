#include "RestaurantCustomer.h"
#include "RestaurantDome.h"
#include "AbyssAssets.h"
#include "AbyssTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

ARestaurantCustomer::ARestaurantCustomer()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	Body = CreateDefaultSubobject<USceneComponent>(TEXT("Body"));
	Body->SetupAttachment(GetRootComponent());
	GuestMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("GuestMesh"));
	GuestMesh->SetupAttachment(Body);
	GuestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GuestMesh->SetVisibility(false);
}

void ARestaurantCustomer::Init(ARestaurantDome* InDome, int32 InSeat, int32 InDish, const FVector& InTableWorld, float InPatience, int32 Look)
{
	using namespace AbyssAssets;
	Dome = InDome;
	Seat = InSeat;
	Dish = InDish;
	TableWorld = InTableWorld;
	Patience = PatienceMax = InPatience;
	Phase = FMath::FRand() * 10.f;

	static const FLinearColor Clothes[] = {
		FLinearColor(0.05f, 0.08f, 0.20f), FLinearColor(0.35f, 0.05f, 0.08f), FLinearColor(0.06f, 0.20f, 0.14f),
		FLinearColor(0.45f, 0.35f, 0.25f), FLinearColor(0.12f, 0.12f, 0.12f), FLinearColor(0.50f, 0.15f, 0.30f) };
	static const FLinearColor Skin[] = {
		FLinearColor(0.80f, 0.60f, 0.45f), FLinearColor(0.60f, 0.42f, 0.30f), FLinearColor(0.35f, 0.22f, 0.15f) };
	static const FLinearColor Hair[] = {
		FLinearColor(0.02f, 0.015f, 0.01f), FLinearColor(0.20f, 0.10f, 0.04f), FLinearColor(0.5f, 0.4f, 0.25f) };

	// --- the Tripo guest: seated, hips on the chair seat, facing the table
	if (USkeletalMesh* SK = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Abyss/Customer/SK_Customer.SK_Customer"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		const float Height = 162.f * FMath::FRandRange(0.96f, 1.04f);
		UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Abyss/Customer/M_Customer.M_Customer"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (AbyssRig::FitHumanoid(GuestMesh, SK, Height, M))
		{
			// hips are ~54% of the height up; sitting puts them on a ~50 cm seat
			const FVector L = GuestMesh->GetRelativeLocation();
			GuestMesh->SetRelativeLocation(FVector(-8.f, 0.f, L.Z - (Height * 0.54f - 50.f)));
			bModel = Rig.Init(GuestMesh);
			Body->SetRelativeScale3D(FVector(0.01f));
			return;
		}
	}
	UMaterialInterface* ClothM = Lit(this, Clothes[Look % 6], 0.8f);
	UMaterialInterface* SkinM = Lit(this, Skin[(Look / 2) % 3], 0.55f);
	UMaterialInterface* HairM = Lit(this, Hair[(Look / 3) % 3], 0.6f);

	// Seated figure: torso, shoulders, arms resting on table, head + hair
	AddPart(this, Body, Cylinder(), FVector(0.f, 0.f, 75.f), FRotator::ZeroRotator, FVector(0.42f, 0.32f, 0.6f), ClothM, false, false);
	AddPart(this, Body, Sphere(), FVector(0.f, 0.f, 105.f), FRotator::ZeroRotator, FVector(0.5f, 0.38f, 0.22f), ClothM, false, false);
	AddPart(this, Body, Cube(), FVector(0.f, 0.f, 48.f), FRotator::ZeroRotator, FVector(0.45f, 0.4f, 0.12f), Lit(this, FLinearColor(0.05f, 0.05f, 0.06f), 0.8f), false, false);
	for (float Y : { 20.f, -20.f })
	{
		AddPart(this, Body, Cylinder(), FVector(22.f, Y, 88.f), FRotator(120.f, 0.f, 0.f), FVector(0.09f, 0.09f, 0.42f), ClothM, false, false);
		AddPart(this, Body, Sphere(), FVector(42.f, Y * 0.8f, 78.f), FRotator::ZeroRotator, FVector(0.09f), SkinM, false, false);
	}
	Head = NewObject<USceneComponent>(this);
	Head->SetupAttachment(Body);
	Head->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	Head->RegisterComponent();
	AddInstanceComponent(Head);
	AddPart(this, Head, Sphere(), FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.22f, 0.2f, 0.26f), SkinM, false, false);
	AddPart(this, Head, Sphere(), FVector(-3.f, 0.f, 5.f), FRotator::ZeroRotator, FVector(0.24f, 0.22f, 0.22f), HairM, false, false);

	Body->SetRelativeScale3D(FVector(0.01f));
}

FVector ARestaurantCustomer::GetBubbleLocation() const
{
	return GetActorLocation() + FVector(0.f, 0.f, bModel ? 150.f : 185.f);
}

void ARestaurantCustomer::Serve(float InQuality)
{
	using namespace AbyssAssets;
	Quality = InQuality;
	State = ECustomerState::Eating;
	StateTime = 0.f;

	// Plate + food appears on the table in front of the customer
	const FVector Rel = GetActorTransform().InverseTransformPosition(FMath::Lerp(GetActorLocation(), TableWorld, 0.55f));
	const FVector PlateLoc(Rel.X, Rel.Y, 88.f);
	// Blender-made dish (Scripts/blender_make_food.py), falls back to primitives
	static const TCHAR* DishMeshes[] = { TEXT("SM_Dish_Nigiri"), TEXT("SM_Dish_Tuna"), TEXT("SM_Dish_Grouper"), TEXT("SM_Dish_Kelp"), TEXT("SM_Dish_Stew") };
	const FString DishPath = FString::Printf(TEXT("/Game/Abyss/Food/%s.%s"), DishMeshes[FMath::Clamp(Dish, 0, 4)], DishMeshes[FMath::Clamp(Dish, 0, 4)]);
	if (UStaticMesh* DM = LoadObject<UStaticMesh>(nullptr, *DishPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		const float Yaw = (FVector(Rel.X, Rel.Y, 0.f)).Rotation().Yaw + 90.f;
		AddPart(this, GetRootComponent(), DM, FVector(Rel.X, Rel.Y, 85.f), FRotator(0.f, Yaw, 0.f), FVector(1.f), nullptr, false, false);
		return;
	}
	AddPart(this, GetRootComponent(), Cylinder(), PlateLoc, FRotator::ZeroRotator, FVector(0.42f, 0.42f, 0.03f), Lit(this, FLinearColor(0.85f, 0.85f, 0.82f), 0.2f), false, false);
	const FLinearColor Food = Dish == 0 ? FLinearColor(0.95f, 0.55f, 0.25f) : Dish == 1 ? FLinearColor(0.70f, 0.08f, 0.10f) : Dish == 2 ? FLinearColor(0.85f, 0.45f, 0.25f) : FLinearColor(0.15f, 0.45f, 0.10f);
	for (int32 i = 0; i < 3; ++i)
	{
		AddPart(this, GetRootComponent(), Cube(), PlateLoc + FVector(-10.f + i * 10.f, 0.f, 4.f), FRotator(0.f, 15.f, 0.f), FVector(0.07f, 0.18f, 0.05f), Lit(this, FLinearColor(0.95f, 0.94f, 0.9f), 0.6f), false, false);
		AddPart(this, GetRootComponent(), Cube(), PlateLoc + FVector(-10.f + i * 10.f, 0.f, 8.f), FRotator(0.f, 15.f, 0.f), FVector(0.08f, 0.2f, 0.03f), Lit(this, Food, 0.25f, 0.f, Quality > 0.9f ? 0.2f : 0.f), false, false);
	}
}

void ARestaurantCustomer::Leave(bool bInAngry)
{
	bAngry = bInAngry;
	State = ECustomerState::Leaving;
	StateTime = 0.f;
}

void ARestaurantCustomer::Tick(float Dt)
{
	Super::Tick(Dt);
	StateTime += Dt;
	Phase += Dt;

	if (bModel)
	{
		if (!Rig.bReady) Rig.Init(GuestMesh);
		EGuestMood Mood = EGuestMood::Waiting;
		if (State == ECustomerState::Ordering) Mood = EGuestMood::Calling;
		else if (State == ECustomerState::Eating) Mood = EGuestMood::Eating;
		else if (State == ECustomerState::Leaving && bAngry) Mood = EGuestMood::Angry;
		else if (IsWaitingForFood() && Patience01() < 0.25f) Mood = EGuestMood::Angry;
		AbyssRig::Seated(Rig, Phase, Mood, StateTime);
	}

	switch (State)
	{
	case ECustomerState::Arriving:
	{
		const float A = FMath::Clamp(StateTime / 0.6f, 0.f, 1.f);
		const float Pop = 1.f + 0.15f * FMath::Sin(A * PI);
		Body->SetRelativeScale3D(FVector(FMath::Max(A * Pop, 0.01f)));
		if (A >= 1.f)
		{
			State = ECustomerState::Ordering;
			StateTime = 0.f;
		}
		break;
	}
	case ECustomerState::Ordering:
	case ECustomerState::Waiting:
	case ECustomerState::Cooking:
	case ECustomerState::Ready:
		Patience -= Dt * (State == ECustomerState::Cooking ? 0.35f : State == ECustomerState::Ready ? 0.25f : 1.f);
		if (Head) Head->SetRelativeRotation(FRotator(0.f, FMath::Sin(Phase * 0.8f) * 25.f, 0.f));
		if (Patience <= 0.f && Dome.IsValid())
		{
			Dome->OnCustomerGaveUp(this);
		}
		break;
	case ECustomerState::Eating:
		if (Head)
		{
			Head->SetRelativeRotation(FRotator(-12.f + FMath::Sin(Phase * 6.f) * 8.f, 0.f, 0.f));
		}
		if (StateTime > 7.f)
		{
			Leave(false);
		}
		break;
	case ECustomerState::Leaving:
	{
		const float A = FMath::Clamp(1.f - StateTime / 0.8f, 0.f, 1.f);
		Body->SetRelativeScale3D(FVector(FMath::Max(A, 0.01f)));
		if (A <= 0.f)
		{
			if (Dome.IsValid()) Dome->OnCustomerLeft(this);
			Destroy();
		}
		break;
	}
	}
}

void ARestaurantCustomer::TakeOrder()
{
	if (State != ECustomerState::Ordering) return;
	State = ECustomerState::Waiting;
	StateTime = 0.f;
	// being noticed calms the guest down a little
	Patience = FMath::Min(PatienceMax, Patience + PatienceMax * 0.3f);
}
