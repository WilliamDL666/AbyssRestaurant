#include "UnderwaterEnvironment.h"
#include "AbyssAssets.h"
#include "AbyssGameMode.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

namespace
{
	UMaterialInterface* LoadMat(const TCHAR* Path)
	{
		return LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
}

AUnderwaterEnvironment::AUnderwaterEnvironment()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetRelativeRotation(FRotator(-72.f, 35.f, 0.f));
	Sun->SetCastShadows(true);
	Sun->bCastVolumetricShadow = true;
	Sun->VolumetricScatteringIntensity = 1.6f;
	Sun->DynamicShadowDistanceMovableLight = 12000.f;

	Ambient = CreateDefaultSubobject<USkyLightComponent>(TEXT("Ambient"));
	Ambient->SetupAttachment(Root);
	Ambient->SetMobility(EComponentMobility::Movable);
	Ambient->SourceType = ESkyLightSourceType::SLS_CapturedScene;
	Ambient->bLowerHemisphereIsBlack = false;

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(Root);
	Fog->SetFogHeightFalloff(0.00004f);
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogScatteringDistribution(0.75f);
	Fog->SetVolumetricFogExtinctionScale(1.0f);
	Fog->SetVolumetricFogDistance(7000.f);
	Fog->SetFogMaxOpacity(1.f);

	Post = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Post"));
	Post->SetupAttachment(Root);
	Post->bUnbound = true;

	GodRays = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GodRays"));
	GodRays->SetupAttachment(Root);
	GodRays->SetUsingAbsoluteLocation(true);
	GodRays->SetUsingAbsoluteRotation(true);
	GodRays->SetUsingAbsoluteScale(true);
	GodRays->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GodRays->SetCastShadow(false);

	Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Surface"));
	Surface->SetupAttachment(Root);
	Surface->SetUsingAbsoluteLocation(true);
	Surface->SetUsingAbsoluteRotation(true);
	Surface->SetUsingAbsoluteScale(true);
	Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Surface->SetCastShadow(false);

	Dust = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Dust"));
	Dust->SetupAttachment(Root);
	Dust->SetUsingAbsoluteLocation(true);
	Dust->bAutoActivate = true;

	Ambience = CreateDefaultSubobject<UAudioComponent>(TEXT("Ambience"));
	Ambience->SetupAttachment(Root);
	Ambience->bAutoActivate = true;
	Ambience->bAllowSpatialization = false;
	Ambience->bIsUISound = false;
}

