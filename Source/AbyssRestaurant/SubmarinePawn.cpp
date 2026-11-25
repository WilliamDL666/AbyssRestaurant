#include "SubmarinePawn.h"
#include "AbyssAssets.h"
#include "AbyssGameMode.h"
#include "CatchableFish.h"
#include "RestaurantDome.h"
#include "Components/SphereComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Engine/World.h"

ASubmarinePawn::ASubmarinePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(170.f);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	SetRootComponent(Collision);

	Hull = CreateDefaultSubobject<USceneComponent>(TEXT("Hull"));
	Hull->SetupAttachment(Collision);

	PropHub = CreateDefaultSubobject<USceneComponent>(TEXT("PropHub"));
	PropHub->SetupAttachment(Hull);
	PropHub->SetRelativeLocation(FVector(-262.f, 0.f, 0.f));

	Headlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Headlight"));
	Headlight->SetupAttachment(Hull);
	Headlight->SetRelativeLocation(FVector(232.f, 0.f, -58.f));
	Headlight->SetRelativeRotation(FRotator(-7.f, 0.f, 0.f));
	Headlight->IntensityUnits = ELightUnits::Candelas;
	Headlight->SetIntensity(HeadlightCandela);
	Headlight->SetLightColor(FLinearColor(1.f, 0.93f, 0.78f));
	Headlight->SetAttenuationRadius(4500.f);
	Headlight->SetInnerConeAngle(12.f);
	Headlight->SetOuterConeAngle(24.f);
	Headlight->SetCastShadows(true);
	Headlight->bCastVolumetricShadow = true;
	Headlight->VolumetricScatteringIntensity = 1.2f;
	Headlight->SourceRadius = 12.f;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Collision);
	SpringArm->TargetArmLength = 950.f;
	SpringArm->SocketOffset = FVector(0.f, 160.f, 170.f);
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 5.f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 9.f;
	SpringArm->ProbeSize = 30.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->SetFieldOfView(78.f);

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = CruiseSpeed;
	Movement->Acceleration = 520.f;
	Movement->Deceleration = 380.f;
	Movement->TurningBoost = 1.5f;

	MarineSnow = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MarineSnow"));
	MarineSnow->SetupAttachment(Collision);
	MarineSnow->SetUsingAbsoluteLocation(true);
	MarineSnow->SetUsingAbsoluteRotation(true);
	MarineSnow->SetUsingAbsoluteScale(true);
	MarineSnow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarineSnow->SetCastShadow(false);

	PropBubbles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PropBubbles"));
	PropBubbles->SetupAttachment(Collision);
	PropBubbles->SetUsingAbsoluteLocation(true);
	PropBubbles->SetUsingAbsoluteRotation(true);
	PropBubbles->SetUsingAbsoluteScale(true);
	PropBubbles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PropBubbles->SetCastShadow(false);

	BubbleFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BubbleFX"));
	BubbleFX->SetupAttachment(PropHub);
	BubbleFX->bAutoActivate = false;
}

void ASubmarinePawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	using namespace AbyssAssets;

	UStaticMesh* Custom = Cast<UStaticMesh>(CustomHullMesh.TryLoad());
	bHasCustomHull = Custom != nullptr;
	if (Custom)
	{
		// Fit the imported model: longest horizontal axis -> +X, scaled to HullLength, centered on the pivot
		const FBoxSphereBounds B = Custom->GetBounds();
		const bool bAlongY = B.BoxExtent.Y > B.BoxExtent.X;
		const float Longest = FMath::Max(B.BoxExtent.X, B.BoxExtent.Y) * 2.f;
		const float Scale = Longest > 1.f ? HullLength / Longest : 1.f;
		const FRotator Rot(0.f, (bAlongY ? -90.f : 0.f) + HullYawOffset, 0.f);
		const FVector Center = Rot.RotateVector(B.Origin * Scale);
		UStaticMeshComponent* M = AddPart(this, Hull, Custom, HullOffset - Center, Rot, FVector(Scale), nullptr);
		M->SetCastShadow(true);
		const FVector E = (bAlongY ? FVector(B.BoxExtent.Y, B.BoxExtent.X, B.BoxExtent.Z) : B.BoxExtent) * Scale;
		if (bCockpitGlass)
		{
			// Sphere mesh radius is 50cm -> scale = radius / 50
			const FVector GC(CockpitGlassCenter.X * E.X, CockpitGlassCenter.Y * E.Y, CockpitGlassCenter.Z * E.Z);
			const FVector GR(CockpitGlassRadii.X * E.X, CockpitGlassRadii.Y * E.Y, CockpitGlassRadii.Z * E.Z);
			UStaticMeshComponent* G = AddPart(this, Hull, Sphere(), HullOffset + GC, FRotator::ZeroRotator, GR / 50.f,
				AbyssAssets::Glass(this, FLinearColor(0.6f, 0.85f, 1.f), CockpitGlassOpacity));
			G->SetCastShadow(false);
		}
		// headlight sits at the nose of the fitted model
		Headlight->SetRelativeLocation(FVector(HullLength * 0.5f + 10.f, 0.f, -B.BoxExtent.Z * Scale * 0.35f));
		PropHub->SetRelativeLocation(FVector(-HullLength * 0.5f - 20.f, 0.f, 0.f));
		PropHub->SetVisibility(false, true);
		BeaconMat = Lit(this, FLinearColor(1.f, 0.35f, 0.05f), 0.3f, 0.f, 20.f);
		Beacon = AddPart(this, Hull, Sphere(), FVector(0.f, 0.f, B.BoxExtent.Z * Scale + 15.f), FRotator::ZeroRotator, FVector(0.14f), BeaconMat);
	}
	else
	{
		BuildPlaceholderHull();
	}

	// Fake volumetric beam (real volumetric scattering also on)
	if (bShowFakeBeam)
	{
		const float L = 1800.f;
		const float Rad = L * FMath::Tan(FMath::DegreesToRadians(20.f));
		BeamMat = Additive(this, FLinearColor(1.f, 0.95f, 0.75f), 0.025f);
		Beam = AddPart(this, Headlight, Cone(), FVector(L * 0.5f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(Rad / 50.f, Rad / 50.f, L / 100.f), BeamMat);
		Beam->SetCastShadow(false);
	}

	// Marine snow particles + propeller bubbles
	MarineSnow->SetStaticMesh(Sphere());
	MarineSnow->SetMaterial(0, Lit(this, FLinearColor(0.8f, 0.95f, 1.f), 0.5f, 0.f, 0.6f));
	if (UParticleSystem* PB = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/AlienOceanEnvironment/Particles/P_Bubbles.P_Bubbles"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		BubbleFX->SetTemplate(PB);
		BubbleFX->SetRelativeScale3D(FVector(0.5f));
	}
	PropBubbles->SetStaticMesh(Sphere());
	PropBubbles->SetMaterial(0, Lit(this, FLinearColor(0.85f, 0.95f, 1.f), 0.05f, 0.6f, 0.4f));
}

