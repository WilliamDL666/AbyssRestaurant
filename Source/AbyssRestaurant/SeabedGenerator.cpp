#include "SeabedGenerator.h"
#include "AbyssAssets.h"
#include "ProceduralMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	float Fbm(float X, float Y, int32 Octaves)
	{
		float Sum = 0.f, Amp = 0.5f, Norm = 0.f;
		for (int32 i = 0; i < Octaves; ++i)
		{
			Sum += Amp * FMath::PerlinNoise2D(FVector2D(X, Y));
			Norm += Amp;
			X = X * 2.03f + 17.1f;
			Y = Y * 2.03f - 9.7f;
			Amp *= 0.5f;
		}
		return Sum / Norm;
	}
}

ASeabedGenerator::ASeabedGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	bRunConstructionScriptOnDrag = false;

	Terrain = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Terrain"));
	SetRootComponent(Terrain);
	Terrain->bUseComplexAsSimpleCollision = true;
	Terrain->SetCollisionProfileName(TEXT("BlockAll"));
	Terrain->SetCastShadow(true);
}

float ASeabedGenerator::HeightAt(float X, float Y) const
{
	const float S = (float)Seed * 13.37f;
	const float Large = Fbm(X * 0.00011f + S, Y * 0.00011f - S, 5);
	const float Ridge = 1.f - FMath::Abs(Fbm(X * 0.00045f - S, Y * 0.00045f + S, 3));

	float H = ShelfZ + Large * 750.f + FMath::Pow(Ridge, 3.f) * 420.f - 150.f;

	// Trench: shelf drops away toward +X, with a wobbly cliff edge
	const float Edge = X + Large * 3000.f;
	const float T = FMath::SmoothStep(TrenchStartX, TrenchEndX, Edge);
	const float Deep = TrenchZ + Large * 1800.f + FMath::Pow(Ridge, 2.f) * 900.f;
	H = FMath::Lerp(H, Deep, T);

	// Flat pad for the restaurant + dock
	const float D = FVector2D(X - 1200.f, Y - 300.f).Size();
	const float Pad = 1.f - FMath::SmoothStep(FlattenRadius, FlattenRadius + 2800.f, D);
	H = FMath::Lerp(H, ShelfZ - 40.f, Pad);

	// Raise the borders into walls so the player never sees the edge
	const float Nx = FMath::Max((MinX + 3000.f) - X, X - (MaxX - 3000.f)) / 3000.f;
	const float Ny = (FMath::Abs(Y) - (HalfY - 3500.f)) / 3500.f;
	const float Wall = FMath::Clamp(FMath::Max(Nx, Ny), 0.f, 1.f);
	H += FMath::Square(Wall) * (ShelfZ * -0.9f - (H - ShelfZ) * 0.2f) * 1.0f;

	return H;
}

void ASeabedGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildTerrain();
	Scatter();
}