void AUnderwaterEnvironment::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	using namespace AbyssAssets;

	if (UMaterialInterface* PC = LoadMat(TEXT("/Game/AlienOceanEnvironment/Materials/Effects/MI_Caustics_Shallow.MI_Caustics_Shallow")))
	{
		Sun->SetLightFunctionMaterial(PC);
		Sun->LightFunctionScale = FVector(2048.f);
	}
	else if (UMaterialInterface* C = Caustics())
	{
		Sun->SetLightFunctionMaterial(C);
	}

	FPostProcessSettings& S = Post->Settings;
	S.bOverride_AutoExposureMethod = true;
	S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	S.bOverride_AutoExposureBias = true;
	S.AutoExposureBias = ExposureCompensation;
	S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	S.AutoExposureApplyPhysicalCameraExposure = false;
	S.bOverride_BloomIntensity = true;
	S.BloomIntensity = 0.5f;
	S.bOverride_BloomThreshold = true;
	S.BloomThreshold = -1.f;
	S.bOverride_WhiteTemp = true;
	S.WhiteTemp = 7400.f;
	S.bOverride_ColorSaturation = true;
	S.ColorSaturation = FVector4(1.0f, 1.04f, 1.1f, 1.05f);
	S.bOverride_ColorContrast = true;
	S.ColorContrast = FVector4(1.05f, 1.05f, 1.05f, 1.08f);
	S.bOverride_SceneColorTint = true;
	S.SceneColorTint = FLinearColor(0.92f, 1.0f, 1.04f);
	S.bOverride_VignetteIntensity = true;
	S.VignetteIntensity = 0.45f;
	S.bOverride_FilmGrainIntensity = true;
	S.FilmGrainIntensity = 0.f;
	S.bOverride_MotionBlurAmount = true;
	S.MotionBlurAmount = 0.f;
	S.bOverride_SceneFringeIntensity = true;
	S.SceneFringeIntensity = 0.f;

	// Underwater post-process material: per-channel absorption (reds die first) + refraction wobble
	S.WeightedBlendables.Array.Reset();
	if (UMaterialInterface* PPM = LoadMat(TEXT("/Game/Abyss/Materials/M_PP_Underwater.M_PP_Underwater")))
	{
		UnderwaterPP = UMaterialInstanceDynamic::Create(PPM, this);
		S.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, UnderwaterPP));
	}
	if (UMaterialInterface* PackPP = LoadMat(TEXT("/Game/AlienOceanEnvironment/Materials/Effects/MI_PP_UnderwaterCameraEffect.MI_PP_UnderwaterCameraEffect")))
	{
		S.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, PackPP));
	}

	// God rays: long soft additive cylinders tilted along the sun direction
	UStaticMesh* RayMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/AlienOceanEnvironment/Meshes/SM_GodRay.SM_GodRay"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UMaterialInterface* RayPackMat = LoadMat(TEXT("/Game/AlienOceanEnvironment/Materials/Effects/MI_GodRay.MI_GodRay"));
	bPackRays = bUsePackGodRays && RayMesh && RayPackMat;
	if (bPackRays)
	{
		GodRays->SetStaticMesh(RayMesh);
		RayMat = UMaterialInstanceDynamic::Create(RayPackMat, this);
		RayMat->GetScalarParameterValue(FName(TEXT("Glow")), RayBaseGlow);
		GodRays->SetMaterial(0, RayMat);
	}
	else
	{
		GodRays->SetStaticMesh(Cylinder());
		RayMat = Additive(this, FLinearColor(0.75f, 0.95f, 1.f), GodRayIntensity);
		GodRays->SetMaterial(0, RayMat);
	}
	FRandomStream R(19);
	RayOffsets.SetNum(FMath::Max(bPackRays ? 16 : NumGodRays, 0));
	RayPhase.SetNum(RayOffsets.Num());
	RayT.SetNum(RayOffsets.Num());
	for (int32 i = 0; i < RayOffsets.Num(); ++i)
	{
		RayOffsets[i] = FVector(R.FRandRange(-4500.f, 4500.f), R.FRandRange(-4500.f, 4500.f), R.FRandRange(0.6f, 2.6f));
		RayPhase[i] = R.FRandRange(0.f, 10.f);
		RayT[i] = FTransform::Identity;
	}
	GodRays->ClearInstances();
	GodRays->AddInstances(RayT, false, true);

	// Water surface seen from below (bright rippling ceiling + Snell's window feel)
	UStaticMesh* PackPlane = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/AlienOceanEnvironment/Meshes/SM_WaterPlane.SM_WaterPlane"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (PackPlane)
	{
		Surface->SetStaticMesh(PackPlane); // uses MI_OceanSurface
		Surface->SetWorldScale3D(FVector(12.f, 12.f, 1.f));
		Surface->SetWorldLocation(FVector(0.f, 0.f, -120.f));
	}
	else if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		Surface->SetStaticMesh(Plane);
		if (UMaterialInterface* WM = LoadMat(TEXT("/Game/Abyss/Materials/M_AbyssWaterSurface.M_AbyssWaterSurface")))
		{
			Surface->SetMaterial(0, WM);
		}
		Surface->SetWorldScale3D(FVector(1200.f, 1200.f, 1.f));
		Surface->SetWorldLocation(FVector(0.f, 0.f, -120.f));
	}

	DustShallow = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/AlienOceanEnvironment/Particles/P_DustMote_Ocean_Shallow.P_DustMote_Ocean_Shallow"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	DustDeep = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/AlienOceanEnvironment/Particles/P_DustMote_Ocean_Deep.P_DustMote_Ocean_Deep"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (DustShallow) Dust->SetTemplate(DustShallow);
	if (USoundBase* Amb = LoadObject<USoundBase>(nullptr, TEXT("/Game/AlienOceanEnvironment/Sounds/SC_UnderwaterAmbience_A.SC_UnderwaterAmbience_A"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		Ambience->SetSound(Amb);
	}

	ApplyLook(0.f, 0.f);
}

void AUnderwaterEnvironment::BeginPlay()
{
	Super::BeginPlay();
	ApplyLook(0.f, 0.f);
	Ambient->RecaptureSky();
}

void AUnderwaterEnvironment::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;

	float Target = 0.f;
	FVector Cam = FVector::ZeroVector;
	if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Cam = CM->GetCameraLocation();
		const float Depth = FMath::Max(0.f, -Cam.Z / 100.f);
		Target = FMath::SmoothStep(DarkStartMeters, DarkFullMeters, Depth);
	}
	const AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	const float InteriorTarget = GM && GM->Phase != EAbyssPhase::Explore ? 1.f : 0.f;

	CurrentAbyss = FMath::FInterpTo(CurrentAbyss, Target, DeltaSeconds, 1.5f);
	CurrentInterior = FMath::FInterpTo(CurrentInterior, InteriorTarget, DeltaSeconds, 4.f);
	ApplyLook(CurrentAbyss, CurrentInterior);
	UpdateRays(Cam, CurrentAbyss, CurrentInterior);
	Surface->SetWorldLocation(FVector(Cam.X, Cam.Y, -120.f));

	// dust motes follow the camera; swap to the deep variant in the trench
	Dust->SetWorldLocation(Cam);
	const bool bWantDeep = CurrentAbyss > 0.5f;
	if (bWantDeep != bDeepDust && DustDeep && DustShallow)
	{
		bDeepDust = bWantDeep;
		Dust->SetTemplate(bDeepDust ? DustDeep : DustShallow);
		Dust->Activate(true);
	}
	Dust->SetVisibility(CurrentInterior < 0.5f);
	Ambience->SetVolumeMultiplier(FMath::Lerp(0.9f, 0.25f, CurrentInterior));
}

