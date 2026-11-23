#include "AbyssGameMode.h"
#include "AbyssPlayerController.h"
#include "AbyssHUD.h"
#include "SubmarinePawn.h"
#include "DiverPawn.h"
#include "RestaurantDome.h"
#include "RestaurantWalker.h"
#include "CatchableFish.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "RestaurantCustomer.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "AbyssRestaurant.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "HAL/PlatformMisc.h"

AAbyssGameMode::AAbyssGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = ASubmarinePawn::StaticClass();
	PlayerControllerClass = AAbyssPlayerController::StaticClass();
	HUDClass = AAbyssHUD::StaticClass();
}

AAbyssGameMode* AAbyssGameMode::Get(const UObject* WorldContext)
{
	const UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	return W ? Cast<AAbyssGameMode>(W->GetAuthGameMode()) : nullptr;
}

void AAbyssGameMode::StartPlay()
{
	for (TActorIterator<ARestaurantDome> It(GetWorld()); It; ++It)
	{
		if (It->bInteriorSpace) ServiceDome = *It;
		else DockDome = *It;
	}
	// Older maps with a single dome: it does both jobs
	if (!ServiceDome.IsValid()) ServiceDome = DockDome;
	if (!DockDome.IsValid()) DockDome = ServiceDome;

	bAutoTest = FParse::Param(FCommandLine::Get(), TEXT("AbyssAutoTest"));
	UE_LOG(LogAbyss, Log, TEXT("StartPlay: ServiceDome=%s DockDome=%s AutoTest=%d"),
		ServiceDome.IsValid() ? *ServiceDome->GetName() : TEXT("none"), DockDome.IsValid() ? *DockDome->GetName() : TEXT("none"), bAutoTest ? 1 : 0);
	Super::StartPlay();
	ShowToast(TEXT("Day 1 - Find fish. Hover & press [E] to dive out with net/speargun. Dock with [E]."), FLinearColor(0.6f, 0.95f, 1.f), 7.f);
}

void AAbyssGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	ToastTime = FMath::Max(0.f, ToastTime - Dt);
	if (Phase == EAbyssPhase::Ending) EndingTime += Dt;
	if (bAutoTest) TickAutoTest(Dt);
	if (!bPlacedSub)
	{
		if (ASubmarinePawn* S = Cast<ASubmarinePawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			Sub = S;
			bPlacedSub = true;
			PlaceSubAtDock();
		}
	}
}

void AAbyssGameMode::ShowToast(const FString& Msg, const FLinearColor& Color, float Duration)
{
	Toast = Msg;
	ToastColor = Color;
	ToastTime = Duration;
}

int32 AAbyssGameMode::CargoCount() const
{
	int32 N = 0;
	for (int32 i = 0; i < AbyssData::NumFish; ++i) N += Stock[i];
	return N;
}

bool AAbyssGameMode::AddFish(EFishType Type, bool bPrime)
{
	if (CargoCount() >= CargoCapacity) return false;
	Stock[(int32)Type]++;
	if (bPrime) Prime[(int32)Type]++;
	if (Type == EFishType::Grouper) bGrouperFoundToday = true;
	return true;
}

bool AAbyssGameMode::TakeFish(int32 Fish, bool bWantPrime)
{
	if (Fish < 0 || Fish >= AbyssData::NumFish || Stock[Fish] <= 0) return false;
	const int32 Normal = Stock[Fish] - Prime[Fish];
	const bool bPrime = Prime[Fish] > 0 && (bWantPrime || Normal <= 0);
	Stock[Fish]--;
	if (bPrime) Prime[Fish]--;
	return bPrime;
}

// ------------------------------------------------------------------ pantry + tonight's menu

int32 AAbyssGameMode::SeasonCount(int32 Id) const
{
	if (Id <= 0 || Id >= AbyssData::NumSeason) return 0;
	int32 N = 0;
	for (int32 d = 0; d < AbyssData::MaxShelf; ++d) N += Pantry[Id][d];
	return N;
}

int32 AAbyssGameMode::SeasonExpiring(int32 Id) const
{
	return Id > 0 && Id < AbyssData::NumSeason && AbyssData::Season(Id).Shelf > 0 ? Pantry[Id][0] : 0;
}

int32 AAbyssGameMode::PantryCount() const
{
	int32 N = 0;
	for (int32 i = 1; i < AbyssData::NumSeason; ++i) N += SeasonCount(i);
	return N;
}

bool AAbyssGameMode::BuySeason(int32 Id)
{
	if (Id <= 0 || Id >= AbyssData::NumSeason) return false;
	const FSeasonDef& D = AbyssData::Season(Id);
	if (PantryCount() >= PantryCapacity)
	{
		ShowToast(TEXT("The pantry is full"), FLinearColor(1.f, 0.6f, 0.3f), 1.5f);
		return false;
	}
	if (Money < D.Cost)
	{
		ShowToast(TEXT("Not enough money"), FLinearColor(1.f, 0.4f, 0.3f), 1.5f);
		return false;
	}
	Money -= D.Cost;
	Pantry[Id][D.Shelf > 0 ? FMath::Clamp(D.Shelf, 1, AbyssData::MaxShelf) - 1 : 0]++;
	BoughtNow[Id]++;
	return true;
}

