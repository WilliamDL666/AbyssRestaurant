#include "RestaurantWalker.h"
#include "AbyssAssets.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "AbyssGameMode.h"
#include "RestaurantDome.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

static const TCHAR* ChefMeshPath = TEXT("/Game/Abyss/Chef/SK_Chef.SK_Chef");
static const TCHAR* ChefMatPath = TEXT("/Game/Abyss/Chef/M_Chef.M_Chef");

ARestaurantWalker::ARestaurantWalker()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	bUseControllerRotationYaw = false;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 90.f);
	UCharacterMovementComponent* M = GetCharacterMovement();
	M->bOrientRotationToMovement = true;
	M->RotationRate = FRotator(0.f, 540.f, 0.f);
	M->MaxWalkSpeed = 330.f;
	M->BrakingDecelerationWalking = 1600.f;

	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual"));
	Visual->SetupAttachment(GetCapsuleComponent());
	Visual->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	LegL = CreateDefaultSubobject<USceneComponent>(TEXT("LegL"));
	LegL->SetupAttachment(Visual);
	LegL->SetRelativeLocation(FVector(0.f, 12.f, 90.f));
	LegR = CreateDefaultSubobject<USceneComponent>(TEXT("LegR"));
	LegR->SetupAttachment(Visual);
	LegR->SetRelativeLocation(FVector(0.f, -12.f, 90.f));

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetCapsuleComponent());
	SpringArm->TargetArmLength = 420.f;
	SpringArm->SocketOffset = FVector(0.f, 40.f, 90.f);
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.f;
	SpringArm->ProbeSize = 15.f;

	ChefMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("ChefMesh"));
	ChefMesh->SetupAttachment(Visual);
	ChefMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChefMesh->SetVisibility(false);

	CarryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarryMesh"));
	CarryMesh->SetupAttachment(Visual);
	CarryMesh->SetRelativeLocation(FVector(42.f, 0.f, 112.f));
	CarryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CarryMesh->SetVisibility(false);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->SetFieldOfView(80.f);
}

void ARestaurantWalker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	using namespace AbyssAssets;
	UMaterialInterface* White = Lit(this, FLinearColor(0.9f, 0.9f, 0.88f), 0.7f);
	UMaterialInterface* Navy  = Lit(this, FLinearColor(0.03f, 0.04f, 0.09f), 0.8f);
	UMaterialInterface* Skin  = Lit(this, FLinearColor(0.75f, 0.55f, 0.4f), 0.55f);
	UMaterialInterface* Hair  = Lit(this, FLinearColor(0.02f, 0.015f, 0.01f), 0.6f);
	UMaterialInterface* Red   = Lit(this, FLinearColor(0.6f, 0.05f, 0.05f), 0.6f);

	USkeletalMesh* SK = LoadObject<USkeletalMesh>(nullptr, ChefMeshPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (AbyssRig::FitHumanoid(ChefMesh, SK, 182.f, LoadObject<UMaterialInterface>(nullptr, ChefMatPath, nullptr, LOAD_NoWarn | LOAD_Quiet)))
	{
		return; // the model replaces the primitive chef
	}

	for (USceneComponent* Leg : { LegL.Get(), LegR.Get() })
	{
		AddPart(this, Leg, Cylinder(), FVector(0.f, 0.f, -45.f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 0.9f), Navy);
		AddPart(this, Leg, Cube(), FVector(8.f, 0.f, -88.f), FRotator::ZeroRotator, FVector(0.3f, 0.14f, 0.08f), Hair);
	}
	AddPart(this, Visual, Cylinder(), FVector(0.f, 0.f, 125.f), FRotator::ZeroRotator, FVector(0.48f, 0.36f, 0.72f), White);
	AddPart(this, Visual, Sphere(), FVector(0.f, 0.f, 160.f), FRotator::ZeroRotator, FVector(0.55f, 0.42f, 0.2f), White);
	AddPart(this, Visual, Cylinder(), FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, FVector(0.5f, 0.38f, 0.08f), Red);
	for (float Y : { 26.f, -26.f })
	{
		AddPart(this, Visual, Cylinder(), FVector(0.f, Y, 128.f), FRotator(0.f, 0.f, Y > 0 ? -8.f : 8.f), FVector(0.1f, 0.1f, 0.55f), White);
		AddPart(this, Visual, Sphere(), FVector(0.f, Y * 1.05f, 98.f), FRotator::ZeroRotator, FVector(0.1f), Skin);
	}
	AddPart(this, Visual, Sphere(), FVector(0.f, 0.f, 192.f), FRotator::ZeroRotator, FVector(0.22f, 0.2f, 0.26f), Skin);
	AddPart(this, Visual, Sphere(), FVector(-3.f, 0.f, 200.f), FRotator::ZeroRotator, FVector(0.22f, 0.2f, 0.16f), Hair);
	AddPart(this, Visual, Cylinder(), FVector(0.f, 0.f, 204.f), FRotator::ZeroRotator, FVector(0.235f, 0.215f, 0.05f), White);
}

