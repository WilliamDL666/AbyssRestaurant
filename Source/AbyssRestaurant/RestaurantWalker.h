#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbyssRig.h"
#include "RestaurantWalker.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UPoseableMeshComponent;

// The owner/chef walking around inside the restaurant (free roam).
// Cooking only works when standing at the kitchen counter.
UCLASS()
class ABYSSRESTAURANT_API ARestaurantWalker : public ACharacter
{
	GENERATED_BODY()

public:
	ARestaurantWalker();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Visual;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> LegL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> LegR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	// Tripo chef model (falls back to primitives when missing)
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPoseableMeshComponent> ChefMesh;
	// the dish being carried to a table
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> CarryMesh;

	// Keep the walker inside the dome floor (set by the game mode on entry)
	FVector RoomCenter = FVector::ZeroVector;
	float RoomRadius = 1000.f;

private:
	float Time = 0.f;
	FAbyssRig Rig;
	float WorkBlend = 0.f;
	float HoldBlend = 0.f;
	int32 ShownCarry = -1;
	void MoveForward(float V);
	void MoveRight(float V);
	void Turn(float V);
	void LookUp(float V);
	void Interact();
};
