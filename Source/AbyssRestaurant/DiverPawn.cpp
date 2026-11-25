#include "DiverPawn.h"
#include "AbyssAssets.h"
#include "AbyssGameMode.h"
#include "AbyssSfx.h"
#include "SubmarinePawn.h"
#include "CatchableFish.h"
#include "Components/SphereComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Components/PoseableMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "Engine/World.h"

ADiverPawn::ADiverPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(55.f);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	SetRootComponent(Collision);

	Body = CreateDefaultSubobject<USceneComponent>(TEXT("Body"));
	Body->SetupAttachment(Collision);
	LegL = CreateDefaultSubobject<USceneComponent>(TEXT("LegL"));
	LegL->SetupAttachment(Body);
	LegL->SetRelativeLocation(FVector(-50.f, 13.f, 0.f));
	LegR = CreateDefaultSubobject<USceneComponent>(TEXT("LegR"));
	LegR->SetupAttachment(Body);
	LegR->SetRelativeLocation(FVector(-50.f, -13.f, 0.f));
	HarpoonRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HarpoonRoot"));
	HarpoonRoot->SetupAttachment(Body);
	HarpoonRoot->SetRelativeLocation(FVector(112.f, 18.f, -14.f));
	NetRoot = CreateDefaultSubobject<USceneComponent>(TEXT("NetRoot"));
	NetRoot->SetupAttachment(Body);
	NetRoot->SetRelativeLocation(FVector(110.f, 20.f, -12.f));

	HeadLamp = CreateDefaultSubobject<USpotLightComponent>(TEXT("HeadLamp"));
	HeadLamp->SetupAttachment(Body);
	HeadLamp->SetRelativeLocation(FVector(100.f, 0.f, 22.f));
	HeadLamp->IntensityUnits = ELightUnits::Candelas;
	HeadLamp->SetIntensity(900.f);
	HeadLamp->SetAttenuationRadius(2200.f);
	HeadLamp->SetOuterConeAngle(28.f);
	HeadLamp->SetInnerConeAngle(16.f);
	HeadLamp->SetLightColor(FLinearColor(0.9f, 0.95f, 1.f));
	HeadLamp->VolumetricScatteringIntensity = 2.f;
	HeadLamp->SetCastShadows(false);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Collision);
	SpringArm->TargetArmLength = 360.f;
	SpringArm->SocketOffset = FVector(0.f, 65.f, 55.f);
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 9.f;
	SpringArm->ProbeSize = 18.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->SetFieldOfView(82.f);

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = SwimSpeed;
	Movement->Acceleration = 420.f;
	Movement->Deceleration = 520.f;

	Bubbles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Bubbles"));
	Bubbles->SetupAttachment(Collision);
	Bubbles->SetUsingAbsoluteLocation(true);
	Bubbles->SetUsingAbsoluteRotation(true);
	Bubbles->SetUsingAbsoluteScale(true);
	Bubbles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bubbles->SetCastShadow(false);

	Spear = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Spear"));
	Spear->SetupAttachment(Collision);
	Spear->SetUsingAbsoluteLocation(true);
	Spear->SetUsingAbsoluteRotation(true);
	Spear->SetUsingAbsoluteScale(true);
	Spear->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Spear->SetHiddenInGame(true);

	DiverMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("DiverMesh"));
	DiverMesh->SetupAttachment(Body);
	DiverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADiverPawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	using namespace AbyssAssets;

	UMaterialInterface* Suit   = Lit(this, FLinearColor(0.02f, 0.035f, 0.07f), 0.55f, 0.f);
	UMaterialInterface* Trim   = Lit(this, FLinearColor(0.9f, 0.6f, 0.05f), 0.5f, 0.f);
	UMaterialInterface* Tank   = Lit(this, FLinearColor(0.7f, 0.72f, 0.74f), 0.25f, 1.f);
	UMaterialInterface* Black  = Lit(this, FLinearColor(0.015f, 0.015f, 0.015f), 0.5f);
	UMaterialInterface* Mask   = Lit(this, FLinearColor(0.05f, 0.25f, 0.35f), 0.05f, 0.8f);
	UMaterialInterface* Skin   = Lit(this, FLinearColor(0.7f, 0.5f, 0.38f), 0.55f);
	UMaterialInterface* Steel  = Lit(this, FLinearColor(0.5f, 0.5f, 0.52f), 0.3f, 1.f);

	USkeletalMesh* SK = Cast<USkeletalMesh>(DiverSkeletalMesh.TryLoad());
	if (SK)
	{
		DiverMesh->SetSkinnedAssetAndUpdate(SK);
		if (UMaterialInterface* DM = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Abyss/Diver/M_Diver.M_Diver"), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			for (int32 m = 0; m < DiverMesh->GetNumMaterials(); ++m) DiverMesh->SetMaterial(m, DM);
		}
		const float Sc = DiverHeight / FMath::Max(SK->GetBounds().BoxExtent.Z * 2.f, 1.f);
		// mesh: Z up, faces +Y  ->  Body: head +X, belly -Z
		const FRotator Auto = FMatrix(FVector(0.f, -1.f, 0.f), FVector(0.f, 0.f, -1.f), FVector(1.f, 0.f, 0.f), FVector::ZeroVector).Rotator();
		DiverMesh->SetRelativeRotation(bUseAutoMeshRotation ? Auto : DiverMeshRotation);
		DiverMesh->SetRelativeScale3D(FVector(Sc));
		// put the hips at the Body origin
		DiverMesh->SetRelativeLocation(FVector(-DiverHeight * 0.55f, 0.f, 0.f));
		DiverMesh->SetVisibility(true);
		LegL->SetVisibility(false, true);
		LegR->SetVisibility(false, true);
	}
	else
	{
		DiverMesh->SetVisibility(false);
	}

	// Prone swimming body (local +X = head direction) - placeholder when no rigged model
	if (!SK) {
	AddPart(this, Body, Cylinder(), FVector::ZeroVector, FRotator(90.f, 0.f, 0.f), FVector(0.5f, 0.42f, 1.05f), Suit);
	AddPart(this, Body, Sphere(), FVector(45.f, 0.f, 0.f), FRotator::ZeroRotator, FVector(0.62f, 0.55f, 0.45f), Suit);
	AddPart(this, Body, Cylinder(), FVector(-12.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(0.53f, 0.45f, 0.08f), Trim);
	AddPart(this, Body, Sphere(), FVector(84.f, 0.f, 8.f), FRotator::ZeroRotator, FVector(0.38f, 0.34f, 0.36f), Suit);
	AddPart(this, Body, Sphere(), FVector(98.f, 0.f, 10.f), FRotator::ZeroRotator, FVector(0.14f, 0.3f, 0.2f), Mask);
	AddPart(this, Body, Cylinder(), FVector(100.f, 0.f, -6.f), FRotator(90.f, 0.f, 0.f), FVector(0.07f, 0.07f, 0.12f), Black);
	// Air tank + valve + hose
	AddPart(this, Body, Cylinder(), FVector(5.f, 0.f, 30.f), FRotator(90.f, 0.f, 0.f), FVector(0.3f, 0.3f, 0.78f), Tank);
	AddPart(this, Body, Cylinder(), FVector(48.f, 0.f, 30.f), FRotator(90.f, 0.f, 0.f), FVector(0.08f, 0.08f, 0.1f), Black);
	AddPart(this, Body, Cylinder(), FVector(70.f, 10.f, 18.f), FRotator(60.f, 0.f, 0.f), FVector(0.03f, 0.03f, 0.45f), Black);
	// Arms reaching forward
	for (float Y : { 22.f, -22.f })
	{
		AddPart(this, Body, Cylinder(), FVector(88.f, Y, -12.f), FRotator(90.f, 0.f, 0.f), FVector(0.13f, 0.13f, 0.55f), Suit);
		AddPart(this, Body, Sphere(), FVector(117.f, Y * 0.85f, -14.f), FRotator::ZeroRotator, FVector(0.11f), Black);
	}
	// Legs + fins (kick animated)
	for (USceneComponent* Leg : { LegL.Get(), LegR.Get() })
	{
		AddPart(this, Leg, Cylinder(), FVector(-38.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(0.19f, 0.19f, 0.8f), Suit);
		AddPart(this, Leg, Cube(), FVector(-95.f, 0.f, 0.f), FRotator::ZeroRotator, FVector(0.5f, 0.24f, 0.025f), Black);
		AddPart(this, Leg, Cube(), FVector(-118.f, 0.f, 0.5f), FRotator::ZeroRotator, FVector(0.1f, 0.25f, 0.03f), Trim);
	}
	} // !SK
	// Speargun
	AddPart(this, HarpoonRoot, Cylinder(), FVector(42.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(0.06f, 0.06f, 0.9f), Black);
	AddPart(this, HarpoonRoot, Cube(), FVector(0.f, 0.f, -9.f), FRotator(-15.f, 0.f, 0.f), FVector(0.08f, 0.05f, 0.2f), Black);
	AddPart(this, HarpoonRoot, Cylinder(), FVector(50.f, 0.f, 4.f), FRotator(90.f, 0.f, 0.f), FVector(0.025f, 0.025f, 0.95f), Steel);
	AddPart(this, HarpoonRoot, Cone(), FVector(100.f, 0.f, 4.f), FRotator(-90.f, 0.f, 0.f), FVector(0.05f, 0.05f, 0.1f), Steel);
	// Hand net: handle + hoop + mesh
	AddPart(this, NetRoot, Cylinder(), FVector(30.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(0.04f, 0.04f, 0.6f), Black);
	UInstancedStaticMeshComponent* Hoop = AddInstanced(this, NetRoot, Cube(), Steel);
	for (int32 i = 0; i < 20; ++i)
	{
		const float A = 2.f * PI * i / 20.f;
		const FVector P(92.f, FMath::Cos(A) * 30.f, FMath::Sin(A) * 30.f);
		Hoop->AddInstance(FTransform(FRotator(0.f, 0.f, FMath::RadiansToDegrees(A)), P, FVector(0.03f, 0.03f, 0.1f)));
	}
	AddPart(this, NetRoot, Cylinder(), FVector(92.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(0.6f, 0.6f, 0.01f), AbyssAssets::Glass(this, FLinearColor(0.9f, 0.9f, 0.85f), 0.35f));
	AddPart(this, NetRoot, Cone(), FVector(70.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(0.55f, 0.55f, 0.4f), AbyssAssets::Glass(this, FLinearColor(0.9f, 0.9f, 0.85f), 0.2f));

	Bubbles->SetStaticMesh(Sphere());
	Bubbles->SetMaterial(0, Lit(this, FLinearColor(0.85f, 0.95f, 1.f), 0.05f, 0.6f, 0.4f));
	Spear->SetStaticMesh(Cylinder());
	Spear->SetMaterial(0, Steel);
	ShowTool();
}

void ADiverPawn::BeginPlay()
{
	Super::BeginPlay();
	Movement->MaxSpeed = SwimSpeed;
	BubbleData.SetNum(28);
	BubbleT.SetNum(BubbleData.Num());
	for (int32 i = 0; i < BubbleData.Num(); ++i)
	{
		BubbleData[i] = { GetActorLocation(), 0.f, 60.f, (float)i };
		BubbleT[i] = FTransform(FQuat::Identity, GetActorLocation(), FVector(0.001f));
	}
	Bubbles->ClearInstances();
	Bubbles->AddInstances(BubbleT, false, true);
	SetupRig();
}

int32 ADiverPawn::Bone(const TCHAR* Name) const
{
	if (const FBoneRef* B = Bones.Find(FName(Name))) return B->Index;
	return INDEX_NONE;
}

void ADiverPawn::SetupRig()
{
	bRigReady = false;
	USkeletalMesh* SK = Cast<USkeletalMesh>(DiverMesh->GetSkinnedAsset());
	if (!SK) return;
	const FReferenceSkeleton& Ref = SK->GetRefSkeleton();
	const TArray<FTransform>& Pose = Ref.GetRefBonePose();
	RefLocal = Pose;
	RefComp.SetNum(Pose.Num());
	Bones.Reset();
	for (int32 i = 0; i < Pose.Num(); ++i)
	{
		const int32 P = Ref.GetParentIndex(i);
		RefComp[i] = P == INDEX_NONE ? Pose[i] : Pose[i] * RefComp[P];
		// register by short name: "mixamorig:LeftArm" / "mixamorig_LeftArm" / "LeftArm" -> "LeftArm"
		FString N = Ref.GetBoneName(i).ToString();
		int32 Cut = INDEX_NONE;
		if (N.FindLastChar(TEXT(':'), Cut)) N = N.Mid(Cut + 1);
		N.RemoveFromStart(TEXT("mixamorig_"));
		N.RemoveFromStart(TEXT("mixamorig"));
		Bones.Add(FName(*N), { i, P });
	}
	bRigReady = Bone(TEXT("Hips")) != INDEX_NONE && DiverMesh->BoneSpaceTransforms.Num() == Pose.Num();
	AnimSwim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Abyss/Diver/A_Diver_Swim.A_Diver_Swim"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	AnimIdle = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Abyss/Diver/A_Diver_Idle.A_Diver_Idle"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	AnimAim  = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Abyss/Diver/A_Diver_Aim.A_Diver_Aim"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	ArmMask.Init(0.f, Pose.Num());
	for (int32 i = 0; i < Pose.Num(); ++i)
	{
		const FString N = Ref.GetBoneName(i).ToString();
		if (N.Contains(TEXT("Shoulder")) || N.Contains(TEXT("Arm")) || N.Contains(TEXT("Hand"))) ArmMask[i] = 1.f;
	}
	if (AnimSwim && AnimIdle && Sampler.Sample(SK, AnimIdle, 0.f, PoseIdle))
	{
		// sanity: an un-animated bone should match the bind pose
		const int32 H = Bone(TEXT("Hips"));
		UE_LOG(LogTemp, Log, TEXT("Diver clips loaded, hips delta %.2f deg"), FMath::RadiansToDegrees(PoseIdle[H].GetRotation().AngularDistance(RefLocal[H].GetRotation())));
	}
	UE_LOG(LogTemp, Log, TEXT("Diver rig: %d bones, ready=%d"), Pose.Num(), bRigReady ? 1 : 0);
}

void ADiverPawn::RotateBone(TArray<FTransform>& Local, int32 i, const FVector& AxisCS, float Deg) const
{
	if (i == INDEX_NONE || FMath::IsNearlyZero(Deg)) return;
	const USkeletalMesh* SK = Cast<USkeletalMesh>(DiverMesh->GetSkinnedAsset());
	const int32 P = SK->GetRefSkeleton().GetParentIndex(i);
	const FVector AxisParent = P == INDEX_NONE ? AxisCS : RefComp[P].InverseTransformVectorNoScale(AxisCS).GetSafeNormal();
	Local[i].SetRotation(FQuat(AxisParent, FMath::DegreesToRadians(Deg)) * Local[i].GetRotation());
}

void ADiverPawn::AnimateRig(float Dt, float Speed01)
{
	if (!bRigReady) return;
	TArray<FTransform> L = RefLocal;
	USkeletalMesh* SKM = Cast<USkeletalMesh>(DiverMesh->GetSkinnedAsset());
	if (AnimSwim && AnimIdle && AnimAim)
	{
		// --- Blender clips: hover <-> swim by speed, tool-ready arms on top
		const float Swim = FMath::Clamp(Speed01, 0.f, 1.f);
		SwimBlend = FMath::FInterpTo(SwimBlend, Swim > 0.15f ? 1.f : 0.f, Dt, 4.f);
		SwimClock += Dt * FMath::Lerp(0.7f, 1.25f, Swim);
		IdleClock += Dt;
		Sampler.Sample(SKM, AnimSwim, SwimClock, PoseSwim);
		Sampler.Sample(SKM, AnimIdle, IdleClock, PoseIdle);
		Sampler.Sample(SKM, AnimAim, 0.f, PoseAim);
		if (PoseSwim.Num() == L.Num() && PoseIdle.Num() == L.Num() && PoseAim.Num() == L.Num())
		{
			for (int32 i = 0; i < L.Num(); ++i)
			{
				FQuat Q = FQuat::Slerp(PoseIdle[i].GetRotation(), PoseSwim[i].GetRotation(), SwimBlend);
				if (ArmMask[i] > 0.f) Q = FQuat::Slerp(Q, PoseAim[i].GetRotation(), ToolOut);
				L[i].SetRotation(Q.GetNormalized()); // keep bind translations (scale-safe)
			}
			// recoil / net sweep on top of the clip
			const FVector Side(1.f, 0.f, 0.f);
			if (Tool == EDiverTool::Harpoon) RotateBone(L, Bone(TEXT("RightForeArm")), Side, -ShootKick * 20.f * ToolOut);
			else if (!bSwingResolved) RotateBone(L, Bone(TEXT("RightArm")), Side, FMath::Lerp(-60.f, 70.f, FMath::Clamp(SwingTime / 0.4f, 0.f, 1.f)));
			DiverMesh->BoneSpaceTransforms = L;
			DiverMesh->MarkRefreshTransformDirty();
			ShootKick = FMath::FInterpTo(ShootKick, 0.f, Dt, 8.f);
			return;
		}
	}
	// component-space axes of the upright skeleton: X = character's left, Y = forward, Z = up
	const FVector Side(1.f, 0.f, 0.f), Fwd(0.f, 1.f, 0.f), Up(0.f, 0.f, 1.f);
	const float Swim = FMath::Clamp(Speed01, 0.f, 1.f);
	const float Rate = 3.f + 5.f * Swim;
	const float Ph = Time * Rate;
	const float KS = KickSign;

	// --- legs: flutter kick (swim) / slow scissor (tread water)
	const float KickAmp = FMath::Lerp(14.f, 26.f, Swim);
	RotateBone(L, Bone(TEXT("LeftUpLeg")), Side, KS * KickAmp * FMath::Sin(Ph));
	RotateBone(L, Bone(TEXT("RightUpLeg")), Side, -KS * KickAmp * FMath::Sin(Ph));
	RotateBone(L, Bone(TEXT("LeftLeg")), Side, KS * (12.f + 14.f * FMath::Max(0.f, FMath::Sin(Ph - 0.8f))));
	RotateBone(L, Bone(TEXT("RightLeg")), Side, KS * (12.f + 14.f * FMath::Max(0.f, -FMath::Sin(Ph - 0.8f))));
	RotateBone(L, Bone(TEXT("LeftFoot")), Side, -KS * 35.f);   // point the fins
	RotateBone(L, Bone(TEXT("RightFoot")), Side, -KS * 35.f);

	// --- spine undulation + head looking ahead while prone
	RotateBone(L, Bone(TEXT("Spine")), Side, 4.f * FMath::Sin(Ph + 1.f) * Swim);
	RotateBone(L, Bone(TEXT("Neck")), Side, KS * 25.f * Swim);
	RotateBone(L, Bone(TEXT("Head")), Side, KS * 20.f * Swim);

	// --- arms: overhead holding the tool while swimming, sculling in front when idle
	const float AS = ArmSign;
	// T-pose -> arms down along the body (hands free) or toward the head (holding the tool)
	const float Overhead = FMath::Lerp(-75.f, FMath::Lerp(55.f, 80.f, Swim), ToolOut);
	const float Scull = (1.f - Swim) * 20.f * FMath::Sin(Time * 2.2f);
	float RightReach = 0.f, RightSweep = 0.f;
	if (Tool == EDiverTool::Harpoon)
	{
		RightReach = (8.f - ShootKick * 25.f) * ToolOut; // recoil
	}
	else if (!bSwingResolved)
	{
		const float A = FMath::Clamp(SwingTime / 0.4f, 0.f, 1.f);
		RightSweep = FMath::Lerp(-60.f, 70.f, A);
	}
	// raise toward +Z (head): left arm lies along +X, right along -X
	RotateBone(L, Bone(TEXT("LeftArm")), Fwd, -AS * (Overhead + Scull));
	RotateBone(L, Bone(TEXT("RightArm")), Fwd, AS * (Overhead + RightReach - Scull));
	RotateBone(L, Bone(TEXT("RightArm")), Side, RightSweep);
	RotateBone(L, Bone(TEXT("LeftForeArm")), Up, -AS * 15.f);
	RotateBone(L, Bone(TEXT("RightForeArm")), Up, AS * 15.f);

	DiverMesh->BoneSpaceTransforms = L;
	DiverMesh->MarkRefreshTransformDirty();
	ShootKick = FMath::FInterpTo(ShootKick, 0.f, Dt, 8.f);
}

void ADiverPawn::SetupPlayerInputComponent(UInputComponent* IC)
{
	Super::SetupPlayerInputComponent(IC);
	IC->BindAxis(TEXT("MoveForward"), this, &ADiverPawn::MoveForward);
	IC->BindAxis(TEXT("MoveRight"), this, &ADiverPawn::MoveRight);
	IC->BindAxis(TEXT("MoveUp"), this, &ADiverPawn::MoveUp);
	IC->BindAxis(TEXT("Turn"), this, &ADiverPawn::Turn);
	IC->BindAxis(TEXT("LookUp"), this, &ADiverPawn::LookUp);
	IC->BindAction(TEXT("Catch"), IE_Pressed, this, &ADiverPawn::Fire);
	IC->BindAction(TEXT("Boost"), IE_Pressed, this, &ADiverPawn::BoostPressed);
	IC->BindAction(TEXT("Boost"), IE_Released, this, &ADiverPawn::BoostReleased);
	IC->BindAction(TEXT("Order1"), IE_Pressed, this, &ADiverPawn::SelectHarpoon);
	IC->BindAction(TEXT("Order2"), IE_Pressed, this, &ADiverPawn::SelectNet);
	IC->BindAction(TEXT("Interact"), IE_Pressed, this, &ADiverPawn::Interact);
	IC->BindAction(TEXT("ToggleLight"), IE_Pressed, this, &ADiverPawn::ToggleLamp);
}

void ADiverPawn::BeginDive(ASubmarinePawn* FromSub)
{
	Sub = FromSub;
	Oxygen01 = 1.f;
	Cooldown = 0.5f;
	FMemory::Memzero(DiveCatch, sizeof(DiveCatch));
	Tool = EDiverTool::Harpoon;
	ShowTool();
}

float ADiverPawn::GetHeading() const
{
	return Controller ? Controller->GetControlRotation().Yaw : GetActorRotation().Yaw;
}

float ADiverPawn::OxygenSeconds() const
{
	const AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	return BaseOxygenSeconds * (1.f + 0.33f * (GM ? GM->OxygenLevel : 0));
}

void ADiverPawn::ShowTool()
{
	const bool bOut = ToolOut > 0.35f;
	HarpoonRoot->SetVisibility(bOut && Tool == EDiverTool::Harpoon, true);
	NetRoot->SetVisibility(bOut && Tool == EDiverTool::Net, true);
}

void ADiverPawn::SelectHarpoon() { Tool = EDiverTool::Harpoon; ToolIdleTime = 0.f; ShowTool(); }
void ADiverPawn::SelectNet()     { Tool = EDiverTool::Net; ToolIdleTime = 0.f; ShowTool(); }
void ADiverPawn::ToggleLamp()    { HeadLamp->SetVisibility(!HeadLamp->IsVisible()); }

void ADiverPawn::Interact()
{
	if (bNearHatch)
	{
		if (AAbyssGameMode* GM = AAbyssGameMode::Get(this))
		{
			GM->ReturnDiverToSub(false);
		}
	}
}

ACatchableFish* ADiverPawn::FindFishOnAim(float MaxRange) const
{
	const FVector O = Camera->GetComponentLocation();
	const FVector D = Camera->GetForwardVector();
	const FVector Me = GetActorLocation();
	ACatchableFish* Best = nullptr;
	float BestT = 1e9f;
	for (TActorIterator<ACatchableFish> It(GetWorld()); It; ++It)
	{
		ACatchableFish* F = *It;
		if (!F->IsCatchable()) continue;
		const FVector P = F->GetActorLocation();
		if (FVector::Dist(P, Me) > MaxRange) continue;
		const float T = FVector::DotProduct(P - O, D);
		if (T < 0.f) continue;
		const float Perp = FVector::Dist(O + D * T, P);
		if (Perp > F->GetRadius() * 1.6f + 30.f) continue;
		if (T < BestT)
		{
			FHitResult Hit;
			FCollisionQueryParams Q(SCENE_QUERY_STAT(DiverAim), false, this);
			if (GetWorld()->LineTraceSingleByChannel(Hit, Me, P, ECC_Visibility, Q)) continue;
			BestT = T;
			Best = F;
		}
	}
	return Best;
}

void ADiverPawn::GatherNetTargets(TArray<ACatchableFish*>& Out) const
{
	Out.Reset();
	const FVector Me = GetActorLocation();
	const FVector D = Camera->GetForwardVector();
	const float CosCone = FMath::Cos(FMath::DegreesToRadians(NetConeDegrees));
	for (TActorIterator<ACatchableFish> It(GetWorld()); It; ++It)
	{
		ACatchableFish* F = *It;
		if (!F->IsCatchable()) continue;
		const FVector To = F->GetActorLocation() - Me;
		const float Dist = To.Size();
		if (Dist > NetRange + F->GetRadius()) continue;
		if (Dist > 1.f && FVector::DotProduct(To / Dist, D) < CosCone) continue;
		Out.Add(F);
	}
	Out.Sort([Me](const ACatchableFish& A, const ACatchableFish& B) { return FVector::DistSquared(A.GetActorLocation(), Me) < FVector::DistSquared(B.GetActorLocation(), Me); });
}

bool ADiverPawn::LandFish(ACatchableFish* F, ECatchMethod Method, bool bPrime)
{
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	if (!GM) return false;
	if (GM->AddFish(F->Type, bPrime, Method != ECatchMethod::Harpoon))
	{
		DiveCatch[(int32)F->Type]++;
		F->OnCaught();
		return true;
	}
	GM->ShowToast(TEXT("Cargo hold full - return to the sub [E]"), FLinearColor(1.f, 0.6f, 0.2f));
	return false;
}

void ADiverPawn::Fire()
{
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	if (!GM || Cooldown > 0.f) return;
	ToolIdleTime = 0.f;
	if (ToolOut < 0.6f)
	{
		// draw the tool; shoot on the next click
		Cooldown = 0.25f;
		return;
	}

	if (Tool == EDiverTool::Harpoon)
	{
		Cooldown = HarpoonCooldown;
		ShootKick = 1.f;
		AbyssSfx::Play(this, TEXT("Thunk"));
		const FVector Muzzle = HarpoonRoot->GetComponentLocation() + GetActorForwardVector() * 100.f;
		ACatchableFish* F = FindFishOnAim(HarpoonRange);
		FVector AimPoint = Camera->GetComponentLocation() + Camera->GetForwardVector() * (HarpoonRange + SpringArm->TargetArmLength);
		if (F)
		{
			AimPoint = F->GetActorLocation();
		}
		else
		{
			FHitResult Hit;
			FCollisionQueryParams Q(SCENE_QUERY_STAT(DiverShot), false, this);
			if (GetWorld()->LineTraceSingleByChannel(Hit, Camera->GetComponentLocation(), AimPoint, ECC_Visibility, Q))
			{
				AimPoint = Hit.ImpactPoint;
			}
		}
		SpearFrom = Muzzle;
		SpearTo = AimPoint;
		SpearTime = 0.f;
		Spear->SetHiddenInGame(false);

		if (F)
		{
			const FFishDef& D = F->Def();
			if (!D.bHarpoon)
			{
				F->Escape(GetActorLocation());
				GM->ShowToast(FString::Printf(TEXT("The spear glances off the %s"), D.Name), FLinearColor(1.f, 0.6f, 0.3f));
			}
			else if (F->OnHarpoonHit())
			{
				const bool bPrime = F->PrimeIfCaught(ECatchMethod::Harpoon, 1);
				const EFishType FT = F->Type;
				F->Spook(GetActorLocation(), true);   // the rest of the school bolts
				if (LandFish(F, ECatchMethod::Harpoon, bPrime))
				{
					if (bPrime) GM->ShowToast(FString::Printf(TEXT("PRIME catch: %s!  Sells for +35%%"), D.Name), FLinearColor(1.f, 0.85f, 0.3f), 3.f);
					else GM->ShowToast(FString::Printf(TEXT("Speared: %s (ordinary). %s"), D.Name, AbyssData::PrimeTip(FT)), FLinearColor(0.4f, 1.f, 0.8f), 3.5f);
				}
			}
			else
			{
				GM->ShowToast(FString::Printf(TEXT("Hit! The %s is stunned - shoot again!"), D.Name), FLinearColor(1.f, 0.9f, 0.4f), 1.6f);
			}
		}
	}
	else
	{
		Cooldown = 1.1f;
		SwingTime = 0.f;
		AbyssSfx::Play(this, TEXT("Whoosh"));
		bSwingResolved = false;
	}
}

void ADiverPawn::ResolveNet()
{
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	TArray<ACatchableFish*> Targets;
	GatherNetTargets(Targets);
	int32 Caught = 0, PrimeN = 0;
	bool bEscaped = false;
	// how many fit in this throw decides the quality: a full, calm haul is prime
	int32 Haul = 0;
	for (ACatchableFish* F : Targets) if (F->Def().bNet) ++Haul;
	Haul = FMath::Min(Haul, NetMaxCatch);
	ACatchableFish* Missed = nullptr;
	for (ACatchableFish* F : Targets)
	{
		if (!F->Def().bNet)
		{
			F->Escape(GetActorLocation());
			bEscaped = true;
			continue;
		}
		if (Caught >= NetMaxCatch) { Missed = F; break; }
		const bool bPrime = F->PrimeIfCaught(ECatchMethod::Net, Haul);
		if (LandFish(F, ECatchMethod::Net, bPrime))
		{
			++Caught;
			if (bPrime) ++PrimeN;
		}
	}
	// whatever the net missed scatters
	if (Caught > 0)
	{
		for (TActorIterator<ACatchableFish> It(GetWorld()); It; ++It)
		{
			if (It->IsCatchable() && It->Def().bNet && FVector::Dist(It->GetActorLocation(), GetActorLocation()) < 1400.f) It->Spook(GetActorLocation(), false);
		}
	}
	(void)Missed;
	if (!GM) return;
	if (Caught > 0 && PrimeN > 0)
	{
		GM->ShowToast(FString::Printf(TEXT("Clean haul! Netted %d PRIME fish - sells for +35%%"), Caught), FLinearColor(1.f, 0.85f, 0.3f), 3.f);
	}
	else if (Caught > 0)
	{
		GM->ShowToast(FString::Printf(TEXT("Netted %d fish (ordinary). %s"), Caught, AbyssData::PrimeTip(EFishType::Yellowtail)), FLinearColor(0.4f, 1.f, 0.8f), 3.5f);
	}
	else if (Targets.Num() == 0)
	{
		GM->ShowToast(TEXT("Missed - let the school settle, then drift in slowly"), FLinearColor(1.f, 0.8f, 0.4f), 2.5f);
	}
	else if (bEscaped)
	{
		GM->ShowToast(TEXT("Too strong for the net - it broke free! Use the speargun [1]"), FLinearColor(1.f, 0.55f, 0.3f));
	}
}

void ADiverPawn::Tick(float Dt)
{
	Super::Tick(Dt);
	Time += Dt;
	Cooldown = FMath::Max(0.f, Cooldown - Dt);
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);

	// --- swim
	const FRotator Ctrl = Controller ? Controller->GetControlRotation() : GetActorRotation();
	const FVector Fwd = Ctrl.Vector();
	const FVector Right = FRotationMatrix(FRotator(0.f, Ctrl.Yaw, 0.f)).GetUnitAxis(EAxis::Y);
	Movement->MaxSpeed = SwimSpeed * (bBoost ? 1.6f : 1.f);
	AddMovementInput(Fwd, MoveInput.X);
	AddMovementInput(Right, MoveInput.Y);
	AddMovementInput(FVector::UpVector, MoveInput.Z);

	// --- tether to the submarine + depth limit
	if (ASubmarinePawn* S = Sub.Get())
	{
		const FVector ToSub = S->GetActorLocation() - GetActorLocation();
		DistToSub = ToSub.Size();
		bNearHatch = DistToSub < 650.f;
		bTooFar = DistToSub > SafeRange;
		if (DistToSub > SafeRange + 500.f)
		{
			AddMovementInput(ToSub.GetSafeNormal(), 1.f);
		}
	}
	bTooDeep = -GetActorLocation().Z > AbyssData::DiverMaxDepth;
	if (bTooDeep)
	{
		AddMovementInput(FVector::UpVector, 1.f);
	}
	FVector Loc = GetActorLocation();
	if (Loc.Z > -200.f) { Loc.Z = -200.f; SetActorLocation(Loc); }

	const float Speed01 = FMath::Clamp(GetVelocity().Size() / SwimSpeed, 0.f, 1.5f);
	const float WantPitch = FMath::Clamp(Ctrl.Pitch, -70.f, 70.f) * FMath::Clamp(MoveInput.X, 0.f, 1.f);
	if (!bFreezeRotation) SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(WantPitch, Ctrl.Yaw, 0.f), Dt, 3.f));

	// body: idle = more upright treading water, swimming = prone
	const float Upright = FMath::Lerp(55.f, 0.f, FMath::Clamp(Speed01 * 1.5f, 0.f, 1.f));
	Body->SetRelativeRotation(FMath::RInterpTo(Body->GetRelativeRotation(), FRotator(Upright, 0.f, -MoveInput.Y * 15.f + FMath::Sin(Time * 1.1f) * 3.f), Dt, 3.f));
	Body->SetRelativeLocation(FVector(0.f, 0.f, FMath::Sin(Time * 1.3f) * 6.f));

	const float KickRate = 3.f + 6.f * Speed01;
	const float Kick = FMath::Sin(Time * KickRate) * (12.f + 14.f * Speed01);
	LegL->SetRelativeRotation(FRotator(Kick, 0.f, 0.f));
	LegR->SetRelativeRotation(FRotator(-Kick, 0.f, 0.f));
	AnimateRig(Dt, Speed01);

	// --- oxygen
	if (GM && GM->Phase == EAbyssPhase::Explore)
	{
		const float Drain = (bBoost ? 1.5f : 1.f) * (bTooDeep ? 2.f : 1.f);
		if (!GM->IsTutorial()) Oxygen01 = FMath::Max(0.f, Oxygen01 - Dt * Drain / OxygenSeconds());
		if (Oxygen01 <= 0.f)
		{
			GM->ReturnDiverToSub(true);
			return;
		}
	}

	// --- aim + tool animations
	Aimed = Tool == EDiverTool::Harpoon ? FindFishOnAim(HarpoonRange) : nullptr;
	ToolIdleTime += Dt;
	{
		TArray<ACatchableFish*> Near;
		GatherNetTargets(Near);
		if (Aimed.IsValid() || Near.Num() > 0) ToolIdleTime = FMath::Min(ToolIdleTime, 0.5f); // fish nearby -> ready the tool
	}
	const float WantOut = (ToolIdleTime < 2.f || !bSwingResolved) ? 1.f : 0.f;
	ToolOut = FMath::FInterpTo(ToolOut, WantOut, Dt, 6.f);
	ShowTool();
	NetTargets.Reset();
	if (Tool == EDiverTool::Net)
	{
		TArray<ACatchableFish*> T;
		GatherNetTargets(T);
		for (ACatchableFish* F : T) NetTargets.Add(F);
	}

	if (!bSwingResolved)
	{
		SwingTime += Dt;
		const float A = FMath::Clamp(SwingTime / 0.4f, 0.f, 1.f);
		NetRoot->SetRelativeRotation(FRotator(0.f, FMath::Lerp(55.f, -55.f, A), 0.f));
		if (A >= 0.5f)
		{
			bSwingResolved = true;
			ResolveNet();
		}
	}
	else
	{
		NetRoot->SetRelativeRotation(FMath::RInterpTo(NetRoot->GetRelativeRotation(), FRotator::ZeroRotator, Dt, 6.f));
	}

	if (!Spear->bHiddenInGame)
	{
		SpearTime += Dt;
		const float A = FMath::Clamp(SpearTime / 0.12f, 0.f, 1.f);
		const FVector Tip = FMath::Lerp(SpearFrom, SpearTo, A);
		const FVector Dir = (SpearTo - SpearFrom).GetSafeNormal();
		Spear->SetWorldLocationAndRotation(Tip - Dir * 45.f, FRotationMatrix::MakeFromZ(Dir).Rotator());
		Spear->SetWorldScale3D(FVector(0.025f, 0.025f, 0.9f));
		if (SpearTime > 0.5f) Spear->SetHiddenInGame(true);
	}

	UpdateBubbles(Dt);
}

void ADiverPawn::UpdateBubbles(float Dt)
{
	if (BubbleData.Num() == 0) return;
	BubbleTimer -= Dt;
	// regulator exhale: a burst every ~3 s plus a trickle
	const bool bBurst = FMath::Fmod(Time, 3.2f) < 0.6f;
	if (BubbleTimer <= 0.f)
	{
		BubbleTimer = bBurst ? 0.04f : 0.35f;
		FBubble& B = BubbleData[NextBubble];
		NextBubble = (NextBubble + 1) % BubbleData.Num();
		B.P = Body->GetComponentTransform().TransformPosition(FVector(100.f, 0.f, 0.f)) + FMath::VRand() * 6.f;
		B.Life = 3.5f;
		B.Speed = FMath::FRandRange(70.f, 120.f);
		B.Phase = FMath::FRand() * 6.f;
	}
	for (int32 i = 0; i < BubbleData.Num(); ++i)
	{
		FBubble& B = BubbleData[i];
		float S = 0.001f;
		if (B.Life > 0.f)
		{
			B.Life -= Dt;
			B.P.Z += B.Speed * Dt;
			B.P.X += FMath::Sin(Time * 6.f + B.Phase) * 12.f * Dt;
			B.P.Y += FMath::Cos(Time * 5.f + B.Phase) * 12.f * Dt;
			S = FMath::Lerp(0.03f, 0.09f, 1.f - B.Life / 3.5f);
		}
		BubbleT[i] = FTransform(FQuat::Identity, B.P, FVector(S, S, S * 0.8f));
	}
	Bubbles->BatchUpdateInstancesTransforms(0, BubbleT, true, true, true);
}
