// Fill out your copyright notice in the Description page of Project Settings.

#include "KinectAnimationInstance.h"
#include "KinematicChain.h"
#include "KinectDevice.h"
#include <Runtime/Engine/Classes/Kismet/KismetMathLibrary.h>
#include <Runtime/Engine/Classes/Engine/SkeletalMeshSocket.h>
#include <Runtime/Engine/Public/ReferenceSkeleton.h>
#include "KinematicChain.h"
#include "KinectDeviceUtil.h"

UKinectAnimationInstance::UKinectAnimationInstance() {
	mirrorSkeletons = true;
}

void UKinectAnimationInstance::NativeInitializeAnimation()  {
	const TMap<JointId, TArray<JointId>>& _bones = Kinect20Chain::getInstance()->getBones();

	bones.Reset();

	TArray<JointId> keys;
	_bones.GetKeys(keys);
	for (JointId parent : keys) {
		const TArray<JointId>& _childs = _bones[parent];
		TArray<EKinectJointType> childs;
		for (JointId child : _childs) {
			childs.Add((EKinectJointType)child);
		}
		bones.Add((EKinectJointType)parent, childs);
	}

	for (int i = 0; i < 25; i++) {
		jointTransforms.Add((EKinectJointType)i, FRotator(0, 0, 0));
	}

	jointTransforms.Add((EKinectJointType)SpineBase, FRotator(0, -90, 90));
	jointTransforms.Add((EKinectJointType)SpineMid, FRotator(180, 90, -90));
	jointTransforms.Add((EKinectJointType)Neck, FRotator(0, -90, 90));
	jointTransforms.Add((EKinectJointType)ElbowLeft, FRotator(0, -90, 180));
	jointTransforms.Add((EKinectJointType)WristLeft, FRotator(0, -90, 180));
	jointTransforms.Add((EKinectJointType)ElbowRight, FRotator(0, 90, 0));
	jointTransforms.Add((EKinectJointType)WristRight, FRotator(0, 90, 180));
	jointTransforms.Add((EKinectJointType)KneeLeft, FRotator(0, 90, 0));
	jointTransforms.Add((EKinectJointType)AnkleLeft, FRotator(0, 90, 0));
	jointTransforms.Add((EKinectJointType)KneeRight, FRotator(0, -90, 180));
	jointTransforms.Add((EKinectJointType)AnkleRight, FRotator(0, -90, 180));
		
}

void UKinectAnimationInstance::NativeUpdateAnimation(float DeltaSeconds)  {
	frwLock.WriteLock();
	bodies = KinectDevice::GetInstance()->getBodies();
	if (mirrorSkeletons) {
		TArray<Body20> _bodies;
		for ( const Body20& body: bodies ) {
			_bodies.Add(body.tracked?KinectDeviceUtil::mirrorBody(body):body);
		}
		bodies = _bodies;
	}
	frwLock.WriteUnlock();
}


FRotator UKinectAnimationInstance::GetConvertedJointOrientation(EKinectJointType joinType, EKinectBody bodyNumber) {
	
	frwLock.ReadLock();
	int b = (int)bodyNumber;
	int jointId = (int)joinType;
	FRotator rot (0);
	if ( b >= 0 && b < bodies.Num() && jointId >= 0 && jointId < 25) {
		USkeletalMeshComponent* skeletalMesh = GetSkelMeshComponent();
		const Body20& body = bodies[b];
		if (body.tracked) {
			FQuat quat = FQuat(mirrorSkeletons?FRotator(180, 0, 90): FRotator(0, 0, 90)) *body.joints[jointId].orientation;
			FTransform ParentWorldTransform = skeletalMesh->GetComponentTransform();
			FMatrix matrix = FRotationMatrix::Make(quat);

			FVector Forward = ParentWorldTransform.InverseTransformVector(matrix.GetScaledAxis(EAxis::X));
			FVector Right = ParentWorldTransform.InverseTransformVector(matrix.GetScaledAxis(EAxis::Y));
			FVector Up = ParentWorldTransform.InverseTransformVector(matrix.GetScaledAxis(EAxis::Z));

			FMatrix RotMatrix(Forward, Right, Up, FVector::ZeroVector);
			
			if (mirrorSkeletons) {
				rot = UKismetMathLibrary::ComposeRotators(UKismetMathLibrary::ComposeRotators(jointTransforms[joinType], FRotator(0, 180, 0)), RotMatrix.Rotator());
			} else {
				rot = UKismetMathLibrary::ComposeRotators(jointTransforms[joinType], RotMatrix.Rotator());
			}
		}
	}

	frwLock.ReadUnlock();
	return rot;
}
