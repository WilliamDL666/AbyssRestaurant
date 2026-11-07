#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FishSchool.generated.h"

class UInstancedStaticMeshComponent;

// Ambient school of small fish (one draw call). Drifts around its home and scatters around the sub.
UCLASS()
class ABYSSRESTAURANT_API AFishSchool : public AActor
{
	GENERATED_BODY()

public:
	AFishSchool();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return false; }

	UPROPERTY(EditAnywhere, Category = "School") int32 NumFish = 90;
	UPROPERTY(EditAnywhere, Category = "School") float SchoolRadius = 550.f;
	UPROPERTY(EditAnywhere, Category = "School") float WanderRange = 2200.f;
	UPROPERTY(EditAnywhere, Category = "School") float FishScale = 0.55f;
	UPROPERTY(EditAnywhere, Category = "School") float Speed = 0.12f;
	UPROPERTY(EditAnywhere, Category = "School") FLinearColor BodyColor = FLinearColor(0.85f, 0.72f, 0.12f);
	UPROPERTY(EditAnywhere, Category = "School") FLinearColor BackColor = FLinearColor(0.12f, 0.30f, 0.60f);
	UPROPERTY(EditAnywhere, Category = "School") float ScatterRadius = 700.f;

private:
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Bodies;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Backs;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Tails;

	struct FMember { FVector Offset; float Phase; FVector Push; };
	TArray<FMember> Members;
	float Time = 0.f;
	TArray<FTransform> BodyT, BackT, TailT;

	FVector CenterAt(float T) const;
	void Layout(float T, const FVector& Avoid, float Dt);
};
