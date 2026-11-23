#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssTypes.h"
#include "CatchableFish.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

// A fish the player can catch. Each species behaves differently, and how calmly it is caught decides its quality:
//  - Yellowtail: tight school that scatters together when startled -> swim in slowly, net several at once
//  - Tuna: fast patrol loop, bolts when chased -> wait on its route and intercept
//  - Grouper: lurks at its den, retreats from light and noise -> lights off, creep in, or lure it with bait
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
	// Fish with the same id school together (the first one spawned leads)
	int32 SchoolId = -1;
	TWeakObjectPtr<ACatchableFish> Leader;
	FVector SchoolOffset = FVector::ZeroVector;

	const FFishDef& Def() const { return AbyssData::Fish(Type); }
	bool IsCatchable() const { return !bCaught && !bHidden; }
	bool IsHiding() const { return bHidden && !bCaught; }
	void SetBeingCaught(bool bOn) { bBeingCaught = bOn; }
	void OnCaught();
	float GetRadius() const { return 45.f * Def().Scale; }

	// 0 = calm .. 1 = spooked. A calm fish can be landed as a PRIME catch.
	float Alarm = 0.f;
	bool IsCalm() const { return Alarm < 0.45f; }
	bool IsFeeding() const { return bFeeding; }
	// Would catching it right now with this method give a prime fish? (Haul = fish in the same net throw)
	bool PrimeIfCaught(ECatchMethod Method, int32 Haul) const;
	// Startle it (and its school): bolts away from the point
	void Spook(const FVector& From, bool bWholeSchool = true);

	// Speargun hit. Returns true when this hit lands the fish (tuna needs 2 hits).
	bool OnHarpoonHit();
	// Fish breaks free / dodges and bolts away from a point
	void Escape(const FVector& From);
	int32 HarpoonHitsTaken = 0;
	bool IsStunned() const { return StunTime > 0.f; }
	FVector GetSchoolAnchor() const { return Anchor; }
	// Where it will be in a few seconds (tuna follow their loop)
	FVector PredictRoute(float Seconds) const;
	FVector GetDen() const { return Den; }

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
	bool bCalmAtFirstHit = false;
	bool bModel = false;

	// school (leader only): the point the school gathers around
	FVector Anchor = FVector::ZeroVector;
	FVector AnchorGoal = FVector::ZeroVector;
	// tuna patrol loop
	float RouteA = 2600.f, RouteB = 1400.f, RouteYaw = 0.f, RouteT = 0.f, RouteDir = 1.f;
	FVector RoutePoint(float T) const;
	// grouper den
	FVector Den = FVector::ZeroVector;
	bool bHidden = false;
	bool bRetreating = false;
	bool bFeeding = false;
	float HideTime = 0.f;

	void PickGoal();
	float TraceFloor(const FVector& At) const;
	void BuildVisual();
	void SetHiding(bool bHide);
	void TickSenses(float Dt, const FVector& P);
};

// A chunk of bait dropped by the submarine: sinks, glows, and lures groupers out of their dens.
UCLASS()
class ABYSSRESTAURANT_API AAbyssBait : public AActor
{
	GENERATED_BODY()

public:
	AAbyssBait();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	float Life = 28.f;

private:
	float FloorZ = -100000.f;
	float Time = 0.f;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> Light;
};
