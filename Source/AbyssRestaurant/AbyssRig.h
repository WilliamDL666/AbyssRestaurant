#pragma once

#include "CoreMinimal.h"

class UPoseableMeshComponent;
class USkeletalMesh;
class UMaterialInterface;
class UAnimSequence;
struct FBoneContainer;

// Samples an imported AnimSequence into a mesh-indexed local pose (for poseable meshes).
struct ABYSSRESTAURANT_API FAbyssAnimSampler
{
	TSharedPtr<FBoneContainer> BC;
	TWeakObjectPtr<USkeletalMesh> SK;
	bool Sample(USkeletalMesh* Mesh, const UAnimSequence* Seq, float Time, TArray<FTransform>& OutLocal);
};

// Procedural animation for the Tripo/Mixamo humanoids (chef, customers).
// Component-space axes of the upright skeleton: X = character's left, Y = forward, Z = up.
struct ABYSSRESTAURANT_API FAbyssRig
{
	TWeakObjectPtr<UPoseableMeshComponent> Mesh;
	TArray<FTransform> RefLocal, RefComp, Work;
	TArray<int32> Parent;
	TMap<FName, int32> Bones;
	bool bReady = false;

	bool Init(UPoseableMeshComponent* InMesh);
	int32 Bone(const TCHAR* ShortName) const;
	void Begin() { Work = RefLocal; }
	void Rot(const TCHAR* ShortName, const FVector& AxisCS, float Deg);
	void Commit();
};

enum class EChefWork : uint8 { None, Chop, Stir, Plate };

namespace AbyssRig
{
	// Put a skeletal mesh on a poseable component: scaled to Height, feet on the parent origin, facing parent +X.
	ABYSSRESTAURANT_API bool FitHumanoid(UPoseableMeshComponent* C, USkeletalMesh* SK, float Height, UMaterialInterface* Mat);

	// Walk cycle / idle breathing / kitchen work on top of the T-pose.
	ABYSSRESTAURANT_API void Humanoid(FAbyssRig& R, float Time, float Walk01, EChefWork Work, float WorkBlend, float Hold01 = 0.f);
}
