// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnumClasses.h"
#include "Components/ActorComponent.h"
#include "KinectDTO.h"
#include "KinectDeviceComponent.generated.h"

UCLASS( ClassGroup=(Kinect), meta=(BlueprintSpawnableComponent) )
class KINECTUNREAL_API UKinectDeviceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UKinectDeviceComponent();

	UFUNCTION(Category = "Kinect|BodyTracking", BlueprintCallable)
	void GetCenteredBody(EKinectBody& body, bool& isTracked);

	UFUNCTION(Category = "Kinect|BodyTracking", BlueprintCallable)
	void GetNearestBody(EKinectBody& body, bool& isTracked);

	UFUNCTION(Category = "Kinect|BodyTracking", BlueprintCallable)
	void GetTrackedBodies(TArray<EKinectBody>& body);

	UFUNCTION(Category = "Kinect|Joints", BlueprintCallable, meta = (Tooltip="Returns an absolute position FVector for the specified joint, in unreal coordinate system"))
	FVector GetJointAbsolutePosition(EKinectJointType joinType, EKinectBody bodyNumber);

	UFUNCTION(Category = "Kinect|Joints", BlueprintCallable)
	FRotator GetJointOrientation(EKinectJointType joinType, EKinectBody bodyNumber);

	/*Returns the vector between two given points in unreal coordinate system*/
	UFUNCTION(Category = "Kinect|Joints", BlueprintCallable)
	FVector GetJointRelativePosition(EKinectJointType startJoint, EKinectBody startJointBodyNumber, EKinectJointType endJoint, EKinectBody endJointBodyNumber);

	/*Returns the distance between two joints in unreal coordinate system*/
	UFUNCTION(Category = "Kinect|Joints", BlueprintCallable)
	float GetDistanceBetweenJoints(EKinectJointType startJoint, EKinectJointType endJoint, EKinectBody bodyNumber);

	UFUNCTION(Category = "Kinect|Joints", BlueprintCallable)
	EKinectJointType GetMainJoint();

	UFUNCTION(Category = "Kinect|Joints", BlueprintCallable)
	const TArray<EKinectJointType>& GetChildJointsForParent(EKinectJointType parent);

	UFUNCTION(Category = "Kinect|Camera Stream", BlueprintCallable)
	void SetCameraFrameIsUpdating(EKinectCameraType kinectCameraType, const bool isUpdating );

	UFUNCTION(Category = "Kinect|Camera Stream", BlueprintCallable)
	UTexture2D* GetCameraFrame(EKinectCameraType kinectCameraType);

	


	UFUNCTION(Category = "Kinect|Utilities", BlueprintPure)
	static FRotator MakeRotator(const FVector& from, const FVector& to);

	UPROPERTY(EditAnywhere, Category = Skeletons)
	bool mirrorSkeletons;


protected:
	// Called when the game starts
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	TMap<EKinectJointType, TArray<EKinectJointType>> bones;
	
	TArray<Body20> bodies;


		
	
};
