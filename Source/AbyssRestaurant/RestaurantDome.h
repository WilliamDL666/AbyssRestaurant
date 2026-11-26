#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KitchenGame.h"
#include "KitchenCounter.h"
#include "AbyssRig.h"
#include "RestaurantDome.generated.h"

class UCameraComponent;
class UPointLightComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPostProcessComponent;
class UStaticMesh;
class ARestaurantCustomer;

USTRUCT()
struct FDomeSeat
{
	GENERATED_BODY()
	FVector Loc = FVector::ZeroVector;    // local
	FVector Table = FVector::ZeroVector;  // local
	float Yaw = 0.f;
	int32 Tier = 0; // expansion level that unlocks this seat
	TWeakObjectPtr<ARestaurantCustomer> Occupant;
};

// The underwater restaurant. Two instances live in the map:
//  - Exterior (bInteriorSpace = false): what you see while diving + the docking bay.
//  - Interior (bInteriorSpace = true): a separate space you are moved to when you dock; runs the night service.
// Both are built procedurally from basic shapes, OR from your own models via CustomShellMesh / CustomInteriorMesh.
UCLASS()
class ABYSSRESTAURANT_API ARestaurantDome : public AActor
{
	GENERATED_BODY()

public:
	ARestaurantDome();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> InteriorCamera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> ChefRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UPoseableMeshComponent> ChefMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> GrillLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> TankFish;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPostProcessComponent> InteriorPost;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SharkPivot;

	UPROPERTY(EditAnywhere, Category = "Dome") bool bInteriorSpace = false;

	// --- Your own models (e.g. from Tripo). When set, the procedural version of that part is skipped.
	UPROPERTY(EditAnywhere, Category = "Dome|Custom Model") TObjectPtr<UStaticMesh> CustomShellMesh;
	UPROPERTY(EditAnywhere, Category = "Dome|Custom Model") FTransform CustomShellTransform;
	UPROPERTY(EditAnywhere, Category = "Dome|Custom Model") TObjectPtr<UStaticMesh> CustomInteriorMesh;
	UPROPERTY(EditAnywhere, Category = "Dome|Custom Model") FTransform CustomInteriorTransform;
	// Keep generated tables/kitchen even when a custom interior mesh is used
	UPROPERTY(EditAnywhere, Category = "Dome|Custom Model") bool bKeepProceduralFurniture = true;

	UPROPERTY(EditAnywhere, Category = "Dome|Look") float InteriorExposure = 0.4f;
	UPROPERTY(EditAnywhere, Category = "Dome|Look") float LightScale = 1.f;

	void SetInteriorActive(bool bActive);

	// --- expansion: 0 = opening layout, 1 = east wing tables + plants, 2 = VIP booths
	UPROPERTY(EditAnywhere, Category = "Dome|Expansion") int32 PreviewTier = 0;
	UPROPERTY(EditAnywhere, Category = "Dome|Expansion") float ChairYawOffset = 0.f;
	void ApplyTier(int32 Tier);
	int32 CurrentTier = 0;

	// Free-roam interior: where the walking player appears, and the kitchen counter to cook at
	FTransform GetEntranceTransform() const;
	FVector GetKitchenLocation() const
	{
		if (bLuxLayout) return GetStationLocation(EKitchenStep::Plate);
		return GetActorTransform().TransformPosition(bConceptLayout ? FVector(-80.f, -420.f, 0.f) * FMath::Max(Radius / 800.f, 1.f) : FVector(-470.f, 0.f, 0.f));
	}

