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
class AAbyssBait;
class AAquariumTank;

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
	// how many of Stock[] are prime catches (sell for more)
	int32 Prime[AbyssData::NumFish] = {};

	// --- pantry: seasonings + fresh garnishes bought before opening
	// [id][nights left - 1]; things that never spoil sit in [0]
	int32 Pantry[AbyssData::NumSeason][AbyssData::MaxShelf] = {};
	int32 BoughtNow[AbyssData::NumSeason] = {};   // bought during this prep (can still be returned)
	int32 MenuMain[AbyssData::NumDishes] = {};    // tonight's seasoning per dish (ESeason, 0 = house style)
	int32 MenuSide[AbyssData::NumDishes] = {};    // tonight's garnish per dish
	ETaste TonightTaste = ETaste::Salty;          // what most of tonight's guests fancy
	int32 SpoiledLastNight = 0;
	static constexpr int32 PantryCapacity = 30;
	int32 SeasonCount(int32 Id) const;
	int32 SeasonExpiring(int32 Id) const;         // portions that spoil after tonight
	int32 PantryCount() const;
	bool BuySeason(int32 Id);
	bool ReturnSeason(int32 Id);
	bool UseSeason(int32 Id);
	void CycleMenu(int32 Dish, int32 Slot, int32 Dir);
	// what a dish sells for with tonight's menu (before the cooking score)
	int32 DishFinalPrice(int32 Dish, bool bPrimeFish, int32 Main, int32 Side) const;
	void OpenRestaurant();                        // prep -> service

	// --- aquarium: fish caught alive (net / claw) can be sold, cooked, or put on show.
	// On show they bring ticket money and extra guests every night and get studied, but cost upkeep and cannot be cooked.
	int32 Live[AbyssData::NumFish] = {};          // how many of Stock[] are alive
	TArray<FTankFish> Tank;
	int32 TankSlots = 3;
	static constexpr int32 MaxTankSlots = 6;
	int32 Studied[AbyssData::NumFish] = {};       // nights each species has been on show
	int32 PrepTab = 0;                            // 0 menu + market, 1 aquarium
	int32 NightTickets = 0, NightUpkeep = 0;
	bool TankAdd(int32 Fish);
	bool TankRemove(int32 Index);
	bool SellLive(int32 Fish);
	bool ExpandTank();
	int32 ExpandCost() const { return 300 * (TankSlots - 2); }
	int32 TankTicketsPerNight() const;
	int32 TankUpkeepPerNight() const;
	int32 TankExtraGuests() const;
	bool IsStudied(EFishType Type) const { return Studied[(int32)Type] >= AbyssData::ResearchNights; }
	void RefreshTank();

	// --- guided first day: a coach panel walks the player through the whole loop, one step at a time
	enum { TutorialSteps = 16 };
	bool bTutorial = false;
	int32 TutStep = 0;
	float TutTime = 0.f;       // time on the current step
	float TutMoved = 0.f;      // seconds spent moving (step 1)
	bool bTutLight0 = true;    // searchlight state when step 2 began
	bool bTutSawAquarium = false;
	bool IsTutorial() const { return bTutorial; }
	void StartTutorial();
	void ToggleTutorial();     // [F1]: skip / replay
	void TickTutorial(float Dt);
	// title + up to three lines of text for the current step (English; translated when drawn)
	void TutorialText(FString& Title, TArray<FString>& Lines) const;

	// --- bait for luring groupers
	int32 Bait = 2;
	int32 BaitBoughtNow = 0;
	static constexpr int32 BaitCost = 15;
	static constexpr int32 MaxBait = 6;
	bool BuyBait();
	bool ReturnBait();
	bool DropBait(const FVector& At);
	bool GetBait(FVector& Out) const;

	FVector2D PrepMouse = FVector2D::ZeroVector;   // cursor on the prep screen (hover highlights)
	int32 NightPrimeServed = 0;
	int32 NightTasteHits = 0;
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
	static constexpr int32 DemoDays = 5;

	// --- save game: written at the start of every day, loaded when the game starts
	bool bLoadedSave = false;
	float NewGameArmed = 0.f;      // [F8] twice within a few seconds wipes the save and restarts
	void SaveGame();
	bool LoadGame();
	void NewGamePressed();

	FString Toast;
	FLinearColor ToastColor = FLinearColor::White;
	float ToastTime = 0.f;

	static constexpr int32 CargoCapacity = 12;
	static constexpr int32 MaxUpgrade = 3;

	void ShowToast(const FString& Msg, const FLinearColor& Color = FLinearColor::White, float Duration = 2.5f);
	bool AddFish(EFishType Type, bool bPrime = false, bool bAlive = false);
	// takes one fish out of the fridge; returns true when it was a prime one
	bool TakeFish(int32 Fish, bool bWantPrime = true);
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
	TWeakObjectPtr<AAbyssBait> ActiveBait;
	TWeakObjectPtr<AAquariumTank> TankActor;
	bool bAutoTestPrepShot = false;
	bool bPlacedSub = false;
	// -AbyssAutoTest: scripted playtest that logs state and takes screenshots, then quits
	bool bAutoTest = false;
	float AutoTestTime = 0.f;
	int32 AutoTestStep = 0;
	bool bAutoTestChefPosed = false, bAutoTestChefShot = false, bAutoTestPropsPosed = false, bAutoTestPropsShot = false, bAutoTestAerialPosed = false, bAutoTestAerialShot = false, bAutoTestOutsidePosed = false, bAutoTestOutsideShot = false, bAutoTestBookPosed = false, bAutoTestBookShot = false, bAutoTestKitchenShot = false, bAutoTestKitchenShot2 = false, bAutoTestKitchenBack = false, bAutoTestSummaryShot = false;
	void TickAutoTest(float Dt);
	void TutNext();
	void SpawnTutorialFish();
	bool bTutFishSpawned = false;
	FTimerHandle FadeTimer;

	void PlaceSubAtDock();
	void FadeTo(AActor* NewViewTarget, TFunction<void()> AtBlack);
};
