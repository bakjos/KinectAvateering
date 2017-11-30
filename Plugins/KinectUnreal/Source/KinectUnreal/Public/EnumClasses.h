// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

UENUM(BluePrintType)
enum class EKinectJointType : uint8 {
	SpineBase = 0,
	SpineMid = 1,
	Neck = 2,
	Head = 3,
	ShoulderLeft = 4,
	ElbowLeft = 5,
	WristLeft = 6,
	HandLeft = 7,
	ShoulderRight = 8,
	ElbowRight = 9,
	WristRight = 10,
	HandRight = 11,
	HipLeft = 12,
	KneeLeft = 13,
	AnkleLeft = 14,
	FootLeft = 15,
	HipRight = 16,
	KneeRight = 17,
	AnkleRight = 18,
	FootRight = 19,
	SpineShoulder = 20,
	HandTipLeft = 21,
	ThumbLeft = 22,
	HandTipRight = 23,
	ThumbRight = 24
};

UENUM(BluePrintType)
enum class EKinectBody : uint8 {
	Body0 = 0,
	Body1 = 1,
	Body2 = 2,
	Body3 = 3,
	Body4 = 4,
	Body5 = 5,
};

UENUM(BluePrintType)
enum class EKinectCameraType : uint8 {
	RawColorCamera = 0,
	DepthCamera = 1,
	IR = 2
};
