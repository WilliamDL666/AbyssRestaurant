#include "AbyssGameMode.h"
#include "AbyssHUD.h"
#include "AbyssLoc.h"
#include "SubmarinePawn.h"
#include "DiverPawn.h"
#include "CatchableFish.h"
#include "RestaurantDome.h"
#include "RestaurantCustomer.h"
#include "KitchenGame.h"
#include "PrepMenu.h"
#include "AbyssSfx.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"

// The guided first day. Each step has one thing to do; it advances by itself when the player has done it.
//  0 welcome            1 drive              2 searchlight        3 find the tuna      4 claw it
//  5 dive out           6 net yellowtail     7 back aboard        8 dock               9 menu + market
// 10 aquarium tab      11 open the doors    12 take an order     13 open the counter  14 cook a dish
// 15 serve it -> done

void AAbyssGameMode::StartTutorial()
{
	bTutorial = true;
	TutStep = 0;
	TutTime = TutMoved = 0.f;
	bTutSawAquarium = false;
	// the tutorial is its own level before the real game: "day 0". Day 1 starts after its night has been closed.
	Day = 0;
	SpawnTutorialFish();
}

void AAbyssGameMode::ToggleTutorial()
{
	if (bTutorial)
	{
		bTutorial = false;
		// skipped before it really began: this is simply day 1 then
		if (Day == 0 && Phase == EAbyssPhase::Explore && CargoCount() == 0) Day = 1;
		ShowToast(TEXT("Tutorial skipped - press [F1] to bring it back"), FLinearColor(0.6f, 0.95f, 1.f), 3.f);
		return;
	}
	// pick it up wherever the player is now
	bTutorial = true;
	TutTime = 0.f;
	TutStep = Phase == EAbyssPhase::Explore ? 1 : Phase == EAbyssPhase::Prep ? 9 : Phase == EAbyssPhase::Restaurant ? 12 : 0;
	if (Phase == EAbyssPhase::Explore) SpawnTutorialFish();
}