void AUnderwaterEnvironment::UpdateRays(const FVector& Cam, float A, float I)
{
	if (RayT.Num() == 0) return;
	const FVector SunDir = -Sun->GetForwardVector(); // toward the sun
	const FQuat Tilt = FRotationMatrix::MakeFromZ(SunDir).ToQuat();
	const float Span = 9000.f;
	for (int32 i = 0; i < RayT.Num(); ++i)
	{
		FVector O = RayOffsets[i];
		// wrap rays around the camera so they are always nearby, with slow drift
		float X = FMath::Fmod(O.X + Time * 25.f - Cam.X, Span); if (X < 0) X += Span; X -= Span * 0.5f;
		float Y = FMath::Fmod(O.Y + Time * 12.f - Cam.Y, Span); if (Y < 0) Y += Span; Y -= Span * 0.5f;
		const float W = O.Z * (0.85f + 0.15f * FMath::Sin(Time * 0.6f + RayPhase[i]));
		if (bPackRays)
		{
			// SM_GodRay: base at the pivot, 36 m tall -> stretch so it reaches from the seabed to the surface
			const float S = 1.25f;
			const FVector Base(Cam.X + X, Cam.Y + Y, -4600.f);
			RayT[i] = FTransform(Tilt, Base, FVector(0.6f + 0.25f * W, 0.6f + 0.25f * W, S));
		}
		else
		{
			const float Len = 7000.f;
			const FVector Mid = FVector(Cam.X + X, Cam.Y + Y, -Len * 0.5f);
			RayT[i] = FTransform(Tilt, Mid, FVector(W, W, Len / 100.f));
		}
	}
	GodRays->BatchUpdateInstancesTransforms(0, RayT, true, true, true);
	if (RayMat)
	{
		if (bPackRays) RayMat->SetScalarParameterValue(TEXT("Glow"), RayBaseGlow * FMath::Square(1.f - A) * (1.f - I));
		else RayMat->SetScalarParameterValue(TEXT("Intensity"), GodRayIntensity * FMath::Square(1.f - A) * (1.f - I));
	}
	GodRays->SetVisibility(A < 0.95f && I < 0.95f);
}

void AUnderwaterEnvironment::ApplyLook(float A, float I)
{
	const FLinearColor Water = FMath::Lerp(WaterColor, AbyssWaterColor, A);

	Sun->SetIntensity(SunLux * FMath::Square(1.f - A));
	Sun->SetLightColor(SunColor);

	Ambient->SetIntensity(AmbientIntensity * (1.f - 0.92f * A));
	Ambient->SetLowerHemisphereColor(Water * 4.f);

	Fog->SetFogInscatteringColor(Water);
	Fog->SetFogDensity(FMath::Lerp(FogDensity, AbyssFogDensity, A) * FMath::Lerp(1.f, InteriorFogScale, I));
	Fog->SetVolumetricFogAlbedo(FMath::Lerp(VolumetricAlbedo, FLinearColor(0.2f, 0.3f, 0.35f), A).ToFColor(true));
	Fog->SetVolumetricFogEmissive(Water * 0.0015f);

	Post->Settings.AutoExposureBias = ExposureCompensation;
	if (UnderwaterPP)
	{
		UnderwaterPP->SetScalarParameterValue(TEXT("Strength"), UnderwaterPostStrength * (1.f - I));
	}
}
