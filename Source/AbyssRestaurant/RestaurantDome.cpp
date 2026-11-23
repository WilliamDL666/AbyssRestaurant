#include "RestaurantDome.h"
#include "RestaurantCustomer.h"
#include "AbyssGameMode.h"
#include "AbyssAssets.h"
#include "AbyssTypes.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Components/LightComponent.h"
#include "Engine/Level.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"

namespace
{
	UTextRenderComponent* AddText(AActor* Owner, USceneComponent* Parent, const FString& Str, const FVector& Loc, float Yaw, float Size, const FColor& Color)
	{
		UTextRenderComponent* T = NewObject<UTextRenderComponent>(Owner);
		T->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		T->SetText(FText::FromString(Str));
		T->SetTextRenderColor(Color);
		T->SetWorldSize(Size);
		T->SetHorizontalAlignment(EHTA_Center);
		T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetRelativeLocationAndRotation(Loc, FRotator(0.f, Yaw, 0.f));
		T->SetupAttachment(Parent);
		T->RegisterComponent();
		return T;
	}
}

ARestaurantDome::ARestaurantDome()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	InteriorCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("InteriorCamera"));
	InteriorCamera->SetupAttachment(GetRootComponent());
	InteriorCamera->SetFieldOfView(72.f);

	ChefRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ChefRoot"));
	ChefRoot->SetupAttachment(GetRootComponent());
	ChefRoot->SetRelativeLocation(FVector(-700.f, 0.f, 0.f));
	ChefMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("ChefMesh"));
	ChefMesh->SetupAttachment(ChefRoot);
	ChefMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChefMesh->SetVisibility(false);

	GrillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GrillLight"));
	GrillLight->SetupAttachment(GetRootComponent());
	GrillLight->SetRelativeLocation(FVector(-560.f, -130.f, 150.f));
	GrillLight->IntensityUnits = ELightUnits::Candelas;
	GrillLight->SetIntensity(80.f);
	GrillLight->SetLightColor(FLinearColor(1.f, 0.45f, 0.12f));
	GrillLight->SetAttenuationRadius(700.f);
	GrillLight->SetCastShadows(false);

	TankFish = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TankFish"));
	TankFish->SetupAttachment(GetRootComponent());
	TankFish->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteriorPost = CreateDefaultSubobject<UPostProcessComponent>(TEXT("InteriorPost"));
	InteriorPost->SetupAttachment(GetRootComponent());
	InteriorPost->bUnbound = true;
	InteriorPost->bEnabled = false;
	InteriorPost->Priority = 10.f;

	SharkPivot = CreateDefaultSubobject<USceneComponent>(TEXT("SharkPivot"));
	SharkPivot->SetupAttachment(GetRootComponent());
}

void ARestaurantDome::SetInteriorActive(bool bActive)
{
	InteriorPost->bEnabled = bActive;
}

FVector ARestaurantDome::GetStationLocation(EKitchenStep Step) const
{
	FVector L(-80.f, -420.f, 0.f);
	const float K = FMath::Max(Radius / 800.f, 1.f);
	if (bLuxLayout)
	{
		L = KitchenSpot[Step == EKitchenStep::Slice ? 0 : Step == EKitchenStep::Cook ? 1 : 2];
	}
	else if (bConceptLayout)
	{
		switch (Step)
		{
		case EKitchenStep::Slice: L = FVector(-250.f, -470.f, 0.f); break; // 01 prep table
		case EKitchenStep::Cook:  L = FVector(90.f, -500.f, 0.f); break;   // 02 stove
		case EKitchenStep::Plate: L = FVector(-300.f, -230.f, 0.f); break; // 06 serving counter
		default: break;
		}
		L = FVector(L.X * K, L.Y * K, 0.f);
	}
	else
	{
		L = FVector(-470.f, 0.f, 0.f);
	}
	return GetActorTransform().TransformPosition(L);
}

const TCHAR* ARestaurantDome::GetStationName(EKitchenStep Step) const
{
	switch (Step)
	{
	case EKitchenStep::Slice: return TEXT("PREP TABLE");
	case EKitchenStep::Cook:  return TEXT("STOVE");
	case EKitchenStep::Plate: return TEXT("SERVING COUNTER");
	default: return TEXT("KITCHEN");
	}
}

FTransform ARestaurantDome::GetEntranceTransform() const
{
	if (bLuxLayout)
	{
		// start behind the serving counter, facing the dining room
		return FTransform(FRotator(0.f, 90.f, 0.f), KitchenSpot[2] + FVector(0.f, 0.f, 100.f)) * GetActorTransform();
	}
	const FTransform Local = bConceptLayout
		? FTransform(FRotator(0.f, 0.f, 0.f), FVector(-Radius + 220.f, 0.f, 100.f))
		: FTransform(FRotator(0.f, 180.f, 0.f), FVector(420.f, 160.f, 100.f));
	return Local * GetActorTransform();
}

FTransform ARestaurantDome::GetDockSpawn() const
{
	const FVector ToDome = (FVector::ZeroVector - DockLocal()).GetSafeNormal2D();
	const FTransform Local(FRotator(0.f, ToDome.Rotation().Yaw, 0.f), DockLocal());
	return Local * GetActorTransform();
}

void ARestaurantDome::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	using namespace AbyssAssets;

	if (CustomShellMesh)
	{
		UStaticMeshComponent* M = AddPart(this, GetRootComponent(), CustomShellMesh, CustomShellTransform.GetLocation(), CustomShellTransform.Rotator(), CustomShellTransform.GetScale3D(), nullptr, true);
		M->SetCastShadow(true);
	}
	else
	{
		BuildShell();
	}
	if (CustomInteriorMesh)
	{
		AddPart(this, GetRootComponent(), CustomInteriorMesh, CustomInteriorTransform.GetLocation(), CustomInteriorTransform.Rotator(), CustomInteriorTransform.GetScale3D(), nullptr, false);
	}
	bConceptLayout = false;
	bLuxLayout = false;
	if (!((bUseLuxLevel && BuildLuxInterior()) || (bInteriorSpace && !CustomInteriorMesh && (BuildConceptInterior() || BuildLuxuryInterior()))))
	{
		BuildInterior(!CustomInteriorMesh || bKeepProceduralFurniture);
	}
	ApplyTier(PreviewTier);

	if (bInteriorSpace)
	{
		BuildSurroundings();
		BuildShark();
	}
	else
	{
		BuildDock();
	}

	// Interior gets its own exposure + warm grading (enabled only while you are inside)
	FPostProcessSettings& P = InteriorPost->Settings;
	P.bOverride_AutoExposureMethod = true;
	P.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	P.bOverride_AutoExposureBias = true;
	P.AutoExposureBias = InteriorExposure + (bUseLuxLevel ? LuxExposureBoost : 0.f);
	if (bUseLuxLevel)
	{
		// dreamy grade: violet-blue shadows, warm highlights, a touch of bloom
		P.bOverride_ColorGain = true;      P.ColorGain = FVector4(0.92f, 0.86f, 1.08f, 1.f);
		P.bOverride_ColorGammaShadows = true; P.ColorGammaShadows = FVector4(0.95f, 0.9f, 1.1f, 1.f);
		P.bOverride_ColorGainHighlights = true; P.ColorGainHighlights = FVector4(1.08f, 0.98f, 0.92f, 1.f);
		P.bOverride_ColorSaturation = true; P.ColorSaturation = FVector4(1.12f, 1.12f, 1.12f, 1.f);
		P.bOverride_BloomIntensity = true; P.BloomIntensity = 0.9f;
		P.bOverride_VignetteIntensity = true; P.VignetteIntensity = 0.45f;
	}
	P.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	P.AutoExposureApplyPhysicalCameraExposure = false;
	P.bOverride_BloomIntensity = true;
	P.BloomIntensity = 0.45f;
	P.bOverride_WhiteTemp = true;
	P.WhiteTemp = 5600.f;
	P.bOverride_ColorSaturation = true;
	P.ColorSaturation = FVector4(1.f, 1.f, 1.f, 1.02f);
	P.bOverride_ColorContrast = true;
	P.ColorContrast = FVector4(1.f, 1.f, 1.f, 1.1f);
	P.bOverride_ColorGamma = true;
	P.ColorGamma = FVector4(1.f, 1.f, 1.f, 0.95f);
	P.bOverride_SceneColorTint = true;
	P.SceneColorTint = FLinearColor::White;
	P.bOverride_VignetteIntensity = true;
	P.VignetteIntensity = 0.45f;
	P.bOverride_SceneFringeIntensity = true;
	P.SceneFringeIntensity = 0.f;
	P.bOverride_FilmGrainIntensity = true;
	P.FilmGrainIntensity = 0.f;
	P.bOverride_MotionBlurAmount = true;
	P.MotionBlurAmount = 0.f;

	const FVector CamLoc(760.f, 380.f, 560.f);
	InteriorCamera->SetRelativeLocation(CamLoc);
	InteriorCamera->SetRelativeRotation((FVector(-380.f, -40.f, 70.f) - CamLoc).Rotation());

	// Aquarium fish
	using namespace AbyssAssets;
	TankFish->SetStaticMesh(Sphere());
	TankFish->SetMaterial(0, Lit(this, FLinearColor(0.95f, 0.55f, 0.1f), 0.3f, 0.3f, 0.3f));
	FRandomStream R(21);
	TankFishData.SetNum(14);
	TankT.SetNum(TankFishData.Num());
	for (FTankFish& F : TankFishData)
	{
		F.Angle = (float)R.FRandRange(0.f, 2.f * PI);
		F.Radius = (float)R.FRandRange(50.f, 120.f);
		F.Height = (float)R.FRandRange(90.f, 470.f);
		F.Speed = (float)R.FRandRange(0.4f, 0.9f) * (R.FRand() < 0.5f ? -1.f : 1.f);
		F.Scale = (float)R.FRandRange(0.18f, 0.32f);
	}
	TankFish->ClearInstances();
	UpdateTank(0.f);
	TankFish->AddInstances(TankT, false, false);
}

void ARestaurantDome::BuildShell()
{
	using namespace AbyssAssets;
	USceneComponent* Root = GetRootComponent();
	const float D = Radius * 2.f / 100.f;

	UMaterialInterface* BaseM   = Lit(this, FLinearColor(0.08f, 0.09f, 0.09f), 0.55f, 0.8f);
	UMaterialInterface* Bronze  = Lit(this, FLinearColor(0.28f, 0.19f, 0.10f), 0.35f, 0.9f);
	UMaterialInterface* FloorM  = Lit(this, FLinearColor(0.035f, 0.03f, 0.028f), 0.12f, 0.f);
	UMaterialInterface* GoldM   = Lit(this, FLinearColor(0.6f, 0.38f, 0.12f), 0.25f, 1.f, 0.3f);
	UMaterialInterface* Bulb    = Lit(this, FLinearColor(1.f, 0.72f, 0.38f), 0.3f, 0.f, bInteriorSpace ? 0.8f : 2.f);
	UMaterialInterface* GlassM  = AbyssAssets::Glass(this, FLinearColor(0.55f, 0.85f, 1.f), bInteriorSpace ? 0.02f : 0.05f);

	// Foundation, polished floor with gold inlay, glass dome
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, -250.f), FRotator::ZeroRotator, FVector(D + 0.5f, D + 0.5f, 5.f), BaseM, true);
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 2.f), FRotator::ZeroRotator, FVector(D, D, 0.04f), FloorM, true);
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 2.2f), FRotator::ZeroRotator, FVector(D * 0.6f, D * 0.6f, 0.042f), GoldM);
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 2.4f), FRotator::ZeroRotator, FVector(D * 0.6f - 0.25f, D * 0.6f - 0.25f, 0.044f), FloorM);
	AddPart(this, Root, Sphere(), FVector::ZeroVector, FRotator::ZeroRotator, FVector(D), GlassM, !bInteriorSpace);

	// Steel/bronze ribs: meridians + rings
	UInstancedStaticMeshComponent* Ribs = AddInstanced(this, Root, Cube(), Bronze);
	const float RR = Radius + 6.f;
	const int32 Meridians = 12, Segs = 20;
	for (int32 m = 0; m < Meridians; ++m)
	{
		const float Phi = 2.f * PI * m / Meridians;
		for (int32 s = 0; s < Segs; ++s)
		{
			const float Th = (s + 0.5f) / Segs * (PI * 0.5f);
			const FVector Radial(FMath::Cos(Th) * FMath::Cos(Phi), FMath::Cos(Th) * FMath::Sin(Phi), FMath::Sin(Th));
			const FVector Tangent(-FMath::Sin(Th) * FMath::Cos(Phi), -FMath::Sin(Th) * FMath::Sin(Phi), FMath::Cos(Th));
			const float Len = RR * (PI * 0.5f) / Segs * 1.05f;
			Ribs->AddInstance(FTransform(FRotationMatrix::MakeFromXZ(Tangent, Radial).Rotator(), Radial * RR, FVector(Len / 100.f, 0.14f, 0.22f)));
		}
	}
	for (float ElevDeg : { 14.f, 36.f, 58.f, 76.f })
	{
		const float Th = FMath::DegreesToRadians(ElevDeg);
		const int32 N = 56;
		for (int32 i = 0; i < N; ++i)
		{
			const float Phi = 2.f * PI * (i + 0.5f) / N;
			const FVector Radial(FMath::Cos(Th) * FMath::Cos(Phi), FMath::Cos(Th) * FMath::Sin(Phi), FMath::Sin(Th));
			const FVector Tangent(-FMath::Sin(Phi), FMath::Cos(Phi), 0.f);
			const float Len = 2.f * PI * RR * FMath::Cos(Th) / N * 1.05f;
			Ribs->AddInstance(FTransform(FRotationMatrix::MakeFromXZ(Tangent, Radial).Rotator(), Radial * RR, FVector(Len / 100.f, 0.1f, 0.16f)));
		}
	}
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, Radius + 5.f), FRotator::ZeroRotator, FVector(2.2f, 2.2f, 0.3f), Bronze);
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, Radius + 60.f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 1.0f), Bronze);
	AddPart(this, Root, Sphere(), FVector(0.f, 0.f, Radius + 115.f), FRotator::ZeroRotator, FVector(0.25f), Lit(this, FLinearColor(1.f, 0.3f, 0.1f), 0.3f, 0.f, 20.f));

	// Warm light bulbs around the rim (outside + inside glow)
	UInstancedStaticMeshComponent* Bulbs = AddInstanced(this, Root, Sphere(), Bulb);
	for (int32 i = 0; i < 48; ++i)
	{
		const float Phi = 2.f * PI * i / 48.f;
		Bulbs->AddInstance(FTransform(FVector(FMath::Cos(Phi), FMath::Sin(Phi), 0.f) * (Radius + 30.f) + FVector(0.f, 0.f, 15.f)));
		Bulbs->SetCastShadow(false);
	}

	if (bInteriorSpace) return; // the rest is the exterior's docking-side dressing

	// Support pillars down into the seabed
	for (int32 i = 0; i < 8; ++i)
	{
		const float Phi = 2.f * PI * (i + 0.5f) / 8.f;
		AddPart(this, Root, Cylinder(), FVector(FMath::Cos(Phi), FMath::Sin(Phi), 0.f) * (Radius - 60.f) + FVector(0.f, 0.f, -700.f), FRotator::ZeroRotator, FVector(1.1f, 1.1f, 6.f), BaseM, true);
	}

	// Exterior spill lights + restaurant sign facing the docking approach
	AddPointLight(this, Root, FVector(Radius + 250.f, 600.f, 250.f), FLinearColor(1.f, 0.7f, 0.4f), 180.f, 2500.f);
	AddPointLight(this, Root, FVector(-Radius - 250.f, -400.f, 250.f), FLinearColor(1.f, 0.7f, 0.4f), 150.f, 2500.f);
	AddPointLight(this, Root, FVector(0.f, -Radius - 250.f, 250.f), FLinearColor(1.f, 0.7f, 0.4f), 150.f, 2500.f);

	const float SignYaw = FMath::RadiansToDegrees(FMath::Atan2(DockLocal().Y, DockLocal().X));
	const FVector SignDir = FRotator(0.f, SignYaw, 0.f).Vector();
	AddPart(this, Root, Cube(), SignDir * (Radius + 52.f) + FVector(0.f, 0.f, -220.f), FRotator(0.f, SignYaw, 0.f), FVector(0.06f, 11.f, 1.9f), Lit(this, FLinearColor(0.02f, 0.02f, 0.025f), 0.3f, 0.6f));
	AddPart(this, Root, Cube(), SignDir * (Radius + 50.f) + FVector(0.f, 0.f, -220.f), FRotator(0.f, SignYaw, 0.f), FVector(0.05f, 11.4f, 2.2f), GoldM);
	AddText(this, Root, TEXT("THE ABYSS RESTAURANT"), SignDir * (Radius + 58.f) + FVector(0.f, 0.f, -200.f), SignYaw, 95.f, FColor(255, 196, 110));
	AddText(this, Root, TEXT("- DEEP SEA DINING -"), SignDir * (Radius + 58.f) + FVector(0.f, 0.f, -275.f), SignYaw, 40.f, FColor(120, 230, 220));
	AddPointLight(this, Root, SignDir * (Radius + 250.f) + FVector(0.f, 0.f, -150.f), FLinearColor(1.f, 0.8f, 0.55f), 250.f, 900.f);
}