// A school of yellowtail and one tuna close to the dock, so the first lesson does not start with a search
void AAbyssGameMode::SpawnTutorialFish()
{
	if (bTutFishSpawned || !DockDome.IsValid()) return;
	bTutFishSpawned = true;
	UWorld* W = GetWorld();
	const FVector Dock = DockDome->GetDockLocation();
	const FVector Out = (Dock - DockDome->GetActorLocation()).GetSafeNormal2D();
	const FVector Side(-Out.Y, Out.X, 0.f);
	auto Spawn = [&](EFishType Type, const FVector& P, float Wander, int32 School, ACatchableFish* Lead, const FVector& Off) -> ACatchableFish*
	{
		const FTransform T(FRotator::ZeroRotator, P);
		ACatchableFish* F = W->SpawnActorDeferred<ACatchableFish>(ACatchableFish::StaticClass(), T, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (F)
		{
			F->Type = Type;
			F->WanderRadius = Wander;
			F->SchoolId = School;
			F->Leader = Lead;
			F->SchoolOffset = Off;
			F->FinishSpawning(T);
		}
		return F;
	};
	auto Floor = [&](const FVector& At)
	{
		FHitResult Hit;
		return W->LineTraceSingleByChannel(Hit, FVector(At.X, At.Y, -200.f), FVector(At.X, At.Y, -30000.f), ECC_WorldStatic) ? Hit.ImpactPoint.Z : Dock.Z - 600.f;
	};
	FVector SP = Dock + Out * 2600.f + Side * 1400.f;
	SP.Z = Floor(SP) + 420.f;
	ACatchableFish* Lead = nullptr;
	for (int32 i = 0; i < 5; ++i)
	{
		const float A = 2.f * PI * i / 5.f;
		const FVector Off(FMath::Cos(A) * 150.f, FMath::Sin(A) * 150.f, (i % 2) * 50.f - 25.f);
		ACatchableFish* F = Spawn(EFishType::Yellowtail, SP + Off, 600.f, 9001, Lead, Off);
		if (!Lead) Lead = F;
	}
	FVector TP = Dock + Out * 3600.f - Side * 1800.f;
	TP.Z = Floor(TP) + 650.f;
	Spawn(EFishType::Tuna, TP, 2000.f, -1, nullptr, FVector::ZeroVector);
}

void AAbyssGameMode::TutNext()
{
	TutStep = FMath::Min(TutStep + 1, TutorialSteps - 1);
	AbyssSfx::Play(this, TEXT("Chime"), 0.8f);
	TutTime = 0.f;
	if (TutStep == 2) { if (ASubmarinePawn* S = Sub.Get()) bTutLight0 = S->bLightOn; }
}

void AAbyssGameMode::TickTutorial(float Dt)
{
	TutTime += Dt;
	ASubmarinePawn* S = Sub.Get();
	ARestaurantDome* D = ServiceDome.Get();
	// the player ran ahead of the lesson: catch up with where they actually are
	if (Phase == EAbyssPhase::Prep && TutStep < 9) { TutStep = 9; TutTime = 0.f; }
	if (Phase == EAbyssPhase::Restaurant && TutStep < 12) { TutStep = 12; TutTime = 0.f; }
	if (Phase == EAbyssPhase::Summary || Phase == EAbyssPhase::Ending) { bTutorial = false; return; }

	bool bDone = false;
	switch (TutStep)
	{
	case 1:
		if (S && S->GetVelocity().Size() > 250.f) TutMoved += Dt;
		bDone = TutMoved > 1.5f;
		break;
	case 2: bDone = S && S->bLightOn != bTutLight0; break;
	case 3: bDone = (S && S->Target.IsValid() && S->Target->Type == EFishType::Tuna) || Stock[(int32)EFishType::Tuna] > 0; break;
	case 4: bDone = Stock[(int32)EFishType::Tuna] > 0 || (TutTime > 75.f); break;
	case 5: bDone = IsDiving() || Stock[(int32)EFishType::Yellowtail] > 0; break;
	case 6: bDone = Stock[(int32)EFishType::Yellowtail] > 0; break;
	case 7: bDone = !IsDiving(); break;
	case 8: bDone = Phase != EAbyssPhase::Explore; break;
	case 9:
	{
		bool bMenu = false;
		for (int32 d = 0; d < AbyssData::NumDishes; ++d) bMenu |= MenuMain[d] > 0 || MenuSide[d] > 0;
		bDone = bMenu && PantryCount() > 0;
		break;
	}
	case 10:
		bTutSawAquarium |= PrepTab == 1;
		bDone = bTutSawAquarium && TutTime > 2.f;
		break;
	case 11: bDone = Phase == EAbyssPhase::Restaurant; break;
	case 12:
	{
		TArray<ARestaurantCustomer*> Q;
		if (D) D->GetQueue(Q);
		bDone = Q.Num() > 0;
		break;
	}
	case 13: bDone = D && (D->Counter.bOpen || D->IsCarrying()); break;
	case 14: bDone = NightServed > 0; break;
	case 15: if (TutTime > 12.f) bTutorial = false; break;
	default: break;
	}
	if (bDone) TutNext();
}

void AAbyssGameMode::TutorialText(FString& Title, TArray<FString>& L) const
{
	L.Reset();
	const ARestaurantDome* D = ServiceDome.Get();
	switch (TutStep)
	{
	case 0:
		Title = TEXT("TUTORIAL LEVEL");
		L = { TEXT("By day you catch fish from your submarine. By night you cook them."), TEXT("How well you catch a fish decides what its dish sells for."),
			TEXT("This practice day does not count: battery and oxygen never run out,"), TEXT("guests are patient, and Day 1 begins once tonight's service is over."),
			TEXT("[Enter] start the tutorial          [F1] skip to Day 1") };
		break;
	case 1:
		Title = TEXT("Drive the submarine");
		L = { TEXT("[W A S D] move   -   mouse to look   -   [Space] / [Ctrl] up and down"), TEXT("[Shift] boosts, but burns the battery faster. Try moving around.") };
		break;
	case 2:
		Title = TEXT("The searchlight");
		L = { TEXT("Press [F] to switch the searchlight off and on."), TEXT("Light shows you the fish - but shy fish hide from it."), TEXT("Running dark, the claw only reaches about half as far.") };
		break;
	case 3:
		Title = TEXT("Find the tuna");
		L = { TEXT("A tuna patrols a loop nearby (blue dot on the sonar, bottom right)."), TEXT("The dotted line in the water shows where it is heading."), TEXT("Point the searchlight at it to lock on.") };
		break;
	case 4:
		Title = TEXT("Catch it calm = PRIME");
		L = { TEXT("Hold [Left Mouse] to close the claw on it."), TEXT("Chasing spooks it. Park AHEAD on its route and let it come to you."), TEXT("Green bar = calm = a PRIME catch that sells for +35%.") };
		break;
	case 5:
		Title = TEXT("Dive out");
		L = { TEXT("Small fish slip through the claw. Stop the submarine (hover),"), TEXT("then press [E] to swim out as a diver.") };
		break;
	case 6:
		Title = TEXT("Net the yellowtail");
		L = { TEXT("Press [2] for the hand net. A school of yellowtail is close by."), TEXT("Swim in SLOWLY - let go of [Shift] - or the whole school scatters."), TEXT("[Left Mouse] to swing. Three in one calm throw = PRIME.") };
		break;
	case 7:
		Title = TEXT("Back aboard");
		L = { TEXT("Fish caught alive (net or claw) can also go into the aquarium later."), TEXT("Swim back to the submarine's hatch and press [E].") };
		break;
	case 8:
		Title = TEXT("Head home");
		L = { TEXT("Follow the gold DOCK marker on the compass back to the restaurant."), TEXT("In the glowing ring at the docking bay, press [E].") };
		break;
	case 9:
		Title = TEXT("Before opening: menu and market");
		L = { TEXT("Buy a seasoning on the right with [+], then use the arrows on a dish to pick it."), TEXT("A seasoning that matches a guest's taste doubles the tip."), TEXT("House style is always free. Fresh garnishes spoil - buy what you can sell.") };
		break;
	case 10:
		Title = TEXT("The aquarium");
		L = { TEXT("Click the Aquarium tab at the top left."), TEXT("A live fish on show earns ticket money and draws guests every night,"), TEXT("but costs upkeep and cannot be cooked while it is in the tank.") };
		break;
	case 11:
		Title = TEXT("Open the doors");
		L = { TEXT("When the menu looks right, press OPEN THE RESTAURANT (or [Enter]).") };
		break;
	case 12:
		Title = TEXT("Take an order");
		L = { TEXT("The kitchen is behind glass: walk up to its doors and they swing open."), TEXT("Guests sit down and wave. Walk up to the one with the [?] bubble"), TEXT("and press [E] to take the order.") };
		break;
	case 13:
		Title = TEXT("To the kitchen");
		L = { TEXT("Go back in through the kitchen doors, walk behind the counter"), TEXT("and press [E] to start cooking.") };
		break;
	case 14:
	{
		const ARestaurantCustomer* C = D ? D->Counter.Active.Get() : nullptr;
		const FRecipe* R = C ? &AbyssRecipes::Get(C->Dish) : nullptr;
		if (R && R->bStew)
		{
			Title = TEXT("Stew: what goes in when");
			L = { TEXT("Drag KELP into the pot to start. Cut a yellowtail fillet on the board."), TEXT("Add the fish, then the greens, when the marker reaches each icon."), TEXT("When it is ready drag a ladle to the plate, add sesame, ring the bell.") };
		}
		else if (R && R->bCook)
		{
			Title = TEXT("Teppanyaki: mind the heat");
			L = { TEXT("Drag the fillet to the board, pull the knife down the gold lines, drag the steak to the pan."), TEXT("Click the knob: HIGH to sear at the start, then turn it down. Click the pan to flip."), TEXT("In the green zone drag it to the plate, add the sides on the ticket, ring the bell.") };
		}
		else if (R && R->Fish >= 0)
		{
			Title = TEXT("Nigiri: knife work");
			L = { TEXT("Drag the yellowtail fillet onto the board (the gold strip gives the PRIME one)."), TEXT("Press on a gold line and pull the knife straight down - five thin slices."), TEXT("Plate what the ticket shows: rice x2, fish x2, wasabi. Then ring the bell.") };
		}
		else
		{
			Title = TEXT("Cook the order");
			L = { TEXT("The ticket at the top shows what goes on the plate."), TEXT("Drag each ingredient from the bar onto the plate, then ring the bell.") };
		}
		if (D && D->IsCarrying()) { Title = TEXT("Serve it"); L = { TEXT("Carry the plate to the guest - follow the green marker - and press [E].") }; }
		break;
	}
	default:
		Title = TEXT("That's the whole loop!");
		L = { TEXT("Catch calmly for PRIME fish, season to your guests' tastes, cook with care."), TEXT("After closing you can buy upgrades, then dive again."), TEXT("[Enter] close     [F1] replays the tutorial any time") };
		break;
	}
}

void AAbyssHUD::DrawTutorial(AAbyssGameMode* GM)
{
	FString Title;
	TArray<FString> Lines;
	GM->TutorialText(Title, Lines);
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	if (GM->TutStep == 0)
	{
		// opening card: the game starts here
		Panel(0.f, 0.f, W, H, FLinearColor(0.f, 0.02f, 0.06f, 0.72f));
		const float CW = 980.f * S, CH = 600.f * S, CX = W * 0.5f - CW * 0.5f, CY = H * 0.5f - CH * 0.5f;
		Panel(CX, CY, CW, CH, FLinearColor(0.02f, 0.06f, 0.16f, 0.95f));
		Border(CX, CY, CW, CH, FLinearColor(1.f, 0.78f, 0.32f, 0.95f), 3.f);
		Panel(CX, CY, CW, 6.f * S, FLinearColor(1.f, 0.78f, 0.32f, 0.95f));
		// the logo already carries the game's name
		if (UTexture2D* Logo = UITex(TEXT("H_Logo"))) { const float LW = 300.f * S; Tex(Logo, W * 0.5f - LW * 0.5f, CY + 18.f * S, LW, LW * Logo->GetSizeY() / FMath::Max(1, Logo->GetSizeX())); }
		Text(LOCS("ABYSS DINING", "深海食堂"), W * 0.5f, CY + 145.f * S, 24.f, FLinearColor(0.32f, 0.18f, 0.06f), true);   // on the logo's ribbon
		Panel(CX + 140.f * S, CY + 214.f * S, CW - 280.f * S, 2.f * S, FLinearColor(1.f, 0.78f, 0.32f, 0.6f));
		Text(Title, W * 0.5f, CY + 226.f * S, 46.f, FLinearColor(1.f, 0.82f, 0.38f), true);
		for (int32 i = 0; i < Lines.Num(); ++i)
		{
			const bool bKeys = i == Lines.Num() - 1;
			const float Pu = bKeys ? 0.7f + 0.3f * FMath::Sin(Time * 3.5f) : 1.f;
			Text(Lines[i], W * 0.5f, CY + (310.f + i * 36.f + (bKeys ? 34.f : 0.f)) * S, bKeys ? 24.f : 20.f,
				bKeys ? FLinearColor(1.f, 0.9f, 0.6f, Pu) : FLinearColor(1.f, 0.96f, 0.86f), true);
		}
		return;
	}
	// sized to the longest line
	float TW = TextW(Title, 22.f);
	for (const FString& Ln : Lines) TW = FMath::Max(TW, TextW(Ln, 16.f));
	const float PW = FMath::Clamp(TW + 120.f * S, 520.f * S, W - 40.f * S);
	const float PH = (74.f + Lines.Num() * 26.f) * S;
	// where it sits: clear of what each screen needs the player to see
	float X = W * 0.5f - PW * 0.5f, Y = 168.f * S;
	const ARestaurantDome* D = GM->GetDome();
	if (GM->Phase == EAbyssPhase::Prep)
	{
		const FPrepLayout L = FPrepLayout::Make(W, H);
		X = L.Pos.X + 20.f * L.U;
		Y = L.Pos.Y + L.Size.Y - PH - 100.f * L.U;
	}
	else if (GM->Phase == EAbyssPhase::Restaurant && D && D->Counter.bOpen)
	{
		X = W - PW - 16.f * S;
		Y = 200.f * S;
	}
	const float Pulse = 0.75f + 0.25f * FMath::Sin(Time * 3.f);
	Panel(X, Y, PW, PH, FLinearColor(0.02f, 0.06f, 0.16f, 0.93f));
	Panel(X, Y, PW, 4.f * S, FLinearColor(1.f, 0.78f, 0.32f, 0.9f));
	Border(X, Y, PW, PH, FLinearColor(1.f, 0.78f, 0.32f, Pulse), 2.f);
	const float IS = 46.f * S;
	Tex(UITex(TEXT("B_Research")), X + 14.f * S, Y + 12.f * S, IS, IS);
	Text(Title, X + 72.f * S, Y + 12.f * S, 22.f, FLinearColor(1.f, 0.82f, 0.38f));
	const FString Count = FString::Printf(TEXT("%d / %d"), FMath::Min(GM->TutStep + 1, (int32)AAbyssGameMode::TutorialSteps), (int32)AAbyssGameMode::TutorialSteps);
	Text(Count, X + PW - 16.f * S, Y + 14.f * S, 15.f, FLinearColor(0.7f, 0.82f, 0.85f), false, true);
	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		Text(Lines[i], X + 72.f * S, Y + (46.f + i * 26.f) * S, 16.f, FLinearColor(1.f, 0.96f, 0.86f));
	}
	if (GM->TutStep > 0 && GM->TutStep < AAbyssGameMode::TutorialSteps - 1)
		Text(TEXT("[F1] skip tutorial"), X + PW - 16.f * S, Y + PH - 24.f * S, 12.f, FLinearColor(0.7f, 0.82f, 0.85f, 0.75f), false, true);
	// progress dots
	for (int32 i = 0; i < AAbyssGameMode::TutorialSteps; ++i)
	{
		Panel(X + 72.f * S + i * 12.f * S, Y + PH - 14.f * S, 8.f * S, 4.f * S, i <= GM->TutStep ? FLinearColor(1.f, 0.78f, 0.32f) : FLinearColor(1.f, 1.f, 1.f, 0.18f));
	}
}
