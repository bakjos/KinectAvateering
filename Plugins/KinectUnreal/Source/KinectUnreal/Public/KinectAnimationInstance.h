// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnumClasses.h"
#include "Animation/AnimInstance.h"
#include "KinectDTO.h"
#include "KinectAnimationInstance.generated.h"



/**
 * 
 */
UCLASS(ClassGroup = (Kinect))
class KINECTUNREAL_API UKinectAnimationInstance : public UAnimInstance
{
	GENERATED_BODY()
public:

	UKinectAnimationInstance();

	UFUNCTION(Category = "Kinect|Animation", BlueprintPure, meta = (BlueprintThreadSafe))
	FRotator GetConvertedJointOrientation(EKinectJointType joinType, EKinectBody bodyNumber);

	UPROPERTY(EditAnywhere, Category = Skeletons)
	bool mirrorSkeletons;

	
	

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	
	TMap<EKinectJointType, TArray<EKinectJointType>> bones;
	TArray<Body20> bodies;
	FRWLock		  frwLock;
	TMap<EKinectJointType, FRotator> jointTransforms;
};
