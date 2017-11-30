// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KinectDTO.h"

/**
 * 
 */
class KinectDeviceUtil
{
public:
	KinectDeviceUtil();
	~KinectDeviceUtil();

	static FVector kinectToUnreal(const FVector& vector);
	static FVector unrealToKinect(const FVector& vector);
	static FQuat   makeRotate(const FVector& from, const FVector& to);
	static FQuat   makeRotate(float angle, const FVector& axis);
	static FQuat   makeRotate(float angle, float x, float y, float z);
	static Body20 mirrorBody(const Body20& body);

protected: 
	static void swapJoints(Body20& body, JointId left, JointId right);
};
