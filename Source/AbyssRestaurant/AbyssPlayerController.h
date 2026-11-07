#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AbyssPlayerController.generated.h"

// Restaurant / summary-phase input. Submarine input lives on the pawn.
UCLASS()
class ABYSSRESTAURANT_API AAbyssPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	bool bKitchenCursor = false;
	void Order(int32 Index);
	void Order1() { Order(0); }
	void Order2() { Order(1); }
	void Order3() { Order(2); }
	void Order4() { Order(3); }
	void Cook();
	void Confirm();
	void UpgradeBattery();
	void UpgradeLight();
	void UpgradeOxygen();
	void ToggleLanguage();
	void ToggleRecipeBook();
	void DebugRestaurant();
	void UpgradeRestaurant();
};