void ARestaurantDome::BuildInterior(bool bFurniture)
{
	using namespace AbyssAssets;
	USceneComponent* Root = GetRootComponent();

	UMaterialInterface* Wood    = Lit(this, FLinearColor(0.07f, 0.035f, 0.02f), 0.25f);
	UMaterialInterface* Leather = Lit(this, FLinearColor(0.16f, 0.06f, 0.035f), 0.45f);
	UMaterialInterface* Metal   = Lit(this, FLinearColor(0.2f, 0.2f, 0.2f), 0.3f, 1.f);
	UMaterialInterface* Marble  = Lit(this, FLinearColor(0.55f, 0.53f, 0.5f), 0.15f);
	UMaterialInterface* Lantern = Lit(this, FLinearColor(1.f, 0.7f, 0.35f), 0.4f, 0.f, 1.8f);
	UMaterialInterface* Cyan    = Lit(this, FLinearColor(0.2f, 0.9f, 1.f), 0.3f, 0.f, 1.5f);
	UMaterialInterface* Navy    = Lit(this, FLinearColor(0.02f, 0.03f, 0.09f), 0.8f);
	UMaterialInterface* White   = Lit(this, FLinearColor(0.85f, 0.85f, 0.82f), 0.6f);
	UMaterialInterface* Kelp    = Lit(this, FLinearColor(0.15f, 0.5f, 0.15f), 0.6f, 0.f, 0.3f);
	UMaterialInterface* Sand    = Lit(this, FLinearColor(0.6f, 0.55f, 0.4f), 0.9f);

	// Central aquarium column
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 20.f), FRotator::ZeroRotator, FVector(3.8f, 3.8f, 0.4f), Metal, true);
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 265.f), FRotator::ZeroRotator, FVector(3.3f, 3.3f, 4.5f), nullptr, true)->SetVisibility(false);
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 45.f), FRotator::ZeroRotator, FVector(3.1f, 3.1f, 0.1f), Sand);
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 42.f), FRotator::ZeroRotator, FVector(3.9f, 3.9f, 0.06f), Cyan);
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 265.f), FRotator::ZeroRotator, FVector(3.1f, 3.1f, 4.4f), AbyssAssets::Glass(this, FLinearColor(0.05f, 0.35f, 0.55f), 0.1f));
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 265.f), FRotator::ZeroRotator, FVector(3.25f, 3.25f, 4.5f), AbyssAssets::Glass(this, FLinearColor(0.5f, 0.7f, 0.8f), 0.03f));
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 510.f), FRotator::ZeroRotator, FVector(3.8f, 3.8f, 0.4f), Metal);
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 488.f), FRotator::ZeroRotator, FVector(3.9f, 3.9f, 0.06f), Cyan);
	FRandomStream R(4);
	for (int32 i = 0; i < 7; ++i)
	{
		const float A = R.FRandRange(0.f, 2.f * PI), Rad = R.FRandRange(20.f, 110.f), H = R.FRandRange(1.8f, 3.8f);
		AddPart(this, Root, Cube(), FVector(FMath::Cos(A) * Rad, FMath::Sin(A) * Rad, 50.f + H * 50.f), FRotator(0.f, R.FRandRange(0.f, 180.f), 0.f), FVector(0.03f, 0.22f, H), Kelp);
	}
	AddPointLight(this, Root, FVector(0.f, 0.f, 300.f), FLinearColor(0.3f, 0.8f, 1.f), 18.f * LightScale, 700.f);

	// Glowing chandelier ring above the tank
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 650.f), FRotator::ZeroRotator, FVector(6.4f, 6.4f, 0.12f), Lantern);
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 651.f), FRotator::ZeroRotator, FVector(5.8f, 5.8f, 0.14f), Metal);
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, 590.f), FRotator::ZeroRotator, FVector(0.1f, 0.1f, 1.2f), Metal);

	// Dining ring: 7 tables, 2 seats each
	Seats.Reset();
	const float TableR = 720.f;
	for (int32 t = 0; t < 7; ++t)
	{
		const float Deg = -126.f + t * 42.f;
		const float A = FMath::DegreesToRadians(Deg);
		const FVector Tab(FMath::Cos(A) * TableR, FMath::Sin(A) * TableR, 0.f);
		if (bFurniture)
		{
		AddPart(this, Root, Cylinder(), Tab + FVector(0.f, 0.f, 82.f), FRotator::ZeroRotator, FVector(1.5f, 1.5f, 0.06f), Wood, true);
		AddPart(this, Root, Cylinder(), Tab + FVector(0.f, 0.f, 45.f), FRotator::ZeroRotator, FVector(1.4f, 1.4f, 0.9f), nullptr, true)->SetVisibility(false);
		AddPart(this, Root, Cylinder(), Tab + FVector(0.f, 0.f, 41.f), FRotator::ZeroRotator, FVector(0.22f, 0.22f, 0.8f), Metal);
		AddPart(this, Root, Cylinder(), Tab + FVector(0.f, 0.f, 3.f), FRotator::ZeroRotator, FVector(0.8f, 0.8f, 0.04f), Metal);
		AddPart(this, Root, Cylinder(), Tab + FVector(0.f, 0.f, 97.f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 0.22f), Lantern);
		AddPart(this, Root, Cylinder(), Tab + FVector(0.f, 0.f, 330.f), FRotator::ZeroRotator, FVector(0.32f, 0.32f, 0.45f), Lantern);
		AddPart(this, Root, Cylinder(), Tab + FVector(0.f, 0.f, 470.f), FRotator::ZeroRotator, FVector(0.015f, 0.015f, 2.4f), Metal);
		}

		const FVector TangentDir(-FMath::Sin(A), FMath::Cos(A), 0.f);
		for (float Side : { 1.f, -1.f })
		{
			const FVector SeatLoc = Tab + TangentDir * Side * 105.f;
			const FVector ToTable = (Tab - SeatLoc).GetSafeNormal();
			const float Yaw = ToTable.Rotation().Yaw;
			if (bFurniture)
			{
			AddPart(this, Root, Cube(), SeatLoc + FVector(0.f, 0.f, 44.f), FRotator(0.f, Yaw, 0.f), FVector(0.5f, 0.5f, 0.1f), Leather);
			AddPart(this, Root, Cube(), SeatLoc - ToTable * 26.f + FVector(0.f, 0.f, 78.f), FRotator(0.f, Yaw, 0.f), FVector(0.08f, 0.5f, 0.6f), Leather);
			AddPart(this, Root, Cylinder(), SeatLoc + FVector(0.f, 0.f, 20.f), FRotator::ZeroRotator, FVector(0.08f, 0.08f, 0.4f), Metal);
			}

			FDomeSeat S;
			S.Loc = SeatLoc;
			S.Table = Tab;
			S.Yaw = Yaw;
			Seats.Add(S);
		}
	}

	const float LS = LightScale;
	AddPointLight(this, Root, FVector(650.f, 0.f, 430.f), FLinearColor(1.f, 0.72f, 0.45f), 55.f * LS, 1500.f, true);
	AddPointLight(this, Root, FVector(0.f, 650.f, 430.f), FLinearColor(1.f, 0.72f, 0.45f), 45.f * LS, 1400.f);
	AddPointLight(this, Root, FVector(0.f, -650.f, 430.f), FLinearColor(1.f, 0.72f, 0.45f), 45.f * LS, 1400.f);
	AddPointLight(this, Root, FVector(-560.f, 0.f, 360.f), FLinearColor(1.f, 0.78f, 0.55f), 60.f * LS, 1100.f, true);

	if (!bFurniture) return;

	// Kitchen counter (teppanyaki grill + ingredient trays)
	AddPart(this, Root, Cube(), FVector(-560.f, 0.f, 46.f), FRotator::ZeroRotator, FVector(1.0f, 5.4f, 0.92f), Wood, true);
	AddPart(this, Root, Cube(), FVector(-560.f, 0.f, 95.f), FRotator::ZeroRotator, FVector(1.25f, 5.6f, 0.06f), Marble);
	AddPart(this, Root, Cube(), FVector(-497.f, 0.f, 80.f), FRotator::ZeroRotator, FVector(0.02f, 5.4f, 0.04f), Lantern);
	AddPart(this, Root, Cube(), FVector(-560.f, -130.f, 100.f), FRotator::ZeroRotator, FVector(0.9f, 1.3f, 0.03f), Metal);
	FireMat = Lit(this, FLinearColor(1.f, 0.35f, 0.05f), 0.5f, 0.f, 6.f);
	AddPart(this, Root, Cube(), FVector(-560.f, -130.f, 103.f), FRotator::ZeroRotator, FVector(0.6f, 0.9f, 0.02f), FireMat);
	AddPart(this, Root, Cylinder(), FVector(-560.f, 40.f, 120.f), FRotator::ZeroRotator, FVector(0.35f, 0.35f, 0.4f), Metal);
	const FLinearColor TrayColors[] = { FLinearColor(0.95f, 0.45f, 0.2f), FLinearColor(0.65f, 0.06f, 0.08f), FLinearColor(0.9f, 0.88f, 0.82f) };
	for (int32 i = 0; i < 3; ++i)
	{
		AddPart(this, Root, Cube(), FVector(-560.f, 130.f + i * 55.f, 102.f), FRotator::ZeroRotator, FVector(0.6f, 0.45f, 0.04f), Metal);
		AddPart(this, Root, Cube(), FVector(-560.f, 130.f + i * 55.f, 106.f), FRotator::ZeroRotator, FVector(0.45f, 0.35f, 0.05f), Lit(this, TrayColors[i], 0.3f));
	}

	// Back shelf, banners, hanging lanterns
	AddPart(this, Root, Cube(), FVector(-900.f, 0.f, 120.f), FRotator::ZeroRotator, FVector(0.5f, 4.6f, 2.4f), Wood, true);
	for (int32 i = 0; i < 7; ++i)
	{
		AddPart(this, Root, Cylinder(), FVector(-890.f, -180.f + i * 60.f, 258.f), FRotator::ZeroRotator, FVector(0.16f, 0.16f, 0.36f), Lit(this, TrayColors[i % 3] * 0.6f, 0.2f, 0.f, 0.2f));
	}
	for (float Y : { -200.f, 0.f, 200.f })
	{
		AddPart(this, Root, Cube(), FVector(-600.f, Y, 330.f), FRotator::ZeroRotator, FVector(0.02f, 1.7f, 1.1f), Navy);
		AddPart(this, Root, Cylinder(), FVector(-588.f, Y, 330.f), FRotator(90.f, 0.f, 0.f), FVector(0.55f, 0.55f, 0.01f), White);
		AddPart(this, Root, Cylinder(), FVector(-520.f, Y, 300.f), FRotator::ZeroRotator, FVector(0.3f, 0.3f, 0.42f), Lantern);
	}

	BuildChef();
}

void ARestaurantDome::BuildChef()
{
	using namespace AbyssAssets;
	UMaterialInterface* Navy = Lit(this, FLinearColor(0.02f, 0.03f, 0.09f), 0.8f);
	// Chef
	UMaterialInterface* ChefWhite = Lit(this, FLinearColor(0.9f, 0.9f, 0.88f), 0.7f);
	UMaterialInterface* Skin = Lit(this, FLinearColor(0.75f, 0.55f, 0.4f), 0.55f);
	USkeletalMesh* SK = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Abyss/Chef/SK_Chef.SK_Chef"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (AbyssRig::FitHumanoid(ChefMesh, SK, 178.f, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Abyss/Chef/M_Chef.M_Chef"), nullptr, LOAD_NoWarn | LOAD_Quiet)))
	{
		// face the counter / guests
		ChefMesh->SetRelativeRotation(FRotator(0.f, bConceptLayout ? 0.f : -90.f, 0.f));
		return;
	}
	AddPart(this, ChefRoot, Cylinder(), FVector(0.f, 0.f, 45.f), FRotator::ZeroRotator, FVector(0.4f, 0.32f, 0.9f), Navy);
	AddPart(this, ChefRoot, Cylinder(), FVector(0.f, 0.f, 125.f), FRotator::ZeroRotator, FVector(0.5f, 0.38f, 0.7f), ChefWhite);
	AddPart(this, ChefRoot, Sphere(), FVector(0.f, 0.f, 158.f), FRotator::ZeroRotator, FVector(0.55f, 0.42f, 0.2f), ChefWhite);
	for (float Y : { 24.f, -24.f })
	{
		AddPart(this, ChefRoot, Cylinder(), FVector(26.f, Y, 135.f), FRotator(120.f, 0.f, 0.f), FVector(0.1f, 0.1f, 0.5f), ChefWhite);
		AddPart(this, ChefRoot, Sphere(), FVector(50.f, Y * 0.85f, 112.f), FRotator::ZeroRotator, FVector(0.1f), Skin);
	}
	AddPart(this, ChefRoot, Sphere(), FVector(0.f, 0.f, 192.f), FRotator::ZeroRotator, FVector(0.22f, 0.2f, 0.26f), Skin);
	AddPart(this, ChefRoot, Cylinder(), FVector(0.f, 0.f, 203.f), FRotator::ZeroRotator, FVector(0.235f, 0.215f, 0.05f), ChefWhite);
	AddPart(this, ChefRoot, Sphere(), FVector(-3.f, 0.f, 199.f), FRotator::ZeroRotator, FVector(0.22f, 0.2f, 0.16f), Lit(this, FLinearColor(0.02f, 0.015f, 0.01f), 0.6f));
}

void ARestaurantDome::BuildDock()
{
	USceneComponent* Root = GetRootComponent();
	const int32 First = Root->GetNumChildrenComponents();
	BuildDockParts();
	const float Shift = DockShift();
	if (Shift > 0.f)
	{
		for (int32 c = First; c < Root->GetNumChildrenComponents(); ++c)
		{
			if (USceneComponent* Ch = Root->GetChildComponent(c)) Ch->SetRelativeLocation(Ch->GetRelativeLocation() + FVector(Shift, 0.f, 0.f));
		}
	}
}

