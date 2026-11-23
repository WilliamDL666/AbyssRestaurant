#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SubmarinePawn.generated.h"

class USphereComponent;
class USpotLightComponent;
class USpringArmComponent;
class UCameraComponent;
class UFloatingPawnMovement;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ACatchableFish;
class UParticleSystemComponent;

UCLASS()
class ABYSSRESTAURANT_API ASubmarinePawn : public APawn
{
	GENERATED_BODY()

public:
	ASubmarinePawn();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Hull;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> PropHub;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpotLightComponent> Headlight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFloatingPawnMovement> Movement;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> MarineSnow;

	UPROPERTY(EditAnywhere, Category = "Sub") float CruiseSpeed = 850.f;
	UPROPERTY(EditAnywhere, Category = "Sub") float BoostMultiplier = 1.9f;
	UPROPERTY(EditAnywhere, Category = "Sub") float BaseCatchRange = 2200.f;
	UPROPERTY(EditAnywhere, Category = "Sub") float CatchConeDegrees = 16.f;
	UPROPERTY(EditAnywhere, Category = "Sub") float HeadlightCandela = 5000.f;
	UPROPERTY(EditAnywhere, Category = "Sub") bool bShowFakeBeam = true;

	// Imported hull model (Tripo etc). If the asset exists it replaces the placeholder hull,
	// auto-scaled to HullLength and aligned along its longest axis. Flip with HullYawOffset = 180 if it faces backwards.
	UPROPERTY(EditAnywhere, Category = "Sub|Model") FSoftObjectPath CustomHullMesh = FSoftObjectPath(TEXT("/Game/Abyss/Submarine/SM_Submarine.SM_Submarine"));
	UPROPERTY(EditAnywhere, Category = "Sub|Model") float HullLength = 480.f;
	UPROPERTY(EditAnywhere, Category = "Sub|Model") float HullYawOffset = 0.f;
	UPROPERTY(EditAnywhere, Category = "Sub|Model") FVector HullOffset = FVector::ZeroVector;
	// Glass bubble over the open cockpit of the imported model. Position/radii are fractions of the
	// fitted model's half-extents (X = length, Y = width, Z = height).
	UPROPERTY(EditAnywhere, Category = "Sub|Model") bool bCockpitGlass = false; // glass now lives in the model (Blender)
	UPROPERTY(EditAnywhere, Category = "Sub|Model") FVector CockpitGlassCenter = FVector(0.38f, 0.f, 0.15f);
	UPROPERTY(EditAnywhere, Category = "Sub|Model") FVector CockpitGlassRadii = FVector(0.22f, 0.45f, 0.5f);
	UPROPERTY(EditAnywhere, Category = "Sub|Model") float CockpitGlassOpacity = 0.12f;

	// --- read by HUD / GameMode ---
	float Battery01 = 1.f;
	float CatchProgress = 0.f;
	bool bCatching = false;
	bool bLightOn = true;
	bool bControlsEnabled = true;
	TWeakObjectPtr<ACatchableFish> Target;

	float GetDepthMeters() const { return FMath::Max(0.f, -GetActorLocation().Z / 100.f); }
	float GetHeading() const;
	float GetCatchRange() const;
	// where the searchlight points (fish react to being lit)
	FVector GetLightDirection() const;
	// set while the claw is on a fish: would it be a prime catch right now?
	bool bTargetPrime = false;
	bool IsInDockZone() const;
	bool CanDiveOut() const { return GetVelocity().Size() < 250.f && !IsInDockZone(); }
	bool bHasCustomHull = false;
	void Refill();
	void SetControlsEnabled(bool bEnabled);

private:
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Beam;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Beacon;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BeaconMat;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BeamMat;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> PropBubbles;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UParticleSystemComponent> BubbleFX;
	struct FBubble { FVector P; float Life; float Phase; };
	TArray<FBubble> BubbleData;
	TArray<FTransform> BubbleT;
	int32 NextBubble = 0;
	float BubbleTimer = 0.f;
	void UpdateBubbles(float Dt);
	void BuildPlaceholderHull();

	FVector MoveInput = FVector::ZeroVector;
	bool bBoost = false;
	bool bCatchHeld = false;
	float Time = 0.f;
	TArray<FVector> SnowPos;
	TArray<FTransform> SnowT;

	void MoveForward(float V) { MoveInput.X = V; }
	void MoveRight(float V) { MoveInput.Y = V; }
	void MoveUp(float V) { MoveInput.Z = V; }
	void Turn(float V);
	void LookUp(float V);
	void CatchPressed() { bCatchHeld = true; }
	void CatchReleased() { bCatchHeld = false; }
	void BoostPressed() { bBoost = true; }
	void BoostReleased() { bBoost = false; }
	void ToggleLight();
	void Interact();
	void DropBait();

	void UpdateTarget(float Dt);
	void UpdateSnow(float Dt);
	void UpdateLightLevel();
};
