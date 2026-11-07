#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SeabedGenerator.generated.h"

class UProceduralMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;

// Procedural seabed: a reef shelf (~40 m) around the restaurant, sloping into a trench (+X)
// that drops to ~160 m. Scatters rocks, coral, kelp and deep glow-worms.
// Place at world origin. Change Seed / sizes in Details and it regenerates.
UCLASS()
class ABYSSRESTAURANT_API ASeabedGenerator : public AActor
{
	GENERATED_BODY()

public:
	ASeabedGenerator();
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Terrain;

	UPROPERTY(EditAnywhere, Category = "Seabed") int32 Seed = 7;
	UPROPERTY(EditAnywhere, Category = "Seabed") float MinX = -22000.f;
	UPROPERTY(EditAnywhere, Category = "Seabed") float MaxX = 34000.f;
	UPROPERTY(EditAnywhere, Category = "Seabed") float HalfY = 24000.f;
	UPROPERTY(EditAnywhere, Category = "Seabed") float CellSize = 250.f;
	UPROPERTY(EditAnywhere, Category = "Seabed") float ShelfZ = -4000.f;
	UPROPERTY(EditAnywhere, Category = "Seabed") float TrenchStartX = 13000.f;
	UPROPERTY(EditAnywhere, Category = "Seabed") float TrenchEndX = 24000.f;
	UPROPERTY(EditAnywhere, Category = "Seabed") float TrenchZ = -16000.f;
	UPROPERTY(EditAnywhere, Category = "Seabed") float FlattenRadius = 3600.f;
	UPROPERTY(EditAnywhere, Category = "Seabed") float ScatterDensity = 1.f;
	// Use the AlienOceanEnvironment pack meshes (falls back to basic shapes if the pack is missing)
	UPROPERTY(EditAnywhere, Category = "Seabed") bool bUseOceanPack = true;

	UFUNCTION(BlueprintPure, Category = "Seabed")
	float HeightAt(float X, float Y) const;

private:
	void BuildTerrain();
	void Scatter();
	bool ScatterOceanPack();
	UHierarchicalInstancedStaticMeshComponent* MakeLayer(class UStaticMesh* Mesh, const FLinearColor& Color, float Rough, float Emissive, bool bCollision, float CullDistance);
};