void ASubmarinePawn::BuildPlaceholderHull()
{
	using namespace AbyssAssets;
	UMaterialInterface* Paint  = Lit(this, FLinearColor(0.78f, 0.50f, 0.06f), 0.55f, 0.35f);
	UMaterialInterface* Rust   = Lit(this, FLinearColor(0.22f, 0.10f, 0.04f), 0.95f, 0.1f);
	UMaterialInterface* Metal  = Lit(this, FLinearColor(0.10f, 0.10f, 0.10f), 0.4f, 0.9f);
	UMaterialInterface* Steel  = Lit(this, FLinearColor(0.45f, 0.45f, 0.45f), 0.3f, 1.0f);
	UMaterialInterface* Cockpit = Lit(this, FLinearColor(0.02f, 0.05f, 0.06f), 0.3f, 0.2f, 0.f);
	UMaterialInterface* Screen = Lit(this, FLinearColor(0.1f, 0.8f, 1.0f), 0.3f, 0.f, 3.f);
	UMaterialInterface* Lens   = Lit(this, FLinearColor(1.f, 0.95f, 0.8f), 0.1f, 0.f, 25.f);
	UMaterialInterface* Glass  = AbyssAssets::Glass(this, FLinearColor(0.6f, 0.85f, 1.f), 0.12f);
	BeaconMat = Lit(this, FLinearColor(1.f, 0.35f, 0.05f), 0.3f, 0.f, 20.f);

	// Main hull (4.2 m) + nose cap + aft taper
	AddPart(this, Hull, Sphere(), FVector::ZeroVector, FRotator::ZeroRotator, FVector(4.2f, 1.9f, 1.9f), Paint);
	AddPart(this, Hull, Cylinder(), FVector(-120.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(1.75f, 1.75f, 1.2f), Paint);
	AddPart(this, Hull, Cylinder(), FVector(40.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(1.93f, 1.93f, 0.12f), Metal);
	AddPart(this, Hull, Cylinder(), FVector(-60.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(1.9f, 1.9f, 0.1f), Metal);

	// Rust patches scattered on the paint
	FRandomStream R(11);
	for (int32 i = 0; i < 16; ++i)
	{
		const float X = R.FRandRange(-170.f, 150.f);
		const float A = R.FRandRange(0.f, 2.f * PI);
		const float Rad = 95.f * FMath::Sqrt(FMath::Max(0.05f, 1.f - FMath::Square(X / 210.f)));
		const FVector N(X / 210.f * 0.4f, FMath::Cos(A), FMath::Sin(A));
		const FVector P(X, FMath::Cos(A) * Rad, FMath::Sin(A) * Rad);
		const FRotator Rot = FRotationMatrix::MakeFromZ(N.GetSafeNormal()).Rotator();
		const float S = R.FRandRange(0.25f, 0.7f);
		AddPart(this, Hull, Sphere(), P, Rot, FVector(S, S * R.FRandRange(0.5f, 1.f), 0.04f), Rust);
	}

	// Conning tower, hatch, antenna, beacon, handrails
	AddPart(this, Hull, Cylinder(), FVector(-40.f, 0.f, 105.f), FRotator::ZeroRotator, FVector(0.95f, 0.8f, 0.55f), Paint);
	AddPart(this, Hull, Cylinder(), FVector(-40.f, 0.f, 134.f), FRotator::ZeroRotator, FVector(0.7f, 0.6f, 0.06f), Metal);
	AddPart(this, Hull, Cylinder(), FVector(-70.f, 0.f, 190.f), FRotator::ZeroRotator, FVector(0.04f, 0.04f, 1.1f), Steel);
	AddPart(this, Hull, Cylinder(), FVector(-20.f, 0.f, 155.f), FRotator::ZeroRotator, FVector(0.1f, 0.1f, 0.35f), Metal);
	Beacon = AddPart(this, Hull, Sphere(), FVector(-20.f, 0.f, 178.f), FRotator::ZeroRotator, FVector(0.16f), BeaconMat);
	for (float Y : { 52.f, -52.f })
	{
		AddPart(this, Hull, Cylinder(), FVector(-30.f, Y, 98.f), FRotator(90.f, 0.f, 0.f), FVector(0.05f, 0.05f, 2.8f), Steel);
		for (float X : { -150.f, -30.f, 90.f })
		{
			AddPart(this, Hull, Cylinder(), FVector(X, Y, 88.f), FRotator::ZeroRotator, FVector(0.04f, 0.04f, 0.2f), Steel);
		}
	}

	// Glass cockpit bubble with dark interior + glowing screens
	AddPart(this, Hull, Sphere(), FVector(150.f, 0.f, 22.f), FRotator::ZeroRotator, FVector(1.35f, 1.45f, 1.45f), Cockpit);
	AddPart(this, Hull, Cube(), FVector(165.f, 25.f, 20.f), FRotator(0.f, 0.f, 20.f), FVector(0.02f, 0.3f, 0.2f), Screen);
	AddPart(this, Hull, Sphere(), FVector(172.f, 0.f, 24.f), FRotator::ZeroRotator, FVector(1.45f, 1.55f, 1.55f), Glass);
	AddPart(this, Hull, Cylinder(), FVector(140.f, 0.f, 22.f), FRotator(90.f, 0.f, 0.f), FVector(1.62f, 1.62f, 0.1f), Metal);

	// Headlight housing + lens
	AddPart(this, Hull, Cylinder(), FVector(215.f, 0.f, -58.f), FRotator(90.f, 0.f, 0.f), FVector(0.5f, 0.5f, 0.35f), Metal);
	AddPart(this, Hull, Sphere(), FVector(232.f, 0.f, -58.f), FRotator::ZeroRotator, FVector(0.1f, 0.4f, 0.4f), Lens);
	AddPart(this, Hull, Cube(), FVector(195.f, 0.f, -35.f), FRotator::ZeroRotator, FVector(0.3f, 0.1f, 0.4f), Metal);

	// Aft ducted thruster, side thrusters, fins
	AddPart(this, Hull, Cylinder(), FVector(-255.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(1.6f, 1.6f, 0.55f), Metal);
	AddPart(this, Hull, Cylinder(), FVector(-232.f, 0.f, 0.f), FRotator(90.f, 0.f, 0.f), FVector(1.7f, 1.7f, 0.08f), Paint);
	for (float Y : { 118.f, -118.f })
	{
		AddPart(this, Hull, Cylinder(), FVector(-110.f, Y, -25.f), FRotator(90.f, 0.f, 0.f), FVector(0.6f, 0.6f, 1.2f), Metal);
		AddPart(this, Hull, Cube(), FVector(-110.f, Y * 0.8f, -15.f), FRotator::ZeroRotator, FVector(0.5f, 0.35f, 0.08f), Metal);
		AddPart(this, Hull, Sphere(), FVector(-172.f, Y, -25.f), FRotator::ZeroRotator, FVector(0.08f, 0.5f, 0.5f), Lit(this, FLinearColor(0.3f, 0.7f, 1.f), 0.2f, 0.f, 8.f));
	}
	AddPart(this, Hull, Cube(), FVector(-195.f, 0.f, 0.f), FRotator::ZeroRotator, FVector(0.8f, 3.0f, 0.06f), Paint);
	AddPart(this, Hull, Cube(), FVector(-195.f, 0.f, 50.f), FRotator::ZeroRotator, FVector(0.8f, 0.06f, 1.3f), Paint);

	// Propeller blades
	for (int32 i = 0; i < 4; ++i)
	{
		AddPart(this, PropHub, Cube(), FVector::ZeroVector, FRotator(0.f, 0.f, i * 90.f + 20.f), FVector(0.05f, 0.25f, 1.35f), Steel);
	}
	AddPart(this, PropHub, Sphere(), FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.3f), Steel);

	// Robotic catch arm under the nose
	AddPart(this, Hull, Cylinder(), FVector(130.f, 55.f, -85.f), FRotator(60.f, 0.f, 0.f), FVector(0.12f, 0.12f, 0.9f), Steel);
	AddPart(this, Hull, Cube(), FVector(170.f, 55.f, -110.f), FRotator(20.f, 0.f, 0.f), FVector(0.3f, 0.12f, 0.12f), Metal);

	// Hull number
	for (float Side : { 1.f, -1.f })
	{
		UTextRenderComponent* T = NewObject<UTextRenderComponent>(this);
		T->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		T->SetText(FText::FromString(TEXT("NS-07")));
		T->SetTextRenderColor(FColor(30, 20, 8));
		T->SetWorldSize(48.f);
		T->SetHorizontalAlignment(EHTA_Center);
		T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetRelativeLocation(FVector(30.f, Side * 97.f, 5.f));
		T->SetRelativeRotation(FRotator(0.f, Side * 90.f, 0.f));
		T->SetupAttachment(Hull);
		T->RegisterComponent();
	}

}

void ASubmarinePawn::BeginPlay()
{
	Super::BeginPlay();
	Movement->MaxSpeed = CruiseSpeed;

	FRandomStream R(5);
	SnowPos.SetNum(450);
	SnowT.SetNum(SnowPos.Num());
	MarineSnow->ClearInstances();
	for (int32 i = 0; i < SnowPos.Num(); ++i)
	{
		SnowPos[i] = GetActorLocation() + FVector(R.FRandRange(-1400.f, 1400.f), R.FRandRange(-1400.f, 1400.f), R.FRandRange(-900.f, 900.f));
		SnowT[i] = FTransform(FQuat::Identity, SnowPos[i], FVector(R.FRandRange(0.03f, 0.07f)));
	}
	MarineSnow->AddInstances(SnowT, false, true);

	BubbleData.SetNum(40);
	BubbleT.SetNum(BubbleData.Num());
	for (int32 i = 0; i < BubbleData.Num(); ++i)
	{
		BubbleData[i] = { GetActorLocation(), 0.f, (float)i };
		BubbleT[i] = FTransform(FQuat::Identity, GetActorLocation(), FVector(0.001f));
	}
	PropBubbles->ClearInstances();
	PropBubbles->AddInstances(BubbleT, false, true);
	UpdateLightLevel();
}

void ASubmarinePawn::SetupPlayerInputComponent(UInputComponent* IC)
{
	Super::SetupPlayerInputComponent(IC);
	IC->BindAxis(TEXT("MoveForward"), this, &ASubmarinePawn::MoveForward);
	IC->BindAxis(TEXT("MoveRight"), this, &ASubmarinePawn::MoveRight);
	IC->BindAxis(TEXT("MoveUp"), this, &ASubmarinePawn::MoveUp);
	IC->BindAxis(TEXT("Turn"), this, &ASubmarinePawn::Turn);
	IC->BindAxis(TEXT("LookUp"), this, &ASubmarinePawn::LookUp);
	IC->BindAction(TEXT("Catch"), IE_Pressed, this, &ASubmarinePawn::CatchPressed);
	IC->BindAction(TEXT("Catch"), IE_Released, this, &ASubmarinePawn::CatchReleased);
	IC->BindAction(TEXT("Boost"), IE_Pressed, this, &ASubmarinePawn::BoostPressed);
	IC->BindAction(TEXT("Boost"), IE_Released, this, &ASubmarinePawn::BoostReleased);
	IC->BindAction(TEXT("ToggleLight"), IE_Pressed, this, &ASubmarinePawn::ToggleLight);
	IC->BindAction(TEXT("Interact"), IE_Pressed, this, &ASubmarinePawn::Interact);
	IC->BindAction(TEXT("Bait"), IE_Pressed, this, &ASubmarinePawn::DropBait);
}

FVector ASubmarinePawn::GetLightDirection() const
{
	return Headlight ? Headlight->GetForwardVector() : GetActorForwardVector();
}

void ASubmarinePawn::DropBait()
{
	if (!bControlsEnabled) return;
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this))
	{
		// tossed out ahead of the sub; it sinks to the seabed
		GM->DropBait(GetActorLocation() + GetLightDirection() * 900.f - FVector(0.f, 0.f, 120.f));
	}
}

void ASubmarinePawn::Turn(float V)
{
	if (bControlsEnabled) AddControllerYawInput(V);
}

void ASubmarinePawn::LookUp(float V)
{
	if (bControlsEnabled) AddControllerPitchInput(V);
}

void ASubmarinePawn::ToggleLight()
{
	if (!bControlsEnabled) return;
	bLightOn = !bLightOn;
	Headlight->SetVisibility(bLightOn);
	if (Beam) Beam->SetVisibility(bLightOn);
}

void ASubmarinePawn::Interact()
{
	if (!bControlsEnabled) return;
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this))
	{
		if (IsInDockZone())
		{
			GM->EnterRestaurant(false);
		}
		else if (CanDiveOut())
		{
			GM->ExitSubToDive();
		}
		else
		{
			GM->ShowToast(TEXT("Slow down / hover to dive out"), FLinearColor(1.f, 0.8f, 0.4f), 1.5f);
		}
	}
}

