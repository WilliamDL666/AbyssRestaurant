#include "AbyssAssets.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"

namespace
{
	template <typename T>
	T* Load(const TCHAR* Path)
	{
		return LoadObject<T>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	UMaterialInterface* LoadMat(const TCHAR* Path)
	{
		if (UMaterialInterface* M = Load<UMaterialInterface>(Path))
		{
			return M;
		}
		return Load<UMaterialInterface>(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	void FinishComponent(AActor* Owner, USceneComponent* C, USceneComponent* Parent, bool bFromConstruction)
	{
		C->CreationMethod = bFromConstruction ? EComponentCreationMethod::UserConstructionScript : EComponentCreationMethod::Instance;
		C->SetupAttachment(Parent ? Parent : Owner->GetRootComponent());
		C->RegisterComponent();
		if (!bFromConstruction)
		{
			Owner->AddInstanceComponent(C);
		}
	}
}

namespace AbyssAssets
{
	UStaticMesh* Cube()     { return Load<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube.Cube")); }
	UStaticMesh* Sphere()   { return Load<UStaticMesh>(TEXT("/Engine/BasicShapes/Sphere.Sphere")); }
	UStaticMesh* Cylinder() { return Load<UStaticMesh>(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")); }
	UStaticMesh* Cone()     { return Load<UStaticMesh>(TEXT("/Engine/BasicShapes/Cone.Cone")); }

	UMaterialInterface* BaseLit()      { return LoadMat(TEXT("/Game/Abyss/Materials/M_AbyssLit.M_AbyssLit")); }
	UMaterialInterface* BaseGlass()    { return LoadMat(TEXT("/Game/Abyss/Materials/M_AbyssGlass.M_AbyssGlass")); }
	UMaterialInterface* BaseAdditive() { return LoadMat(TEXT("/Game/Abyss/Materials/M_AbyssAdditive.M_AbyssAdditive")); }
	UMaterialInterface* BaseSeabed()
	{
		if (UMaterialInterface* M = Load<UMaterialInterface>(TEXT("/Game/Abyss/Materials/M_AbyssSeabedPBR.M_AbyssSeabedPBR"))) return M;
		return LoadMat(TEXT("/Game/Abyss/Materials/M_AbyssSeabed.M_AbyssSeabed"));
	}
	UMaterialInterface* Caustics()     { return Load<UMaterialInterface>(TEXT("/Game/Abyss/Materials/M_AbyssCaustics.M_AbyssCaustics")); }

	UMaterialInstanceDynamic* Lit(UObject* Outer, const FLinearColor& Color, float Roughness, float Metallic, float Emissive)
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(BaseLit(), Outer);
		M->SetVectorParameterValue(TEXT("Color"), Color);
		M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		M->SetScalarParameterValue(TEXT("Metallic"), Metallic);
		M->SetScalarParameterValue(TEXT("Emissive"), Emissive);
		return M;
	}

	UMaterialInstanceDynamic* Glass(UObject* Outer, const FLinearColor& Color, float Opacity)
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(BaseGlass(), Outer);
		M->SetVectorParameterValue(TEXT("Color"), Color);
		M->SetScalarParameterValue(TEXT("Opacity"), Opacity);
		return M;
	}

	UMaterialInstanceDynamic* Additive(UObject* Outer, const FLinearColor& Color, float Intensity)
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(BaseAdditive(), Outer);
		M->SetVectorParameterValue(TEXT("Color"), Color);
		M->SetScalarParameterValue(TEXT("Intensity"), Intensity);
		return M;
	}

	UStaticMeshComponent* AddPart(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh,
		const FVector& Loc, const FRotator& Rot, const FVector& Scale, UMaterialInterface* Mat,
		bool bCollision, bool bFromConstruction)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
		C->SetStaticMesh(Mesh);
		C->SetRelativeLocationAndRotation(Loc, Rot);
		C->SetRelativeScale3D(Scale);
		if (Mat)
		{
			C->SetMaterial(0, Mat);
		}
		C->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (bCollision)
		{
			C->SetCollisionProfileName(TEXT("BlockAll"));
		}
		C->SetCanEverAffectNavigation(false);
		FinishComponent(Owner, C, Parent, bFromConstruction);
		return C;
	}

	UInstancedStaticMeshComponent* AddInstanced(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh,
		UMaterialInterface* Mat, bool bCollision, bool bFromConstruction)
	{
		UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(Owner);
		C->SetStaticMesh(Mesh);
		if (Mat)
		{
			C->SetMaterial(0, Mat);
		}
		C->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		FinishComponent(Owner, C, Parent, bFromConstruction);
		return C;
	}

	UPointLightComponent* AddPointLight(AActor* Owner, USceneComponent* Parent, const FVector& Loc,
		const FLinearColor& Color, float Candela, float Radius, bool bShadows, bool bFromConstruction)
	{
		UPointLightComponent* L = NewObject<UPointLightComponent>(Owner);
		L->SetMobility(EComponentMobility::Movable);
		L->SetRelativeLocation(Loc);
		L->IntensityUnits = ELightUnits::Candelas;
		L->SetIntensity(Candela);
		L->SetLightColor(Color);
		L->SetAttenuationRadius(Radius);
		L->SetCastShadows(bShadows);
		FinishComponent(Owner, L, Parent, bFromConstruction);
		return L;
	}
}
