#include "AbyssRig.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AttributesRuntime.h"

bool FAbyssAnimSampler::Sample(USkeletalMesh* Mesh, const UAnimSequence* Seq, float Time, TArray<FTransform>& Out)
{
	if (!Mesh || !Seq) return false;
	const int32 N = Mesh->GetRefSkeleton().GetNum();
	if (!BC.IsValid() || SK.Get() != Mesh)
	{
		TArray<FBoneIndexType> Req;
		for (int32 i = 0; i < N; ++i) Req.Add((FBoneIndexType)i);
		BC = MakeShared<FBoneContainer>(Req, UE::Anim::FCurveFilterSettings(), *Mesh);
		SK = Mesh;
	}
	FMemMark Mark(FMemStack::Get());
	FCompactPose Pose;
	Pose.SetBoneContainer(BC.Get());
	FBlendedCurve Curve;
	Curve.InitFrom(*BC);
	UE::Anim::FStackAttributeContainer Attr;
	FAnimationPoseData PD(Pose, Curve, Attr);
	const float Len = FMath::Max(Seq->GetPlayLength(), 0.01f);
	Seq->GetAnimationPose(PD, FAnimExtractContext((double)FMath::Fmod(FMath::Max(Time, 0.f), Len)));
	Out.SetNum(N);
	for (FCompactPoseBoneIndex CI : Pose.ForEachBoneIndex())
	{
		const int32 MI = BC->MakeMeshPoseIndex(CI).GetInt();
		if (Out.IsValidIndex(MI)) Out[MI] = Pose[CI];
	}
	return true;
}

bool FAbyssRig::Init(UPoseableMeshComponent* InMesh)
{
	bReady = false;
	Mesh = InMesh;
	const USkeletalMesh* SK = InMesh ? Cast<USkeletalMesh>(InMesh->GetSkinnedAsset()) : nullptr;
	if (!SK) return false;
	const FReferenceSkeleton& Ref = SK->GetRefSkeleton();
	const TArray<FTransform>& Pose = Ref.GetRefBonePose();
	RefLocal = Pose;
	RefComp.SetNum(Pose.Num());
	Parent.SetNum(Pose.Num());
	Bones.Reset();
	for (int32 i = 0; i < Pose.Num(); ++i)
	{
		const int32 P = Ref.GetParentIndex(i);
		Parent[i] = P;
		RefComp[i] = P == INDEX_NONE ? Pose[i] : Pose[i] * RefComp[P];
		// "mixamorig:LeftArm" / "mixamorig_LeftArm" / "LeftArm" -> "LeftArm"
		FString N = Ref.GetBoneName(i).ToString();
		int32 Cut = INDEX_NONE;
		if (N.FindLastChar(TEXT(':'), Cut)) N = N.Mid(Cut + 1);
		N.RemoveFromStart(TEXT("mixamorig_"));
		N.RemoveFromStart(TEXT("mixamorig"));
		Bones.Add(FName(*N), i);
	}
	bReady = Bone(TEXT("Hips")) != INDEX_NONE && InMesh->BoneSpaceTransforms.Num() == Pose.Num();
	return bReady;
}

int32 FAbyssRig::Bone(const TCHAR* ShortName) const
{
	const int32* I = Bones.Find(FName(ShortName));
	return I ? *I : INDEX_NONE;
}

void FAbyssRig::Rot(const TCHAR* ShortName, const FVector& AxisCS, float Deg)
{
	const int32 i = Bone(ShortName);
	if (i == INDEX_NONE || FMath::IsNearlyZero(Deg) || !Work.IsValidIndex(i)) return;
	const int32 P = Parent[i];
	const FVector AxisParent = P == INDEX_NONE ? AxisCS : RefComp[P].InverseTransformVectorNoScale(AxisCS).GetSafeNormal();
	Work[i].SetRotation(FQuat(AxisParent, FMath::DegreesToRadians(Deg)) * Work[i].GetRotation());
}

void FAbyssRig::Commit()
{
	UPoseableMeshComponent* M = Mesh.Get();
	if (!bReady || !M || M->BoneSpaceTransforms.Num() != Work.Num()) return;
	M->BoneSpaceTransforms = Work;
	M->MarkRefreshTransformDirty();
}

bool AbyssRig::FitHumanoid(UPoseableMeshComponent* C, USkeletalMesh* SK, float Height, UMaterialInterface* Mat)
{
	if (!C || !SK) return false;
	C->SetSkinnedAssetAndUpdate(SK);
	if (Mat)
	{
		for (int32 m = 0; m < C->GetNumMaterials(); ++m) C->SetMaterial(m, Mat);
	}
	const FBoxSphereBounds B = SK->GetBounds();
	const float Sc = Height / FMath::Max(B.BoxExtent.Z * 2.f, 1.f);
	// Tripo/Mixamo import: Z up, faces +Y  ->  face parent +X
	C->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	C->SetRelativeScale3D(FVector(Sc));
	C->SetRelativeLocation(FVector(0.f, 0.f, -(B.Origin.Z - B.BoxExtent.Z) * Sc));
	C->SetVisibility(true);
	return true;
}