	// --- Luxury showcase map streamed in as the interior (Scripts/setup_lux_interior.py)
	UPROPERTY(EditAnywhere, Category = "Dome|Lux") bool bUseLuxLevel = false;
	// the showcase was lit for its own post volume; brighten it a bit inside the dome
	UPROPERTY(EditAnywhere, Category = "Dome|Lux") float LuxExposureBoost = 0.9f;
	// yaw that turns the Tripo kitchen models so their front faces the cook
	UPROPERTY(EditAnywhere, Category = "Dome|Lux") float KitchenModelYaw = 0.f;
	// kitchen line behind the bar (dome-local Y), gap between pieces, how far in front of a piece the cook stands
	UPROPERTY(EditAnywhere, Category = "Dome|Lux") float KitchenRowY = -225.f;    // the kitchen stands in the middle of the room, behind glass
	UPROPERTY(EditAnywhere, Category = "Dome|Lux") float KitchenIslandY = 155.f;
	// half width of the U-shaped kitchen enclosure
	UPROPERTY(EditAnywhere, Category = "Dome|Lux") float KitchenHalfW = 460.f;
	UPROPERTY(EditAnywhere, Category = "Dome|Lux") float KitchenGap = 18.f;
	UPROPERTY(EditAnywhere, Category = "Dome|Lux") float KitchenStandOff = 105.f;
	FVector KitchenSpot[3] = { FVector(0.f, -450.f, 0.f), FVector(-200.f, -450.f, 0.f), FVector(200.f, -450.f, 0.f) };
	// point of the lux map that sits on the dome origin (x, y = building center, z = main floor top)
	UPROPERTY(EditAnywhere, Category = "Dome|Lux") FVector LuxOrigin = FVector(381.f, -997.f, 70.f);
	// where the cook stands for each step, in lux-map coordinates (inside the bar)
	UPROPERTY(EditAnywhere, Category = "Dome|Lux") FVector LuxSlice = FVector(-700.f, -300.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Dome|Lux") FVector LuxCook = FVector(-900.f, -300.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Dome|Lux") FVector LuxPlate = FVector(-520.f, -300.f, 0.f);
	bool bLuxLayout = false;
	// Station where the player stands for a cooking step (world)
	FVector GetStationLocation(EKitchenStep Step) const;
	const TCHAR* GetStationName(EKitchenStep Step) const;
	bool bConceptLayout = false;
	bool bLuxSeatsGathered = false;

	// --- service: the plated dish travels in the chef's hands to the table
	TWeakObjectPtr<ARestaurantCustomer> CarryFor;
	float CarryScore = 0.f, CarryMult = 1.f;
	int32 CarryDish = -1;
	bool bCarryPrime = false;      // made with a prime catch
	bool bHasBooths = false;
	TArray<FDomeSeat> BoothSeats;   // one seat per booth, added to the lux seats
	// where the display tank stands (dome-local): the old kitchen site, now that the kitchen is in the middle
	FVector GetTankLocal() const { return FVector(0.f, -1010.f, 0.f); }
	// swing doors of the glazed kitchen: pushed open by whoever walks up to them
	UPROPERTY(Transient) TObjectPtr<USceneComponent> DoorHinge[2];
	float DoorOpen = 0.f;       // -1..1: which way the leaves have swung
	bool bDoorWanted = false;   // [F] toggles
	float DoorSide = 1.f;
	float DoorAway = 0.f;       // seconds the cook has been away from an open door (it falls shut)
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> DoorBlock;
	// is this world position close enough to work the kitchen door?
	bool IsNearDoor(const FVector& World) const;
	bool IsDoorOpen() const { return bDoorWanted; }
	void ToggleDoor(const FVector& FromWorld);
	int32 CarryMain = 0, CarrySide = 0;   // seasoning / garnish actually used
	bool IsCarrying() const { return CarryFor.IsValid(); }
	ARestaurantCustomer* GetCarryTarget() const { return CarryFor.Get(); }
	void TakeOrder(ARestaurantCustomer* C);
	void DeliverDish();
	void DebugTakeAllOrders();
	TWeakObjectPtr<class ULevelStreamingDynamic> LuxExteriorLevel;
	bool bLuxLightsTamed = false;
	FVector LuxToLocal(const FVector& P) const { return FVector(P.X - LuxOrigin.X, P.Y - LuxOrigin.Y, 0.f); }
	bool BuildLuxInterior();
	void BuildDockParts();
	void GatherLuxSeats();
	float GetFloorRadius() const { return Radius - 90.f; }

	UPROPERTY(EditAnywhere, Category = "Dome") float Radius = 1100.f;
	UPROPERTY(EditAnywhere, Category = "Dome") FVector DockSpawnLocal = FVector(2750.f, 1250.f, 380.f);
	UPROPERTY(EditAnywhere, Category = "Dome") float DockZoneRadius = 1100.f;