void ARestaurantDome::BuildDockParts()
{
	using namespace AbyssAssets;
	USceneComponent* Root = GetRootComponent();

	UMaterialInterface* Hull   = Lit(this, FLinearColor(0.10f, 0.12f, 0.13f), 0.6f, 0.7f);
	UMaterialInterface* Bronze = Lit(this, FLinearColor(0.28f, 0.19f, 0.10f), 0.35f, 0.9f);
	UMaterialInterface* Window = Lit(this, FLinearColor(1.f, 0.75f, 0.45f), 0.3f, 0.f, 8.f);
	UMaterialInterface* Dark   = Lit(this, FLinearColor(0.01f, 0.01f, 0.012f), 0.8f);
	UMaterialInterface* Strip  = Lit(this, FLinearColor(0.8f, 0.9f, 1.f), 0.3f, 0.f, 12.f);
	UMaterialInterface* Hazard = Lit(this, FLinearColor(1.f, 0.7f, 0.05f), 0.5f, 0.f, 2.f);
	UMaterialInterface* CyanM  = Lit(this, FLinearColor(0.2f, 0.9f, 1.f), 0.3f, 0.f, 8.f);

	// Pressurised tunnel from dome to dock
	AddPart(this, Root, Cylinder(), FVector(1650.f, 0.f, 250.f), FRotator(90.f, 0.f, 0.f), FVector(2.6f, 2.6f, 13.f), Hull, true);
	for (int32 i = 0; i < 6; ++i)
	{
		const float X = 1180.f + i * 190.f;
		AddPart(this, Root, Cylinder(), FVector(X, 0.f, 250.f), FRotator(90.f, 0.f, 0.f), FVector(2.78f, 2.78f, 0.14f), Bronze);
		for (float Y : { 128.f, -128.f })
		{
			AddPart(this, Root, Cube(), FVector(X + 95.f, Y, 260.f), FRotator::ZeroRotator, FVector(1.1f, 0.04f, 0.35f), Window);
		}
	}
	for (int32 i = 0; i < 4; ++i)
	{
		AddPart(this, Root, Cylinder(), FVector(1250.f + i * 300.f, 0.f, 60.f), FRotator::ZeroRotator, FVector(0.5f, 0.5f, 2.4f), Hull, true);
	}

	// Docking bay building
	AddPart(this, Root, Cube(), FVector(2650.f, 0.f, 300.f), FRotator::ZeroRotator, FVector(9.f, 8.f, 6.f), Hull, true);
	AddPart(this, Root, Cube(), FVector(2650.f, 0.f, 625.f), FRotator::ZeroRotator, FVector(7.f, 6.f, 0.5f), Hull, true);
	AddPart(this, Root, Cube(), FVector(2650.f, 402.f, 240.f), FRotator::ZeroRotator, FVector(6.2f, 0.06f, 3.8f), Dark);
	AddPart(this, Root, Cube(), FVector(2650.f, 405.f, 445.f), FRotator::ZeroRotator, FVector(6.4f, 0.06f, 0.08f), Strip);
	for (int32 i = 0; i < 9; ++i)
	{
		AddPart(this, Root, Cube(), FVector(2340.f + i * 78.f, 405.f, 40.f), FRotator(45.f, 0.f, 0.f), FVector(0.06f, 0.06f, 0.55f), Hazard);
	}
	AddText(this, Root, TEXT("DOCKING BAY  A-01"), FVector(2650.f, 410.f, 530.f), 90.f, 75.f, FColor(255, 205, 70));
	for (float X : { 2350.f, 2950.f })
	{
		AddPart(this, Root, Cylinder(), FVector(X, -150.f, 820.f), FRotator::ZeroRotator, FVector(0.08f, 0.08f, 3.6f), Bronze);
		AddPart(this, Root, Sphere(), FVector(X, -150.f, 1000.f), FRotator::ZeroRotator, FVector(0.14f), Lit(this, FLinearColor(1.f, 0.15f, 0.05f), 0.3f, 0.f, 25.f));
	}

	// Landing pad where the sub parks
	const FVector Pad(DockSpawnLocal.X, DockSpawnLocal.Y, 8.f);
	AddPart(this, Root, Cylinder(), Pad, FRotator::ZeroRotator, FVector(7.4f, 7.4f, 0.16f), Lit(this, FLinearColor(0.05f, 0.055f, 0.06f), 0.5f, 0.6f), true);
	AddPart(this, Root, Cylinder(), Pad + FVector(0.f, 0.f, 2.f), FRotator::ZeroRotator, FVector(7.6f, 7.6f, 0.1f), CyanM);
	AddPart(this, Root, Cylinder(), Pad + FVector(0.f, 0.f, 4.f), FRotator::ZeroRotator, FVector(7.2f, 7.2f, 0.12f), Lit(this, FLinearColor(0.05f, 0.055f, 0.06f), 0.5f, 0.6f));
	AddPointLight(this, Root, Pad + FVector(0.f, -300.f, 700.f), FLinearColor(0.85f, 0.95f, 1.f), 700.f, 2200.f, true);
}

void ARestaurantDome::UpdateTank(float Dt)
{
	for (int32 i = 0; i < TankFishData.Num(); ++i)
	{
		FTankFish& F = TankFishData[i];
		F.Angle += Dt * F.Speed;
		const FVector P(FMath::Cos(F.Angle) * F.Radius, FMath::Sin(F.Angle) * F.Radius, F.Height + FMath::Sin(Time * 0.7f + i) * 20.f);
		const FVector Tangent = FVector(-FMath::Sin(F.Angle), FMath::Cos(F.Angle), 0.f) * FMath::Sign(F.Speed);
		TankT[i] = FTransform(Tangent.Rotation(), P, FVector(0.9f, 0.25f, 0.35f) * F.Scale);
	}
}

void ARestaurantDome::BeginNight(int32 NumCustomers)
{
	for (ARestaurantCustomer* C : Customers)
	{
		if (IsValid(C)) C->Destroy();
	}
	Customers.Reset();
	for (FDomeSeat& S : Seats) S.Occupant = nullptr;
	Cooking = nullptr;
	Kitchen.Cancel();
	CarryFor = nullptr;
	CarryDish = -1;
	Counter.Reset();
	NightClock = 0.f;
	ToSpawn = NumCustomers;
	SpawnTimer = 1.f;
	ResultTime = 0.f;
	bNight = true;
}

void ARestaurantDome::EndNight()
{
	bNight = false;
	Counter.Reset();
	Cooking = nullptr;
}

int32 ARestaurantDome::PickDish() const
{
	const AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	if (!GM || FMath::FRand() < 0.12f) return AbyssData::SaladDish;

	int32 Avail[AbyssData::NumFish];
	int32 Total = 0;
	for (int32 i = 0; i < AbyssData::NumFish; ++i)
	{
		Avail[i] = GM->Stock[i];
	}
	for (const ARestaurantCustomer* C : Customers)
	{
		const int32 Fish = IsValid(C) ? AbyssData::DishFish(C->Dish) : -1;
		if (Fish >= 0 && (C->IsWaitingForFood() || C->WantsToOrder())) Avail[Fish]--;
	}
	for (int32 i = 0; i < AbyssData::NumFish; ++i)
	{
		Avail[i] = FMath::Max(Avail[i], 0);
		Total += Avail[i];
	}
	if (Total == 0) return AbyssData::SaladDish;
	int32 Pick = FMath::RandRange(0, Total - 1);
	for (int32 i = 0; i < AbyssData::NumFish; ++i)
	{
		// yellowtail goes into two dishes: nigiri (knife work) or the kelp stew (timing the pot)
		if (Pick < Avail[i]) return i == (int32)EFishType::Yellowtail && FMath::FRand() < 0.45f ? AbyssData::StewDish : i;
		Pick -= Avail[i];
	}
	return AbyssData::SaladDish;
}

void ARestaurantDome::SpawnCustomer()
{
	if (bLuxLayout && !bLuxSeatsGathered) GatherLuxSeats();
	TArray<int32> Free;
	for (int32 i = 0; i < Seats.Num(); ++i)
	{
		if (!Seats[i].Occupant.IsValid() && Seats[i].Tier <= CurrentTier) Free.Add(i);
	}
	if (Free.Num() == 0) return;

	const int32 SeatIdx = Free[FMath::RandRange(0, Free.Num() - 1)];
	const FDomeSeat& S = Seats[SeatIdx];
	const FTransform T = FTransform(FRotator(0.f, S.Yaw, 0.f), S.Loc) * GetActorTransform();

	ARestaurantCustomer* C = GetWorld()->SpawnActor<ARestaurantCustomer>(ARestaurantCustomer::StaticClass(), T);
	if (!C) return;
	C->Init(this, SeatIdx, PickDish(), GetActorTransform().TransformPosition(S.Table), BasePatience + FMath::FRandRange(0.f, 25.f), FMath::RandRange(0, 17));
	// most guests follow tonight's craving, some have their own taste, a few don't care
	{
		const AAbyssGameMode* GMT = AAbyssGameMode::Get(this);
		const float Roll = FMath::FRand();
		C->Taste = Roll < 0.55f && GMT ? GMT->TonightTaste : Roll < 0.88f ? (ETaste)FMath::RandRange((int32)ETaste::Salty, (int32)ETaste::Fresh) : ETaste::None;
	}
	Seats[SeatIdx].Occupant = C;
	Customers.Add(C);
	--ToSpawn;
}

void ARestaurantDome::GetQueue(TArray<ARestaurantCustomer*>& Out) const
{
	Out.Reset();
	for (ARestaurantCustomer* C : Customers)
	{
		if (IsValid(C) && C->IsWaitingForFood()) Out.Add(C);
	}
}

void ARestaurantDome::GetCustomers(TArray<ARestaurantCustomer*>& Out) const
{
	Out.Reset();
	for (ARestaurantCustomer* C : Customers)
	{
		if (IsValid(C)) Out.Add(C);
	}
}

void ARestaurantDome::SelectOrder(int32 QueueIndex)
{
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	if (!bNight || !GM) return;
	if (Kitchen.bActive)
	{
		GM->ShowToast(TEXT("Finish the current dish first"), FLinearColor(1.f, 0.8f, 0.3f), 1.5f);
		return;
	}
	if (IsCarrying())
	{
		GM->ShowToast(TEXT("Serve the dish in your hands first"), FLinearColor(1.f, 0.8f, 0.3f), 1.5f);
		return;
	}
	TArray<ARestaurantCustomer*> Queue;
	GetQueue(Queue);
	if (!Queue.IsValidIndex(QueueIndex)) return;

	ARestaurantCustomer* C = Queue[QueueIndex];
	if (AbyssData::DishFish(C->Dish) >= 0 && GM->Stock[AbyssData::DishFish(C->Dish)] <= 0)
	{
		GM->ShowToast(FString::Printf(TEXT("Out of %s! Catch more tomorrow."), AbyssData::Fish((EFishType)AbyssData::DishFish(C->Dish)).Name), FLinearColor(1.f, 0.4f, 0.3f));
		return;
	}
	Cooking = C;
	C->State = ECustomerState::Cooking;
	Kitchen.Start(C->Dish, C);
}

void ARestaurantDome::Cook()
{
	if (bNight && Kitchen.bActive)
	{
		Kitchen.PressAction();
	}
}

void ARestaurantDome::FinishDish()
{
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	ARestaurantCustomer* C = Kitchen.For.Get();
	const float Score = Kitchen.TotalScore();
	Kitchen.Cancel();
	Cooking = nullptr;
	if (!GM || !C) return;
	CompleteDish(C, Score, true);
}

void ARestaurantDome::CompleteDish(ARestaurantCustomer* C, float Score, bool bTakeFish, bool bPrime)
{
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	if (!GM || !C) return;

	// GDD: >=90 perfect x1.5, 70-89 good x1.0, <70 miss x0.6
	float Mult;
	if (Score >= 90.f)
	{
		Mult = 1.5f;
		ResultText = FString::Printf(TEXT("PERFECT!  %d"), FMath::RoundToInt(Score));
		ResultColor = FLinearColor(0.3f, 1.f, 0.4f);
		GM->NightPerfect++;
	}
	else if (Score >= 70.f)
	{
		Mult = 1.f;
		ResultText = FString::Printf(TEXT("GOOD  %d"), FMath::RoundToInt(Score));
		ResultColor = FLinearColor(1.f, 0.85f, 0.2f);
	}
	else
	{
		Mult = 0.6f;
		ResultText = FString::Printf(TEXT("MISS  %d"), FMath::RoundToInt(Score));
		ResultColor = FLinearColor(1.f, 0.35f, 0.25f);
	}
	ResultTime = 1.6f;

	if (bTakeFish) bPrime = GM->TakeFish(AbyssData::DishFish(C->Dish));
	// tonight's seasoning + garnish go on as the dish leaves the pass (if the pantry still has some)
	bCarryPrime = bPrime;
	CarryMain = GM->MenuMain[FMath::Clamp(C->Dish, 0, AbyssData::NumDishes - 1)];
	CarrySide = GM->MenuSide[FMath::Clamp(C->Dish, 0, AbyssData::NumDishes - 1)];
	if (CarryMain > 0 && !GM->UseSeason(CarryMain)) CarryMain = 0;
	if (CarrySide > 0 && !GM->UseSeason(CarrySide)) CarrySide = 0;
	CarryFor = C;
	CarryScore = Score;
	CarryMult = Mult;
	CarryDish = C->Dish;
	C->State = ECustomerState::Ready;
	GM->ShowToast(TEXT("Carry the dish to the guest - follow the green marker"), FLinearColor(0.5f, 1.f, 0.6f), 2.2f);
}

void ARestaurantDome::DeliverDish()
{
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	ARestaurantCustomer* C = CarryFor.Get();
	CarryFor = nullptr;
	if (!GM || !C) return;
	// catch quality + seasoning + garnish set the price; the cooking score scales it
	const int32 Price = GM->DishFinalPrice(CarryDish, bCarryPrime, CarryMain, CarrySide);
	const int32 Earned = FMath::RoundToInt(Price * CarryMult);
	// the guest's taste: a matching seasoning or garnish doubles the tip
	const bool bTaste = C->Taste != ETaste::None && (AbyssData::Season(CarryMain).Taste == C->Taste || AbyssData::Season(CarrySide).Taste == C->Taste);
	// served while the guest is still happy -> tip
	int32 Tip = FMath::RoundToInt(Price * 0.35f * C->Patience01() * (CarryScore >= 70.f ? 1.f : 0.3f));
	if (bTaste) Tip = Tip * 2 + FMath::RoundToInt(Price * 0.1f);
	GM->Money += Earned + Tip;
	GM->NightEarned += Earned + Tip;
	GM->NightServed++;
	GM->LifetimeServed++;
	if (bCarryPrime) GM->NightPrimeServed++;
	if (bTaste) GM->NightTasteHits++;
	GM->Satisfaction = FMath::Lerp(GM->Satisfaction, FMath::Min(1.f, CarryScore / 100.f + (bTaste ? 0.1f : 0.f)), 0.25f);
	{
		FString Msg = FString::Printf(TEXT("+%d  %s"), Earned, *AbyssData::DishName(CarryDish));
		if (bCarryPrime) Msg += TEXT("  [PRIME]");
		if (Tip > 0) Msg += FString::Printf(TEXT("   (tip +%d)"), Tip);
		if (bTaste) Msg += TEXT("  Just my taste!");
		GM->ShowToast(Msg, bTaste ? FLinearColor(0.6f, 1.f, 0.6f) : FLinearColor(1.f, 0.85f, 0.3f), 2.2f);
	}
	C->bTasteHit = bTaste;
	C->Serve(CarryScore / 100.f);
	CarryDish = -1;
}