void ARestaurantWalker::BeginPlay()
{
	Super::BeginPlay();
	if (ChefMesh->GetSkinnedAsset()) Rig.Init(ChefMesh);
}

void ARestaurantWalker::SetupPlayerInputComponent(UInputComponent* IC)
{
	Super::SetupPlayerInputComponent(IC);
	IC->BindAxis(TEXT("MoveForward"), this, &ARestaurantWalker::MoveForward);
	IC->BindAxis(TEXT("MoveRight"), this, &ARestaurantWalker::MoveRight);
	IC->BindAxis(TEXT("Turn"), this, &ARestaurantWalker::Turn);
	IC->BindAxis(TEXT("LookUp"), this, &ARestaurantWalker::LookUp);
	IC->BindAction(TEXT("Interact"), IE_Pressed, this, &ARestaurantWalker::Interact);
}

void ARestaurantWalker::Interact()
{
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this)) GM->RestaurantInteract();
}

static bool IsCookingNow(const UObject* W)
{
	const AAbyssGameMode* GM = AAbyssGameMode::Get(W);
	return GM && GM->GetDome() && ((GM->GetDome()->Kitchen.bActive && GM->GetDome()->Kitchen.bAtStation) || GM->GetDome()->Counter.bOpen);
}

void ARestaurantWalker::MoveForward(float V)
{
	if (IsCookingNow(this)) return;
	if (Controller && V != 0.f)
	{
		AddMovementInput(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector(), V);
	}
}

void ARestaurantWalker::MoveRight(float V)
{
	if (IsCookingNow(this)) return;
	if (Controller && V != 0.f)
	{
		AddMovementInput(FRotationMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::Y), V);
	}
}

