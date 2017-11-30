// Fill out your copyright notice in the Description page of Project Settings.

#include "KinematicChain.h"

KinematicChain::KinematicChain()
{
}

KinematicChain::~KinematicChain()
{
}

JointId KinematicChain::getMainJoint() const {
	return mainJoint;
}

const TMap<JointId, TArray<JointId>>&  KinematicChain::getBones() const {
	return bones;
}

bool KinematicChain::getParentJoint(JointId child, JointId& parent) const {
	if ( parents.Contains(child)) {
		parent = parents[child];
		return true;
	}
	return false;
}

void KinematicChain::generateParents() {
	TArray<JointId> keys;
	bones.GetKeys(keys);
	for ( JointId parent: keys) {
		const TArray<JointId>& childs = bones[parent];
		for (JointId child: childs) {
			if ( !parents.Contains(child)) {
				parents.Add(child, parent);
			} else {
				UE_LOG(LogTemp, Warning, TEXT("The node %d has more than one parent"), (int)child);
			}
		}
	}
}


Kinect20Chain* Kinect20Chain::instance = nullptr;


Kinect20Chain* Kinect20Chain::getInstance() {
	if ( instance == nullptr) {
		instance = new Kinect20Chain();
	}
	return instance;
}
void Kinect20Chain::releaseInstance() {
	if (instance) {
		delete instance;
		instance = nullptr;
	}

}

Kinect20Chain::Kinect20Chain () {
	mainJoint = SpineBase;
	
	for ( int i = 0; i < 25; i++) {
		bones.Add((JointId)i, TArray<JointId>());
	}
	

	
	bones[SpineShoulder].Add(Neck);
	bones[SpineShoulder].Add(ShoulderRight);
	bones[SpineShoulder].Add(ShoulderLeft);
	bones[SpineMid].Add(SpineShoulder);
	bones[Neck].Add(Head);
	bones[ShoulderLeft].Add(ElbowLeft);
	bones[ShoulderRight].Add(ElbowRight);
	bones[ElbowLeft].Add(WristLeft);
	bones[ElbowRight].Add(WristRight);
	bones[WristLeft].Add(HandLeft);
	bones[WristRight].Add(HandRight);
	bones[HandLeft].Add(HandTipLeft);
	bones[HandLeft].Add(ThumbLeft);
	bones[HandRight].Add(HandTipRight);
	bones[HandRight].Add(ThumbRight);
	bones[SpineBase].Add(SpineMid);
	bones[SpineBase].Add(HipLeft);
	bones[SpineBase].Add(HipRight);
	bones[HipLeft].Add(KneeLeft);
	bones[HipRight].Add(KneeRight);
	bones[KneeLeft].Add(AnkleLeft);
	bones[KneeRight].Add(AnkleRight);
	bones[AnkleLeft].Add(FootLeft);
	bones[AnkleRight].Add(FootRight);

	generateParents();

}

Kinect20Chain::~Kinect20Chain() {
	
}
