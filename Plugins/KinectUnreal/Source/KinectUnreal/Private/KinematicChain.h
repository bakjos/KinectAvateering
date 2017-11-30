// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KinectDTO.h"

/**
 * 
 */
class KinematicChain
{
public:
	KinematicChain();
	~KinematicChain();

	JointId getMainJoint() const;
	const TMap<JointId, TArray<JointId>>& getBones() const;
	bool getParentJoint(JointId child, JointId& parent) const;

protected:
	void generateParents();
	JointId mainJoint;
	TMap<JointId, TArray<JointId>> bones;
	TMap<JointId, JointId> parents;
};

class Kinect20Chain: public KinematicChain {
public:
	static Kinect20Chain* getInstance();
	static void releaseInstance();

private:
	Kinect20Chain();
	~Kinect20Chain();
	static Kinect20Chain* instance;
	
};
