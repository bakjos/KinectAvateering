// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "KinectUnreal.h"
#include "KinectDevice.h"
#include "KinematicChain.h"

#define LOCTEXT_NAMESPACE "FKinectUnrealModule"

void FKinectUnrealModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	//UnrealKinematicChain::getInstance();
}

void FKinectUnrealModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	KinectDevice::DeleteInstance();
	Kinect20Chain::releaseInstance();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FKinectUnrealModule, KinectUnreal)