bool AAbyssGameMode::ReturnSeason(int32 Id)
{
	if (Id <= 0 || Id >= AbyssData::NumSeason || BoughtNow[Id] <= 0) return false;
	const FSeasonDef& D = AbyssData::Season(Id);
	int32& Slot = Pantry[Id][D.Shelf > 0 ? FMath::Clamp(D.Shelf, 1, AbyssData::MaxShelf) - 1 : 0];
	if (Slot <= 0) return false;
	--Slot;
	BoughtNow[Id]--;
	Money += D.Cost;
	return true;
}

bool AAbyssGameMode::UseSeason(int32 Id)
{
	if (Id <= 0 || Id >= AbyssData::NumSeason) return false;
	// oldest first
	for (int32 d = 0; d < AbyssData::MaxShelf; ++d)
	{
		if (Pantry[Id][d] > 0) { Pantry[Id][d]--; return true; }
	}
	return false;
}

void AAbyssGameMode::CycleMenu(int32 Dish, int32 Slot, int32 Dir)
{
	if (Dish < 0 || Dish >= AbyssData::NumDishes) return;
	int32& Cur = Slot == 0 ? MenuMain[Dish] : MenuSide[Dish];
	for (int32 k = 0; k < AbyssData::NumSeason; ++k)
	{
		Cur = (Cur + (Dir >= 0 ? 1 : AbyssData::NumSeason - 1)) % AbyssData::NumSeason;
		if (Cur == 0 || AbyssData::Season(Cur).Slot == Slot) return;
	}
}

int32 AAbyssGameMode::DishFinalPrice(int32 Dish, bool bPrimeFish, int32 Main, int32 Side) const
{
	float P = (float)AbyssData::DishPrice(Dish);
	if (bPrimeFish) P *= 1.f + AbyssData::PrimeBonus;
	const FSeasonDef& M = AbyssData::Season(Main);
	const FSeasonDef& G = AbyssData::Season(Side);
	P *= bPrimeFish ? M.PrimeMult : M.Mult;
	P *= bPrimeFish ? G.PrimeMult : G.Mult;
	return FMath::RoundToInt(P);
}

bool AAbyssGameMode::BuyBait()
{
	if (Bait >= MaxBait) { ShowToast(TEXT("The bait box is full"), FLinearColor(1.f, 0.6f, 0.3f), 1.5f); return false; }
	if (Money < BaitCost) { ShowToast(TEXT("Not enough money"), FLinearColor(1.f, 0.4f, 0.3f), 1.5f); return false; }
	Money -= BaitCost;
	++Bait;
	++BaitBoughtNow;
	return true;
}

bool AAbyssGameMode::ReturnBait()
{
	if (BaitBoughtNow <= 0 || Bait <= 0) return false;
	--Bait;
	--BaitBoughtNow;
	Money += BaitCost;
	return true;
}

bool AAbyssGameMode::DropBait(const FVector& At)
{
	if (Phase != EAbyssPhase::Explore) return false;
	if (Bait <= 0)
	{
		ShowToast(TEXT("No bait left - buy more before opening tonight"), FLinearColor(1.f, 0.6f, 0.3f), 2.f);
		return false;
	}
	if (ActiveBait.IsValid()) ActiveBait->Destroy();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveBait = GetWorld()->SpawnActor<AAbyssBait>(AAbyssBait::StaticClass(), At, FRotator::ZeroRotator, P);
	--Bait;
	ShowToast(FString::Printf(TEXT("Bait dropped (%d left) - lights off and wait for the grouper"), Bait), FLinearColor(1.f, 0.75f, 0.45f), 3.f);
	return true;
}

bool AAbyssGameMode::GetBait(FVector& Out) const
{
	if (!ActiveBait.IsValid()) return false;
	Out = ActiveBait->GetActorLocation();
	return true;
}

int32 AAbyssGameMode::UpgradeLevel(int32 Which) const
{
	return Which == 0 ? BatteryLevel : Which == 1 ? LightLevel : Which == 2 ? OxygenLevel : RestaurantTier;
}

int32 AAbyssGameMode::UpgradeCost(int32 Which) const
{
	static const int32 Base[] = { 300, 250, 250, 600 };
	// each level costs ~1.8x the previous one (expansion 2.5x)
	const float Growth = Which == 3 ? 2.5f : 1.8f;
	return FMath::RoundToInt(Base[FMath::Clamp(Which, 0, 3)] * FMath::Pow(Growth, (float)UpgradeLevel(Which)) / 10.f) * 10;
}

void AAbyssGameMode::TryUpgrade(int32 Which)
{
	if (Phase != EAbyssPhase::Summary) return;
	int32& Lvl = Which == 0 ? BatteryLevel : Which == 1 ? LightLevel : Which == 2 ? OxygenLevel : RestaurantTier;
	if (Lvl >= (Which == 3 ? 2 : MaxUpgrade))
	{
		ShowToast(TEXT("Already at max level"), FLinearColor(1.f, 0.8f, 0.3f));
		return;
	}
	const int32 Cost = UpgradeCost(Which);
	if (Money < Cost)
	{
		ShowToast(TEXT("Not enough money"), FLinearColor(1.f, 0.4f, 0.3f));
		return;
	}
	Money -= Cost;
	++Lvl;
	static const TCHAR* Msg[] = {
		TEXT("Battery upgraded - longer dives!"),
		TEXT("Searchlight upgraded - brighter beam, longer claw range!"),
		TEXT("Oxygen tank upgraded - the diver stays out longer!"),
		TEXT("Restaurant expanded - more tables and more guests tonight!") };
	ShowToast(Msg[FMath::Clamp(Which, 0, 3)], FLinearColor(0.4f, 1.f, 0.7f));
	if (Which == 3 && ServiceDome.IsValid()) ServiceDome->ApplyTier(RestaurantTier);
}

