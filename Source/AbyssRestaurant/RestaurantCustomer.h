#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssRig.h"
#include "AbyssTypes.h"
#include "RestaurantCustomer.generated.h"

class ARestaurantDome;
class UStaticMeshComponent;

UENUM()
enum class ECustomerState : uint8
{
	Arriving,
	Ordering,   // sat down, waiting for the chef to come and take the order
	Waiting,
	Cooking,
	Ready,      // plated: the chef is carrying it to the table
	Eating,
	Leaving
};

UCLASS()
class ABYSSRESTAURANT_API ARestaurantCustomer : public AActor
{
	GENERATED_BODY()

public:
	ARestaurantCustomer();
	virtual void Tick(float DeltaSeconds) override;

	void Init(ARestaurantDome* InDome, int32 InSeat, int32 InDish, const FVector& InTableWorld, float InPatience, int32 Look);
	void Serve(float Quality);
	void Leave(bool bAngry);

	ECustomerState State = ECustomerState::Arriving;
	int32 Dish = 0;
	ETaste Taste = ETaste::None;   // a seasoning / garnish with this taste earns a bigger tip
	bool bTasteHit = false;
	int32 Seat = -1;
	float Patience = 30.f;
	float PatienceMax = 30.f;
	float Quality = 0.f;
	bool bAngry = false;
	float StateTime = 0.f;

	float Patience01() const { return PatienceMax > 0.f ? FMath::Clamp(Patience / PatienceMax, 0.f, 1.f) : 0.f; }
	FVector GetBubbleLocation() const;
	bool IsWaitingForFood() const { return State == ECustomerState::Waiting || State == ECustomerState::Cooking; }
	bool WantsToOrder() const { return State == ECustomerState::Ordering; }
	void TakeOrder();

private:
	UPROPERTY() TObjectPtr<USceneComponent> Body;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> Head;
	// Tripo guest model (falls back to primitives when missing)
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UPoseableMeshComponent> GuestMesh;
	FAbyssRig Rig;
	bool bModel = false;
	TWeakObjectPtr<ARestaurantDome> Dome;
	FVector TableWorld;
	float Phase = 0.f;
};
