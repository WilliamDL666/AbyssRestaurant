#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AbyssHUD.generated.h"

class UFont;
class AAbyssGameMode;
class ASubmarinePawn;
class ADiverPawn;
class ACatchableFish;

// Entire game UI drawn on the canvas (no UMG assets needed for v0.1).
UCLASS()
class ABYSSRESTAURANT_API AAbyssHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	bool bRecipeBook = false;

private:
	UPROPERTY(Transient) TObjectPtr<UFont> Font;
	// cooking-UI pictures (/Game/Abyss/UI/T_*), loaded on demand
	UPROPERTY(Transient) TMap<FName, TObjectPtr<class UTexture2D>> UITextures;
	class UTexture2D* UITex(const FString& Name);
	void Tex(class UTexture2D* T, float X, float Y, float W, float H, const FLinearColor& C = FLinearColor::White);
	int32 LastCuts = 0;
	float ChopT = 99.f;
	float S = 1.f;        // UI scale (1080p = 1)
	float FontBaseH = 24.f;
	float Time = 0.f;

	// primitives
	void Panel(float X, float Y, float W, float H, const FLinearColor& C = FLinearColor(0.01f, 0.06f, 0.08f, 0.72f));
	void Border(float X, float Y, float W, float H, const FLinearColor& C, float T = 1.5f);
	void Line(float X1, float Y1, float X2, float Y2, const FLinearColor& C, float T = 2.f);
	void Circle(float CX, float CY, float R, const FLinearColor& C, float T = 2.f, int32 Seg = 48, float A0 = 0.f, float A1 = 2.f * PI);
	void Text(const FString& Str, float X, float Y, float Px, const FLinearColor& C = FLinearColor::White, bool bCenter = false, bool bRight = false);
	float TextW(const FString& Str, float Px);
	void Bar(float X, float Y, float W, float H, float V, const FLinearColor& Fill, int32 Segments = 0);

	void DrawExplore(AAbyssGameMode* GM, ASubmarinePawn* Sub);
	void DrawDive(AAbyssGameMode* GM, ADiverPawn* Diver);
	void DrawObjectives(AAbyssGameMode* GM);
	void DrawCargo(AAbyssGameMode* GM, float Y);
	void DrawCompass(AAbyssGameMode* GM, const FVector& Origin, float Heading, bool bSubMarker);
	void DrawSonar(AAbyssGameMode* GM, const FVector& Origin, float Heading, float Range);
	void DrawFishTag(ACatchableFish* F, const FVector& From, bool bDiver, float Progress, bool bActive);
	void DrawRestaurant(AAbyssGameMode* GM);
	void DrawSummary(AAbyssGameMode* GM);
	void DrawTopBar(AAbyssGameMode* GM);
	void DrawToast(AAbyssGameMode* GM);
	void DrawKitchen(AAbyssGameMode* GM, class ARestaurantDome* Dome);
	void DrawRecipeBook(AAbyssGameMode* GM);
	void DrawCounter(AAbyssGameMode* GM, class ARestaurantDome* Dome);
	void DrawCounterItem(const struct FCItem& I, const FVector2D& C, float Size, float Alpha = 1.f, float MaxW = 0.f);
	void DrawEnding(AAbyssGameMode* GM);
	void Disc(float CX, float CY, float R, const FLinearColor& C, int32 Seg = 40);
};