void ASeabedGenerator::BuildTerrain()
{
	CellSize = FMath::Max(CellSize, 100.f);
	const int32 NX = FMath::FloorToInt((MaxX - MinX) / CellSize) + 1;
	const int32 NY = FMath::FloorToInt((2.f * HalfY) / CellSize) + 1;

	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector> Normals;
	TArray<FVector2D> UV;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	Verts.Reserve(NX * NY);
	Normals.Reserve(NX * NY);
	UV.Reserve(NX * NY);
	Colors.Reserve(NX * NY);
	Tris.Reserve((NX - 1) * (NY - 1) * 6);

	const FLinearColor Sand(0.62f, 0.56f, 0.42f);
	const FLinearColor DarkSand(0.30f, 0.29f, 0.25f);
	const FLinearColor Rock(0.26f, 0.27f, 0.26f);
	const FLinearColor Abyss(0.07f, 0.08f, 0.09f);

	for (int32 iy = 0; iy < NY; ++iy)
	{
		for (int32 ix = 0; ix < NX; ++ix)
		{
			const float X = MinX + ix * CellSize;
			const float Y = -HalfY + iy * CellSize;
			const float Z = HeightAt(X, Y);
			Verts.Add(FVector(X, Y, Z));

			const float E = CellSize;
			const float Dx = HeightAt(X + E, Y) - HeightAt(X - E, Y);
			const float Dy = HeightAt(X, Y + E) - HeightAt(X, Y - E);
			const FVector N = FVector(-Dx, -Dy, 2.f * E).GetSafeNormal();
			Normals.Add(N);
			UV.Add(FVector2D(X / 500.f, Y / 500.f));
			Tangents.Add(FProcMeshTangent(1.f, 0.f, 0.f));

			const float Slope = 1.f - N.Z;
			const float DepthT = FMath::SmoothStep(-5500.f, -12000.f, Z);
			const float Var = 0.85f + 0.3f * (FMath::PerlinNoise2D(FVector2D(X * 0.002f, Y * 0.002f)) * 0.5f + 0.5f);
			FLinearColor C = FMath::Lerp(Sand, DarkSand, FMath::Clamp(DepthT * 1.4f, 0.f, 1.f));
			C = FMath::Lerp(C, Rock, FMath::SmoothStep(0.08f, 0.30f, Slope));
			C = FMath::Lerp(C, Abyss, FMath::SmoothStep(-9000.f, -14000.f, Z));
			Colors.Add(C * Var);
		}
	}

	for (int32 iy = 0; iy < NY - 1; ++iy)
	{
		for (int32 ix = 0; ix < NX - 1; ++ix)
		{
			const int32 V00 = iy * NX + ix;
			const int32 V10 = V00 + 1;
			const int32 V01 = V00 + NX;
			const int32 V11 = V01 + 1;
			Tris.Add(V00); Tris.Add(V10); Tris.Add(V01);
			Tris.Add(V10); Tris.Add(V11); Tris.Add(V01);
		}
	}

	Terrain->ClearAllMeshSections();
	Terrain->CreateMeshSection_LinearColor(0, Verts, Tris, Normals, UV, Colors, Tangents, true);
	Terrain->SetMaterial(0, AbyssAssets::BaseSeabed());
}

UHierarchicalInstancedStaticMeshComponent* ASeabedGenerator::MakeLayer(UStaticMesh* Mesh, const FLinearColor& Color, float Rough, float Emissive, bool bCollision, float CullDistance)
{
	UHierarchicalInstancedStaticMeshComponent* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
	C->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	C->SetStaticMesh(Mesh);
	C->SetMaterial(0, AbyssAssets::Lit(this, Color, Rough, 0.f, Emissive));
	C->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (bCollision)
	{
		C->SetCollisionProfileName(TEXT("BlockAll"));
	}
	C->SetCullDistances(0, (int32)CullDistance);
	C->SetCanEverAffectNavigation(false);
	C->SetupAttachment(Terrain);
	C->RegisterComponent();
	return C;
}

namespace
{
	struct FScatterSpec
	{
		const TCHAR* Mesh;      // asset name under /Game/AlienOceanEnvironment/Meshes/
		int32 Count;            // instances (x ScatterDensity)
		float MinScale, MaxScale;
		float MinZ, MaxZ;       // allowed seabed height band (cm)
		int32 Cluster;          // >1: spawn in clumps of this size
		float ClusterRadius;
		bool bCollision;
		float Cull;             // cull distance (cm)
		float Sink;             // fraction of height pushed into the ground
		float Tilt;             // random tilt (deg)
	};