// ------------------------------------------------------------------ diving

bool AAbyssGameMode::IsAtStation(EKitchenStep Step) const
{
	const ARestaurantWalker* W = Walker.Get();
	const ARestaurantDome* D = ServiceDome.Get();
	if (!W || !D) return true;
	return FVector::Dist2D(W->GetActorLocation(), D->GetStationLocation(Step)) < 190.f;
}

bool AAbyssGameMode::IsNearKitchen() const
{
	if (ServiceDome.IsValid() && (ServiceDome->bConceptLayout || ServiceDome->bLuxLayout))
	{
		return IsAtStation(EKitchenStep::Slice) || IsAtStation(EKitchenStep::Cook) || IsAtStation(EKitchenStep::Plate);
	}
	const ARestaurantWalker* W = Walker.Get();
	const ARestaurantDome* D = ServiceDome.Get();
	if (!W || !D) return true; // no walker (fallback): allow cooking anywhere
	return FVector::Dist2D(W->GetActorLocation(), D->GetKitchenLocation()) < 480.f;
}

void AAbyssGameMode::RestaurantInteract()
{
	ARestaurantDome* D = ServiceDome.Get();
	if (Phase != EAbyssPhase::Restaurant || !D) return;
	// 0) at the counter: E goes back to the dining room
	if (D->Counter.bOpen)
	{
		D->Counter.bOpen = false;
		return;
	}
	const APawn* P = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Me = P ? P->GetActorLocation() : FVector::ZeroVector;
	// 2) carrying a plate: serve it at the guest's table
	if (D->IsCarrying())
	{
		ARestaurantCustomer* T = D->GetCarryTarget();
		if (T && FVector::Dist2D(Me, T->GetActorLocation()) < ServeReach)
		{
			D->DeliverDish();
		}
		else
		{
			ShowToast(TEXT("Carry the dish to the guest - follow the green marker"), FLinearColor(0.5f, 1.f, 0.6f), 1.5f);
		}
		return;
	}
	// 3) a guest nearby wants to order
	{
		TArray<ARestaurantCustomer*> All;
		D->GetCustomers(All);
		ARestaurantCustomer* Best = nullptr;
		float BestD = ServeReach;
		for (ARestaurantCustomer* C : All)
		{
			const float Dd = FVector::Dist2D(Me, C->GetActorLocation());
			if (C->WantsToOrder() && Dd < BestD) { BestD = Dd; Best = C; }
		}
		if (Best)
		{
			D->TakeOrder(Best);
			return;
		}
	}
	// 4) at the kitchen: start the next order
	if (!IsNearKitchen())
	{
		ShowToast(TEXT("Walk to the kitchen counter first"), FLinearColor(1.f, 0.8f, 0.35f), 1.5f);
		return;
	}
	// 4) at the kitchen: open the counter
	TArray<ARestaurantCustomer*> Q;
	D->GetQueue(Q);
	D->Counter.bOpen = true;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		FVector Out = (Me - D->GetActorLocation()).GetSafeNormal2D();
		if (Out.IsNearlyZero()) Out = -D->GetActorForwardVector();
		PC->SetControlRotation(FRotator(-8.f, Out.Rotation().Yaw, 0.f));
	}
	if (Q.Num() == 0) ShowToast(TEXT("No orders yet - take orders at the tables [E]"), FLinearColor(1.f, 0.85f, 0.4f), 2.f);
}

bool AAbyssGameMode::IsDiving() const
{
	APawn* P = UGameplayStatics::GetPlayerPawn(this, 0);
	return P && P == Diver.Get();
}

void AAbyssGameMode::ExitSubToDive()
{
	ASubmarinePawn* S = Sub.Get();
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!S || !PC || Phase != EAbyssPhase::Explore || IsDiving()) return;

	const FVector Out = S->GetActorLocation() - S->GetActorUpVector() * 230.f + S->GetActorRightVector() * 120.f;
	const FRotator Rot(0.f, S->GetActorRotation().Yaw, 0.f);
	if (!Diver.IsValid())
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Diver = GetWorld()->SpawnActor<ADiverPawn>(ADiverPawn::StaticClass(), Out, Rot, P);
	}
	ADiverPawn* D = Diver.Get();
	if (!D) return;
	D->SetActorHiddenInGame(false);
	D->SetActorEnableCollision(true);
	D->SetActorLocationAndRotation(Out, Rot, false, nullptr, ETeleportType::TeleportPhysics);
	D->BeginDive(S);

	S->SetControlsEnabled(false);
	const FRotator Ctrl = PC->GetControlRotation();
	PC->Possess(D);
	PC->SetControlRotation(Ctrl);
	ShowToast(TEXT("Diving! [1] Speargun  [2] Net  [LMB] Use  -  Return to the hatch with [E]"), FLinearColor(0.6f, 0.95f, 1.f), 5.f);
}

