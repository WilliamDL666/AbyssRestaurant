#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AbyssTypes.h"
#include "KitchenGame.h"
#include "AbyssGameMode.generated.h"

class ARestaurantDome;
class ASubmarinePawn;
class ADiverPawn;
class ARestaurantWalker;

// Owns the day loop: Explore (sub + diver) -> Restaurant (night service, separate interior space)
// -> Summary (upgrades) -> next day.
UCLASS()
class ABYSSRESTAURANT_API AAbyssGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAbyssGameMode();
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static AAbyssGameMode* Get(const UObject* WorldContext);

	EAbyssPhase Phase = EAbyssPhase::Explore;
	int32 Money = 200;
	int32 Day = 1;
	int32 Stock[AbyssData::NumFish] = {};
	int32 BatteryLevel = 0;
	int32 LightLevel = 0;
	int32 OxygenLevel = 0;
	int32 RestaurantTier = 0;   // expansion (0..2)
	float Satisfaction = 0.8f;
	bool bGrouperFoundToday = false;

	int32 NightServed = 0;
	int32 LifetimeServed = 0;
	int32 NightLeft = 0;
	int32 NightEarned = 0;
	int32 NightPerfect = 0;
	int32 NightCustomers = 0;
	bool bEmergencyTow = false;
	bool bEndingShown = false;
	float EndingTime = 0.f;
	// Demo length: after this night's summary the ending plays
	static constexpr int32 DemoDays = 2;

	FString Toast;
	FLinearColor ToastColor = FLinearColor::White;
	float ToastTime = 0.f;

	static constexpr int32 CargoCapacity = 12;
	static constexpr int32 MaxUpgrade = 3;

	void ShowToast(const FString& Msg, const FLinearColor& Color = FLinearColor::White, float Duration = 2.5f);
	bool AddFish(EFishType Type);
	void TakeFish(int32 Dish);
	int32 CargoCount() const;
	float BatterySeconds() const { return 240.f * (1.f + 0.5f * BatteryLevel); }
	float LightMultiplier() const { return 1.f + 0.35f * LightLevel; }
	// 0 = battery, 1 = searchlight, 2 = oxygen, 3 = restaurant expansion
	int32 UpgradeCost(int32 Which) const;
	int32 UpgradeLevel(int32 Which) const;
	void TryUpgrade(int32 Which);

	void ExitSubToDive();
	void ReturnDiverToSub(bool bEmergency);
	bool IsDiving() const;
	bool IsNearKitchen() const;
	bool IsAtStation(EKitchenStep Step) const;
	// [E] in the restaurant: at the counter starts the oldest order, or plates the dish being cooked
	void RestaurantInteract();

	void EnterRestaurant(bool bEmergency);
	// Dev shortcut [F9]: stock the fridge and jump straight into the restaurant
	void DebugJumpToRestaurant();
	void OnNightFinished();
	void StartNextDay();
	void ConfirmPressed();   // [Enter]: summary -> next day / ending -> continue

	// Restaurant where night service happens (separate interior space)
	ARestaurantDome* GetDome() const { return ServiceDome.Get(); }
	// Exterior restaurant with the docking bay
	ARestaurantDome* GetDockDome() const { return DockDome.Get(); }
	float GetServeReach() const { return ServeReach; }
	ASubmarinePawn* GetSub() const { return Sub.Get(); }
	ADiverPawn* GetDiver() const { return Diver.Get(); }

private:
	TWeakObjectPtr<ARestaurantDome> ServiceDome;
	TWeakObjectPtr<ARestaurantDome> DockDome;
	// how close (2D, cm) the chef must be to a guest to take the order / serve the dish
	float ServeReach = 280.f;
	TWeakObjectPtr<ASubmarinePawn> Sub;
	TWeakObjectPtr<ADiverPawn> Diver;
	TWeakObjectPtr<ARestaurantWalker> Walker;
	bool bPlacedSub = false;
	// -AbyssAutoTest: scripted playtest that logs state and takes screenshots, then quits
	bool bAutoTest = false;
	float AutoTestTime = 0.f;
	int32 AutoTestStep = 0;
	bool bAutoTestChefPosed = false, bAutoTestChefShot = false, bAutoTestPropsPosed = false, bAutoTestPropsShot = false, bAutoTestAerialPosed = false, bAutoTestAerialShot = false, bAutoTestOutsidePosed = false, bAutoTestOutsideShot = false, bAutoTestBookPosed = false, bAutoTestBookShot = false, bAutoTestKitchenShot = false, bAutoTestKitchenShot2 = false, bAutoTestKitchenBack = false, bAutoTestSummaryShot = false;
	void TickAutoTest(float Dt);
	FTimerHandle FadeTimer;

	void PlaceSubAtDock();
	void FadeTo(AActor* NewViewTarget, TFunction<void()> AtBlack);
};