float ASubmarinePawn::GetHeading() const
{
	return Controller ? Controller->GetControlRotation().Yaw : GetActorRotation().Yaw;
}

float ASubmarinePawn::GetCatchRange() const
{
	const AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	return BaseCatchRange * (GM ? (1.f + 0.25f * GM->LightLevel) : 1.f);
}

bool ASubmarinePawn::IsInDockZone() const
{
	const AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	return GM && GM->GetDockDome() && GM->GetDockDome()->IsInDockZone(GetActorLocation());
}

void ASubmarinePawn::Refill()
{
	Battery01 = 1.f;
	CatchProgress = 0.f;
	UpdateLightLevel();
}

void ASubmarinePawn::SetControlsEnabled(bool bEnabled)
{
	bControlsEnabled = bEnabled;
	MoveInput = FVector::ZeroVector;
	bCatchHeld = false;
	bBoost = false;
	Movement->StopMovementImmediately();
}

void ASubmarinePawn::UpdateLightLevel()
{
	const AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	const float Mul = GM ? GM->LightMultiplier() : 1.f;
	Headlight->SetIntensity(HeadlightCandela * Mul);
	Headlight->SetAttenuationRadius(4500.f * Mul);
}

void ASubmarinePawn::Tick(float Dt)
{
	Super::Tick(Dt);
	Time += Dt;
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);

	if (bControlsEnabled)
	{
		const FRotator Ctrl = Controller ? Controller->GetControlRotation() : GetActorRotation();
		const FRotator YawOnly(0.f, Ctrl.Yaw, 0.f);
		const FVector Fwd = FRotator(FMath::ClampAngle(Ctrl.Pitch, -40.f, 40.f), Ctrl.Yaw, 0.f).Vector();
		const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

		Movement->MaxSpeed = CruiseSpeed * (bBoost ? BoostMultiplier : 1.f);
		AddMovementInput(Fwd, MoveInput.X);
		AddMovementInput(Right, MoveInput.Y);
		AddMovementInput(FVector::UpVector, MoveInput.Z);

		// Heavy, laggy hull follows the camera
		const float Pitch = FMath::ClampAngle(Ctrl.Pitch, -30.f, 30.f) * FMath::Clamp(MoveInput.X, 0.f, 1.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(Pitch, Ctrl.Yaw, 0.f), Dt, 1.8f));
	}

	// Never leave the water
	FVector Loc = GetActorLocation();
	if (Loc.Z > -300.f)
	{
		Loc.Z = -300.f;
		SetActorLocation(Loc);
		Movement->Velocity.Z = FMath::Min(Movement->Velocity.Z, 0.f);
	}

	// Hull roll from strafing + idle bob
	const float Roll = FMath::FInterpTo(Hull->GetRelativeRotation().Roll, MoveInput.Y * 14.f, Dt, 2.f);
	Hull->SetRelativeRotation(FRotator(FMath::Sin(Time * 0.7f) * 1.5f, 0.f, Roll + FMath::Sin(Time * 0.9f) * 1.2f));
	Hull->SetRelativeLocation(FVector(0.f, 0.f, FMath::Sin(Time * 0.8f) * 12.f));

	const float Speed01 = FMath::Clamp(GetVelocity().Size() / CruiseSpeed, 0.f, 1.f);
	PropHub->AddLocalRotation(FRotator(0.f, 0.f, Dt * (90.f + 1100.f * Speed01)));

	if (BeaconMat)
	{
		BeaconMat->SetScalarParameterValue(TEXT("Emissive"), FMath::Frac(Time * 0.8f) < 0.15f ? 40.f : 1.f);
	}

	// Battery
	if (GM && GM->Phase == EAbyssPhase::Explore && (bControlsEnabled || GM->IsDiving()))
	{
		const float Drain = bControlsEnabled ? (0.45f + 0.55f * Speed01) * (bBoost ? 1.8f : 1.f) * (bLightOn ? 1.f : 0.8f) : 0.25f;
		// (no battery worries while the tutorial is teaching the controls)
		if (!GM->IsTutorial()) Battery01 = FMath::Max(0.f, Battery01 - Dt * Drain / GM->BatterySeconds());
		if (Battery01 <= 0.f)
		{
			GM->EnterRestaurant(true);
		}
	}

	UpdateTarget(Dt);
	if (bControlsEnabled) UpdateSnow(Dt);
	UpdateBubbles(Dt);
	if (BubbleFX && BubbleFX->Template)
	{
		const bool bMoving = GetVelocity().Size() > 150.f;
		if (bMoving && !BubbleFX->IsActive()) BubbleFX->Activate();
		else if (!bMoving && BubbleFX->IsActive()) BubbleFX->Deactivate();
	}
}