void AAbyssGameMode::ReturnDiverToSub(bool bEmergency)
{
	ASubmarinePawn* S = Sub.Get();
	ADiverPawn* D = Diver.Get();
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!S || !D || !PC) return;

	if (bEmergency)
	{
		// lose half of what this dive caught
		int32 Lost = 0;
		for (int32 i = 0; i < AbyssData::NumFish; ++i)
		{
			const int32 L = D->DiveCatch[i] / 2 + (D->DiveCatch[i] % 2);
			const int32 Take = FMath::Min(L, Stock[i]);
			Stock[i] -= Take;
			Prime[i] = FMath::Min(Prime[i], Stock[i]);
			Lost += Take;
		}
		ShowToast(FString::Printf(TEXT("Out of oxygen! Emergency recall to the sub - lost %d fish"), Lost), FLinearColor(1.f, 0.45f, 0.3f), 4.f);
	}
	else
	{
		ShowToast(TEXT("Back aboard the submarine"), FLinearColor(0.6f, 0.95f, 1.f), 1.5f);
	}

	const FRotator Ctrl = PC->GetControlRotation();
	PC->Possess(S);
	PC->SetControlRotation(Ctrl);
	S->SetControlsEnabled(true);
	D->SetActorHiddenInGame(true);
	D->SetActorEnableCollision(false);
	D->SetActorLocation(S->GetActorLocation() - FVector(0.f, 0.f, 600.f));
}

// ------------------------------------------------------------------ restaurant

void AAbyssGameMode::PlaceSubAtDock()
{
	ASubmarinePawn* S = Sub.Get();
	if (!S || !DockDome.IsValid()) return;
	const FTransform T = DockDome->GetDockSpawn();
	S->SetActorLocationAndRotation(T.GetLocation(), T.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* C = S->GetController())
	{
		C->SetControlRotation(FRotator(-8.f, T.Rotator().Yaw, 0.f));
	}
}

void AAbyssGameMode::FadeTo(AActor* NewViewTarget, TFunction<void()> AtBlack)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->PlayerCameraManager)
	{
		if (AtBlack) AtBlack();
		return;
	}
	PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.45f, FLinearColor::Black, false, true);
	TWeakObjectPtr<APlayerController> WeakPC = PC;
	TWeakObjectPtr<AActor> WeakTarget = NewViewTarget;
	GetWorldTimerManager().SetTimer(FadeTimer, FTimerDelegate::CreateWeakLambda(this, [WeakPC, WeakTarget, AtBlack]()
	{
		if (AtBlack) AtBlack();
		if (APlayerController* P = WeakPC.Get())
		{
			if (WeakTarget.IsValid()) P->SetViewTarget(WeakTarget.Get());
			P->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.8f, FLinearColor::Black, false, false);
		}
	}), 0.5f, false);
}

void AAbyssGameMode::EnterRestaurant(bool bEmergency)
{
	if (Phase != EAbyssPhase::Explore || !ServiceDome.IsValid()) return;
	if (IsDiving()) ReturnDiverToSub(false);
	// first the prep screen (menu + seasoning market); OpenRestaurant() starts the service
	Phase = EAbyssPhase::Prep;
	bEmergencyTow = bEmergency;
	TonightTaste = (ETaste)FMath::RandRange((int32)ETaste::Salty, (int32)ETaste::Fresh);
	for (int32& B : BoughtNow) B = 0;
	BaitBoughtNow = 0;

	NightServed = NightLeft = NightEarned = NightPerfect = NightPrimeServed = NightTasteHits = 0;
	NightCustomers = FMath::Clamp(2 + Day, 3, 5) + RestaurantTier * 2;

	if (ASubmarinePawn* S = GetSub())
	{
		S->SetControlsEnabled(false);
	}

	ARestaurantDome* Dome = ServiceDome.Get();
	if (!Walker.IsValid() && Dome)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Walker = GetWorld()->SpawnActor<ARestaurantWalker>(ARestaurantWalker::StaticClass(), Dome->GetEntranceTransform(), P);
		if (Walker.IsValid()) Walker->SetActorHiddenInGame(true);
	}
	ARestaurantWalker* Wk = Walker.Get();
	FadeTo(Wk ? (AActor*)Wk : (AActor*)Dome, [this, Dome, Wk]()
	{
		PlaceSubAtDock();
		if (Dome)
		{
			Dome->SetInteriorActive(true);
			Dome->ApplyTier(RestaurantTier);
			Dome->MaxConcurrentOrders = 2 + RestaurantTier;
		}
		if (Wk && Dome)
		{
			const FTransform T = Dome->GetEntranceTransform();
			Wk->SetActorLocationAndRotation(T.GetLocation(), T.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
			Wk->RoomCenter = Dome->GetActorLocation();
			Wk->RoomRadius = Dome->GetFloorRadius();
			Wk->SetActorHiddenInGame(false);
			Wk->SetActorEnableCollision(true);
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
			{
				PC->Possess(Wk);
				PC->SetControlRotation(FRotator(-15.f, T.Rotator().Yaw, 0.f));
			}
		}
	});

	if (bEmergency) ShowToast(TEXT("Battery depleted! Emergency tow back to base..."), FLinearColor(1.f, 0.5f, 0.3f), 4.f);
}