	// Reef shelf ~ -3600..-5200, trench below -6500
	const FScatterSpec GSpecs[] = {
		// big landmark rocks (collide)
		{ TEXT("Rocks/SM_AlienStone_A_Moss"),          40, 0.8f, 2.2f, -20000.f, 0.f, 1, 0.f, true, 60000.f, 0.25f, 12.f },
		{ TEXT("Rocks/SM_AlienStone_B_Moss"),          40, 0.8f, 2.0f, -20000.f, 0.f, 1, 0.f, true, 60000.f, 0.25f, 12.f },
		{ TEXT("Rocks/SM_AlienRock_Cratered_A_Moss"),  35, 0.7f, 1.8f, -20000.f, 0.f, 1, 0.f, true, 60000.f, 0.2f, 15.f },
		{ TEXT("Rocks/SM_AlienRock_Cratered_B_Moss"),  35, 0.7f, 1.8f, -20000.f, 0.f, 1, 0.f, true, 60000.f, 0.2f, 15.f },
		{ TEXT("Rocks/SM_AlienStone_Cratered_C_Moss"), 35, 0.7f, 1.8f, -20000.f, 0.f, 1, 0.f, true, 60000.f, 0.2f, 15.f },
		{ TEXT("Rocks/SM_AlienStone_Arch_A_Moss"),     10, 1.2f, 2.4f, -9000.f, 0.f, 1, 0.f, true, 60000.f, 0.05f, 4.f },
		{ TEXT("Rocks/SM_AlienStone_Arch_B_Moss"),     10, 1.2f, 2.4f, -9000.f, 0.f, 1, 0.f, true, 60000.f, 0.05f, 4.f },
		{ TEXT("Rocks/SM_AlienStone_Wormlike_A_Moss"), 90, 0.6f, 1.8f, -20000.f, 0.f, 3, 600.f, true, 30000.f, 0.15f, 20.f },
		{ TEXT("Rocks/SM_AlienStone_Wormlike_B_Moss"), 90, 0.6f, 1.8f, -20000.f, 0.f, 3, 600.f, true, 30000.f, 0.15f, 20.f },
		{ TEXT("Rocks/SM_AlienStone_Wormlike_C_Moss"), 90, 0.6f, 1.8f, -20000.f, 0.f, 3, 600.f, true, 30000.f, 0.15f, 20.f },
		// pebbles
		{ TEXT("Rocks/SM_Rock_C_A"), 700, 1.f, 4.f, -20000.f, 0.f, 8, 500.f, false, 9000.f, 0.4f, 40.f },
		{ TEXT("Rocks/SM_Rock_C_B"), 700, 1.f, 4.f, -20000.f, 0.f, 8, 500.f, false, 9000.f, 0.4f, 40.f },
		{ TEXT("Rocks/SM_Rock_C_D"), 700, 1.f, 4.f, -20000.f, 0.f, 8, 500.f, false, 9000.f, 0.4f, 40.f },
		// reef corals (shelf)
		{ TEXT("Plants/SM_Coral_Large_A"),   45, 0.8f, 1.6f, -7000.f, 0.f, 2, 700.f, false, 30000.f, 0.05f, 6.f },
		{ TEXT("Plants/SM_Coral_Large_B"),   45, 0.8f, 1.6f, -7000.f, 0.f, 2, 700.f, false, 30000.f, 0.05f, 6.f },
		{ TEXT("Plants/SM_Coral_Bush_A"),   140, 0.7f, 1.6f, -7000.f, 0.f, 4, 600.f, false, 20000.f, 0.05f, 8.f },
		{ TEXT("Plants/SM_Coral_Basic"),    220, 0.8f, 2.0f, -7000.f, 0.f, 6, 500.f, false, 15000.f, 0.05f, 10.f },
		{ TEXT("Plants/SM_BasicCoral_A"),   300, 1.5f, 3.5f, -7000.f, 0.f, 10, 400.f, false, 9000.f, 0.05f, 15.f },
		{ TEXT("Plants/SM_BasicCoral_B"),   300, 1.5f, 3.5f, -7000.f, 0.f, 10, 400.f, false, 9000.f, 0.05f, 15.f },
		{ TEXT("Plants/SM_BasicCoral_D"),   300, 1.5f, 3.5f, -7000.f, 0.f, 10, 400.f, false, 9000.f, 0.05f, 15.f },
		{ TEXT("Plants/SM_BrainPlant"),     120, 0.8f, 1.8f, -7000.f, 0.f, 3, 500.f, false, 15000.f, 0.1f, 10.f },
		{ TEXT("Plants/SM_AnemoneTree_A"),   40, 0.8f, 1.5f, -8000.f, 0.f, 2, 800.f, false, 30000.f, 0.03f, 5.f },
		{ TEXT("Plants/SM_AnemoneTree_B"),   40, 0.8f, 1.5f, -8000.f, 0.f, 2, 800.f, false, 30000.f, 0.03f, 5.f },
		{ TEXT("Plants/SM_StarUrchine"),    200, 0.8f, 1.6f, -20000.f, 0.f, 5, 600.f, false, 9000.f, 0.1f, 10.f },
		{ TEXT("Plants/SM_SeaCucumber_A"),   60, 0.6f, 1.2f, -20000.f, 0.f, 2, 500.f, false, 15000.f, 0.05f, 6.f },
		{ TEXT("Plants/SM_SnailPlant"),      70, 0.7f, 1.4f, -9000.f, 0.f, 3, 500.f, false, 15000.f, 0.05f, 8.f },
		{ TEXT("Plants/SM_FurryMushroom"),   70, 0.7f, 1.4f, -9000.f, 0.f, 3, 500.f, false, 15000.f, 0.05f, 8.f },
		{ TEXT("Plants/SM_Coral_Fungi_A"),  150, 1.5f, 3.f, -20000.f, 0.f, 6, 500.f, false, 9000.f, 0.f, 10.f },
		// sea plants
		{ TEXT("Plants/LeafyPlants/SM_Kelp_Tall"),   260, 1.2f, 2.6f, -6500.f, 0.f, 18, 1100.f, false, 30000.f, 0.f, 6.f },
		{ TEXT("Plants/LeafyPlants/SM_Kelp_Medium"), 260, 1.0f, 2.2f, -6500.f, 0.f, 18, 1100.f, false, 25000.f, 0.f, 6.f },
		{ TEXT("Plants/LeafyPlants/SM_Kelp_Short"),  260, 1.0f, 2.0f, -6500.f, 0.f, 18, 1100.f, false, 20000.f, 0.f, 6.f },
		{ TEXT("Plants/LeafyPlants/SM_SeaWeed"),     500, 1.0f, 2.5f, -7500.f, 0.f, 12, 700.f, false, 9000.f, 0.f, 10.f },
		{ TEXT("Plants/LeafyPlants/SM_OceanFeathers_A"), 250, 1.0f, 2.2f, -7500.f, 0.f, 8, 500.f, false, 9000.f, 0.f, 10.f },
		{ TEXT("Plants/LeafyPlants/SM_CirclePlant_Tall"), 120, 1.0f, 2.0f, -7500.f, 0.f, 5, 500.f, false, 15000.f, 0.f, 8.f },
		{ TEXT("Plants/LeafyPlants/SM_OceanLeafyPlant"), 120, 0.8f, 1.8f, -7500.f, 0.f, 4, 500.f, false, 15000.f, 0.f, 8.f },
		// deep trench: glowing alien flora
		{ TEXT("Plants/SM_VirusPlant"),        110, 0.8f, 1.8f, -30000.f, -7500.f, 4, 700.f, false, 20000.f, 0.f, 8.f },
		{ TEXT("Plants/SM_FungalStemTree"),     70, 1.0f, 2.2f, -30000.f, -7000.f, 3, 800.f, false, 25000.f, 0.f, 6.f },
		{ TEXT("Plants/SM_FloatingGrapePlant"), 70, 1.0f, 2.0f, -30000.f, -7000.f, 3, 800.f, false, 25000.f, 0.f, 6.f },
		{ TEXT("Plants/LargeLilly/SM_LargeLilly_Complete_A"), 8, 1.0f, 1.6f, -30000.f, -7500.f, 1, 0.f, false, 60000.f, 0.f, 3.f },
		{ TEXT("Plants/LargeLilly/SM_LargeLilly_Complete_B"), 8, 1.0f, 1.6f, -30000.f, -7500.f, 1, 0.f, false, 60000.f, 0.f, 3.f },
	};
}

