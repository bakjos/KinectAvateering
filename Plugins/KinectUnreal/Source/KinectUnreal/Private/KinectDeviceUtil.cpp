// Fill out your copyright notice in the Description page of Project Settings.

#include "KinectDeviceUtil.h"

KinectDeviceUtil::KinectDeviceUtil()
{
}

KinectDeviceUtil::~KinectDeviceUtil()
{
}

FVector KinectDeviceUtil::kinectToUnreal(const FVector& vector) {
	return FVector(vector.X * 100, vector.Z * 100, vector.Y * 100);
}

FVector KinectDeviceUtil::unrealToKinect(const FVector& vector) {
	return FVector(vector.X / 100, vector.Z / 100, vector.Y / 100);
}

FQuat   KinectDeviceUtil::makeRotate(const FVector& from, const FVector& to) {
	FVector sourceVector = from;
	FVector targetVector = to;

	float fromLen2 = from.SizeSquared();
	float fromLen;

	// normalize only when necessary, epsilon test
	if ((fromLen2 < 1.0 - 1e-7) || (fromLen2 > 1.0 + 1e-7)) {
		fromLen = sqrt(fromLen2);
		sourceVector /= fromLen;
	}
	else fromLen = 1.0;

	float toLen2 = to.SizeSquared();
	// normalize only when necessary, epsilon test
	if ((toLen2 < 1.0 - 1e-7) || (toLen2 > 1.0 + 1e-7)) {
		float toLen;
		// re-use fromLen for case of mapping 2 vectors of the same length
		if ((toLen2 > fromLen2 - 1e-7) && (toLen2 < fromLen2 + 1e-7)) {
			toLen = fromLen;
		}
		else toLen = sqrt(toLen2);
		targetVector /= toLen;
	}

	// Now let's get into the real stuff
	// Use "dot product plus one" as test as it can be re-used later on
	double dotProdPlus1 = 1.0 + (sourceVector | targetVector);
	FQuat _v;

	// Check for degenerate case of full u-turn. Use epsilon for detection
	if (dotProdPlus1 < 1e-7) {

		// Get an orthogonal vector of the given vector
		// in a plane with maximum vector coordinates.
		// Then use it as quaternion axis with pi angle
		// Trick is to realize one value at least is >0.6 for a normalized vector.
		if (fabs(sourceVector.X) < 0.6) {
			const double norm = sqrt(1.0 - sourceVector.X * sourceVector.X);
			_v.X = 0.0;
			_v.Y = sourceVector.Z / norm;
			_v.Z = -sourceVector.Y / norm;
			_v.W = 0.0;
		}
		else if (fabs(sourceVector.Y) < 0.6) {
			const double norm = sqrt(1.0 - sourceVector.Y * sourceVector.Y);
			_v.X = -sourceVector.Z / norm;
			_v.Y = 0.0;
			_v.Z = sourceVector.X / norm;
			_v.W = 0.0;
		}
		else {
			const double norm = sqrt(1.0 - sourceVector.Z * sourceVector.Z);
			_v.X = sourceVector.Y / norm;
			_v.Y = -sourceVector.X / norm;
			_v.Z = 0.0;
			_v.W = 0.0;
		}
	}

	else {
		// Find the shortest angle quaternion that transforms normalized vectors
		// into one other. Formula is still valid when vectors are colinear
		const double s = sqrt(0.5 * dotProdPlus1);
		const FVector tmp = (sourceVector^targetVector) / (2.0 * s);
		_v.X = tmp.X;
		_v.Y = tmp.Y;
		_v.Z = tmp.Z;
		_v.W = s;
	}

	return _v;
}

FQuat   KinectDeviceUtil::makeRotate(float angle, const FVector& axis) {
	return makeRotate(angle, axis.X, axis.Y, axis.Z);
}
FQuat   KinectDeviceUtil::makeRotate(float angle, float x, float y, float z) {


	FQuat _v;

	angle = FMath::DegreesToRadians(angle);

	const float epsilon = 0.0000001f;

	float length = sqrtf(x * x + y * y + z * z);
	if (length < epsilon) {
		// ~zero length axis, so reset rotation to zero.
		return _v;
	}

	float inversenorm = 1.0f / length;
	float coshalfangle = cosf(0.5f * angle);
	float sinhalfangle = sinf(0.5f * angle);

	_v.X = x * sinhalfangle * inversenorm;
	_v.Y = y * sinhalfangle * inversenorm;
	_v.Z = z * sinhalfangle * inversenorm;
	_v.W = coshalfangle;

	return _v;
}

Body20 KinectDeviceUtil::mirrorBody(const Body20& body)
{
	Body20 mirroredBody = body;
	swapJoints(mirroredBody, ShoulderLeft, ShoulderRight);
	swapJoints(mirroredBody, ElbowLeft, ElbowRight);
	swapJoints(mirroredBody, WristLeft, WristRight);
	swapJoints(mirroredBody, HandLeft, HandRight);
	swapJoints(mirroredBody, HandTipLeft, HandTipRight);
	swapJoints(mirroredBody, HipLeft, HipRight);
	swapJoints(mirroredBody, KneeLeft, KneeRight);
	swapJoints(mirroredBody, AnkleLeft, AnkleRight);
	swapJoints(mirroredBody, FootLeft, FootRight);
	swapJoints(mirroredBody, ThumbLeft, ThumbRight);

	FQuat rotation = makeRotate(180, 0, 1, 0);
	FVector center = mirroredBody.joints[SpineBase].cameraSpacePos;

	for ( int i = 0; i < 25; i++) {
		mirroredBody.joints[i].cameraSpacePos = rotation.RotateVector(mirroredBody.joints[i].cameraSpacePos - center);
		mirroredBody.joints[i].orientation = mirroredBody.joints[i].orientation*rotation;
		mirroredBody.joints[i].cameraSpacePos += center;
	}


	return mirroredBody;
}

void KinectDeviceUtil::swapJoints(Body20& body, JointId left, JointId right)
{
	Joint20& jL = body.joints[left];
	Joint20& jR = body.joints[right];

	FVector tmpPos = jL.cameraSpacePos;
	jL.cameraSpacePos = jR.cameraSpacePos;
	jR.cameraSpacePos = tmpPos;

	tmpPos = jL.depthSpacePos;
	jL.depthSpacePos = jR.depthSpacePos;
	jR.depthSpacePos = tmpPos;
	
	KinectTrackingState	tmpTrackingState = jL.trackingState;
	jL.trackingState = jR.trackingState;
	jR.trackingState = tmpTrackingState;

	FQuat tmpOrientation = jL.orientation;
	jL.orientation = jR.orientation;
	jR.orientation = tmpOrientation;
	
	float	tmpDepth = jL.depth;
	jL.depth = jR.depth;
	jR.depth = tmpDepth;
	
}