void ARestaurantWalker::Tick(float Dt)
{
	Super::Tick(Dt);
	Time += Dt;

	// stay on the dome floor
	FVector P = GetActorLocation();
	FVector2D Off(P.X - RoomCenter.X, P.Y - RoomCenter.Y);
	if (Off.Size() > RoomRadius)
	{
		Off = Off.GetSafeNormal() * RoomRadius;
		SetActorLocation(FVector(RoomCenter.X + Off.X, RoomCenter.Y + Off.Y, P.Z));
	}

	// keep the camera inside the glass: shorten the boom near the wall
	{
		const FVector2D Rel(P.X - RoomCenter.X, P.Y - RoomCenter.Y);
		const FVector Back = -(Controller ? FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f).Vector() : GetActorForwardVector());
		const FVector2D B(Back.X, Back.Y);
		// distance along Back until the circle of RoomRadius+60
		const float Rr = RoomRadius + 60.f;
		const float Bd = FVector2D::DotProduct(Rel, B);
		const float Disc = Bd * Bd - (Rel.SizeSquared() - Rr * Rr);
		const float MaxArm = Disc > 0.f ? (-Bd + FMath::Sqrt(Disc)) : 150.f;
		SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, FMath::Clamp(MaxArm - 40.f, 150.f, 420.f), Dt, 8.f);
	}

	// simple walk cycle
	const float Speed01 = FMath::Clamp(GetVelocity().Size2D() / 330.f, 0.f, 1.f);
	if (Rig.bReady)
	{
		EChefWork Work = EChefWork::None;
		const AAbyssGameMode* GM = AAbyssGameMode::Get(this);
		if (GM && GM->GetDome() && GM->GetDome()->Counter.bOpen)
		{
			Work = GM->GetDome()->Counter.Pans[0].Busy() || GM->GetDome()->Counter.Pans[1].Busy() ? EChefWork::Stir : EChefWork::Chop;
		}
		else if (IsCookingNow(this) && GM && GM->GetDome())
		{
			switch (GM->GetDome()->Kitchen.Step)
			{
			case EKitchenStep::Slice: Work = EChefWork::Chop; break;
			case EKitchenStep::Cook:  Work = EChefWork::Stir; break;
			default:                  Work = EChefWork::Plate; break;
			}
		}
		WorkBlend = FMath::FInterpTo(WorkBlend, Work != EChefWork::None ? 1.f : 0.f, Dt, 6.f);
		// show the plated dish in the chef's hands while it is carried to the table
		{
			const ARestaurantDome* Dm = GM ? GM->GetDome() : nullptr;
			const int32 Want = Dm && Dm->IsCarrying() ? Dm->CarryDish : -1;
			if (Want != ShownCarry)
			{
				ShownCarry = Want;
				static const TCHAR* DishMeshes[] = { TEXT("SM_Dish_Nigiri"), TEXT("SM_Dish_Tuna"), TEXT("SM_Dish_Grouper"), TEXT("SM_Dish_Kelp") };
				UStaticMesh* M = nullptr;
				if (Want >= 0)
				{
					const TCHAR* N = DishMeshes[FMath::Clamp(Want, 0, 3)];
					M = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Abyss/Food/%s.%s"), N, N), nullptr, LOAD_NoWarn | LOAD_Quiet);
				}
				CarryMesh->SetStaticMesh(M);
				CarryMesh->SetVisibility(M != nullptr);
			}
			HoldBlend = FMath::FInterpTo(HoldBlend, Want >= 0 ? 1.f : 0.f, Dt, 8.f);
		}
		static EChefWork LastWork = EChefWork::Chop;
		if (Work != EChefWork::None) LastWork = Work;
		AbyssRig::Humanoid(Rig, Time, Speed01, LastWork, WorkBlend, HoldBlend);
		Visual->SetRelativeLocation(FVector(0.f, 0.f, -90.f + FMath::Abs(FMath::Sin(Time * 8.5f)) * 2.f * Speed01));
		// tight spots (behind the bar): hide the chef when the camera is pushed into him
		const float CamDist = FVector::Dist2D(Camera->GetComponentLocation(), GetActorLocation());
		const bool bAtCounter = GM && GM->GetDome() && GM->GetDome()->Counter.bOpen;
		ChefMesh->SetVisibility(CamDist > 70.f && !bAtCounter);
		CarryMesh->SetVisibility(CamDist > 70.f && ShownCarry >= 0);
		return;
	}
	const float Swing = FMath::Sin(Time * 9.f) * 28.f * Speed01;
	LegL->SetRelativeRotation(FRotator(Swing, 0.f, 0.f));
	LegR->SetRelativeRotation(FRotator(-Swing, 0.f, 0.f));
	Visual->SetRelativeLocation(FVector(0.f, 0.f, -90.f + FMath::Abs(FMath::Sin(Time * 9.f)) * 4.f * Speed01));
}

void ARestaurantWalker::Turn(float V)
{
	if (!IsCookingNow(this)) AddControllerYawInput(V);
}

void ARestaurantWalker::LookUp(float V)
{
	if (!IsCookingNow(this)) AddControllerPitchInput(V);
}