void AAbyssGameMode::OpenRestaurant()
{
	if (Phase != EAbyssPhase::Prep) return;
	Phase = EAbyssPhase::Restaurant;
	for (int32& B : BoughtNow) B = 0;
	BaitBoughtNow = 0;
	if (ARestaurantDome* Dome = ServiceDome.Get()) Dome->BeginNight(NightCustomers);
	ShowToast(TEXT("Welcome in! Walk around freely (WASD). Cook at the kitchen counter."), FLinearColor(1.f, 0.85f, 0.4f), 5.f);
}

void AAbyssGameMode::OnNightFinished()
{
	if (Phase != EAbyssPhase::Restaurant) return;
	Phase = EAbyssPhase::Summary;
}

void AAbyssGameMode::ConfirmPressed()
{
	if (Phase == EAbyssPhase::Prep) { OpenRestaurant(); return; }
	if (Phase == EAbyssPhase::Summary && Day >= DemoDays && !bEndingShown)
	{
		Phase = EAbyssPhase::Ending;
		EndingTime = 0.f;
		return;
	}
	if (Phase == EAbyssPhase::Ending)
	{
		if (EndingTime < 4.f) return; // let the sequence play
		bEndingShown = true;
		Phase = EAbyssPhase::Summary;
	}
	StartNextDay();
}

void AAbyssGameMode::StartNextDay()
{
	if (Phase != EAbyssPhase::Summary) return;
	++Day;
	bGrouperFoundToday = false;
	// fresh garnishes get a night older; what was on its last night is thrown out
	SpoiledLastNight = 0;
	for (int32 i = 1; i < AbyssData::NumSeason; ++i)
	{
		if (AbyssData::Season(i).Shelf <= 0) continue;
		SpoiledLastNight += Pantry[i][0];
		for (int32 d = 0; d + 1 < AbyssData::MaxShelf; ++d) Pantry[i][d] = Pantry[i][d + 1];
		Pantry[i][AbyssData::MaxShelf - 1] = 0;
	}
	Bait = FMath::Max(Bait, 2);

	ASubmarinePawn* S = GetSub();
	ARestaurantDome* Dome = ServiceDome.Get();
	FadeTo(S, [this, S, Dome]()
	{
		Phase = EAbyssPhase::Explore;
		if (Dome) Dome->SetInteriorActive(false);
		if (S)
		{
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
			{
				if (PC->GetPawn() != S) PC->Possess(S);
			}
			PlaceSubAtDock();
			S->Refill();
			S->SetControlsEnabled(true);
		}
		if (ARestaurantWalker* Wk = Walker.Get())
		{
			Wk->SetActorHiddenInGame(true);
			Wk->SetActorEnableCollision(false);
		}
	});
	if (SpoiledLastNight > 0)
		ShowToast(FString::Printf(TEXT("Day %d - %d fresh garnishes spoiled overnight. Calm catches sell as PRIME."), Day, SpoiledLastNight), FLinearColor(1.f, 0.8f, 0.5f), 6.f);
	else
		ShowToast(FString::Printf(TEXT("Day %d - Tuna roam further out. Rare groupers lurk deep in the east trench."), Day), FLinearColor(0.6f, 0.95f, 1.f), 6.f);
}

void AAbyssGameMode::DebugJumpToRestaurant()
{
	if (Phase != EAbyssPhase::Explore) return;
	Stock[0] = FMath::Max(Stock[0], 4);
	Stock[1] = FMath::Max(Stock[1], 3);
	Stock[2] = FMath::Max(Stock[2], 1);
	Prime[0] = FMath::Max(Prime[0], 2);
	Prime[1] = FMath::Max(Prime[1], 1);
	Prime[2] = FMath::Max(Prime[2], 1);
	UE_LOG(LogAbyss, Log, TEXT("DebugJumpToRestaurant"));
	EnterRestaurant(false);
}