void ASubmarinePawn::UpdateTarget(float Dt)
{
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	const FVector Lamp = Headlight->GetComponentLocation();
	const FVector Dir = Headlight->GetForwardVector();
	// running dark: the claw still works, but only up close
	const float Range = GetCatchRange() * (bLightOn ? 1.f : 0.6f);
	const float CosCone = FMath::Cos(FMath::DegreesToRadians(CatchConeDegrees));

	ACatchableFish* Best = nullptr;
	float BestScore = -1.f;
	if (bControlsEnabled)
	{
		for (TActorIterator<ACatchableFish> It(GetWorld()); It; ++It)
		{
			ACatchableFish* F = *It;
			if (!F->IsCatchable()) continue;
			const FVector To = F->GetActorLocation() - Lamp;
			const float D = To.Size();
			if (D > Range || D < 1.f) continue;
			const float Dot = FVector::DotProduct(To / D, Dir);
			if (Dot < CosCone) continue;
			const float Score = Dot - D / Range * 0.05f;
			if (Score > BestScore)
			{
				FHitResult Hit;
				FCollisionQueryParams P(SCENE_QUERY_STAT(SubSight), false, this);
				if (GetWorld()->LineTraceSingleByChannel(Hit, Lamp, F->GetActorLocation(), ECC_Visibility, P)) continue;
				BestScore = Score;
				Best = F;
			}
		}
	}

	if (Target.Get() != Best)
	{
		if (Target.IsValid()) Target->SetBeingCaught(false);
		Target = Best;
		CatchProgress = 0.f;
	}

	bCatching = bCatchHeld && Target.IsValid();
	if (bCatching && !Target->Def().bSubClaw)
	{
		if (GM) GM->ShowToast(FString::Printf(TEXT("%s: too small for the claw - dive out [E] and use the NET"), Target->Def().Name), FLinearColor(1.f, 0.75f, 0.35f), 1.2f);
		bCatching = false;
	}
	else if (bCatching)
	{
		if (GM && GM->CargoCount() >= AAbyssGameMode::CargoCapacity)
		{
			GM->ShowToast(TEXT("Cargo hold full - return to the dock [E]"), FLinearColor(1.f, 0.6f, 0.2f), 1.f);
			bCatching = false;
		}
		else
		{
			Target->SetBeingCaught(true);
			// a calm fish is an easy grab; one that is fleeing fights the claw all the way
			bTargetPrime = Target->PrimeIfCaught(ECatchMethod::SubClaw, 1);
			CatchProgress += Dt / Target->Def().CatchTime * (Target->IsCalm() ? 2.4f : 1.f);
			if (CatchProgress >= 1.f)
			{
				const FFishDef& D = Target->Def();
				const bool bPrime = bTargetPrime;
				if (GM && GM->AddFish(Target->Type, bPrime, true))
				{
					if (bPrime) GM->ShowToast(FString::Printf(TEXT("PRIME catch: %s!  Sells for +35%%"), D.Name), FLinearColor(1.f, 0.85f, 0.3f), 3.f);
					else GM->ShowToast(FString::Printf(TEXT("Caught: %s (ordinary) - it was spooked. %s"), D.Name, AbyssData::PrimeTip(Target->Type)), FLinearColor(0.4f, 1.f, 0.8f), 4.f);
				}
				Target->OnCaught();
				Target = nullptr;
				CatchProgress = 0.f;
				bCatching = false;
			}
		}
	}
	else
	{
		if (Target.IsValid()) Target->SetBeingCaught(false);
		CatchProgress = FMath::Max(0.f, CatchProgress - Dt * 0.8f);
	}

	if (BeamMat)
	{
		BeamMat->SetVectorParameterValue(TEXT("Color"), bCatching ? FLinearColor(0.5f, 1.f, 0.75f) : FLinearColor(1.f, 0.95f, 0.75f));
		BeamMat->SetScalarParameterValue(TEXT("Intensity"), bCatching ? 0.05f : 0.018f);
	}
}