bool ASeabedGenerator::ScatterOceanPack()
{
	if (!bUseOceanPack) return false;
	if (!LoadObject<UStaticMesh>(nullptr, TEXT("/Game/AlienOceanEnvironment/Meshes/Rocks/SM_Rock_C_A.SM_Rock_C_A"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		return false;
	}
	FRandomStream R(Seed * 31 + 7);
	const float D = FMath::Max(ScatterDensity, 0.f);
	auto Blocked = [this](float X, float Y) { return FVector2D(X - 1200.f, Y - 300.f).Size() < FlattenRadius + 400.f; };

	for (const FScatterSpec& Sp : GSpecs)
	{
		const FString Name = FPaths::GetBaseFilename(Sp.Mesh);
		const FString Path = FString::Printf(TEXT("/Game/AlienOceanEnvironment/Meshes/%s.%s"), Sp.Mesh, *Name);
		UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!M) continue;

		UHierarchicalInstancedStaticMeshComponent* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
		C->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		C->SetStaticMesh(M); // keep the pack's own materials
		C->SetCollisionEnabled(Sp.bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (Sp.bCollision) C->SetCollisionProfileName(TEXT("BlockAll"));
		C->SetCullDistances(0, (int32)Sp.Cull);
		C->SetCanEverAffectNavigation(false);
		C->SetCastShadow(true);
		C->SetupAttachment(Terrain);
		C->RegisterComponent();

		const FBoxSphereBounds B = M->GetBounds();
		const float BottomZ = B.Origin.Z - B.BoxExtent.Z; // pivot offset so the mesh base sits on the ground
		const float Height = B.BoxExtent.Z * 2.f;
		TArray<FTransform> Ts;
		const int32 Want = FMath::RoundToInt(Sp.Count * D);
		int32 Tries = 0;
		while (Ts.Num() < Want && Tries++ < Want * 20)
		{
			float CX = R.FRandRange(MinX + 2500.f, MaxX - 2500.f);
			float CY = R.FRandRange(-HalfY + 2500.f, HalfY - 2500.f);
			const float CZ = HeightAt(CX, CY);
			if (CZ < Sp.MinZ || CZ > Sp.MaxZ || Blocked(CX, CY)) continue;
			const int32 N = FMath::Max(1, Sp.Cluster);
			for (int32 k = 0; k < N && Ts.Num() < Want; ++k)
			{
				const float X = CX + (N > 1 ? R.FRandRange(-Sp.ClusterRadius, Sp.ClusterRadius) : 0.f);
				const float Y = CY + (N > 1 ? R.FRandRange(-Sp.ClusterRadius, Sp.ClusterRadius) : 0.f);
				if (Blocked(X, Y)) continue;
				const float Z = HeightAt(X, Y);
				const float S = R.FRandRange(Sp.MinScale, Sp.MaxScale);
				const FRotator Rot(R.FRandRange(-Sp.Tilt, Sp.Tilt), R.FRandRange(0.f, 360.f), R.FRandRange(-Sp.Tilt, Sp.Tilt));
				const FVector Loc(X, Y, Z - (BottomZ + Height * Sp.Sink) * S);
				Ts.Add(FTransform(Rot, Loc, FVector(S)));
			}
		}
		C->AddInstances(Ts, false, false);
	}
	return true;
}

void ASeabedGenerator::Scatter()
{
	if (ScatterOceanPack()) return;
	FRandomStream R(Seed);
	using namespace AbyssAssets;

	auto Blocked = [this](float X, float Y)
	{
		// Keep the restaurant pad + dock clear
		return FVector2D(X - 1200.f, Y - 300.f).Size() < FlattenRadius + 400.f;
	};
	auto RandXY = [&R, this](float& X, float& Y)
	{
		X = R.FRandRange(MinX + 2500.f, MaxX - 2500.f);
		Y = R.FRandRange(-HalfY + 2500.f, HalfY - 2500.f);
	};

	UHierarchicalInstancedStaticMeshComponent* Rocks   = MakeLayer(Sphere(), FLinearColor(0.20f, 0.22f, 0.22f), 0.9f, 0.f, true, 40000.f);
	UHierarchicalInstancedStaticMeshComponent* Boulder = MakeLayer(Cube(), FLinearColor(0.16f, 0.18f, 0.18f), 0.9f, 0.f, true, 40000.f);
	UHierarchicalInstancedStaticMeshComponent* Pink    = MakeLayer(Cone(), FLinearColor(0.85f, 0.25f, 0.42f), 0.7f, 0.12f, false, 14000.f);
	UHierarchicalInstancedStaticMeshComponent* Orange  = MakeLayer(Sphere(), FLinearColor(0.90f, 0.45f, 0.15f), 0.8f, 0.10f, false, 14000.f);
	UHierarchicalInstancedStaticMeshComponent* Purple  = MakeLayer(Cylinder(), FLinearColor(0.50f, 0.25f, 0.85f), 0.6f, 0.15f, false, 14000.f);
	UHierarchicalInstancedStaticMeshComponent* Fan     = MakeLayer(Cube(), FLinearColor(0.95f, 0.70f, 0.20f), 0.7f, 0.10f, false, 14000.f);
	UHierarchicalInstancedStaticMeshComponent* Kelp    = MakeLayer(Cube(), FLinearColor(0.22f, 0.45f, 0.12f), 0.7f, 0.04f, false, 20000.f);
	UHierarchicalInstancedStaticMeshComponent* Worms   = MakeLayer(Cylinder(), FLinearColor(0.20f, 0.95f, 0.80f), 0.5f, 6.0f, false, 12000.f);

	const float D = FMath::Max(ScatterDensity, 0.f);

	// Rocks everywhere
	for (int32 i = 0, N = (int32)(1100 * D); i < N; ++i)
	{
		float X, Y; RandXY(X, Y);
		if (Blocked(X, Y)) continue;
		const float Z = HeightAt(X, Y);
		const float S = R.FRandRange(1.5f, 9.f);
		const FVector Scale(S * R.FRandRange(0.7f, 1.5f), S * R.FRandRange(0.7f, 1.5f), S * R.FRandRange(0.35f, 0.9f));
		const FRotator Rot(R.FRandRange(-15.f, 15.f), R.FRandRange(0.f, 360.f), R.FRandRange(-15.f, 15.f));
		(R.FRand() < 0.7f ? Rocks : Boulder)->AddInstance(FTransform(Rot, FVector(X, Y, Z - S * 12.f), (R.FRand() < 0.7f ? Scale : Scale * 0.6f)));
	}

	// Coral clusters on the shelf
	for (int32 c = 0, N = (int32)(320 * D); c < N; ++c)
	{
		float CX, CY; RandXY(CX, CY);
		if (Blocked(CX, CY) || HeightAt(CX, CY) < -7000.f) continue;
		const int32 Count = R.RandRange(6, 16);
		for (int32 k = 0; k < Count; ++k)
		{
			const float X = CX + R.FRandRange(-450.f, 450.f);
			const float Y = CY + R.FRandRange(-450.f, 450.f);
			if (Blocked(X, Y)) continue;
			const float Z = HeightAt(X, Y);
			const FRotator Yaw(R.FRandRange(-12.f, 12.f), R.FRandRange(0.f, 360.f), R.FRandRange(-12.f, 12.f));
			switch (R.RandRange(0, 3))
			{
			case 0: { const float S = R.FRandRange(0.4f, 1.0f); const float H = R.FRandRange(1.0f, 2.6f);
				Pink->AddInstance(FTransform(Yaw, FVector(X, Y, Z + H * 45.f), FVector(S, S, H))); break; }
			case 1: { const float S = R.FRandRange(1.0f, 2.4f);
				Orange->AddInstance(FTransform(Yaw, FVector(X, Y, Z + S * 15.f), FVector(S, S * R.FRandRange(0.8f, 1.2f), S * 0.65f))); break; }
			case 2: { const float S = R.FRandRange(0.25f, 0.55f); const float H = R.FRandRange(0.8f, 3.0f);
				Purple->AddInstance(FTransform(Yaw, FVector(X, Y, Z + H * 45.f), FVector(S, S, H))); break; }
			default: { const float S = R.FRandRange(1.2f, 2.8f);
				Fan->AddInstance(FTransform(Yaw, FVector(X, Y, Z + S * 45.f), FVector(0.04f, S, S))); break; }
			}
		}
	}

	// Kelp forests: tall swaying-looking strands in patches
	for (int32 c = 0, N = (int32)(70 * D); c < N; ++c)
	{
		float CX, CY; RandXY(CX, CY);
		if (Blocked(CX, CY) || HeightAt(CX, CY) < -6000.f) continue;
		for (int32 k = 0, Count = R.RandRange(10, 26); k < Count; ++k)
		{
			const float X = CX + R.FRandRange(-900.f, 900.f);
			const float Y = CY + R.FRandRange(-900.f, 900.f);
			if (Blocked(X, Y)) continue;
			const float Z = HeightAt(X, Y);
			const float H = R.FRandRange(6.f, 15.f);
			const FRotator Rot(R.FRandRange(-6.f, 6.f), R.FRandRange(0.f, 360.f), R.FRandRange(-6.f, 6.f));
			Kelp->AddInstance(FTransform(Rot, FVector(X, Y, Z + H * 50.f), FVector(0.04f, R.FRandRange(0.25f, 0.5f), H)));
		}
	}

	// Bioluminescent tube worms in the deep
	for (int32 i = 0, N = (int32)(2200 * D); i < N; ++i)
	{
		float X, Y; RandXY(X, Y);
		const float Z = HeightAt(X, Y);
		if (Z > -8500.f) continue;
		const float H = R.FRandRange(0.4f, 1.6f);
		Worms->AddInstance(FTransform(FRotator(R.FRandRange(-20.f, 20.f), 0.f, R.FRandRange(-20.f, 20.f)), FVector(X, Y, Z + H * 45.f), FVector(0.06f, 0.06f, H)));
	}
}