void AAbyssGameMode::TickAutoTest(float Dt)
{
	AutoTestTime += Dt;
	// test helper: walk the chef to whichever station the current cooking step needs
	if (ServiceDome.IsValid() && ServiceDome->Kitchen.bActive && Walker.IsValid())
	{
		const FVector St = ServiceDome->GetStationLocation(ServiceDome->Kitchen.Step);
		if (FVector::Dist2D(St, Walker->GetActorLocation()) > 150.f)
			Walker->SetActorLocation(St + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
	}
	static bool bSideView = false;
	if (!bSideView && AutoTestTime > 2.f)
	{
		bSideView = true;
		if (ASubmarinePawn* S = Sub.Get())
		{
			if (AController* C = S->GetController()) C->SetControlRotation(FRotator(-10.f, S->GetActorRotation().Yaw + 80.f, 0.f));
		}
	}
	auto Shot = [](const TCHAR* Name)
	{
		const FString Path = FPaths::ProjectSavedDir() / TEXT("AutoTest") / Name;
		FScreenshotRequest::RequestScreenshot(Path, true, false);
		UE_LOG(LogAbyss, Log, TEXT("AutoTest screenshot -> %s"), *Path);
	};
	auto LogState = [this](const TCHAR* Tag)
	{
		APawn* P = UGameplayStatics::GetPlayerPawn(this, 0);
		TArray<ARestaurantCustomer*> Q;
		int32 Remaining = 0;
		bool bNight = false;
		if (ARestaurantDome* D = ServiceDome.Get())
		{
			D->GetQueue(Q);
			Remaining = D->GetRemaining();
			bNight = D->IsNightActive();
		}
		UE_LOG(LogAbyss, Log, TEXT("AutoTest[%s] t=%.1f phase=%d pawn=%s loc=%s queue=%d remaining=%d night=%d nearKitchen=%d cooking=%d money=%d stock=%d/%d/%d"),
			Tag, AutoTestTime, (int32)Phase, P ? *P->GetClass()->GetName() : TEXT("none"), P ? *P->GetActorLocation().ToCompactString() : TEXT("-"),
			Q.Num(), Remaining, bNight ? 1 : 0, IsNearKitchen() ? 1 : 0, ServiceDome.IsValid() && ServiceDome->IsCooking() ? 1 : 0,
			Money, Stock[0], Stock[1], Stock[2]);
	};

	// close look at the exterior dome from outside the glass (between the explore shot and the restaurant jump)
	if (AutoTestStep == 6 && AutoTestTime >= 25.4f && !bAutoTestOutsidePosed && DockDome.IsValid())
	{
		bAutoTestOutsidePosed = true;
		const FVector C = DockDome->GetActorLocation();
		const FVector CamLoc = C + FVector(-(DockDome->Radius + 800.f), -600.f, 900.f);
		if (ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(CamLoc, (C + FVector(0.f, 0.f, 200.f) - CamLoc).Rotation()))
		{
			Cam->GetCameraComponent()->SetFieldOfView(80.f);
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->SetViewTarget(Cam);
		}
	}
	if (AutoTestStep == 6 && AutoTestTime >= 26.2f && !bAutoTestOutsideShot)
	{
		bAutoTestOutsideShot = true;
		Shot(TEXT("01b_outside.png"));
	}
	// aerial overview of the open-plan restaurant (after the service, before the ending)
	if (AutoTestStep == 16 && AutoTestTime >= 45.55f && !bAutoTestAerialPosed && ServiceDome.IsValid())
	{
		bAutoTestAerialPosed = true;
		const FVector C = ServiceDome->GetActorLocation();
		const FVector CamLoc = C + FVector(-2600.f, -2600.f, 2300.f);
		if (ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(CamLoc, (C - CamLoc).Rotation()))
		{
			Cam->GetCameraComponent()->SetFieldOfView(75.f);
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->SetViewTarget(Cam);
		}
	}
	if (AutoTestStep == 16 && AutoTestTime >= 45.85f && !bAutoTestAerialShot)
	{
		bAutoTestAerialShot = true;
		Shot(TEXT("06b_aerial.png"));
	}
	if (AutoTestStep == 8 && AutoTestTime >= 33.1f && !bAutoTestBookPosed)
	{
		bAutoTestBookPosed = true;
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (AAbyssHUD* Hd = PC->GetHUD<AAbyssHUD>()) Hd->bRecipeBook = true;
		}
	}
	if (AutoTestStep == 8 && AutoTestTime >= 33.2f && !bAutoTestBookShot)
	{
		bAutoTestBookShot = true;
		Shot(TEXT("06c_recipes.png"));
	}
	if (AutoTestStep == 8 && AutoTestTime >= 33.9f && bAutoTestBookShot)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (AAbyssHUD* Hd = PC->GetHUD<AAbyssHUD>()) Hd->bRecipeBook = false;
		}
	}
	// close-ups of the three fish (and the grouper's den) between the diver shots and the explore shot
	{
		static int32 FishShot = 0;
		const float T0 = 16.f + FishShot / 2 * 2.4f;
		if (AutoTestStep == 5 && FishShot < 6 && AutoTestTime >= T0 + (FishShot % 2) * 1.2f)
		{
			const EFishType Want = (EFishType)(FishShot / 2);
			if (FishShot % 2 == 0)
			{
				ACatchableFish* Pick = nullptr;
				for (TActorIterator<ACatchableFish> It(GetWorld()); It; ++It) if (It->Type == Want && It->IsCatchable()) { Pick = *It; break; }
				if (Pick)
				{
					const FVector F = Pick->GetActorLocation();
					const FVector Side = FRotationMatrix(Pick->GetActorRotation()).GetUnitAxis(EAxis::Y);
					const float Dist = Want == EFishType::Yellowtail ? 420.f : Want == EFishType::Tuna ? 520.f : 900.f;
					const FVector CamLoc = F + Side * Dist + Pick->GetActorForwardVector() * (Want == EFishType::Tuna ? 500.f : 60.f) + FVector(0.f, 0.f, Want == EFishType::Grouper ? 260.f : 60.f);
					const FVector Look = Want == EFishType::Grouper ? Pick->GetDen() + FVector(0.f, 0.f, 80.f) : F + Pick->GetActorForwardVector() * (Want == EFishType::Tuna ? 380.f : 0.f);
					if (ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(CamLoc, (Look - CamLoc).Rotation()))
					{
						Cam->GetCameraComponent()->SetFieldOfView(70.f);
						Cam->GetCameraComponent()->SetConstraintAspectRatio(false);
						// a little light so the model reads in the dark water
						if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->SetViewTarget(Cam);
						if (ASubmarinePawn* Sb = Sub.Get()) Sb->SetActorLocation(CamLoc - (Look - CamLoc).GetSafeNormal() * 500.f + FVector(0.f, 0.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
						if (ASubmarinePawn* Sb = Sub.Get()) { Sb->SetActorRotation((Look - Sb->GetActorLocation()).Rotation()); if (AController* C = Sb->GetController()) C->SetControlRotation((Look - Sb->GetActorLocation()).Rotation()); }
					}
					UE_LOG(LogAbyss, Log, TEXT("AutoTest fish %d at %s alarm=%.2f"), (int32)Want, *F.ToCompactString(), Pick->Alarm);
				}
			}
			else
			{
				static const TCHAR* Names[] = { TEXT("01c_fish_yellowtail.png"), TEXT("01d_fish_tuna.png"), TEXT("01e_fish_grouper.png") };
				Shot(Names[FishShot / 2]);
			}
			++FishShot;
		}
		if (AutoTestStep == 5 && FishShot == 6 && AutoTestTime >= 24.2f)
		{
			++FishShot;
			PlaceSubAtDock();
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->SetViewTarget(PC->GetPawn());
		}
	}
	// the before-opening screen, then open the doors
	if (AutoTestStep == 7 && AutoTestTime >= 28.6f && !bAutoTestPrepShot && Phase == EAbyssPhase::Prep)
	{
		bAutoTestPrepShot = true;
		Shot(TEXT("02p_prep.png"));
	}
	if (AutoTestStep == 7 && AutoTestTime >= 29.2f && Phase == EAbyssPhase::Prep) OpenRestaurant();
	// summary screen shot, then the ending
	if (AutoTestStep == 17 && AutoTestTime >= 46.6f && !bAutoTestSummaryShot)
	{
		bAutoTestSummaryShot = true;
		Shot(TEXT("06d_summary.png"));
	}
	if (AutoTestStep == 17 && AutoTestTime >= 46.8f && Phase == EAbyssPhase::Summary)
	{
		Phase = EAbyssPhase::Ending;
		EndingTime = 0.f;
	}
	// swim forward before the second diver shot so the swim clip shows
	if ((AutoTestStep == 2 || AutoTestStep == 3) && Diver.IsValid())
	{
		Diver->AddMovementInput(FVector(1.f, 0.f, 0.f), 1.f);
	}
	// front view of the chef model (between steps 7 and 8)
	if (AutoTestStep == 8 && AutoTestTime >= 33.3f && AutoTestTime < 34.5f && !bAutoTestChefPosed)
	{
		bAutoTestChefPosed = true;
		if (ARestaurantDome* D = ServiceDome.Get())
		{
			if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
			{
				P->SetActorLocationAndRotation(D->GetActorLocation() + FVector(200.f, 150.f, 100.f), FRotator(0.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
				if (AController* C = P->GetController()) C->SetControlRotation(FRotator(-10.f, 180.f, 0.f));
			}
		}
	}
	if (AutoTestStep == 8 && AutoTestTime >= 34.5f && !bAutoTestChefShot)
	{
		bAutoTestChefShot = true;
		Shot(TEXT("02b_chef.png"));
	}
	// 3D kitchen with the user's stove + prep table models
	if (AutoTestStep == 8 && AutoTestTime >= 34.0f && !bAutoTestKitchenShot && ServiceDome.IsValid())
	{
		bAutoTestKitchenShot = true;
		// both models stand 95 cm behind the cook's spots (-Y in dome space); look at them from the cook's side
		const FVector A = ServiceDome->GetStationLocation(EKitchenStep::Slice), B = ServiceDome->GetStationLocation(EKitchenStep::Cook);
		const FTransform DT = ServiceDome->GetActorTransform();
		(void)A; (void)B;
		const FVector Mid = DT.TransformPosition(FVector(0.f, -1000.f, 60.f));
		const FVector CamLoc = DT.TransformPosition(FVector(-250.f, 80.f, 520.f));
		if (ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(CamLoc, (Mid - CamLoc).Rotation()))
		{
			Cam->GetCameraComponent()->SetFieldOfView(75.f);
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->SetViewTarget(Cam);
		}
	}
	if (AutoTestStep == 8 && AutoTestTime >= 34.4f && !bAutoTestKitchenShot2)
	{
		bAutoTestKitchenShot2 = true;
		Shot(TEXT("02d_kitchen3d.png"));
	}
	if (AutoTestStep == 8 && AutoTestTime >= 34.55f && bAutoTestKitchenShot2 && !bAutoTestKitchenBack)
	{
		bAutoTestKitchenBack = true;
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->SetViewTarget(PC->GetPawn());
	}
	if (AutoTestStep == 16 && AutoTestTime >= 45.2f && !bAutoTestPropsPosed)
	{
		bAutoTestPropsPosed = true;
		// close-up of a seated guest
		if (ARestaurantDome* D = ServiceDome.Get())
		{
			TArray<ARestaurantCustomer*> All;
			D->GetCustomers(All);
			ARestaurantCustomer* C = nullptr;
			for (ARestaurantCustomer* X : All) if (!C || X->State == ECustomerState::Eating) C = X;
			if (C)
			{
				const FVector G = C->GetActorLocation() + FVector(0.f, 0.f, 80.f);
				const FVector ToCenter = (D->GetActorLocation() - G).GetSafeNormal2D();
				const FVector CamLoc = G + ToCenter * 230.f + FVector(0.f, 0.f, 70.f);
				if (ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(CamLoc, (G - CamLoc).Rotation()))
				{
					Cam->GetCameraComponent()->SetFieldOfView(55.f);
					if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->SetViewTarget(Cam);
				}
			}
		}
	}
	if (AutoTestStep == 16 && AutoTestTime >= 45.45f && !bAutoTestPropsShot)
	{
		bAutoTestPropsShot = true;
		Shot(TEXT("02c_guest.png"));
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->SetViewTarget(PC->GetPawn());
	}

	const float Steps[] = { 8.f, 11.f, 12.f, 12.3f, 15.f, 25.f, 27.f, 33.f, 35.f, 36.f, 37.f, 38.5f, 41.f, 43.f, 44.f, 45.f, 46.f, 48.f };
	if (AutoTestStep >= UE_ARRAY_COUNT(Steps) || AutoTestTime < Steps[AutoTestStep]) return;
	switch (AutoTestStep++)
	{
	case 0:
		ExitSubToDive();
		if (ADiverPawn* Dv = Diver.Get())
		{
			Dv->bFreezeRotation = true;
			Dv->SetActorRotation(FRotator(0.f, 0.f, 0.f));
			if (AController* C = Dv->GetController()) C->SetControlRotation(FRotator(-15.f, 110.f, 0.f));
		}
		break;
	case 1: Shot(TEXT("00a_diver.png")); break;
	case 2: if (ADiverPawn* Dv = Diver.Get()) { (void)Dv; } break;
	case 3: Shot(TEXT("00b_diver2.png")); break;
	case 4: ReturnDiverToSub(false); RestaurantTier = 2; break;
	case 5: LogState(TEXT("explore")); Shot(TEXT("01_explore.png")); break;
	case 6:
		DebugJumpToRestaurant();
		Money += 300;
		for (int32 k = 0; k < 4; ++k) { BuySeason((int32)ESeason::Soy); BuySeason((int32)ESeason::Chili); BuySeason((int32)ESeason::Yuzu); }
		BuySeason((int32)ESeason::Saffron); BuySeason((int32)ESeason::Urchin); BuyBait();
		MenuMain[0] = (int32)ESeason::Soy; MenuSide[0] = (int32)ESeason::Yuzu;
		MenuMain[1] = (int32)ESeason::Chili;
		MenuMain[2] = (int32)ESeason::Saffron; MenuSide[2] = (int32)ESeason::Urchin;
		MenuMain[4] = (int32)ESeason::Soy;
		PrepMouse = FVector2D(900.f, 300.f);
		break;
	case 7: LogState(TEXT("inside")); Shot(TEXT("02_restaurant.png")); break;
	case 8:
		if (ARestaurantDome* D = ServiceDome.Get()) D->DebugTakeAllOrders();
		// walk the chef to the kitchen counter
		if (ARestaurantDome* D = ServiceDome.Get())
		{
			if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
			{
				P->SetActorLocation(D->GetStationLocation(EKitchenStep::Slice) + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
		LogState(TEXT("at-kitchen"));
		break;
	case 9:
		RestaurantInteract();   // opens the counter
		if (ARestaurantDome* D = ServiceDome.Get())
		{
			// pick a teppanyaki ticket if there is one so the pans get exercised
			TArray<ARestaurantCustomer*> Q;
			D->GetQueue(Q);
			for (int32 q = 0; q < Q.Num(); ++q) if (AbyssRecipes::Get(Q[q]->Dish).bCook) { D->Counter.SelectTicket(q, D); break; }
			for (ARestaurantCustomer* C : Q) UE_LOG(LogAbyss, Log, TEXT("AutoTest order: dish=%d taste=%d"), C->Dish, (int32)C->Taste);
		}
		LogState(TEXT("started"));
		Shot(TEXT("03_slice.png"));
		break;
	case 10:
		if (ARestaurantDome* D = ServiceDome.Get())
		{
			D->Counter.DebugPrepare(D, 0.62f);
			D->Counter.Mouse = FVector2D(800.f, 600.f);
		}
		LogState(TEXT("sliced"));
		break;
	case 11: Shot(TEXT("04_cook_or_plate.png")); LogState(TEXT("step2")); break;
	case 12:
		if (ARestaurantDome* D = ServiceDome.Get()) D->Counter.Pans[0].Done = 0.86f;
		LogState(TEXT("cooked"));
		break;
	case 13:
		if (ARestaurantDome* D = ServiceDome.Get())
		{
			D->Counter.DebugPlate(D);
			D->Counter.Drag.Kind = ECItem::Lemon;
			D->Counter.Mouse = FVector2D(1100.f, 560.f);
		}
		Shot(TEXT("05_plate.png"));
		LogState(TEXT("plate"));
		break;
	case 14:
		if (ARestaurantDome* D = ServiceDome.Get()) { D->Counter.Drag = FCItem(); D->Counter.Serve(D); }
		LogState(TEXT("served"));
		break;
	case 15:
		// walk the plate to the guest and serve it
		if (ARestaurantDome* D = ServiceDome.Get())
		{
			if (ARestaurantCustomer* T = D->GetCarryTarget())
			{
				if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
				{
					const FVector To = (T->GetActorLocation() - D->GetActorLocation()).GetSafeNormal2D();
					P->SetActorLocation(T->GetActorLocation() - To * 150.f + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
				}
				RestaurantInteract();
			}
		}
		Shot(TEXT("06_after.png")); LogState(TEXT("after"));
		break;
	case 16: Phase = EAbyssPhase::Summary; break;
	case 17: Shot(TEXT("07_ending.png")); FPlatformMisc::RequestExit(false); break;
	}
}
