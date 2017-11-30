// Fill out your copyright notice in the Description page of Project Settings.

#include "KinectDeviceComponent.h"
#include "KinectDevice.h"
#include "KinematicChain.h"
#include <Runtime/Engine/Classes/Kismet/KismetMathLibrary.h>
#include "KinectDeviceUtil.h"


// Sets default values for this component's properties
UKinectDeviceComponent::UKinectDeviceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	mirrorSkeletons = true;
	// ...
}


// Called when the game starts
void UKinectDeviceComponent::BeginPlay()
{
	Super::BeginPlay();

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
	
}


// Called every frame
void UKinectDeviceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	bodies = KinectDevice::GetInstance()->getBodies();
	if (mirrorSkeletons) {
		TArray<Body20> _bodies;
		for (const Body20& body : bodies) {
			_bodies.Add(body.tracked ? KinectDeviceUtil::mirrorBody(body) : body);
		}
		bodies = _bodies;
	}

}

void  UKinectDeviceComponent::GetCenteredBody(EKinectBody& body, bool& isTracked) {
	int centerBody = 0;
	float minDistCenter = 50;
	isTracked = false;
	for (int i = 0; i < bodies.Num(); i++ ) {
		const Body20& _body = bodies[i];
		if ( _body.tracked) {
			isTracked = true;
			float xDist = fabs(_body.joints[SpineMid].cameraSpacePos.X);
			if (  xDist < minDistCenter ) {
				minDistCenter = xDist;
				centerBody = i;
			}
		}
	}
	body = (EKinectBody)centerBody;
}

void  UKinectDeviceComponent::GetNearestBody(EKinectBody& body, bool& isTracked) {
	
	int nearestBody = -1;
	float minDist = 50;
	isTracked = false;
	for (int i = 0; i < bodies.Num(); i++) {
		const Body20& _body = bodies[i];
		if (_body.tracked) {
			isTracked = true;
			if (_body.zDepth < minDist) {
				minDist = _body.zDepth;
				nearestBody = i;
			}
		}
	}
	body = (EKinectBody)nearestBody;
}

void  UKinectDeviceComponent::GetTrackedBodies(TArray<EKinectBody>& _bodies) {
	_bodies.Empty();

	for (int i = 0; i < bodies.Num(); i++) {
		if (bodies[i].tracked) {
			_bodies.Add((EKinectBody)i);
		}
	}
}

FVector UKinectDeviceComponent::GetJointAbsolutePosition(EKinectJointType joinType, EKinectBody bodyNumber) {
	int b = (int)bodyNumber;
	int j = (int)joinType;
	if (b >= 0 && b < bodies.Num() && j >= 0 && j < 25) {
		const Body20& body = bodies[(int)bodyNumber];
		return KinectDeviceUtil::kinectToUnreal(body.joints[(int)joinType].cameraSpacePos);
	}
	return FVector();
}

FRotator  UKinectDeviceComponent::GetJointOrientation(EKinectJointType joinType, EKinectBody bodyNumber) {
	int b = (int)bodyNumber;
	int j = (int)joinType;
	if (b >= 0 && b < bodies.Num() && j >= 0 && j < 25) {
		const Body20& body = bodies[(int)bodyNumber];
		return UKismetMathLibrary::ComposeRotators(FRotator(0, 180, 90), FRotator(body.joints[(int)joinType].orientation));
	}
	return FRotator();
}



FVector UKinectDeviceComponent::GetJointRelativePosition(EKinectJointType startJoint, EKinectBody startJointBodyNumber, EKinectJointType endJoint, EKinectBody endJointBodyNumber) {
	int b1 = (int)startJointBodyNumber;
	int b2 = (int)endJointBodyNumber;
	int j = (int)startJoint;
	int k = (int)endJoint;
	if (b1 >= 0 && b1 < bodies.Num() && b2 >= 0 && b2 < bodies.Num() && j >= 0 && j < 25 && k >= 0 && k < 25) {
		const Body20& body1 = bodies[b1];
		const Body20& body2 = bodies[b2];
		return KinectDeviceUtil::kinectToUnreal(body2.joints[(int)endJoint].cameraSpacePos) - 
			KinectDeviceUtil::kinectToUnreal(body1.joints[(int)startJoint].cameraSpacePos);
	}
	return FVector(0.0f);
}


float UKinectDeviceComponent::GetDistanceBetweenJoints(EKinectJointType startJoint, EKinectJointType endJoint, EKinectBody bodyNumber) {

	int b = (int)bodyNumber;
	int j = (int)startJoint;
	int k = (int)endJoint;
	if (b >= 0 && b < bodies.Num() && j >= 0 && j < 25 && k >= 0 && k < 25) {
		const Body20& body = bodies[(int)bodyNumber];
		return FVector::Distance(KinectDeviceUtil::kinectToUnreal(body.joints[(int)startJoint].cameraSpacePos),
			KinectDeviceUtil::kinectToUnreal(body.joints[(int)endJoint].cameraSpacePos));
	}
	
	return 0.0f;
}

EKinectJointType  UKinectDeviceComponent::GetMainJoint() {
	return (EKinectJointType)Kinect20Chain::getInstance()->getMainJoint();
}

const TArray<EKinectJointType>& UKinectDeviceComponent::GetChildJointsForParent(EKinectJointType parent) {
	return bones[parent];
}

FRotator UKinectDeviceComponent::MakeRotator(const FVector& from, const FVector& to) {
	FQuat quat = KinectDeviceUtil::makeRotate(from, to);
	return FRotator(quat);
}

void UKinectDeviceComponent::SetCameraFrameIsUpdating(EKinectCameraType kinectCameraType, const bool isUpdating) {
	if ( kinectCameraType == EKinectCameraType::RawColorCamera) {
		KinectDevice::GetInstance()->setIsUpdatingColorCamera(isUpdating);
	}
}

UTexture2D* UKinectDeviceComponent::GetCameraFrame(EKinectCameraType kinectCameraType) {

	if (kinectCameraType == EKinectCameraType::RawColorCamera) {
		return KinectDevice::GetInstance()->getColorTexture();
	}

	return NULL;
}