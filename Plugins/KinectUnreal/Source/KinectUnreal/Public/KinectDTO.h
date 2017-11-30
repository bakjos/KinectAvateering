// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#pragma pack(1)
struct DepthRawData20
{
	unsigned char  userIndex;
	unsigned short depth;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned short reserved;
	float    xCoord;
	float	 yCoord;
};
#pragma pack()

enum JointId
{
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
	ThumbRight = 24,
};

enum HandTrackingState {
	Hand_Unknown = 0,
	Hand_NotTracked = 1,
	Hand_Open = 2,
	Hand_Closed = 3,
	Hand_Lasso = 4
};

enum KinectTrackingState {
	NotTracked = 0,
	Inferred = 1,
	Tracked = 2
};

enum DetectionState
{
	StateUnknown = 0,
	StateNo = 1,
	StateMaybe = 2,
	StateYes = 3
};

enum KinectTrackingConfidence
{
	Low = 0,
	High = 1
};



struct Joint20
{
	JointId		joinType;
	FVector		cameraSpacePos;
	FVector		depthSpacePos;
	FQuat		orientation;
	KinectTrackingState	   trackingState;
	double		depth;
};

struct Body20 {

	static const int		cTotalJoints = 25;
	static const int		cActivityCount = 5;
	static const int		cAppearanceCount = 1;
	static const int		cExpressionCount = 2;

	bool tracked;
	bool restricted;
	UINT64  trackingId;
	Joint20 joints[cTotalJoints];
	HandTrackingState leftHandState;
	HandTrackingState rightHandState;
	KinectTrackingState	   trackingState;
	DetectionState		engaged;
	FVector2D			lean;
	DWORD				clippedEdges;
	KinectTrackingConfidence	leftHandConfidence;
	KinectTrackingConfidence	rightHandConfidence;


	DetectionState		activities[cActivityCount];
	DetectionState		appearance[cAppearanceCount];
	DetectionState		expressions[cExpressionCount];

	float				zDepth;
};

struct  HDFace20
{
	bool tracked;
	UINT64 trackingId;
	FQuat orientation;
	FVector	headPivot;
	float	height;
	FIntRect rect;
	FVector	    points[1500];
};


class KinectDTO {
public:

	static const int        cDepthWidth = 512;
	static const int        cDepthHeight = 424;
	static const int        cColorWidth = 1920;
	static const int        cColorHeight = 1080;
	static const int		cTotalBodies = 6;
	static const int		cTotalColorPixels = cColorWidth*cDepthHeight;
	static const int		cTotalDepthPixels = cDepthWidth*cDepthHeight;

	KinectDTO();
	virtual ~KinectDTO();

	DepthRawData20*		getDepthRawData();
	DepthRawData20&		getDepthRawData(int x, int y);
	void				clear();
	INT64				getFrameTime();
	void				setFrameTime(INT64 time);

	INT64				getBodyFrameTime();
	void				setBodyFrameTime(INT64 time);

	Body20&				getBody(int idx);
	HDFace20&				getFace(int idx);

private:
	DepthRawData20*		depthRawData;
	INT64				frameTime;
	INT64				bodyFrameTime;
	Body20				bodies[cTotalBodies];
	HDFace20			faces[cTotalBodies];



};