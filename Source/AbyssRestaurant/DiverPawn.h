#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AbyssTypes.h"
#include "AbyssRig.h"
#include "DiverPawn.generated.h"

class USphereComponent;
class USpringArmComponent;
class UCameraComponent;
class UFloatingPawnMovement;
class USpotLightComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class ACatchableFish;
class ASubmarinePawn;
class UPoseableMeshComponent;

// Second playable mode: the diver leaves the submarine to catch fish by hand.
// Speargun = single precise target (tuna takes 2 hits). Net = close sweep, catches up to 3 small fish.
UCLASS()
class ABYSSRESTAURANT_API ADiverPawn : public APawn
{
	GENERATED_BODY()

public:
	ADiverPawn();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> LegL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> LegR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> HarpoonRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> NetRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpotLightComponent> HeadLamp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UFloatingPawnMovement> Movement;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Bubbles;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Spear;
	// Rigged diver model (Tripo/Mixamo). Procedurally animated: swim kick, tread water, aim, net swing.
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPoseableMeshComponent> DiverMesh;
	UPROPERTY(EditAnywhere, Category = "Diver|Model") FSoftObjectPath DiverSkeletalMesh = FSoftObjectPath(TEXT("/Game/Abyss/Diver/SK_Diver.SK_Diver"));
	UPROPERTY(EditAnywhere, Category = "Diver|Model") float DiverHeight = 180.f;
	// Lay the upright skeleton along the swim direction (Body +X = head, belly down)
	UPROPERTY(EditAnywhere, Category = "Diver|Model") FRotator DiverMeshRotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, Category = "Diver|Model") bool bUseAutoMeshRotation = true;
	UPROPERTY(EditAnywhere, Category = "Diver|Model") float ArmSign = 1.f;
	UPROPERTY(EditAnywhere, Category = "Diver|Model") float KickSign = 1.f;
	bool bFreezeRotation = false; // autotest side-view

	UPROPERTY(EditAnywhere, Category = "Diver") float SwimSpeed = 340.f;
	UPROPERTY(EditAnywhere, Category = "Diver") float BaseOxygenSeconds = 90.f;
	UPROPERTY(EditAnywhere, Category = "Diver") float SafeRange = 3300.f;
	UPROPERTY(EditAnywhere, Category = "Diver") float HarpoonRange = 1400.f;
	UPROPERTY(EditAnywhere, Category = "Diver") float HarpoonCooldown = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Diver") float NetRange = 420.f;
	UPROPERTY(EditAnywhere, Category = "Diver") float NetConeDegrees = 55.f;
	UPROPERTY(EditAnywhere, Category = "Diver") int32 NetMaxCatch = 3;

	// --- read by HUD / GameMode ---
	EDiverTool Tool = EDiverTool::Harpoon;
	float Oxygen01 = 1.f;
	float Cooldown = 0.f;
	float DistToSub = 0.f;
	bool bNearHatch = false;
	bool bTooFar = false;
	bool bTooDeep = false;
	TWeakObjectPtr<ACatchableFish> Aimed;
	TArray<TWeakObjectPtr<ACatchableFish>> NetTargets;
	int32 DiveCatch[AbyssData::NumFish] = {};

	void BeginDive(ASubmarinePawn* FromSub);
	float GetDepthMeters() const { return FMath::Max(0.f, -GetActorLocation().Z / 100.f); }
	float GetHeading() const;
	float OxygenSeconds() const;

private:
	TWeakObjectPtr<ASubmarinePawn> Sub;
	FVector MoveInput = FVector::ZeroVector;
	bool bBoost = false;
	float Time = 0.f;
	float SwingTime = 0.f;
	bool bSwingResolved = true;
	float SpearTime = 0.f;
	FVector SpearFrom, SpearTo;

	struct FBubble { FVector P; float Life; float Speed; float Phase; };
	TArray<FBubble> BubbleData;
	TArray<FTransform> BubbleT;
	float BubbleTimer = 0.f;
	int32 NextBubble = 0;

	void MoveForward(float V) { MoveInput.X = V; }
	void MoveRight(float V) { MoveInput.Y = V; }
	void MoveUp(float V) { MoveInput.Z = V; }
	void Turn(float V) { AddControllerYawInput(V); }
	void LookUp(float V) { AddControllerPitchInput(V); }
	void BoostPressed() { bBoost = true; }
	void BoostReleased() { bBoost = false; }
	void SelectHarpoon();
	void SelectNet();
	void Fire();
	void Interact();
	void ToggleLamp();

	ACatchableFish* FindFishOnAim(float MaxRange) const;
	void GatherNetTargets(TArray<ACatchableFish*>& Out) const;
	void ResolveNet();
	void ShowTool();
	void UpdateBubbles(float Dt);
	bool LandFish(ACatchableFish* F, ECatchMethod Method, bool bPrime);

	// --- procedural skeletal animation
	struct FBoneRef { int32 Index = INDEX_NONE; int32 Parent = INDEX_NONE; };
	TArray<FTransform> RefLocal;
	TArray<FTransform> RefComp;
	TMap<FName, FBoneRef> Bones;
	bool bRigReady = false;
	float ShootKick = 0.f;
	float ToolOut = 0.f;        // 0 = hands free, 1 = tool raised
	float ToolIdleTime = 99.f;  // seconds since the tool was last needed
	void SetupRig();
	int32 Bone(const TCHAR* Name) const;
	void AnimateRig(float Dt, float Speed01);
	void RotateBone(TArray<FTransform>& Local, int32 Index, const FVector& AxisCS, float Degrees) const;

	// Blender-authored clips (Scripts/blender_diver_anims.py): swim / hover / tool-ready arms
	UPROPERTY(Transient) TObjectPtr<class UAnimSequence> AnimSwim;
	UPROPERTY(Transient) TObjectPtr<class UAnimSequence> AnimIdle;
	UPROPERTY(Transient) TObjectPtr<class UAnimSequence> AnimAim;
	FAbyssAnimSampler Sampler;
	TArray<FTransform> PoseSwim, PoseIdle, PoseAim;
	TArray<float> ArmMask;      // 1 for shoulder/arm/hand bones
	float SwimClock = 0.f, IdleClock = 0.f;
	float SwimBlend = 0.f;
};