void ARestaurantDome::TakeOrder(ARestaurantCustomer* C)
{
	if (!C || !C->WantsToOrder()) return;
	C->TakeOrder();
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this))
	{
		GM->ShowToast(FString::Printf(TEXT("Order taken: %s"), *AbyssData::DishName(C->Dish)), FLinearColor(1.f, 0.9f, 0.6f), 1.6f);
	}
}

void ARestaurantDome::DebugTakeAllOrders()
{
	for (ARestaurantCustomer* C : Customers)
	{
		if (IsValid(C) && C->WantsToOrder()) C->TakeOrder();
	}
}

void ARestaurantDome::OnCustomerGaveUp(ARestaurantCustomer* C)
{
	if (Cooking.Get() == C) Cooking = nullptr;
	if (CarryFor.Get() == C)
	{
		CarryFor = nullptr;
		CarryDish = -1;
	}
	if (Kitchen.bActive && Kitchen.For.Get() == C) Kitchen.Cancel();
	C->Leave(true);
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this))
	{
		GM->NightLeft++;
		GM->Satisfaction = FMath::Lerp(GM->Satisfaction, 0.f, 0.2f);
		GM->ShowToast(TEXT("A customer got tired of waiting and left!"), FLinearColor(1.f, 0.4f, 0.3f));
	}
}

void ARestaurantDome::OnCustomerLeft(ARestaurantCustomer* C)
{
	if (Seats.IsValidIndex(C->Seat)) Seats[C->Seat].Occupant = nullptr;
	Customers.Remove(C);
}

void ARestaurantDome::Tick(float Dt)
{
	Super::Tick(Dt);
	Time += Dt;

	UpdateTank(Dt);
	TankFish->BatchUpdateInstancesTransforms(0, TankT, false, true, true);
	if (bInteriorSpace)
	{
		SharkPivot->SetRelativeRotation(FRotator(0.f, Time * 9.f, 0.f));
		SharkPivot->SetRelativeLocation(FVector(0.f, 0.f, FMath::Sin(Time * 0.3f) * 120.f));
	}

	// --- performance: trim the streamed restaurant copies (the full showcase twice exhausted VRAM)
	if (!bLuxLightsTamed && bUseLuxLevel)
	{
		ULevel* Target = nullptr;
		if (!bInteriorSpace)
		{
			if (LuxExteriorLevel.IsValid()) Target = LuxExteriorLevel->GetLoadedLevel();
		}
		else
		{
			for (TActorIterator<AStaticMeshActor> It(GetWorld()); It && !Target; ++It)
			{
				if (FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) < Radius && It->GetLevel() != GetLevel()) Target = It->GetLevel();
			}
		}
		if (Target)
		{
			bLuxLightsTamed = true;
			int32 NL = 0, NHidden = 0, NCull = 0;
			for (AActor* A : Target->Actors)
			{
				if (!A) continue;
				TArray<ULightComponent*> Lights;
				A->GetComponents(Lights);
				for (ULightComponent* Lc : Lights)
				{
					if (!bInteriorSpace) Lc->SetVisibility(false);   // seen from outside the glass: the dome's own glow is enough
					else { Lc->SetCastShadows(false); Lc->SetVolumetricScatteringIntensity(0.f); }
					++NL;
				}
				TArray<UStaticMeshComponent*> Meshes;
				A->GetComponents(Meshes);
				for (UStaticMeshComponent* M : Meshes)
				{
					const float R = M->Bounds.SphereRadius;
					if (!bInteriorSpace)
					{
						M->SetCastShadow(false);
						if (R < 70.f) { M->SetVisibility(false); M->SetHiddenInGame(true); ++NHidden; }
					}
					else if (R < 40.f)
					{
						// tableware / bottles: only drawn up close, no shadows
						M->SetCachedMaxDrawDistance(1800.f);
						M->SetCastShadow(false);
						++NCull;
					}
				}
			}
			UE_LOG(LogTemp, Log, TEXT("Lux %s copy trimmed: %d lights, %d hidden, %d distance-culled"), bInteriorSpace ? TEXT("interior") : TEXT("exterior"), NL, NHidden, NCull);
		}
	}

	// jellyfish lamps drift gently
	if (!bJellyCached)
	{
		bJellyCached = true;
		TArray<UStaticMeshComponent*> Comps;
		GetComponents(Comps);
		for (UStaticMeshComponent* C : Comps)
		{
			if (C->ComponentHasTag(TEXT("JellyLamp"))) JellyLamps.Add({ C, C->GetRelativeLocation(), (float)C->GetRelativeRotation().Yaw });
		}
	}
	for (int32 j = 0; j < JellyLamps.Num(); ++j)
	{
		if (UStaticMeshComponent* C = JellyLamps[j].C.Get())
		{
			const float Ph = Time * 0.7f + j * 1.3f;
			C->SetRelativeLocationAndRotation(JellyLamps[j].Base + FVector(0.f, 0.f, FMath::Sin(Ph) * 8.f),
				FRotator(FMath::Sin(Ph * 0.8f) * 2.5f, JellyLamps[j].Yaw + Time * 6.f, FMath::Cos(Ph * 0.6f) * 2.5f));
			const float Pulse = 1.f + 0.04f * FMath::Sin(Ph * 2.f);
			C->SetRelativeScale3D(FVector(Pulse, Pulse, 2.f - Pulse));
		}
	}

	// Chef: wanders behind the counter, steps to the grill while cooking
	FVector CL = ChefRoot->GetRelativeLocation();
	if (bConceptLayout)
	{
		const float K = FMath::Max(Radius / 800.f, 1.f);
		CL.X = FMath::FInterpTo(CL.X, (90.f + FMath::Sin(Time * 0.45f) * 120.f) * K, Dt, 2.5f);
	}
	else
	{
		const float ChefY = Cooking.IsValid() ? -130.f : FMath::Sin(Time * 0.45f) * 140.f;
		CL.Y = FMath::FInterpTo(CL.Y, ChefY, Dt, 2.5f);
	}
	const float ChefSpeed = FMath::Abs(CL.X - ChefRoot->GetRelativeLocation().X) / FMath::Max(Dt, 1e-3f);
	if (!bChefRigTried && ChefMesh->GetSkinnedAsset() && ChefMesh->IsVisible())
	{
		bChefRigTried = true;
		ChefRig.Init(ChefMesh);
	}
	if (ChefRig.bReady)
	{
		// the assistant chef chops while orders are waiting, otherwise strolls behind the counter
		const float Walk = FMath::Clamp(ChefSpeed / 60.f, 0.f, 1.f);
		const float FaceYaw = (bConceptLayout ? 90.f : 0.f) + (Walk > 0.3f ? (CL.X > ChefRoot->GetRelativeLocation().X ? -90.f : 90.f) * (bConceptLayout ? 1.f : 0.f) : 0.f);
		ChefMesh->SetRelativeRotation(FMath::RInterpTo(ChefMesh->GetRelativeRotation(), FRotator(0.f, FaceYaw - 90.f, 0.f), Dt, 5.f));
		AbyssRig::Humanoid(ChefRig, Time, Walk, EChefWork::Chop, Walk < 0.3f ? 1.f : 0.f);
	}
	else
	{
		CL.Z = Cooking.IsValid() ? FMath::Abs(FMath::Sin(Time * 9.f)) * 3.f : 0.f;
	}
	ChefRoot->SetRelativeLocation(CL);

	const float Flicker = 0.75f + 0.25f * FMath::Sin(Time * 23.f) * FMath::Sin(Time * 7.3f);
	const float Heat = Cooking.IsValid() ? 1.f : 0.15f;
	GrillLight->SetIntensity((10.f + 140.f * Heat) * Flicker);
	if (FireMat) FireMat->SetScalarParameterValue(TEXT("Emissive"), (1.f + 12.f * Heat) * Flicker);

	ResultTime = FMath::Max(0.f, ResultTime - Dt);

	if (!bNight) return;

	if (bNight)
	{
		NightClock += Dt;
		Counter.Tick(Dt, this);
	}
	if (Kitchen.bActive)
	{
		if (AAbyssGameMode* GMk = AAbyssGameMode::Get(this))
		{
			Kitchen.bAtStation = GMk->IsAtStation(Kitchen.Step);
		}
		Kitchen.Tick(Dt);
		if (!Kitchen.For.IsValid()) Kitchen.Cancel();
		else if (Kitchen.IsDone()) FinishDish();
	}

	int32 Waiting = 0;
	for (ARestaurantCustomer* C : Customers)
	{
		if (IsValid(C) && (C->IsWaitingForFood() || C->State == ECustomerState::Arriving)) ++Waiting;
	}
	if (ToSpawn > 0 && Waiting < MaxConcurrentOrders)
	{
		SpawnTimer -= Dt;
		if (SpawnTimer <= 0.f)
		{
			SpawnCustomer();
			SpawnTimer = SpawnInterval * FMath::FRandRange(0.7f, 1.3f);
		}
	}

	Customers.RemoveAll([](const TObjectPtr<ARestaurantCustomer>& C) { return !IsValid(C); });
	if (ToSpawn <= 0 && Customers.Num() == 0)
	{
		EndNight();
		if (AAbyssGameMode* GM = AAbyssGameMode::Get(this))
		{
			GM->OnNightFinished();
		}
	}
}

void ARestaurantDome::BuildSurroundings()
{
	// The interior space floats in open water: give the windows something to look at.
	using namespace AbyssAssets;
	USceneComponent* Root = GetRootComponent();
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, -260.f), FRotator::ZeroRotator, FVector(90.f, 90.f, 4.f), Lit(this, FLinearColor(0.42f, 0.38f, 0.28f), 0.95f));

	FRandomStream R(77);
	auto Layer = [this, Root](UStaticMesh* Mesh, const FLinearColor& C, float E)
	{
		UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
		H->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		H->SetStaticMesh(Mesh);
		H->SetMaterial(0, Lit(this, C, 0.8f, 0.f, E));
		H->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		H->SetupAttachment(Root);
		H->RegisterComponent();
		return H;
	};
	// Ocean pack meshes around the windows if available
	{
		const TCHAR* PackMeshes[] = {
			TEXT("/Game/AlienOceanEnvironment/Meshes/Plants/SM_Coral_Large_A.SM_Coral_Large_A"),
			TEXT("/Game/AlienOceanEnvironment/Meshes/Plants/SM_Coral_Bush_A.SM_Coral_Bush_A"),
			TEXT("/Game/AlienOceanEnvironment/Meshes/Plants/LeafyPlants/SM_Kelp_Tall.SM_Kelp_Tall"),
			TEXT("/Game/AlienOceanEnvironment/Meshes/Rocks/SM_AlienStone_B_Moss.SM_AlienStone_B_Moss"),
			TEXT("/Game/AlienOceanEnvironment/Meshes/Plants/SM_AnemoneTree_A.SM_AnemoneTree_A"),
			TEXT("/Game/AlienOceanEnvironment/Meshes/Plants/SM_BrainPlant.SM_BrainPlant") };
		bool bAny = false;
		for (int32 m = 0; m < UE_ARRAY_COUNT(PackMeshes); ++m)
		{
			UStaticMesh* PM = LoadObject<UStaticMesh>(nullptr, PackMeshes[m], nullptr, LOAD_NoWarn | LOAD_Quiet);
			if (!PM) continue;
			bAny = true;
			UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
			H->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			H->SetStaticMesh(PM);
			H->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			H->SetupAttachment(Root);
			H->RegisterComponent();
			const FBoxSphereBounds PB = PM->GetBounds();
			for (int32 i = 0; i < 40; ++i)
			{
				const float A = R.FRandRange(0.f, 2.f * PI);
				const float Dd = R.FRandRange(Radius + 300.f, Radius + 3500.f);
				const float S = R.FRandRange(0.8f, m == 2 ? 2.4f : 1.6f);
				H->AddInstance(FTransform(FRotator(0.f, R.FRandRange(0.f, 360.f), 0.f), FVector(FMath::Cos(A) * Dd, FMath::Sin(A) * Dd, -60.f - (PB.Origin.Z - PB.BoxExtent.Z) * S), FVector(S)));
			}
		}
		if (bAny)
		{
			AddPointLight(this, Root, FVector(0.f, 0.f, Radius + 1500.f), FLinearColor(0.35f, 0.7f, 0.9f), 300.f, 6000.f);
			return;
		}
	}
	UHierarchicalInstancedStaticMeshComponent* Rocks = Layer(Sphere(), FLinearColor(0.2f, 0.22f, 0.22f), 0.f);
	UHierarchicalInstancedStaticMeshComponent* Pink = Layer(Cone(), FLinearColor(0.85f, 0.25f, 0.42f), 0.12f);
	UHierarchicalInstancedStaticMeshComponent* Orange = Layer(Sphere(), FLinearColor(0.9f, 0.45f, 0.15f), 0.1f);
	UHierarchicalInstancedStaticMeshComponent* Kelp = Layer(Cube(), FLinearColor(0.22f, 0.45f, 0.12f), 0.04f);
	for (int32 i = 0; i < 260; ++i)
	{
		const float A = R.FRandRange(0.f, 2.f * PI);
		const float D = R.FRandRange(Radius + 400.f, Radius + 3500.f);
		const FVector P(FMath::Cos(A) * D, FMath::Sin(A) * D, -60.f);
		const FRotator Yaw(0.f, R.FRandRange(0.f, 360.f), 0.f);
		switch (i % 4)
		{
		case 0: { const float S = R.FRandRange(2.f, 7.f); Rocks->AddInstance(FTransform(Yaw, P, FVector(S, S * 0.8f, S * 0.5f))); break; }
		case 1: { const float S = R.FRandRange(0.5f, 1.f), H = R.FRandRange(1.f, 2.5f); Pink->AddInstance(FTransform(Yaw, P + FVector(0, 0, H * 45.f), FVector(S, S, H))); break; }
		case 2: { const float S = R.FRandRange(1.f, 2.2f); Orange->AddInstance(FTransform(Yaw, P + FVector(0, 0, S * 15.f), FVector(S, S, S * 0.65f))); break; }
		default: { const float H = R.FRandRange(6.f, 13.f); Kelp->AddInstance(FTransform(Yaw, P + FVector(0, 0, H * 50.f), FVector(0.04f, R.FRandRange(0.25f, 0.5f), H))); break; }
		}
	}
	// Soft blue light from above so the windows never read as pure black
	AddPointLight(this, Root, FVector(0.f, 0.f, Radius + 1500.f), FLinearColor(0.35f, 0.7f, 0.9f), 300.f, 6000.f);
}

void ARestaurantDome::BuildShark()
{
	// A shark that circles past the windows
	using namespace AbyssAssets;
	UMaterialInterface* Skin = Lit(this, FLinearColor(0.22f, 0.25f, 0.28f), 0.5f, 0.1f);
	UMaterialInterface* Belly = Lit(this, FLinearColor(0.6f, 0.62f, 0.62f), 0.5f);
	const FVector C(Radius + 700.f, 0.f, 450.f);
	AddPart(this, SharkPivot, Sphere(), C, FRotator(0.f, 90.f, 0.f), FVector(3.6f, 0.9f, 1.0f), Skin);
	AddPart(this, SharkPivot, Sphere(), C + FVector(0.f, 0.f, -18.f), FRotator(0.f, 90.f, 0.f), FVector(3.0f, 0.75f, 0.6f), Belly);
	AddPart(this, SharkPivot, Cone(), C + FVector(0.f, -20.f, 70.f), FRotator(0.f, 90.f, 30.f), FVector(0.6f, 0.05f, 0.7f), Skin);
	AddPart(this, SharkPivot, Cone(), C + FVector(0.f, -210.f, 30.f), FRotator(0.f, 90.f, 50.f), FVector(0.6f, 0.05f, 0.9f), Skin);
	AddPart(this, SharkPivot, Cone(), C + FVector(0.f, -210.f, -30.f), FRotator(0.f, 90.f, 130.f), FVector(0.45f, 0.05f, 0.6f), Skin);
	AddPart(this, SharkPivot, Cone(), C + FVector(40.f, 40.f, -30.f), FRotator(0.f, 0.f, 100.f), FVector(0.4f, 0.04f, 0.6f), Skin);
	AddPart(this, SharkPivot, Cone(), C + FVector(-40.f, 40.f, -30.f), FRotator(0.f, 0.f, -100.f), FVector(0.4f, 0.04f, 0.6f), Skin);
}