void ASubmarinePawn::UpdateSnow(float Dt)
{
	if (SnowPos.Num() == 0 || !Camera) return;
	const FVector C = Camera->GetComponentLocation();
	const FVector Half(1400.f, 1400.f, 900.f);
	for (int32 i = 0; i < SnowPos.Num(); ++i)
	{
		FVector& P = SnowPos[i];
		P.Z -= Dt * (8.f + (i % 7) * 3.f);
		P.X += FMath::Sin(Time * 0.3f + i) * Dt * 6.f;
		FVector Rel = P - C;
		for (int32 a = 0; a < 3; ++a)
		{
			const float H = Half[a];
			if (Rel[a] > H) Rel[a] -= 2.f * H;
			else if (Rel[a] < -H) Rel[a] += 2.f * H;
		}
		P = C + Rel;
		SnowT[i].SetLocation(P);
	}
	MarineSnow->BatchUpdateInstancesTransforms(0, SnowT, true, true, true);
}

void ASubmarinePawn::UpdateBubbles(float Dt)
{
	if (BubbleData.Num() == 0) return;
	const float Speed01 = FMath::Clamp(GetVelocity().Size() / CruiseSpeed, 0.f, 2.f);
	BubbleTimer -= Dt;
	if (Speed01 > 0.1f && BubbleTimer <= 0.f)
	{
		BubbleTimer = 0.09f / Speed01;
		FBubble& B = BubbleData[NextBubble];
		NextBubble = (NextBubble + 1) % BubbleData.Num();
		B.P = Hull->GetComponentTransform().TransformPosition(FVector(-HullLength * 0.5f - 30.f, 0.f, 0.f)) + FMath::VRand() * 35.f;
		B.Life = 2.5f;
		B.Phase = FMath::FRand() * 6.f;
	}
	for (int32 i = 0; i < BubbleData.Num(); ++i)
	{
		FBubble& B = BubbleData[i];
		float S = 0.001f;
		if (B.Life > 0.f)
		{
			B.Life -= Dt;
			B.P.Z += 90.f * Dt;
			B.P += FVector(FMath::Sin(Time * 5.f + B.Phase), FMath::Cos(Time * 4.f + B.Phase), 0.f) * 15.f * Dt;
			S = FMath::Lerp(0.04f, 0.12f, 1.f - B.Life / 2.5f);
		}
		BubbleT[i] = FTransform(FQuat::Identity, B.P, FVector(S));
	}
	PropBubbles->BatchUpdateInstancesTransforms(0, BubbleT, true, true, true);
}