	UPROPERTY(EditAnywhere, Category = "Service") int32 MaxConcurrentOrders = 2;
	UPROPERTY(EditAnywhere, Category = "Service") float SpawnInterval = 7.f;
	UPROPERTY(EditAnywhere, Category = "Service") float BasePatience = 70.f;
	// which way the user's fridge / booth models face (their fronts are not on the same axis)
	UPROPERTY(EditAnywhere, Category = "Lux") float FridgeYaw = 0.f;
	UPROPERTY(EditAnywhere, Category = "Lux") float BoothYaw = 180.f;
	// facing of the kitchen props: hood, porthole, oven, plate cabinet, menu board, herb planter, crates, barrel, ice crate
	// how far the showcase map's bar group is pushed away from the kitchen (cm)
	UPROPERTY(EditAnywhere, Category = "Lux") float BarShift = 260.f;
	float KPropYaw[9] = { 90.f, 90.f, 90.f, 0.f, 90.f, 0.f, 0.f, 0.f, 0.f };

	FTransform GetDockSpawn() const;
	FVector GetDockLocation() const { return GetActorTransform().TransformPosition(DockLocal()); }
	// the dock was laid out for an 11 m dome; a bigger dome pushes it outward along +X
	float DockShift() const { return FMath::Max(0.f, Radius - 1100.f); }
	FVector DockLocal() const { return DockSpawnLocal + FVector(DockShift(), 0.f, 0.f); }
	bool IsInDockZone(const FVector& P) const { return FVector::Dist(P, GetDockLocation()) < DockZoneRadius; }

	// --- service ---
	void BeginNight(int32 NumCustomers);
	void EndNight();
	bool IsNightActive() const { return bNight; }
	void GetQueue(TArray<ARestaurantCustomer*>& Out) const;
	void SelectOrder(int32 QueueIndex);
	void Cook();
	bool IsCooking() const { return Kitchen.bActive; }
	ARestaurantCustomer* GetCookingFor() const { return Kitchen.bActive ? Kitchen.For.Get() : nullptr; }
	// The 3-step cooking minigame (slice -> cook -> plate)
	FKitchenGame Kitchen;
	// Papa's-style counter (the cooking UI)
	FKitchenCounter Counter;
	float NightClock = 0.f;
	void CompleteDish(ARestaurantCustomer* C, float Score, bool bTakeFish, bool bPrime = false);
	int32 GetRemaining() const { return ToSpawn + Customers.Num(); }
	void GetCustomers(TArray<ARestaurantCustomer*>& Out) const;

	void OnCustomerGaveUp(ARestaurantCustomer* C);
	void OnCustomerLeft(ARestaurantCustomer* C);

	static constexpr float PerfectMin = 0.56f;
	static constexpr float PerfectMax = 0.70f;
	static constexpr float GoodMin = 0.42f;
	static constexpr float GoodMax = 0.84f;
	FString ResultText;
	FLinearColor ResultColor = FLinearColor::White;
	float ResultTime = 0.f;

private:
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FireMat;
	UPROPERTY(Transient) TArray<TObjectPtr<ARestaurantCustomer>> Customers;
	TArray<FDomeSeat> Seats;
	TWeakObjectPtr<ARestaurantCustomer> Cooking;
	struct FTankFish { float Angle, Radius, Height, Speed, Scale; };
	TArray<FTankFish> TankFishData;
	FAbyssRig ChefRig;
	bool bChefRigTried = false;
	struct FJellyLamp { TWeakObjectPtr<UStaticMeshComponent> C; FVector Base; float Yaw; };
	TArray<FJellyLamp> JellyLamps;
	bool bJellyCached = false;
	TArray<FTransform> TankT;
	int32 ToSpawn = 0;
	float SpawnTimer = 0.f;
	float Time = 0.f;
	bool bNight = false;

	void BuildShell();
	void BuildInterior(bool bFurniture);
	void BuildDock();
	void BuildSurroundings();
	void BuildShark();
	bool BuildLuxuryInterior();
	bool BuildConceptInterior();
	void BuildChef();
	void SpawnCustomer();
	int32 PickDish() const;
	void UpdateTank(float Dt);
	void FinishDish();
};