// ---------------------------------------------------------------- Luxury Restaurant Pack interior + expansion tiers

namespace
{
	// props modeled in Blender (Scripts/blender_make_props.py)
	UStaticMesh* Prop(const TCHAR* Name)
	{
		const FString P = FString::Printf(TEXT("/Game/Abyss/Props/%s.%s"), Name, Name);
		return LoadObject<UStaticMesh>(nullptr, *P, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	void SetSlot(UStaticMeshComponent* C, const TCHAR* Slot, UMaterialInterface* M)
	{
		if (C && M && C->GetMaterialIndex(FName(Slot)) != INDEX_NONE) C->SetMaterialByName(FName(Slot), M);
	}
	UStaticMesh* Lux(const TCHAR* Name)
	{
		const FString P = FString::Printf(TEXT("/Game/Luxury_Restaurant_Pack/Meshes/%s.%s"), Name, *FPaths::GetBaseFilename(Name));
		return LoadObject<UStaticMesh>(nullptr, *P, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	void TagTier(USceneComponent* C, int32 Tier, bool bLockMarker = false)
	{
		if (!C) return;
		if (bLockMarker) C->ComponentTags.Add(FName(*FString::Printf(TEXT("Lock%d"), Tier)));
		else if (Tier > 0) C->ComponentTags.Add(FName(*FString::Printf(TEXT("Tier%d"), Tier)));
	}
}

void ARestaurantDome::ApplyTier(int32 Tier)
{
	CurrentTier = Tier;
	TArray<USceneComponent*> Comps;
	GetComponents<USceneComponent>(Comps);
	for (USceneComponent* C : Comps)
	{
		for (int32 t = 1; t <= 2; ++t)
		{
			const bool bTierPart = C->ComponentHasTag(FName(*FString::Printf(TEXT("Tier%d"), t)));
			const bool bLockPart = C->ComponentHasTag(FName(*FString::Printf(TEXT("Lock%d"), t)));
			if (!bTierPart && !bLockPart) continue;
			const bool bShow = bTierPart ? Tier >= t : Tier < t;
			C->SetVisibility(bShow);
			if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(C))
			{
				if (P->GetCollisionEnabled() != ECollisionEnabled::NoCollision || P->ComponentHasTag(TEXT("Solid")))
				{
					P->ComponentTags.AddUnique(TEXT("Solid"));
					P->SetCollisionEnabled(bShow ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
				}
			}
		}
	}
}

bool ARestaurantDome::BuildLuxuryInterior()
{
	using namespace AbyssAssets;
	UStaticMesh* TableM = Lux(TEXT("SM_LUX_Table_Round_02"));
	UStaticMesh* ChairM = Lux(TEXT("SM_LUX_Chair_01"));
	if (!TableM || !ChairM) return false;
	USceneComponent* Root = GetRootComponent();

	auto Place = [this, Root](UStaticMesh* M, const FVector& Loc, float Yaw, float Scale = 1.f, bool bCollide = false, int32 Tier = 0) -> UStaticMeshComponent*
	{
		if (!M) return nullptr;
		UStaticMeshComponent* C = AbyssAssets::AddPart(this, Root, M, Loc, FRotator(0.f, Yaw, 0.f), FVector(Scale), nullptr, bCollide);
		TagTier(C, Tier);
		return C;
	};

	// aquarium + chandelier + base lights from the procedural build (seats are rebuilt below)
	BuildInterior(false);
	Seats.Reset();

	// --- floor tiles (2x2 m) inside the glass
	if (UStaticMesh* Floor = Lux(TEXT("SM_LUX_Floor_2X2")))
	{
		const float Lim = Radius - 60.f;
		for (int32 ix = -8; ix < 8; ++ix)
		{
			for (int32 iy = -8; iy < 8; ++iy)
			{
				const float X0 = ix * 200.f, Y0 = iy * 200.f;
				bool bIn = true;
				for (const FVector2D& Cn : { FVector2D(X0, Y0), FVector2D(X0 + 200.f, Y0), FVector2D(X0, Y0 + 200.f), FVector2D(X0 + 200.f, Y0 + 200.f) })
				{
					if (Cn.Size() > Lim) bIn = false;
				}
				if (!bIn) continue;
				if (FVector2D(X0 + 100.f, Y0 + 100.f).Size() < 260.f) continue; // aquarium base
				Place(Floor, FVector(X0, Y0, 5.f), 0.f);
			}
		}
	}

	// --- kitchen bar (teppanyaki counter): 3 bar segments along Y at x = -560
	UStaticMesh* BarMid = Lux(TEXT("SM_LUX_Bar_Middle"));
	for (int32 k = 0; k < 3; ++k)
	{
		Place(BarMid, FVector(-545.f, -300.f + k * 200.f, 0.f), 90.f, 1.f, true);
	}
	const float BarTop = 122.f;
	AddPart(this, Root, Cube(), FVector(-560.f, -130.f, BarTop + 2.f), FRotator::ZeroRotator, FVector(0.9f, 1.3f, 0.03f), Lit(this, FLinearColor(0.2f, 0.2f, 0.2f), 0.3f, 1.f));
	FireMat = Lit(this, FLinearColor(1.f, 0.35f, 0.05f), 0.5f, 0.f, 6.f);
	AddPart(this, Root, Cube(), FVector(-560.f, -130.f, BarTop + 5.f), FRotator::ZeroRotator, FVector(0.6f, 0.9f, 0.02f), FireMat);
	GrillLight->SetRelativeLocation(FVector(-560.f, -130.f, BarTop + 50.f));
	Place(Lux(TEXT("SM_LUX_Coffee_Machine")), FVector(-560.f, 200.f, BarTop), 90.f);
	for (int32 k = 0; k < 4; ++k)
	{
		Place(Lux(TEXT("SM_LUX_Glass_02")), FVector(-555.f, 80.f + k * 18.f, BarTop), 0.f);
	}
	// back wall: shelves with bottles, drinks cooler
	UStaticMesh* Shelf = Lux(TEXT("SM_LUX_Shelf_01"));
	const TCHAR* BottleNames[] = { TEXT("SM_LUX_Bottle_01"), TEXT("SM_LUX_Bottle_02"), TEXT("SM_LUX_Bottle_05"), TEXT("SM_LUX_Bottle_06"), TEXT("SM_LUX_Bottle_07") };
	for (int32 k = 0; k < 3; ++k)
	{
		const float Y = -260.f + k * 170.f;
		Place(Shelf, FVector(-900.f, Y, 0.f), 90.f, 1.f, true);
		for (int32 b = 0; b < 6; ++b)
		{
			Place(Lux(BottleNames[(k + b) % 5]), FVector(-900.f, Y - 50.f + b * 20.f, 150.f), 0.f);
		}
	}
	Place(Lux(TEXT("SM_LUX_Cooler")), FVector(-880.f, 330.f, 0.f), 90.f, 1.f, true);
	BuildChef();

	// --- dining tables
	UStaticMesh* Candle = Lux(TEXT("SM_LUX_Candle_Holder_02"));
	UStaticMesh* Plate = Lux(TEXT("SM_LUX_Plate_Main_Meal"));
	UStaticMesh* Napkin = Lux(TEXT("SM_LUX_Napkin_01"));
	UStaticMesh* Rod = Lux(TEXT("SM_LUX_Ceiling_Decor"));
	UStaticMesh* Spot = Lux(TEXT("SM_LUX_Light_Spot_01"));
	UStaticMesh* Rail = Lux(TEXT("SM_LUX_Railing_Mid"));
	const float LS = LightScale;

	auto Table = [&](const FVector& Tab, int32 Tier)
	{
		Place(TableM, Tab, 0.f, 1.15f, true, Tier);
		Place(Candle, Tab + FVector(0.f, 0.f, 78.f * 1.15f), 0.f, 1.f, false, Tier);
		Place(Rod, Tab + FVector(0.f, 0.f, 560.f), 0.f, 1.f, false, Tier);
		Place(Spot, Tab + FVector(0.f, 0.f, 322.f), 0.f, 1.2f, false, Tier);
		UPointLightComponent* PL = AddPointLight(this, Root, Tab + FVector(0.f, 0.f, 230.f), FLinearColor(1.f, 0.74f, 0.45f), 40.f * LS, 520.f);
		TagTier(PL, Tier);
		const FVector Radial = Tab.GetSafeNormal2D();
		const FVector Tang(-Radial.Y, Radial.X, 0.f);
		for (float Side : { 1.f, -1.f })
		{
			const FVector SeatLoc = Tab + Tang * Side * 95.f;
			const float Yaw = (Tab - SeatLoc).Rotation().Yaw;
			Place(ChairM, SeatLoc, Yaw + ChairYawOffset, 1.f, false, Tier);
			Place(Plate, FMath::Lerp(SeatLoc, Tab, 0.62f) + FVector(0.f, 0.f, 78.f * 1.15f + 1.f), 0.f, 1.f, false, Tier);
			Place(Napkin, FMath::Lerp(SeatLoc, Tab, 0.5f) + Tang * 20.f + FVector(0.f, 0.f, 78.f * 1.15f + 1.f), Yaw, 1.f, false, Tier);
			FDomeSeat S;
			S.Loc = SeatLoc;
			S.Table = Tab;
			S.Yaw = Yaw;
			S.Tier = Tier;
			Seats.Add(S);
		}
		if (Tier > 0)
		{
			// "under construction" railing in front of locked tables
			const FVector Front = Tab - Radial * 170.f;
			UStaticMeshComponent* R1 = AbyssAssets::AddPart(this, Root, Rail, Front - Tang * 200.f, FRotator(0.f, Tang.Rotation().Yaw, 0.f), FVector(1.f), nullptr, false);
			UStaticMeshComponent* R2 = AbyssAssets::AddPart(this, Root, Rail, Front, FRotator(0.f, Tang.Rotation().Yaw, 0.f), FVector(1.f), nullptr, false);
			TagTier(R1, Tier, true);
			TagTier(R2, Tier, true);
		}
	};

	const float R0 = 820.f;
	for (float Deg : { -35.f, 5.f, 45.f, 85.f })     Table(FRotator(0.f, Deg, 0.f).Vector() * R0, 0);
	for (float Deg : { -75.f, -115.f, 125.f })      Table(FRotator(0.f, Deg, 0.f).Vector() * R0, 1);

	// --- VIP booths (tier 2): wall seat + rectangular table near the glass
	UStaticMesh* Booth = Lux(TEXT("SM_LUX_Seat_Wall"));
	UStaticMesh* Rect = Lux(TEXT("SM_LUX_Table_Rectangle_01"));
	UStaticMesh* Lamp = Lux(TEXT("SM_LUX_Lamp_Floor"));
	for (float Deg : { -15.f, 25.f, 65.f })
	{
		const FVector Dir = FRotator(0.f, Deg, 0.f).Vector();
		const FVector BoothLoc = Dir * (Radius - 230.f);
		const FVector Tab = Dir * (Radius - 360.f);
		const float FaceIn = (-Dir).Rotation().Yaw;
		Place(Booth, BoothLoc, FaceIn + 90.f, 1.f, true, 2);
		Place(Rect, Tab, FaceIn + 90.f, 1.f, true, 2);
		Place(Candle, Tab + FVector(0.f, 0.f, 78.f), 0.f, 1.f, false, 2);
		Place(Lamp, BoothLoc + FVector(-Dir.Y, Dir.X, 0.f) * 150.f, 0.f, 1.2f, false, 2);
		UPointLightComponent* PL = AddPointLight(this, Root, Tab + FVector(0.f, 0.f, 200.f), FLinearColor(1.f, 0.62f, 0.35f), 45.f * LS, 500.f);
		TagTier(PL, 2);
		const FVector Tang(-Dir.Y, Dir.X, 0.f);
		for (float Side : { 1.f, -1.f })
		{
			FDomeSeat S;
			S.Loc = BoothLoc + Tang * Side * 45.f - Dir * 10.f;
			S.Table = Tab;
			S.Yaw = (Tab - S.Loc).Rotation().Yaw;
			S.Tier = 2;
			Seats.Add(S);
		}
		const FVector Front = Tab - Dir * 150.f;
		UStaticMeshComponent* R1 = AbyssAssets::AddPart(this, Root, Rail, Front - Tang * 100.f, FRotator(0.f, Tang.Rotation().Yaw, 0.f), FVector(1.f), nullptr, false);
		TagTier(R1, 2, true);
	}

	// --- plants along the glass (half come with the first expansion)
	UStaticMesh* PlantM = Lux(TEXT("SM_LUX_Plant_01"));
	int32 n = 0;
	for (float Deg = -140.f; Deg <= 140.f; Deg += 28.f, ++n)
	{
		if (FMath::Abs(Deg) < 100.f && FMath::Abs(Deg) > 0.f && (int32)Deg % 2 == 0) {}
		Place(PlantM, FRotator(0.f, Deg, 0.f).Vector() * (Radius - 140.f), Deg, 1.1f, false, n % 2);
	}

	// --- entrance decor
	Place(Lux(TEXT("SM_LUX_Dresser")), FVector(Radius - 260.f, -420.f, 0.f), 90.f, 1.f, true);
	Place(Lux(TEXT("SM_LUX_Decor_01")), FVector(Radius - 260.f, 420.f, 0.f), 90.f);
	AddPointLight(this, Root, FVector(0.f, 0.f, 700.f), FLinearColor(1.f, 0.8f, 0.6f), 120.f * LS, Radius * 1.4f, true);
	return true;
}

// ---------------------------------------------------------------- concept-art layout (16 m dome, kitchen west, dining east, entrance south)
// Local axes: +X = "north" (top of the concept plan), +Y = "east" (right of the plan).
bool ARestaurantDome::BuildConceptInterior()
{
	using namespace AbyssAssets;
	// The plan is authored at an 8 m radius (BaseR) and spread out to the real Radius by K.
	const float BaseR = 800.f;
	const float K = FMath::Max(Radius / BaseR, 1.f);
	UStaticMesh* ChairM = Lux(TEXT("SM_LUX_Chair_01"));
	if (!ChairM) return false;
	bConceptLayout = true;
	USceneComponent* Root = GetRootComponent();
	const int32 FirstChild = Root->GetNumChildrenComponents();
	TankFish->SetVisibility(false);
	Seats.Reset();
	const float LS = LightScale;

	auto Place = [this, Root](UStaticMesh* M, const FVector& Loc, float Yaw, const FVector& Scale, UMaterialInterface* Mat = nullptr, bool bCollide = false, int32 Tier = 0) -> UStaticMeshComponent*
	{
		if (!M) return nullptr;
		UStaticMeshComponent* C = AbyssAssets::AddPart(this, Root, M, Loc, FRotator(0.f, Yaw, 0.f), Scale, Mat, bCollide);
		TagTier(C, Tier);
		return C;
	};
	// stylized-realistic palette from the concept: deep blue, aqua, cream, warm orange, coral
	UMaterialInterface* Wood     = Lit(this, FLinearColor(0.32f, 0.17f, 0.08f), 0.45f);
	UMaterialInterface* DarkWood = Lit(this, FLinearColor(0.14f, 0.07f, 0.035f), 0.4f);
	UMaterialInterface* Cream    = Lit(this, FLinearColor(0.85f, 0.8f, 0.68f), 0.55f);
	UMaterialInterface* Steel    = Lit(this, FLinearColor(0.55f, 0.56f, 0.58f), 0.25f, 1.f);
	UMaterialInterface* DeepBlue = Lit(this, FLinearColor(0.04f, 0.1f, 0.32f), 0.7f);
	UMaterialInterface* Aqua     = Lit(this, FLinearColor(0.2f, 0.75f, 0.9f), 0.4f, 0.f, 0.8f);
	UMaterialInterface* Glow     = Lit(this, FLinearColor(1.f, 0.62f, 0.3f), 0.4f, 0.f, 4.f);
	UMaterialInterface* Jelly    = Lit(this, FLinearColor(1.f, 0.72f, 0.55f), 0.3f, 0.f, 3.f);
	UMaterialInterface* Ice      = AbyssAssets::Glass(this, FLinearColor(0.75f, 0.92f, 1.f), 0.35f);
	UMaterialInterface* Red      = Lit(this, FLinearColor(0.85f, 0.18f, 0.08f), 0.45f);
	UMaterialInterface* Salmon   = Lit(this, FLinearColor(0.95f, 0.5f, 0.35f), 0.35f);
	UMaterialInterface* Coral    = Lit(this, FLinearColor(0.9f, 0.35f, 0.55f), 0.6f, 0.f, 0.4f);

	// --- floor: wood tiles + a decorative tile path from the entrance
	if (UStaticMesh* Floor = Lux(TEXT("SM_LUX_Floor_2X2")))
	{
		const float Lim = Radius - 40.f;
		const int32 N = FMath::CeilToInt(Radius / 200.f);
		for (int32 ix = -N; ix < N; ++ix)
			for (int32 iy = -N; iy < N; ++iy)
			{
				const float X0 = ix * 200.f, Y0 = iy * 200.f;
				bool bIn = true;
				for (const FVector2D& Cn : { FVector2D(X0, Y0), FVector2D(X0 + 200.f, Y0), FVector2D(X0, Y0 + 200.f), FVector2D(X0 + 200.f, Y0 + 200.f) })
					if (Cn.Size() > Lim) bIn = false;
				if (bIn) { if (UStaticMeshComponent* T = Place(Floor, FVector(X0, Y0, 5.f), 0.f, FVector(1.f))) T->ComponentTags.Add(TEXT("NoScale")); }
			}
	}
	for (int32 k = 0; k < 7; ++k) // path: alternating cream / blue tiles
	{
		Place(Cube(), FVector(-BaseR + 120.f + k * 110.f, 0.f, 6.f), 0.f, FVector(1.f * K, 1.6f, 0.02f), k % 2 ? DeepBlue : Cream);
	}

	// --- C-shaped serving counter (06) separating kitchen from dining
	const FVector2D ArcC(-100.f, -650.f);
	const float ArcR = 560.f;
	const int32 Segs = FMath::RoundToInt(10.f * K);
	for (int32 k = 0; k <= Segs; ++k)
	{
		const float A = FMath::DegreesToRadians(FMath::Lerp(25.f, 150.f, (float)k / Segs));
		const FVector P(ArcC.X + FMath::Cos(A) * ArcR, ArcC.Y + FMath::Sin(A) * ArcR, 0.f);
		const float Yaw = FMath::RadiansToDegrees(A) + 90.f;
		Place(Cube(), P + FVector(0.f, 0.f, 50.f), Yaw, FVector(1.15f, 0.55f, 1.0f), Wood, true);
		Place(Cube(), P + FVector(0.f, 0.f, 103.f), Yaw, FVector(1.2f, 0.7f, 0.06f), Cream);
		Place(Cube(), P + FVector(FMath::Cos(A) * 28.f, FMath::Sin(A) * 28.f, 55.f), Yaw, FVector(1.1f, 0.02f, 0.12f), Aqua); // glowing wave strip
	}
	// cash register on the counter
	{
		const float A = FMath::DegreesToRadians(110.f);
		const FVector P(ArcC.X + FMath::Cos(A) * ArcR, ArcC.Y + FMath::Sin(A) * ArcR, 106.f);
		Place(Cube(), P, 200.f, FVector(0.3f, 0.25f, 0.08f), DarkWood);
		Place(Cube(), P + FVector(0.f, 0.f, 22.f), 200.f, FVector(0.02f, 0.3f, 0.22f), Lit(this, FLinearColor(0.3f, 0.8f, 1.f), 0.3f, 0.f, 3.f));
	}

	// --- kitchen: 01 prep table, 02 stove, 03 storage
	// 01 prep: steel table with cutting boards and fish
	Place(Cube(), FVector(-250.f, -580.f, 45.f), 0.f, FVector(2.2f, 0.8f, 0.9f), Steel, true);
	Place(Cube(), FVector(-290.f, -580.f, 92.f), 10.f, FVector(0.6f, 0.4f, 0.03f), Wood);
	Place(Cube(), FVector(-180.f, -580.f, 92.f), -8.f, FVector(0.6f, 0.4f, 0.03f), Wood);
	Place(Sphere(), FVector(-290.f, -580.f, 98.f), 10.f, FVector(0.5f, 0.15f, 0.12f), Salmon);
	// 02 stove: counter + teppan + pots + hood
	Place(Cube(), FVector(90.f, -620.f, 45.f), 0.f, FVector(2.4f, 0.9f, 0.9f), Steel, true);
	Place(Cube(), FVector(40.f, -620.f, 92.f), 0.f, FVector(1.0f, 0.75f, 0.03f), Lit(this, FLinearColor(0.08f, 0.08f, 0.08f), 0.3f, 1.f));
	FireMat = Lit(this, FLinearColor(1.f, 0.35f, 0.05f), 0.5f, 0.f, 6.f);
	Place(Cube(), FVector(40.f, -620.f, 95.f), 0.f, FVector(0.7f, 0.55f, 0.02f), FireMat);
	Place(Cylinder(), FVector(170.f, -620.f, 108.f), 0.f, FVector(0.35f, 0.35f, 0.3f), Steel);
	Place(Cylinder(), FVector(170.f, -620.f, 125.f), 0.f, FVector(0.38f, 0.38f, 0.03f), Lit(this, FLinearColor(0.7f, 0.4f, 0.2f), 0.3f, 1.f));
	Place(Cube(), FVector(90.f, -650.f, 260.f), 0.f, FVector(2.4f, 0.9f, 0.5f), Steel);
	for (int32 k = 0; k < 5; ++k) // hanging pans
		Place(Cylinder(), FVector(-30.f + k * 50.f, -700.f, 190.f), 0.f, FVector(0.22f, 0.22f, 0.03f), Steel);
	// 03 storage: shelves with jars & bottles
	UStaticMesh* Shelf = Lux(TEXT("SM_LUX_Shelf_01"));
	const TCHAR* BottleNames[] = { TEXT("SM_LUX_Bottle_01"), TEXT("SM_LUX_Bottle_02"), TEXT("SM_LUX_Bottle_05"), TEXT("SM_LUX_Bottle_06") };
	for (int32 k = 0; k < 2; ++k)
	{
		const FVector P(380.f + k * 150.f, -560.f + k * 90.f, 0.f);
		Place(Shelf, P, 30.f + k * 15.f, FVector(1.f), nullptr, true);
		for (int32 b = 0; b < 5; ++b) Place(Lux(BottleNames[b % 4]), P + FVector(-40.f + b * 20.f, 0.f, 150.f), 0.f, FVector(1.f));
	}
	UPointLightComponent* KL = AddPointLight(this, Root, FVector(0.f, -560.f, 320.f), FLinearColor(1.f, 0.85f, 0.65f), 90.f * LS, 900.f, true);
	(void)KL;

	// --- 07 seafood display case near the entrance
	{
		const FVector P(-560.f, -330.f, 0.f);
		Place(Cube(), P + FVector(0.f, 0.f, 40.f), 35.f, FVector(1.6f, 0.8f, 0.8f), Wood, true);
		Place(Cube(), P + FVector(0.f, 0.f, 82.f), 35.f, FVector(1.5f, 0.7f, 0.05f), Cream);
		Place(Cube(), P + FVector(0.f, 0.f, 112.f), 35.f, FVector(1.5f, 0.7f, 0.6f), Ice);
		for (int32 k = 0; k < 4; ++k)
		{
			const FVector F = P + FRotator(0.f, 35.f, 0.f).RotateVector(FVector(-55.f + k * 36.f, 0.f, 92.f));
			Place(Sphere(), F, 35.f + k * 20.f, FVector(0.45f, 0.14f, 0.12f), k % 2 ? Red : Salmon);
		}
		Place(Cube(), P + FVector(0.f, 0.f, 60.f) + FRotator(0.f, 35.f, 0.f).RotateVector(FVector(0.f, 42.f, 0.f)), 35.f, FVector(1.5f, 0.02f, 0.1f), Aqua);
	}

	// --- 04 dining: 4 round tables with cloths, rugs, 4 chairs, jellyfish lamps
	UStaticMesh* TableM = Lux(TEXT("SM_LUX_Table_Round_02"));
	UStaticMesh* Candle = Lux(TEXT("SM_LUX_Candle_Holder_02"));
	UStaticMesh* Plate = Lux(TEXT("SM_LUX_Plate_Main_Meal"));
	UStaticMesh* Rail = Lux(TEXT("SM_LUX_Railing_Mid"));
	UStaticMesh* JellyM = Prop(TEXT("SM_JellyLamp"));
	UMaterialInterface* JellyBell = AbyssAssets::Glass(this, FLinearColor(1.f, 0.55f, 0.4f), 0.55f);
	UMaterialInterface* JellyCore = Lit(this, FLinearColor(1.f, 0.6f, 0.35f), 0.3f, 0.f, 3.5f);
	UMaterialInterface* JellyTent = Lit(this, FLinearColor(1.f, 0.62f, 0.5f), 0.35f, 0.f, 1.6f);
	auto Jellyfish = [&](const FVector& At, int32 Tier)
	{
		if (JellyM)
		{
			UStaticMeshComponent* J = Place(JellyM, At, FMath::Fmod(At.X * 7.f + At.Y * 3.f, 360.f), FVector(1.f), JellyTent, false, Tier);
			SetSlot(J, TEXT("Bell"), JellyBell);
			SetSlot(J, TEXT("Glow"), JellyCore);
			if (J) J->ComponentTags.Add(TEXT("JellyLamp"));
			Place(Cylinder(), At + FVector(0.f, 0.f, 160.f), 0.f, FVector(0.02f, 0.02f, 2.6f), DarkWood, false, Tier); // cord
			UPointLightComponent* PL = AddPointLight(this, Root, At - FVector(0.f, 0.f, 10.f), FLinearColor(1.f, 0.7f, 0.5f), 55.f * LS, 520.f);
			TagTier(PL, Tier);
			return;
		}
		Place(Sphere(), At, 0.f, FVector(0.9f, 0.9f, 0.55f), Jelly, false, Tier);                        // bell
		Place(Cylinder(), At + FVector(0.f, 0.f, 140.f), 0.f, FVector(0.02f, 0.02f, 2.6f), DarkWood, false, Tier); // cord
		for (int32 t = 0; t < 8; ++t)
		{
			const float A = 2.f * PI * t / 8.f;
			Place(Cylinder(), At + FVector(FMath::Cos(A) * 25.f, FMath::Sin(A) * 25.f, -45.f - (t % 3) * 8.f), 0.f, FVector(0.03f, 0.03f, 0.7f + (t % 3) * 0.2f), Jelly, false, Tier);
		}
		UPointLightComponent* PL = AddPointLight(this, Root, At - FVector(0.f, 0.f, 30.f), FLinearColor(1.f, 0.7f, 0.5f), 55.f * LS, 520.f);
		TagTier(PL, Tier);
	};
	auto Table = [&](const FVector& Tab, int32 Tier)
	{
		Place(Cylinder(), Tab + FVector(0.f, 0.f, 6.5f), 0.f, FVector(3.4f, 3.4f, 0.01f), DeepBlue, false, Tier);   // rug
		Place(Cylinder(), Tab + FVector(0.f, 0.f, 7.f), 0.f, FVector(2.9f, 2.9f, 0.012f), Aqua, false, Tier);      // rug ring
		Place(Cylinder(), Tab + FVector(0.f, 0.f, 7.5f), 0.f, FVector(2.75f, 2.75f, 0.013f), DeepBlue, false, Tier);
		Place(TableM, Tab, 0.f, FVector(1.15f), nullptr, true, Tier);
		Place(Cylinder(), Tab + FVector(0.f, 0.f, 82.f), 0.f, FVector(1.42f, 1.42f, 0.16f), Cream, false, Tier); // table cloth
		Place(Candle, Tab + FVector(0.f, 0.f, 91.f), 0.f, FVector(1.f), nullptr, false, Tier);
		Jellyfish(Tab + FVector(0.f, 0.f, 300.f), Tier);
		for (int32 c = 0; c < 4; ++c)
		{
			const FVector Dir = FRotator(0.f, 45.f + c * 90.f, 0.f).Vector();
			const FVector SeatLoc = Tab + Dir * 110.f;
			const float Yaw = (-Dir).Rotation().Yaw;
			Place(ChairM, SeatLoc, Yaw + ChairYawOffset, FVector(1.f), nullptr, false, Tier);
			Place(Plate, Tab + Dir * 45.f + FVector(0.f, 0.f, 91.f), 0.f, FVector(1.f), nullptr, false, Tier);
			FDomeSeat S;
			S.Loc = SeatLoc;
			S.Table = Tab;
			S.Yaw = Yaw;
			S.Tier = Tier;
			Seats.Add(S);
		}
		if (Tier > 0)
		{
			for (int32 r = 0; r < 4; ++r)
			{
				const FVector Dir = FRotator(0.f, r * 90.f, 0.f).Vector();
				UStaticMeshComponent* RC = AbyssAssets::AddPart(this, Root, Rail, Tab + Dir * 170.f - FVector(-Dir.Y, Dir.X, 0.f) * 100.f, FRotator(0.f, FVector(-Dir.Y, Dir.X, 0.f).Rotation().Yaw, 0.f), FVector(1.f), nullptr, false);
				TagTier(RC, Tier, true);
			}
		}
	};
	Table(FVector(470.f, 60.f, 0.f), 0);
	Table(FVector(170.f, 260.f, 0.f), 0);
	Table(FVector(-140.f, 420.f, 0.f), 0);
	Table(FVector(-420.f, 250.f, 0.f), 0);
	Table(FVector(440.f, -150.f, 0.f), 1);
	Table(FVector(-150.f, 150.f, 0.f), 2);

	// --- 05 curved booth sofa along the glass (top-right), tier 2
	for (int32 k = 0; k < 7; ++k)
	{
		const float Deg = FMath::Lerp(12.f, 78.f, k / 6.f);
		const FVector Dir = FRotator(0.f, Deg, 0.f).Vector();
		const FVector P = Dir * (BaseR - 110.f);
		const float Yaw = Deg + 90.f;
		Place(Cube(), P + FVector(0.f, 0.f, 22.f), Yaw, FVector(1.0f * K, 0.65f, 0.44f), DeepBlue, true, 2);           // seat
		Place(Cube(), P + Dir * 30.f + FVector(0.f, 0.f, 70.f), Yaw, FVector(1.0f * K, 0.15f, 0.7f), DeepBlue, false, 2); // back
		Place(Cone(), P + FVector(0.f, 0.f, 62.f) + Dir * 12.f, Yaw, FVector(0.25f, 0.06f, 0.25f), Lit(this, FLinearColor(0.95f, 0.6f, 0.4f), 0.6f), false, 2); // starfish pillow
		if (k % 2 == 0)
		{
			FDomeSeat S;
			S.Loc = P - Dir * 10.f;
			S.Table = P - Dir * 140.f;
			S.Yaw = (-Dir).Rotation().Yaw;
			S.Tier = 2;
			Seats.Add(S);
			Place(Cylinder(), P - Dir * 140.f + FVector(0.f, 0.f, 36.f), 0.f, FVector(0.8f, 0.8f, 0.72f), Wood, true, 2);
			if (UStaticMesh* ShellM = Prop(TEXT("SM_ShellLamp")))
			{
				UStaticMeshComponent* SL = Place(ShellM, P - Dir * 140.f + FVector(0.f, 0.f, 73.f), Yaw + 180.f, FVector(1.f), Lit(this, FLinearColor(0.95f, 0.78f, 0.7f), 0.35f, 0.f, 0.3f), false, 2);
				SetSlot(SL, TEXT("Pearl"), Lit(this, FLinearColor(1.f, 0.9f, 0.75f), 0.2f, 0.f, 8.f));
			}
			else Place(Sphere(), P - Dir * 140.f + FVector(0.f, 0.f, 82.f), 0.f, FVector(0.18f, 0.18f, 0.12f), Glow, false, 2); // shell lamp
		}
		if (k == 1 || k == 5)
		{
			UStaticMeshComponent* RC = AbyssAssets::AddPart(this, Root, Rail, P - Dir * 220.f, FRotator(0.f, Yaw, 0.f), FVector(1.f), nullptr, false);
			TagTier(RC, 2, true);
		}
	}

	// --- planter ring along the glass + corals + anchor banner
	UStaticMesh* PlantM = Lux(TEXT("SM_LUX_Plant_01"));
	UStaticMesh* CoralMs[3] = { Prop(TEXT("SM_CoralBranch")), Prop(TEXT("SM_CoralFan")), Prop(TEXT("SM_BrainCoral")) };
	UMaterialInterface* CoralMats[3] = {
		Lit(this, FLinearColor(0.95f, 0.38f, 0.45f), 0.6f, 0.f, 0.35f),
		Lit(this, FLinearColor(0.95f, 0.55f, 0.2f), 0.6f, 0.f, 0.3f),
		Lit(this, FLinearColor(0.75f, 0.45f, 0.85f), 0.7f, 0.f, 0.2f) };
	UMaterialInterface* RockMat = Lit(this, FLinearColor(0.16f, 0.14f, 0.13f), 0.85f);
	int32 CoralN = 0;
	for (int32 k = 0; k < 16; ++k)
	{
		const float Deg = -170.f + k * 22.f;
		if (FMath::Abs(FMath::FindDeltaAngleDegrees(Deg, 180.f)) < 18.f) continue; // entrance gap
		const FVector Dir = FRotator(0.f, Deg, 0.f).Vector();
		Place(Cube(), Dir * (BaseR - 45.f) + FVector(0.f, 0.f, 25.f), Deg + 90.f, FVector(2.6f * K, 0.5f, 0.5f), DarkWood, true);
		if (k % 2 == 0) Place(PlantM, Dir * (BaseR - 55.f) + FVector(0.f, 0.f, 45.f), Deg, FVector(0.6f), nullptr, false, k % 4 == 0 ? 0 : 1);
		else if (CoralMs[CoralN % 3])
		{
			const int32 c = CoralN++ % 3;
			const FVector Along = FRotator(0.f, Deg + 90.f, 0.f).Vector();
			// a small coral group per planter: the main piece plus a brain coral
			UStaticMeshComponent* CC = Place(CoralMs[c], Dir * (BaseR - 50.f) + FVector(0.f, 0.f, 50.f), Deg + 90.f + k * 37.f, FVector(c == 1 ? 1.1f : 1.3f), CoralMats[c]);
			SetSlot(CC, TEXT("Rock"), RockMat);
			if (c != 2 && CoralMs[2])
			{
				Place(CoralMs[2], Dir * (BaseR - 50.f) + Along * 60.f + FVector(0.f, 0.f, 50.f), k * 53.f, FVector(0.45f), CoralMats[2]);
			}
		}
		else Place(Cone(), Dir * (BaseR - 50.f) + FVector(0.f, 0.f, 80.f), Deg, FVector(0.35f, 0.35f, 0.7f), Coral);
	}
	// anchor banner above the kitchen
	Place(Cube(), FVector(150.f, -740.f, 330.f), 0.f, FVector(1.1f, 0.03f, 1.5f), DeepBlue);
	if (UStaticMesh* AnchorM = Prop(TEXT("SM_Anchor")))
	{
		Place(AnchorM, FVector(150.f, -730.f, 335.f), 0.f, FVector(0.85f), Lit(this, FLinearColor(0.85f, 0.62f, 0.3f), 0.3f, 1.f, 0.15f));
	}
	else Place(Cylinder(), FVector(150.f, -735.f, 340.f), 90.f, FVector(0.5f, 0.5f, 0.02f), Cream);

	// --- entrance arch (08)
	for (float Y : { -110.f, 110.f })
	{
		Place(Cube(), FVector(-BaseR + 40.f, Y, 130.f), 0.f, FVector(0.3f, 0.3f, 2.6f), Wood, true);
		Place(Sphere(), FVector(-BaseR + 70.f, Y, 230.f), 0.f, FVector(0.2f), Glow);
	}
	Place(Cube(), FVector(-BaseR + 40.f, 0.f, 262.f), 0.f, FVector(0.3f, 2.5f, 0.25f), Wood);

	BuildChef();
	AddPointLight(this, Root, FVector(-500.f, 0.f, 250.f), FLinearColor(1.f, 0.75f, 0.5f), 50.f * LS, 600.f);

	// spread the plan out to the real dome size (object sizes stay the same)
	for (int32 c = FirstChild; c < Root->GetNumChildrenComponents(); ++c)
	{
		USceneComponent* Ch = Root->GetChildComponent(c);
		if (!Ch || Ch->ComponentHasTag(TEXT("NoScale"))) continue;
		FVector L = Ch->GetRelativeLocation();
		Ch->SetRelativeLocation(FVector(L.X * K, L.Y * K, L.Z));
	}
	for (FDomeSeat& St : Seats)
	{
		St.Loc = FVector(St.Loc.X * K, St.Loc.Y * K, St.Loc.Z);
		St.Table = FVector(St.Table.X * K, St.Table.Y * K, St.Table.Z);
	}
	ChefRoot->SetRelativeLocation(FVector(90.f * K, -700.f * K, 0.f));
	GrillLight->SetRelativeLocation(FVector(40.f * K, -600.f * K, 150.f));
	AddPointLight(this, Root, FVector(0.f, 150.f * K, 700.f), FLinearColor(1.f, 0.8f, 0.6f), 160.f * LS, Radius * 1.6f, true);
	AddPointLight(this, Root, FVector(200.f * K, 300.f * K, 500.f), FLinearColor(1.f, 0.75f, 0.55f), 80.f * LS, Radius, false);
	return true;
}


// ---------------------------------------------------------------- luxury showcase map as the interior

bool ARestaurantDome::BuildLuxInterior()
{
	using namespace AbyssAssets;
	if (!bUseLuxLevel) return false;
	bLuxLayout = true;
	bLuxSeatsGathered = false;
	ChefMesh->SetVisibility(false); // the player is the chef in here
	Seats.Reset();
	TankFish->SetVisibility(false);
	USceneComponent* Root = GetRootComponent();
	const float LS = LightScale;

	// sand floor around the building, under the glass (the building brings its own floors)
	UMaterialInterface* Sand = Lit(this, FLinearColor(0.42f, 0.36f, 0.27f), 0.9f);
	if (UMaterialInterface* SB = AbyssAssets::BaseSeabed()) Sand = SB;
	AddPart(this, Root, Cylinder(), FVector(0.f, 0.f, -6.f), FRotator::ZeroRotator, FVector(Radius / 50.f, Radius / 50.f, 0.1f), Sand, true);

	// round wooden floor (the open-plan map lost its rectangular floors): pack floor tiles clipped to a circle,
	// tile tops at local z = 0 (= the old main floor height)
	if (UStaticMesh* Tile = Lux(TEXT("SM_LUX_Floor_2X2")))
	{
		UInstancedStaticMeshComponent* Floor = NewObject<UInstancedStaticMeshComponent>(this, TEXT("LuxFloor"));
		Floor->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		Floor->SetupAttachment(Root);
		Floor->SetStaticMesh(Tile);
		Floor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Floor->SetCollisionProfileName(TEXT("BlockAll"));
		Floor->RegisterComponent();
		AddInstanceComponent(Floor);
		const FBox B = Tile->GetBoundingBox();
		const float Size = FMath::Max(B.GetSize().X, 50.f);
		const float Lim = Radius - 260.f;
		const int32 N = FMath::CeilToInt(Lim / Size) + 1;
		for (int32 ix = -N; ix < N; ++ix)
			for (int32 iy = -N; iy < N; ++iy)
			{
				const float X0 = ix * Size, Y0 = iy * Size;
				if (FVector2D(X0 + Size * 0.5f, Y0 + Size * 0.5f).Size() > Lim) continue;
				Floor->AddInstance(FTransform(FVector(X0 - B.Min.X, Y0 - B.Min.Y, -B.Max.Z)));
			}
	}

	// kitchen inside the bar: prep table + teppan stove next to where the cook stands
	UMaterialInterface* Wood  = Lit(this, FLinearColor(0.32f, 0.17f, 0.08f), 0.45f);
	UMaterialInterface* Board = Lit(this, FLinearColor(0.62f, 0.42f, 0.24f), 0.6f);
	UMaterialInterface* Steel = Lit(this, FLinearColor(0.55f, 0.56f, 0.58f), 0.25f, 1.f);
	UMaterialInterface* Iron  = Lit(this, FLinearColor(0.05f, 0.05f, 0.055f), 0.35f, 1.f);
	// --- kitchen line behind the bar (dome -Y side): shelf | stove | cook counter | prep table, fronts facing the bar.
	// Each model is scaled to a real height and placed edge to edge so nothing overlaps.
	FireMat = Lit(this, FLinearColor(1.f, 0.35f, 0.05f), 0.5f, 0.f, 6.f);
	struct FPiece { const TCHAR* Path; float Height; float Width; FVector At; bool bOk; };
	FPiece Pieces[4] = {
		{ TEXT("/Game/Abyss/Kitchen/SM_Shelf.SM_Shelf"), 200.f, 0.f, FVector::ZeroVector, false },
		{ TEXT("/Game/Abyss/Kitchen/SM_Stove.SM_Stove"), 210.f, 0.f, FVector::ZeroVector, false },
		{ TEXT("/Game/Abyss/Kitchen/SM_CookCounter.SM_CookCounter"), 100.f, 0.f, FVector::ZeroVector, false },
		{ TEXT("/Game/Abyss/Kitchen/SM_PrepTable.SM_PrepTable"), 190.f, 0.f, FVector::ZeroVector, false } };
	UStaticMesh* PieceMesh[4] = {};
	float Total = 0.f;
	for (int32 k = 0; k < 4; ++k)
	{
		PieceMesh[k] = LoadObject<UStaticMesh>(nullptr, Pieces[k].Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		const FBox B = PieceMesh[k] ? PieceMesh[k]->GetBoundingBox() : FBox(FVector(-50.f, -30.f, 0.f), FVector(50.f, 30.f, 100.f));
		Pieces[k].Width = B.GetSize().X * Pieces[k].Height / FMath::Max(B.GetSize().Z, 1.f);
		Total += Pieces[k].Width + (k > 0 ? KitchenGap : 0.f);
	}
	// reference layout (U-shaped kitchen): stove back-left and prep table back-right against a back wall,
	// the shelf on the right side wall facing inward, the cook counter as an island in front.
	float PieceYaw[4] = { 90.f, 0.f, 0.f, 0.f };
	{
		Pieces[1].At = FVector(-KitchenHalfW + Pieces[1].Width * 0.5f + 40.f, KitchenRowY, 0.f);   // stove
		Pieces[3].At = FVector(KitchenHalfW - Pieces[3].Width * 0.5f - 140.f, KitchenRowY, 0.f);   // prep table
		Pieces[0].At = FVector(KitchenHalfW - 60.f, KitchenRowY + 250.f, 0.f);                      // shelf on the right wall
		Pieces[2].At = FVector(0.f, KitchenIslandY, 0.f);                                          // island
	}
	{
		// raised kitchen floor, back wall with an anchor banner, low side walls (all visual, walker walks over the floor)
		UMaterialInterface* Tile = Lit(this, FLinearColor(0.62f, 0.6f, 0.66f), 0.25f, 0.f);
		UMaterialInterface* NavyW = Lit(this, FLinearColor(0.008f, 0.02f, 0.07f), 0.5f);
		UMaterialInterface* CreamW = Lit(this, FLinearColor(0.85f, 0.78f, 0.6f), 0.5f);
		UMaterialInterface* BrassW = Lit(this, FLinearColor(0.85f, 0.6f, 0.25f), 0.25f, 1.f);
		const float Y0 = KitchenRowY - 90.f, Y1 = KitchenIslandY + 160.f;
		const float D = Y1 - Y0;
		AddPart(this, Root, Cube(), FVector(0.f, (Y0 + Y1) * 0.5f, 1.5f), FRotator::ZeroRotator, FVector(KitchenHalfW * 2.f / 100.f, D / 100.f, 0.03f), Tile, false);
		AddPart(this, Root, Cube(), FVector(0.f, Y1, 3.f), FRotator::ZeroRotator, FVector(KitchenHalfW * 2.f / 100.f, 0.06f, 0.06f), BrassW, false);
		// back wall
		AddPart(this, Root, Cube(), FVector(0.f, Y0 - 15.f, 125.f), FRotator::ZeroRotator, FVector(KitchenHalfW * 2.f / 100.f, 0.3f, 2.5f), NavyW, true);
		for (float Z : { 6.f, 120.f, 248.f })
			AddPart(this, Root, Cube(), FVector(0.f, Y0 + 2.f, Z), FRotator::ZeroRotator, FVector(KitchenHalfW * 2.f / 100.f + 0.04f, 0.06f, Z > 200.f ? 0.08f : 0.05f), BrassW, false);
		// anchor banner between stove and prep
		const float BX = (Pieces[1].At.X + Pieces[3].At.X) * 0.5f;
		AddPart(this, Root, Cube(), FVector(BX, Y0 + 6.f, 175.f), FRotator::ZeroRotator, FVector(0.8f, 0.03f, 1.1f), CreamW, false);
		AddPart(this, Root, Cube(), FVector(BX, Y0 + 8.f, 232.f), FRotator::ZeroRotator, FVector(0.9f, 0.05f, 0.05f), BrassW, false);
		if (UStaticMesh* AnchorM = Prop(TEXT("SM_Anchor"))) AddPart(this, Root, AnchorM, FVector(BX, Y0 + 16.f, 168.f), FRotator(0.f, 0.f, 0.f), FVector(0.65f), NavyW, false);
		// low side walls with brass caps, the right one carries the shelf
		for (float SX : { -1.f, 1.f })
		{
			const float X = SX * (KitchenHalfW + 15.f);
			const float L = D * (SX > 0.f ? 0.8f : 0.55f);
			AddPart(this, Root, Cube(), FVector(X, Y0 + L * 0.5f, SX > 0.f ? 125.f : 60.f), FRotator::ZeroRotator, FVector(0.3f, L / 100.f, SX > 0.f ? 2.5f : 1.2f), NavyW, true);
			AddPart(this, Root, Cube(), FVector(X, Y0 + L * 0.5f, SX > 0.f ? 252.f : 122.f), FRotator::ZeroRotator, FVector(0.36f, L / 100.f, 0.06f), BrassW, false);
		}
		if (UStaticMesh* PackShelf = Lux(TEXT("SM_LUX_Shelf_01")))
			AddPart(this, Root, PackShelf, FVector(-KitchenHalfW + 40.f, (Y0 + Y1) * 0.5f + 40.f, 0.f), FRotator(0.f, -90.f, 0.f), FVector(1.f), nullptr, false);
		if (UStaticMesh* PlantM = Lux(TEXT("SM_LUX_Plant_01")))
			for (float SX : { -1.f, 1.f }) AddPart(this, Root, PlantM, FVector(SX * (KitchenHalfW + 10.f), Y1 + 40.f, 0.f), FRotator(0.f, SX * 40.f, 0.f), FVector(1.1f), nullptr, false);
		// hanging pan rail over the stove + brass lanterns at the wall corners
		AddPart(this, Root, Cylinder(), FVector(Pieces[1].At.X, Y0 + 40.f, 250.f), FRotator(0.f, 0.f, 90.f), FVector(0.05f, 0.05f, Pieces[1].Width / 100.f), BrassW, false);
		UMaterialInterface* LampM = Lit(this, FLinearColor(0.45f, 0.85f, 1.f), 0.3f, 0.f, 5.f);
		for (float SX : { -1.f, 1.f })
		{
			const FVector LP(SX * (KitchenHalfW - 10.f), Y0 + 20.f, 225.f);
			AddPart(this, Root, Sphere(), LP, FRotator::ZeroRotator, FVector(0.3f), LampM, false);
			AddPointLight(this, Root, LP + FVector(0.f, 40.f, 0.f), FLinearColor(0.5f, 0.85f, 1.f), 30.f * LS, 500.f, false);
		}
	}
	for (int32 k = 0; k < 4; ++k)
	{
		if (UStaticMesh* M = PieceMesh[k])
		{
			const FBox B = M->GetBoundingBox();
			const float Sc = Pieces[k].Height / FMath::Max(B.GetSize().Z, 1.f);
			const FRotator Rot(0.f, KitchenModelYaw + PieceYaw[k], 0.f);
			const FVector Cn = Rot.RotateVector(FVector(B.GetCenter().X, B.GetCenter().Y, B.Min.Z) * Sc);
			if (UStaticMeshComponent* SMC = AddPart(this, Root, M, Pieces[k].At - Cn, Rot, FVector(Sc), nullptr, true))
			{
				SMC->SetCastShadow(true);
				Pieces[k].bOk = true;
			}
		}
		else
		{
			AddPart(this, Root, Cube(), Pieces[k].At + FVector(0.f, 0.f, 45.f), FRotator::ZeroRotator, FVector(Pieces[k].Width / 100.f, 0.6f, 0.9f), k == 2 ? Wood : Steel, true);
		}
	}
	KitchenSpot[0] = Pieces[3].At + FVector(0.f, KitchenStandOff, 0.f);   // slice: prep table
	KitchenSpot[1] = Pieces[1].At + FVector(0.f, KitchenStandOff, 0.f);   // cook: stove
	KitchenSpot[2] = Pieces[2].At - FVector(0.f, KitchenStandOff + 10.f, 0.f);   // plate / counter UI: behind the island
	const FVector Stove = Pieces[1].At;
	// warm work lights over the line + a magic glow on the shelf
	for (int32 k = 1; k < 4; ++k) AddPointLight(this, Root, Pieces[k].At + FVector(0.f, 120.f, 280.f), FLinearColor(1.f, 0.82f, 0.62f), 45.f * LS, 520.f, false);
	AddPointLight(this, Root, Pieces[0].At + FVector(-70.f, 0.f, 150.f), FLinearColor(0.7f, 0.45f, 1.f), 30.f * LS, 360.f, false);
	GrillLight->SetRelativeLocation(Stove + FVector(0.f, 0.f, 150.f));
	AddPointLight(this, Root, LuxToLocal(LuxCook) + FVector(100.f, 0.f, 260.f), FLinearColor(1.f, 0.85f, 0.65f), 60.f * LS, 700.f, false);
	// soft glow over the whole dome so the sand and the outside of the building read
	AddPointLight(this, Root, FVector(0.f, 0.f, Radius * 0.6f), FLinearColor(0.75f, 0.7f, 1.f), 90.f * LS, Radius * 1.3f, false);

	// --- dreamy pass (concept art): jellyfish pendants over the dining rings, coral + violet/pink glow along the glass
	{
		UStaticMesh* JellyM = Prop(TEXT("SM_JellyLamp"));
		UMaterialInterface* JBell = AbyssAssets::Glass(this, FLinearColor(0.95f, 0.45f, 0.75f), 0.35f);
		UMaterialInterface* JCore = Lit(this, FLinearColor(1.f, 0.5f, 0.45f), 0.3f, 0.f, 1.6f);
		UMaterialInterface* JTent = Lit(this, FLinearColor(0.95f, 0.55f, 0.85f), 0.35f, 0.f, 0.6f);
		const float Rings[] = { Radius * 0.36f, Radius * 0.53f, Radius * 0.7f };
		const int32 Counts[] = { 8, 12, 16 };
		int32 L = 0;
		for (int32 r = 0; r < 3; ++r)
		{
			for (int32 k = 0; k < Counts[r]; ++k)
			{
				const float A = 2.f * PI * (k + 0.5f * (r % 2)) / Counts[r];
				const FVector P(FMath::Cos(A) * Rings[r], FMath::Sin(A) * Rings[r], 380.f + 60.f * FMath::Sin(k * 1.7f));
				if (JellyM)
				{
					UStaticMeshComponent* J = AbyssAssets::AddPart(this, Root, JellyM, P, FRotator(0.f, k * 37.f, 0.f), FVector(1.1f), JTent, false);
					SetSlot(J, TEXT("Bell"), JBell);
					SetSlot(J, TEXT("Glow"), JCore);
					if (J) J->ComponentTags.Add(TEXT("JellyLamp"));
				}
				// a light on every other lamp (keeps the light count sane)
				if (bInteriorSpace && k % 2 == 0) AddPointLight(this, Root, P - FVector(0.f, 0.f, 60.f), FLinearColor(1.f, 0.68f, 0.62f), 40.f * LS, 650.f, false);
				++L;
			}
		}
		UStaticMesh* CoralMs[3] = { Prop(TEXT("SM_CoralBranch")), Prop(TEXT("SM_CoralFan")), Prop(TEXT("SM_BrainCoral")) };
		UMaterialInterface* CoralMats[3] = {
			Lit(this, FLinearColor(1.f, 0.4f, 0.65f), 0.6f, 0.f, 0.8f),
			Lit(this, FLinearColor(0.75f, 0.45f, 1.f), 0.6f, 0.f, 0.8f),
			Lit(this, FLinearColor(1.f, 0.6f, 0.3f), 0.7f, 0.f, 0.5f) };
		const FLinearColor Accents[3] = { FLinearColor(1.f, 0.35f, 0.7f), FLinearColor(0.55f, 0.4f, 1.f), FLinearColor(0.3f, 0.8f, 1.f) };
		const int32 NC = 36;
		for (int32 k = 0; k < NC; ++k)
		{
			const float A = 2.f * PI * k / NC;
			const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
			const int32 c = k % 3;
			if (CoralMs[c])
			{
				const float R = Radius - 170.f - 60.f * (k % 2);
				AbyssAssets::AddPart(this, Root, CoralMs[c], Dir * R, FRotator(0.f, k * 53.f, 0.f), FVector(c == 2 ? 1.4f : 2.4f), CoralMats[c], false);
				if (CoralMs[2] && c != 2) AbyssAssets::AddPart(this, Root, CoralMs[2], Dir * (R + 90.f) + FRotator(0.f, 90.f, 0.f).RotateVector(Dir) * 70.f, FRotator(0.f, k * 31.f, 0.f), FVector(0.8f), CoralMats[(c + 1) % 3], false);
			}
			if (k % 3 == 0 && (bInteriorSpace || k % 6 == 0)) AddPointLight(this, Root, Dir * (Radius - 220.f) + FVector(0.f, 0.f, 120.f), Accents[(k / 3) % 3], 70.f * LS, 900.f, false);
		}
		UE_LOG(LogTemp, Log, TEXT("Dreamy pass: %d jelly lamps, %d corals"), L, NC);
		if (JellyM)
		{
			const FVector C = KitchenSpot[2] + FVector(0.f, 320.f, 620.f);
			UStaticMeshComponent* J = AbyssAssets::AddPart(this, Root, JellyM, C, FRotator::ZeroRotator, FVector(1.9f, 1.9f, 1.4f), JTent, false);
			SetSlot(J, TEXT("Bell"), JBell);
			SetSlot(J, TEXT("Glow"), JCore);
			if (J) J->ComponentTags.Add(TEXT("JellyLamp"));
			for (int32 k = 0; k < 6; ++k)
			{
				const FVector P = C + FRotator(0.f, k * 60.f, 0.f).Vector() * 230.f - FVector(0.f, 0.f, 40.f + 30.f * (k % 2));
				UStaticMeshComponent* Jk = AbyssAssets::AddPart(this, Root, JellyM, P, FRotator(0.f, k * 40.f, 0.f), FVector(0.75f, 0.75f, 0.6f), JTent, false);
				SetSlot(Jk, TEXT("Bell"), JBell);
				SetSlot(Jk, TEXT("Glow"), JCore);
				if (Jk) Jk->ComponentTags.Add(TEXT("JellyLamp"));
			}
			AddPointLight(this, Root, C - FVector(0.f, 0.f, 120.f), FLinearColor(1.f, 0.7f, 0.75f), 120.f * LS, 1300.f, false);
		}
		// entrance arch at the front of the dome (+Y): brass posts, a shell crest, lanterns
		{
			UMaterialInterface* Brass = Lit(this, FLinearColor(0.85f, 0.6f, 0.25f), 0.25f, 1.f);
			UMaterialInterface* Navy = Lit(this, FLinearColor(0.03f, 0.08f, 0.25f), 0.4f);
			UMaterialInterface* Lamp = Lit(this, FLinearColor(1.f, 0.78f, 0.45f), 0.3f, 0.f, 6.f);
			const float Y = Radius - 260.f;
			for (float X : { -180.f, 180.f })
			{
				AbyssAssets::AddPart(this, Root, Cube(), FVector(X, Y, 160.f), FRotator::ZeroRotator, FVector(0.4f, 0.4f, 3.2f), Navy, true);
				AbyssAssets::AddPart(this, Root, Cube(), FVector(X, Y, 160.f), FRotator::ZeroRotator, FVector(0.46f, 0.46f, 0.12f), Brass, false);
				AbyssAssets::AddPart(this, Root, Cube(), FVector(X, Y, 318.f), FRotator::ZeroRotator, FVector(0.5f, 0.5f, 0.08f), Brass, false);
				AbyssAssets::AddPart(this, Root, Sphere(), FVector(X, Y - 30.f, 260.f), FRotator::ZeroRotator, FVector(0.22f), Lamp, false);
				AddPointLight(this, Root, FVector(X, Y - 60.f, 260.f), FLinearColor(1.f, 0.78f, 0.5f), 40.f * LS, 600.f, false);
			}
			for (int32 k = 0; k <= 12; ++k)
			{
				const float A = PI * k / 12.f;
				AbyssAssets::AddPart(this, Root, Cube(), FVector(-FMath::Cos(A) * 180.f, Y, 330.f + FMath::Sin(A) * 110.f), FRotator(0.f, 0.f, 0.f), FVector(0.32f, 0.42f, 0.22f), Brass, false);
			}
			if (UStaticMesh* ShellM = Prop(TEXT("SM_ShellLamp"))) AbyssAssets::AddPart(this, Root, ShellM, FVector(0.f, Y - 20.f, 430.f), FRotator(0.f, 180.f, 0.f), FVector(3.2f), Brass, false);
			// welcome mat
			AbyssAssets::AddPart(this, Root, Cylinder(), FVector(0.f, Y - 220.f, 2.f), FRotator::ZeroRotator, FVector(3.4f, 2.2f, 0.01f), Navy, false);
		}
	}
	return true;
}

void ARestaurantDome::GatherLuxSeats()
{
	bLuxSeatsGathered = true;
	Seats.Reset();
	const FTransform DT = GetActorTransform();
	TArray<FVector> Chairs, Tables;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		const UStaticMeshComponent* SMC = It->GetStaticMeshComponent();
		const UStaticMesh* M = SMC ? SMC->GetStaticMesh() : nullptr;
		if (!M) continue;
		const FVector L = DT.InverseTransformPosition(It->GetActorLocation());
		if (FVector2D(L.X, L.Y).Size() > Radius) continue;
		const FString N = M->GetName();
		if (N.Contains(TEXT("Chair"))) Chairs.Add(L);
		else if (N.Contains(TEXT("_Table_"))) Tables.Add(L);
	}
	struct FCand { FDomeSeat S; float D = 0.f; };
	TArray<FCand> Cands;
	const FVector CounterPt = LuxToLocal(LuxPlate);
	for (const FVector& Ch : Chairs)
	{
		int32 Best = INDEX_NONE;
		float BestD = 170.f;
		for (int32 t = 0; t < Tables.Num(); ++t)
		{
			const float D = FVector::Dist2D(Ch, Tables[t]);
			if (D < BestD) { BestD = D; Best = t; }
		}
		if (Best == INDEX_NONE) continue;
		FCand C;
		C.S.Loc = Ch;
		C.S.Table = Tables[Best];
		C.S.Yaw = (Tables[Best] - Ch).GetSafeNormal2D().Rotation().Yaw;
		C.D = FVector::Dist2D(Ch, CounterPt);
		Cands.Add(C);
	}
	Cands.Sort([](const FCand& A, const FCand& B) { return A.D < B.D; });
	// nearest chairs open first; expansions open the rooms further away
	for (int32 i = 0; i < Cands.Num() && i < 30; ++i)
	{
		FDomeSeat S = Cands[i].S;
		S.Tier = i < 12 ? 0 : i < 21 ? 1 : 2;
		Seats.Add(S);
	}
	UE_LOG(LogTemp, Log, TEXT("Lux interior: %d chairs, %d tables -> %d seats"), Chairs.Num(), Tables.Num(), Seats.Num());
}


void ARestaurantDome::BeginPlay()
{
	Super::BeginPlay();
	// The exterior dome shows the same restaurant as the playable interior: stream a second copy of the lux map
	// under this glass (the interior dome gets its copy as an always-loaded sublevel of L_Abyss).
	if (!bInteriorSpace && bUseLuxLevel)
	{
		bool bOk = false;
		const FVector Loc = GetActorTransform().TransformPosition(-LuxOrigin);
		LuxExteriorLevel = ULevelStreamingDynamic::LoadLevelInstance(GetWorld(), TEXT("/Game/Abyss/Maps/L_LuxInterior"), Loc, GetActorRotation(), bOk, TEXT("L_LuxExterior"));
		UE_LOG(LogTemp, Log, TEXT("Lux exterior copy streamed: %d"), bOk ? 1 : 0);
	}
}
