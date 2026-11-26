#include "AbyssPlayerController.h"
#include "AbyssGameMode.h"
#include "AbyssHUD.h"
#include "RestaurantDome.h"
#include "RestaurantCustomer.h"
#include "Components/InputComponent.h"
#include "AbyssLoc.h"
#include "KitchenGame.h"
#include "PrepMenu.h"
#include "InputCoreTypes.h"

void AAbyssPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void AAbyssPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// bConsumeInput = false so SPACE still reaches the submarine's MoveUp axis while diving
	auto Bind = [this](const TCHAR* Name, void (AAbyssPlayerController::*Fn)())
	{
		FInputActionBinding& B = InputComponent->BindAction(Name, IE_Pressed, this, Fn);
		B.bConsumeInput = false;
	};
	Bind(TEXT("Order1"), &AAbyssPlayerController::Order1);
	Bind(TEXT("Order2"), &AAbyssPlayerController::Order2);
	Bind(TEXT("Order3"), &AAbyssPlayerController::Order3);
	Bind(TEXT("Order4"), &AAbyssPlayerController::Order4);
	Bind(TEXT("Cook"), &AAbyssPlayerController::Cook);
	Bind(TEXT("Confirm"), &AAbyssPlayerController::Confirm);
	Bind(TEXT("UpgradeBattery"), &AAbyssPlayerController::UpgradeBattery);
	Bind(TEXT("UpgradeLight"), &AAbyssPlayerController::UpgradeLight);
	Bind(TEXT("UpgradeOxygen"), &AAbyssPlayerController::UpgradeOxygen);
	Bind(TEXT("ToggleLanguage"), &AAbyssPlayerController::ToggleLanguage);
	Bind(TEXT("DebugRestaurant"), &AAbyssPlayerController::DebugRestaurant);
	Bind(TEXT("UpgradeRestaurant"), &AAbyssPlayerController::UpgradeRestaurant);
	Bind(TEXT("RecipeBook"), &AAbyssPlayerController::ToggleRecipeBook);
	Bind(TEXT("Tutorial"), &AAbyssPlayerController::ToggleTutorial);
	Bind(TEXT("NewGame"), &AAbyssPlayerController::NewGame);
}

void AAbyssPlayerController::Order(int32 Index)
{
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	if (GM && GM->Phase == EAbyssPhase::Restaurant && GM->GetDome())
	{
		if (GM->GetDome()->Counter.bOpen)
		{
			GM->GetDome()->Counter.SelectTicket(Index, GM->GetDome());
			return;
		}
		if (!GM->IsNearKitchen())
		{
			GM->ShowToast(TEXT("Walk to the kitchen counter first"), FLinearColor(1.f, 0.8f, 0.35f), 1.5f);
			return;
		}
		GM->GetDome()->SelectOrder(Index);
	}
}

void AAbyssPlayerController::Cook()
{
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	if (GM && GM->Phase == EAbyssPhase::Restaurant && GM->GetDome())
	{
		GM->GetDome()->Cook();
	}
}

void AAbyssPlayerController::Confirm()
{
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this))
	{
		GM->ConfirmPressed();
	}
}

void AAbyssPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	AAbyssGameMode* GM = AAbyssGameMode::Get(this);
	ARestaurantDome* D = GM ? GM->GetDome() : nullptr;
	const bool bCounter = GM && D && GM->Phase == EAbyssPhase::Restaurant && D->Counter.bOpen;
	const bool bPrep = GM && GM->Phase == EAbyssPhase::Prep;
	const bool bKitchen = bPrep || bCounter || (GM && D && GM->Phase == EAbyssPhase::Restaurant && D->Kitchen.bActive && D->Kitchen.bAtStation);

	if (bKitchen != bKitchenCursor)
	{
		bKitchenCursor = bKitchen;
		bShowMouseCursor = bKitchen;
		if (bKitchen)
		{
			FInputModeGameAndUI Mode;
			Mode.SetHideCursorDuringCapture(false);
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
			SetInputMode(Mode);
		}
		else
		{
			SetInputMode(FInputModeGameOnly());
		}
	}
	if (!bKitchen) return;

	float MX = 0.f, MY = 0.f;
	int32 VW = 0, VH = 0;
	GetViewportSize(VW, VH);
	if (!GetMousePosition(MX, MY) || VW <= 0) return;
	const FVector2D P(MX, MY);
	if (bPrep)
	{
		// before opening: menu + market are worked with the mouse
		GM->PrepMouse = P;
		if (WasInputKeyJustPressed(EKeys::LeftMouseButton)) FPrepLayout::Make((float)VW, (float)VH).Click(GM, P);
		return;
	}
	if (bCounter)
	{
		TArray<ARestaurantCustomer*> Q;
		D->GetQueue(Q);
		const FCounterLayout CL = FCounterLayout::Make((float)VW, (float)VH, Q.Num());
		D->Counter.Mouse = P;
		if (WasInputKeyJustPressed(EKeys::LeftMouseButton)) D->Counter.MouseDown(P, CL, D);
		if (WasInputKeyJustReleased(EKeys::LeftMouseButton)) D->Counter.MouseUp(P, CL, D);
		return;
	}
	const FKitchenLayout L = FKitchenLayout::Make((float)VW, (float)VH);
	D->Kitchen.MouseMove(P);
	if (WasInputKeyJustPressed(EKeys::LeftMouseButton)) D->Kitchen.MouseDown(P, L);
	if (WasInputKeyJustReleased(EKeys::LeftMouseButton)) D->Kitchen.MouseUp(P, L);
}

void AAbyssPlayerController::UpgradeBattery()
{
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this)) GM->TryUpgrade(0);
}

void AAbyssPlayerController::UpgradeLight()
{
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this)) GM->TryUpgrade(1);
}

void AAbyssPlayerController::UpgradeOxygen()
{
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this)) GM->TryUpgrade(2);
}

void AAbyssPlayerController::ToggleLanguage()
{
	AbyssLoc::Toggle();
}

void AAbyssPlayerController::DebugRestaurant()
{
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this)) GM->DebugJumpToRestaurant();
}

void AAbyssPlayerController::UpgradeRestaurant()
{
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this)) GM->TryUpgrade(3);
}

void AAbyssPlayerController::NewGame()
{
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this)) GM->NewGamePressed();
}

void AAbyssPlayerController::ToggleTutorial()
{
	if (AAbyssGameMode* GM = AAbyssGameMode::Get(this)) GM->ToggleTutorial();
}

void AAbyssPlayerController::ToggleRecipeBook()
{
	if (AAbyssHUD* H = GetHUD<AAbyssHUD>()) H->bRecipeBook = !H->bRecipeBook;
}
