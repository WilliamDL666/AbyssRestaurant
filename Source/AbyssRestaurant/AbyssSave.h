#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AbyssSave.generated.h"

// Everything that carries over from one day to the next. Written at the start of each day (slot "AbyssDining").
UCLASS()
class ABYSSRESTAURANT_API UAbyssSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 Version = 1;
	UPROPERTY() int32 Money = 200;
	UPROPERTY() int32 Day = 1;
	UPROPERTY() TArray<int32> Stock;
	UPROPERTY() TArray<int32> Prime;
	UPROPERTY() TArray<int32> Live;
	UPROPERTY() int32 BatteryLevel = 0;
	UPROPERTY() int32 LightLevel = 0;
	UPROPERTY() int32 OxygenLevel = 0;
	UPROPERTY() int32 RestaurantTier = 0;
	UPROPERTY() float Satisfaction = 0.8f;
	UPROPERTY() int32 LifetimeServed = 0;
	UPROPERTY() bool bEndingShown = false;
	// pantry flattened: [season * MaxShelf + nightsLeft]
	UPROPERTY() TArray<int32> Pantry;
	UPROPERTY() TArray<int32> MenuMain;
	UPROPERTY() TArray<int32> MenuSide;
	UPROPERTY() int32 Bait = 2;
	// aquarium
	UPROPERTY() TArray<uint8> TankType;
	UPROPERTY() TArray<bool> TankPrime;
	UPROPERTY() int32 TankSlots = 3;
	UPROPERTY() TArray<int32> Studied;
};
