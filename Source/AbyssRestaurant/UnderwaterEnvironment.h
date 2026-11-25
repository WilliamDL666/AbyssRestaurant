#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnderwaterEnvironment.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UParticleSystemComponent;
class UAudioComponent;
class UParticleSystem;

// One actor = the whole underwater look. Fog, sun with caustics, ambient, grading,
// underwater post-process (red absorption + refraction wobble), god rays, the water surface overhead.
// Darkens toward the abyss based on depth; switches to clear air inside the restaurant.
UCLASS()
class ABYSSRESTAURANT_API AUnderwaterEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AUnderwaterEnvironment();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkyLightComponent> Ambient;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPostProcessComponent> Post;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> GodRays;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Surface;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UParticleSystemComponent> Dust;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UAudioComponent> Ambience;

	UPROPERTY(EditAnywhere, Category = "Underwater|Shallow") float SunLux = 13.f;
	UPROPERTY(EditAnywhere, Category = "Underwater|Shallow") FLinearColor SunColor = FLinearColor(0.75f, 0.95f, 1.0f);
	UPROPERTY(EditAnywhere, Category = "Underwater|Shallow") FLinearColor WaterColor = FLinearColor(0.02f, 0.17f, 0.27f);
	UPROPERTY(EditAnywhere, Category = "Underwater|Shallow") FLinearColor VolumetricAlbedo = FLinearColor(0.3f, 0.75f, 0.9f);
	UPROPERTY(EditAnywhere, Category = "Underwater|Shallow") float FogDensity = 0.032f;
	UPROPERTY(EditAnywhere, Category = "Underwater|Shallow") float AmbientIntensity = 1.8f;

	UPROPERTY(EditAnywhere, Category = "Underwater|Abyss") FLinearColor AbyssWaterColor = FLinearColor(0.0f, 0.004f, 0.008f);
	UPROPERTY(EditAnywhere, Category = "Underwater|Abyss") float AbyssFogDensity = 0.08f;
	UPROPERTY(EditAnywhere, Category = "Underwater|Abyss") float DarkStartMeters = 45.f;
	UPROPERTY(EditAnywhere, Category = "Underwater|Abyss") float DarkFullMeters = 130.f;

	UPROPERTY(EditAnywhere, Category = "Underwater|Realism") float UnderwaterPostStrength = 0.3f;
	UPROPERTY(EditAnywhere, Category = "Underwater|Realism") float GodRayIntensity = 0.018f;
	UPROPERTY(EditAnywhere, Category = "Underwater|Realism") int32 NumGodRays = 36;
	UPROPERTY(EditAnywhere, Category = "Underwater|Realism") bool bShowRayMeshes = false;
	// Pack SM_GodRay meshes rendered as dark slabs here (lighting interaction) - keep our soft additive rays
	UPROPERTY(EditAnywhere, Category = "Underwater|Realism") bool bUsePackGodRays = false;
	// Fog density multiplier while inside the restaurant (keeps the windows blue, the room clear)
	UPROPERTY(EditAnywhere, Category = "Underwater|Realism") float InteriorFogScale = 0.03f;

	UPROPERTY(EditAnywhere, Category = "Underwater|Camera") float ExposureCompensation = 2.2f;

private:
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> UnderwaterPP;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RayMat;
	UPROPERTY(Transient) TObjectPtr<UParticleSystem> DustShallow;
	UPROPERTY(Transient) TObjectPtr<UParticleSystem> DustDeep;
	bool bPackRays = false;
	bool bDeepDust = false;
	float RayBaseGlow = 1.f;
	void ApplyLook(float Abyss01, float Interior01);
	void UpdateRays(const FVector& Cam, float Abyss01, float Interior01);
	float CurrentAbyss = 0.f;
	float CurrentInterior = 0.f;
	float Time = 0.f;
	TArray<FVector> RayOffsets;
	TArray<float> RayPhase;
	TArray<FTransform> RayT;
};
