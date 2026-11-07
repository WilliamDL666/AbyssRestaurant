#pragma once

#include "CoreMinimal.h"

class AActor;
class UStaticMesh;
class USceneComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// Placeholder geometry + material helpers. All visuals in v0.1 are built from engine
// basic shapes so the project runs with zero imported assets; swap meshes for Tripo/Fab later.
namespace AbyssAssets
{
	ABYSSRESTAURANT_API UStaticMesh* Cube();
	ABYSSRESTAURANT_API UStaticMesh* Sphere();
	ABYSSRESTAURANT_API UStaticMesh* Cylinder();
	ABYSSRESTAURANT_API UStaticMesh* Cone();

	ABYSSRESTAURANT_API UMaterialInterface* BaseLit();
	ABYSSRESTAURANT_API UMaterialInterface* BaseGlass();
	ABYSSRESTAURANT_API UMaterialInterface* BaseAdditive();
	ABYSSRESTAURANT_API UMaterialInterface* BaseSeabed();
	ABYSSRESTAURANT_API UMaterialInterface* Caustics();

	ABYSSRESTAURANT_API UMaterialInstanceDynamic* Lit(UObject* Outer, const FLinearColor& Color, float Roughness = 0.6f, float Metallic = 0.f, float Emissive = 0.f);
	ABYSSRESTAURANT_API UMaterialInstanceDynamic* Glass(UObject* Outer, const FLinearColor& Color, float Opacity);
	ABYSSRESTAURANT_API UMaterialInstanceDynamic* Additive(UObject* Outer, const FLinearColor& Color, float Intensity);

	// Creates + registers a mesh component. bFromConstruction: component is owned by the
	// construction script (rebuilt each OnConstruction); otherwise it is a runtime instance component.
	ABYSSRESTAURANT_API UStaticMeshComponent* AddPart(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh,
		const FVector& Loc, const FRotator& Rot, const FVector& Scale, UMaterialInterface* Mat,
		bool bCollision = false, bool bFromConstruction = true);

	ABYSSRESTAURANT_API UInstancedStaticMeshComponent* AddInstanced(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh,
		UMaterialInterface* Mat, bool bCollision = false, bool bFromConstruction = true);

	ABYSSRESTAURANT_API UPointLightComponent* AddPointLight(AActor* Owner, USceneComponent* Parent, const FVector& Loc,
		const FLinearColor& Color, float Candela, float Radius, bool bShadows = false, bool bFromConstruction = true);
}
