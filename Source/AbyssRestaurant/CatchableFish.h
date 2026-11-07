#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssTypes.h"
#include "CatchableFish.generated.h"

class UPointLightComponent;

// A single fish the submarine can catch with its capture beam.
// Wanders around its home point, flees when the sub gets close, struggles while being caught.
UCLASS()
class ABYSSRESTAURANT_API ACatchableFish : public AActor
{
	GENERATED_BODY()

public:
	ACatchableFish();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Fish") EFishType Type = EFishType::Yellowtail;
	UPROPERTY(EditAnywhere, Category = "Fish") float WanderRadius = 1800.f;
	UPROPERTY(EditAnywhere, Category = "Fish") float RespawnSeconds = 45.f;

	const FFishDef& Def() const { return AbyssData::Fish(Type); }
	bool IsCatchable() const { return !bCaught; }
	void SetBeingCaught(bool bOn) { bBeingCaught = bOn; }
	void OnCaught();
	float GetRadius() const { return 45.f * Def().Scale; }

	// Speargun hit. Returns true when this hit lands the fish (tuna needs 2 hits).
	bool OnHarpoonHit();
	// Fish breaks free / dodges and bolts away from a point
	void Escape(const FVector& From);
	int32 HarpoonHitsTaken = 0;
	bool IsStunned() const { return StunTime > 0.f; }

private:
	UPROPERTY() TObjectPtr<USceneComponent> Visual;
	UPROPERTY() TObjectPtr<USceneComponent> TailPivot;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> Glow;

	FVector Home;
	FVector Goal;
	FVector Velocity = FVector::ZeroVector;
	float FloorZ = -100000.f;
	float Phase = 0.f;
	float RespawnTimer = 0.f;
	bool bCaught = false;
	bool bBeingCaught = false;
	float StunTime = 0.f;
	float BoltTime = 0.f;
	FVector BoltDir = FVector::ZeroVector;

	void PickGoal();
	float TraceFloor(const FVector& At) const;
};
