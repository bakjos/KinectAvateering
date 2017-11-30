// Fill out your copyright notice in the Description page of Project Settings.

#include "KinectDTO.h"

KinectDTO::KinectDTO()
{
	depthRawData = new DepthRawData20[cDepthWidth*cDepthHeight];
	clear();
}



void KinectDTO::clear() {
	frameTime = 0;
	memset(depthRawData, 0, sizeof(DepthRawData20)*cDepthWidth*cDepthHeight);
	memset(bodies, 0, sizeof(Body20)*cTotalBodies);
}

KinectDTO::~KinectDTO()
{
	delete depthRawData;
	depthRawData = NULL;
}

DepthRawData20* KinectDTO::getDepthRawData()
{
	return depthRawData;
}

DepthRawData20& KinectDTO::getDepthRawData(int x, int y)
{
	return depthRawData[x + y * cDepthWidth];
}

INT64 KinectDTO::getFrameTime() {
	return frameTime;
}

void KinectDTO::setFrameTime(INT64 time) {
	frameTime = time;
}

INT64 KinectDTO::getBodyFrameTime() {
	return bodyFrameTime;
}

void KinectDTO::setBodyFrameTime(INT64 time) {
	bodyFrameTime = time;
}

Body20& KinectDTO::getBody(int idx)
{
	return bodies[idx];
}
HDFace20&	KinectDTO::getFace(int idx) {
	return faces[idx];
}