// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <CoreUObject.h>
#include <Vector.h>
#include <Kinect.h>
#include <kinect.Face.h>
#include <Runtime/Engine/Classes/Engine/Texture2D.h>
#include "KinectDTO.h"

template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease) {
	if (pInterfaceToRelease != NULL) {
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

/**
 * 
 */
class KinectDevice : public FRunnable
{
public:
	/* Begin FRunnable interface. */
	virtual bool	Init()	override;
	virtual uint32	Run()	override;
	virtual void	Stop()	override;
	/* End FRunnable interface */

	/* Begin Singleton Interface*/
	static KinectDevice* const GetInstance();
	static void DeleteInstance();
	/* End Singleton Interface*/

	DepthSpacePoint*		getDepthCoordinates() const;
	CameraIntrinsics		getCameraIntrinsics() const;
	TArray<Body20>			getBodies() const;
	Body20					getBody(int idx) const;
	void					setIsUpdatingColorCamera(bool value);

	UTexture2D*				getColorTexture();

private:
	KinectDevice();
	~KinectDevice();


	void loadBody(Body20& body, IBody* pBody, UINT16* pDepthBuffer);
	void copyBufferToTexture(TWeakObjectPtr<UTexture2D> Texture, unsigned char * pData, int width, int height, int numColors);
	



	IKinectSensor*				m_pKinectSensor;
	ICoordinateMapper*			m_pCoordinateMapper;
	ColorSpacePoint*			m_pColorCoordinates;
	ColorSpacePoint*			m_pColorCoordinatesAux;
	DepthSpacePoint*			m_pDepthCoordinates;

	// Frame readers
	//IMultiSourceFrameReader*	m_pMultiSourceFrameReader;
	IDepthFrameReader*			m_pDepthFrameReader;
	IColorFrameReader*			m_pColorFrameReader;
	IBodyFrameReader*			m_pBodyFrameReader;
	IBodyIndexFrameReader*		m_pBodyIndexFrameReader;
	IInfraredFrameReader*		m_pInfraredFrameReader;

	//Face Tracking
	bool							extract2DFaces;
	IFaceFrameSource*				m_pFaceFrameSources[KinectDTO::cTotalBodies]; // Face sources
	IFaceFrameReader*				m_pfaceFrameReaders[KinectDTO::cTotalBodies]; // Face readers
	TArray<UINT32>					faceTriangles;
	int								totalFaceVertices;


	//HD Face Tracking
	IHighDefinitionFaceFrameReader*	m_pFaceFrameReader[KinectDTO::cTotalBodies];
	IHighDefinitionFaceFrameSource* m_pHDFaceFrameSource[KinectDTO::cTotalBodies];
	IFaceAlignment*					m_pFaceAlignment[KinectDTO::cTotalBodies];
	IFaceModel*						m_pFaceModel[KinectDTO::cTotalBodies];
	IFaceModelBuilder*				m_pFaceModelBuilder[KinectDTO::cTotalBodies];
	TArray<CameraSpacePoint>		m_pFaceVertices[KinectDTO::cTotalBodies];

	// Frame data

	RGBQUAD*                m_pColorRGBX;
	BYTE*					infraRedBuffer;
	UINT16*					m_pInfraRedBuffer;
	//TODO: Remove this
	UINT16*					m_pDethRawBuffer;


	TWeakObjectPtr<UTexture2D> 				cameraTexture;
	TWeakObjectPtr<UTexture2D> 				IRTexture;

	KinectDTO*			kinectData;
	DepthRawData20*		depthRawData;
	DepthRawData20		tempDepthData[KinectDTO::cDepthWidth * KinectDTO::cDepthHeight];


	/* Frunnable Helper. */
	bool						bRunning;
	static KinectDevice*		Instance;
	FRunnableThread*			Thread;
	FRWLock						frwLock;


	CameraIntrinsics		cameraIntrinsics;
	bool					paused;
	FString					deviceId;
	WAITABLE_HANDLE			waitableHandle;
	BOOLEAN					lastAvailable;

	bool				extractColorData;
	bool				extractIRData;
	bool				extractSkeletonData;
	bool				extractDepthCoordinates;
	bool				extractFaces;
	bool				extractDepthData;
	bool				isUpdatingColorFeed;

};