void AbyssRig::Humanoid(FAbyssRig& R, float T, float Walk01, EChefWork Work, float WB, float Hold)
{
	if (!R.bReady) return;
	const FVector Side(1.f, 0.f, 0.f), Fwd(0.f, 1.f, 0.f), Up(0.f, 0.f, 1.f);
	const float W = FMath::Clamp(Walk01, 0.f, 1.f);
	WB = FMath::Clamp(WB, 0.f, 1.f) * (1.f - W);
	const float Ph = T * 8.5f;
	const float S = FMath::Sin(Ph);
	R.Begin();

	// --- legs: +Side swings a hanging bone forward, -Side bends a knee back
	const float LegAmp = 26.f * W;
	R.Rot(TEXT("LeftUpLeg"), Side, LegAmp * S);
	R.Rot(TEXT("RightUpLeg"), Side, -LegAmp * S);
	R.Rot(TEXT("LeftLeg"), Side, -(4.f + 38.f * FMath::Max(0.f, FMath::Cos(Ph + 0.4f))) * W - 3.f * WB);
	R.Rot(TEXT("RightLeg"), Side, -(4.f + 38.f * FMath::Max(0.f, -FMath::Cos(Ph + 0.4f))) * W - 3.f * WB);
	R.Rot(TEXT("LeftFoot"), Side, 10.f * W * FMath::Max(0.f, -S));
	R.Rot(TEXT("RightFoot"), Side, 10.f * W * FMath::Max(0.f, S));

	// --- body: breathing, a forward lean when walking or working, look down at the work
	const float Breath = FMath::Sin(T * 1.7f);
	R.Rot(TEXT("Spine"), Side, -(3.f * W + 8.f * WB) + Breath * 1.2f);
	R.Rot(TEXT("Spine1"), Up, 6.f * W * S);
	R.Rot(TEXT("Neck"), Side, -6.f * WB);
	R.Rot(TEXT("Head"), Side, -14.f * WB + Breath * 0.8f);
	R.Rot(TEXT("Head"), Up, (1.f - W) * (1.f - WB) * 10.f * FMath::Sin(T * 0.35f));

	// --- arms: T-pose -> down along the body (left arm along +X, right along -X)
	R.Rot(TEXT("LeftArm"), Fwd, 74.f + Breath * 1.5f);
	R.Rot(TEXT("RightArm"), Fwd, -74.f - Breath * 1.5f);
	R.Rot(TEXT("LeftShoulder"), Fwd, 6.f);
	R.Rot(TEXT("RightShoulder"), Fwd, -6.f);

	// natural swing opposite to the legs
	float LFwd = -22.f * W * S, RFwd = 22.f * W * S;
	float LElbow = 12.f + 10.f * W, RElbow = 12.f + 10.f * W;

	// kitchen work: forearms in front of the belly, plus the motion of the step
	if (WB > 0.f)
	{
		float LW = 35.f, RW = 45.f, LE = 70.f, RE = 75.f;
		switch (Work)
		{
		case EChefWork::Chop:  // knife hand chops, the other holds the fish
			RW = 40.f + 14.f * FMath::Max(0.f, FMath::Sin(T * 11.f));
			RE = 70.f + 25.f * FMath::Max(0.f, FMath::Sin(T * 11.f));
			break;
		case EChefWork::Stir:  // spatula hand pushes back and forth, left on the pan handle
			RW = 50.f + 12.f * FMath::Sin(T * 4.f);
			RE = 65.f + 10.f * FMath::Cos(T * 4.f);
			LW = 45.f;
			LE = 55.f;
			break;
		case EChefWork::Plate: // both hands arranging slowly
			LW = 42.f + 6.f * FMath::Sin(T * 2.f);
			RW = 42.f + 6.f * FMath::Sin(T * 2.f + 1.7f);
			LE = 80.f;
			RE = 80.f;
			break;
		default: break;
		}
		LFwd = FMath::Lerp(LFwd, LW, WB);
		RFwd = FMath::Lerp(RFwd, RW, WB);
		LElbow = FMath::Lerp(LElbow, LE, WB);
		RElbow = FMath::Lerp(RElbow, RE, WB);
	}
	// carrying a plate: both forearms level in front of the chest, even while walking
	if (Hold > 0.f)
	{
		LFwd = FMath::Lerp(LFwd, 48.f, Hold);
		RFwd = FMath::Lerp(RFwd, 48.f, Hold);
		LElbow = FMath::Lerp(LElbow, 62.f, Hold);
		RElbow = FMath::Lerp(RElbow, 62.f, Hold);
	}
	R.Rot(TEXT("LeftArm"), Side, LFwd);
	R.Rot(TEXT("RightArm"), Side, RFwd);
	// elbows bend forward around the T-pose up axis
	R.Rot(TEXT("LeftForeArm"), Up, LElbow);
	R.Rot(TEXT("RightForeArm"), Up, -RElbow);
	R.Commit();
}
