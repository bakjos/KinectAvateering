// Fill out your copyright notice in the Description page of Project Settings.

#include "KinectDevice.h"
#include "KinematicChain.h"
#include <Runtime/Engine/Classes/Engine/Texture2D.h>
#ifdef UpdateResource
#undef  UpdateResource
#endif
 
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

KinectDevice::KinectDevice()
{
	m_pKinectSensor = NULL;
	m_pCoordinateMapper = NULL;
	m_pColorCoordinates = NULL;
	m_pColorCoordinatesAux = NULL;
	m_pDepthCoordinates = NULL;
	m_pColorRGBX = NULL;
	infraRedBuffer = NULL;
	m_pInfraRedBuffer = NULL;
	kinectData = NULL;
	//m_pMultiSourceFrameReader = NULL;
	m_pDepthFrameReader = NULL;
	m_pColorFrameReader = NULL;
	m_pBodyFrameReader = NULL;
	m_pInfraredFrameReader = NULL;
	m_pBodyIndexFrameReader = NULL;

	waitableHandle = 0;
	paused = false;
	lastAvailable = FALSE;

	extractColorData = true;
	extractIRData = false;
	extractSkeletonData = true;
	extractDepthCoordinates = false;
	extractFaces = false;
	extractDepthData = true;
	
	memset(&cameraIntrinsics, sizeof(CameraIntrinsics), 0);
	cameraIntrinsics.FocalLengthX = cameraIntrinsics.FocalLengthY = -1;
	

	m_pDethRawBuffer = new UINT16[KinectDTO::cDepthWidth*KinectDTO::cDepthHeight];

	
	for (int i = 0; i < KinectDTO::cTotalBodies; i++) {
		m_pFaceFrameReader[i] = NULL;
		m_pHDFaceFrameSource[i] = NULL;
		m_pFaceAlignment[i] = NULL;
		m_pFaceModel[i] = NULL;
		m_pFaceModelBuilder[i] = NULL;
	}

	totalFaceVertices = 0;
	bRunning = false;
	isUpdatingColorFeed = false;
}

KinectDevice::~KinectDevice()
{
	Stop();
	
	if ( Thread != nullptr )
		Thread->WaitForCompletion();

	if (m_pColorRGBX) {
		delete[] m_pColorRGBX;
		m_pColorRGBX = NULL;
	}


	if (m_pColorCoordinates) {
		delete[] m_pColorCoordinates;
		m_pColorCoordinates = NULL;
	}

	if (m_pColorCoordinatesAux) {
		delete[] m_pColorCoordinatesAux;
		m_pColorCoordinatesAux = NULL;
	}

	if (m_pDepthCoordinates) {
		delete[] m_pDepthCoordinates;
		m_pDepthCoordinates = NULL;
	}

	if (infraRedBuffer) {
		delete infraRedBuffer;
		infraRedBuffer = NULL;
	}

	if (m_pInfraRedBuffer) {
		delete m_pInfraRedBuffer;
		m_pInfraRedBuffer = NULL;
	}

	if (kinectData)
		delete kinectData;

	SafeRelease(m_pDepthFrameReader);
	SafeRelease(m_pColorFrameReader);
	SafeRelease(m_pBodyFrameReader);
	SafeRelease(m_pInfraredFrameReader);
	SafeRelease(m_pBodyIndexFrameReader);


	for (int i = 0; i < KinectDTO::cTotalBodies; i++) {
		SafeRelease(m_pFaceFrameReader[i]);
		SafeRelease(m_pHDFaceFrameSource[i]);
		SafeRelease(m_pFaceAlignment[i]);
		SafeRelease(m_pFaceModelBuilder[i]);
		SafeRelease(m_pFaceModel[i]);
	}


	// done with coordinate mapper
	SafeRelease(m_pCoordinateMapper);

	// close the Kinect Sensor
	if (m_pKinectSensor) {
		UE_LOG(LogTemp, Display, TEXT("Kinect sensor closed"));
		m_pKinectSensor->Close();
	}

	SafeRelease(m_pKinectSensor);
}

//Singleton access
KinectDevice* KinectDevice::Instance = nullptr;

KinectDevice* const KinectDevice::GetInstance()
{
	if (Instance == nullptr)
	{
		//TODO (OS): Make this a little more RAII
		Instance = new KinectDevice();
		Instance->Thread = FRunnableThread::Create(Instance, TEXT("KinectThread"), 0, EThreadPriority::TPri_Normal);
	}
	return Instance;
}

void KinectDevice::DeleteInstance() {
	if ( Instance != nullptr) {
		delete Instance;
		Instance = nullptr;
	}
}

bool KinectDevice::Init() {

	HRESULT hr;

	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	if (FAILED(hr)) {
		UE_LOG(LogTemp, Error, TEXT("Device Initialization failed"));
		return false;
	}

	if (m_pKinectSensor) {
		hr = m_pKinectSensor->Open();

		if (SUCCEEDED(hr)) {

			m_pKinectSensor->SubscribeIsAvailableChanged(&waitableHandle);
			hr = m_pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);

			IDepthFrameSource* pDepthFrameSource = NULL;
			hr = m_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource);
			if (SUCCEEDED(hr)) {
				hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
			}
			SafeRelease(pDepthFrameSource);

			
			if (extractColorData) {
				// Initialize the Kinect and get the color reader
				IColorFrameSource* pColorFrameSource = NULL;
				hr = m_pKinectSensor->get_ColorFrameSource(&pColorFrameSource);
				if (SUCCEEDED(hr)) {
					hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);
				}
				SafeRelease(pColorFrameSource);

				m_pColorRGBX = new RGBQUAD[KinectDTO::cColorWidth * KinectDTO::cColorHeight];
				m_pColorCoordinates = new ColorSpacePoint[KinectDTO::cDepthWidth * KinectDTO::cDepthHeight];

				if (extractDepthCoordinates)
					m_pDepthCoordinates = new DepthSpacePoint[KinectDTO::cColorWidth * KinectDTO::cColorHeight];

				cameraTexture = UTexture2D::CreateTransient(KinectDTO::cColorWidth, KinectDTO::cColorHeight);

				ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
					UpdateTextureResource,
					UTexture2D*, texture, cameraTexture.Get(),
					{
						texture->UpdateResource();
						/*if (texture->Resource)
						{
							texture->BeginInitResource(texture->Resource);
						}*/

					});



				
			}

			if (extractIRData) {
				// Initialize the Kinect and get the infrared reader
				IInfraredFrameSource* pInfraredFrameSource = NULL;
				hr = m_pKinectSensor->get_InfraredFrameSource(&pInfraredFrameSource);

				if (SUCCEEDED(hr)) {
					hr = pInfraredFrameSource->OpenReader(&m_pInfraredFrameReader);
				}

				SafeRelease(pInfraredFrameSource);

				infraRedBuffer = new BYTE[KinectDTO::cDepthWidth*KinectDTO::cDepthHeight];
				memset(infraRedBuffer, 0, KinectDTO::cDepthWidth*KinectDTO::cDepthHeight);
				m_pInfraRedBuffer = new UINT16[KinectDTO::cDepthWidth*KinectDTO::cDepthHeight];
				memset(m_pInfraRedBuffer, 0, KinectDTO::cDepthWidth*KinectDTO::cDepthHeight * sizeof(UINT16));
				
				IRTexture = UTexture2D::CreateTransient(KinectDTO::cDepthWidth, KinectDTO::cDepthWidth);
			}

			if (extractSkeletonData) {
				// Initialize the Kinect and get coordinate mapper and the body reader
				IBodyFrameSource* pBodyFrameSource = NULL;
				hr = m_pKinectSensor->get_BodyFrameSource(&pBodyFrameSource);

				if (SUCCEEDED(hr)) {
					hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
				}
				SafeRelease(pBodyFrameSource);

				IBodyIndexFrameSource* pBodyIndexFrameSource = NULL;
				hr = m_pKinectSensor->get_BodyIndexFrameSource(&pBodyIndexFrameSource);

				if (SUCCEEDED(hr)) {
					hr = pBodyIndexFrameSource->OpenReader(&m_pBodyIndexFrameReader);
				}
				SafeRelease(pBodyIndexFrameSource);
			}

			if (extractSkeletonData || extractDepthData) {
				if ( kinectData != nullptr) {
					delete kinectData;
				}

				kinectData = new KinectDTO();
			}

			if (extractFaces) {
				//char currentDir[2048];


				UINT32 vertexCount = 0;
				GetFaceModelVertexCount(&vertexCount);
				totalFaceVertices = vertexCount;
				UINT32 triangleCount = 0;
				if (SUCCEEDED(GetFaceModelTriangleCount(&triangleCount))) {
					
					faceTriangles.SetNumZeroed(triangleCount * 3);
					hr = GetFaceModelTriangles(faceTriangles.Num(), &faceTriangles[0]);
				}

				for (int count = 0; count < KinectDTO::cTotalBodies; count++) {
					hr = CreateHighDefinitionFaceFrameSource(m_pKinectSensor, &m_pHDFaceFrameSource[count]);
					if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
						char strDLLPath[_MAX_PATH];
						GetModuleFileNameA((HINSTANCE)&__ImageBase, strDLLPath, _MAX_PATH);
						
						break;
					}
					if (FAILED(hr)) {
						UE_LOG(LogTemp, Error, TEXT("Error : CreateHighDefinitionFaceFrameSource()"));
						extractFaces = false;
						hr = S_OK;
						break;
					}
					else {
						hr = m_pHDFaceFrameSource[count]->OpenReader(&m_pFaceFrameReader[count]);

						if (FAILED(hr)) {
							UE_LOG(LogTemp, Error, TEXT("Error : Open Face reader()"));
							extractFaces = false;
							hr = S_OK;
							break;
						}
						else {
							hr = CreateFaceAlignment(&m_pFaceAlignment[count]);
							if (FAILED(hr)) {
								UE_LOG(LogTemp, Error, TEXT("Error : CreateFaceAlignment()"));
								extractFaces = false;
								hr = S_OK;
								break;
							}
							else {
								hr = m_pHDFaceFrameSource[count]->OpenModelBuilder(FaceModelBuilderAttributes_None, &m_pFaceModelBuilder[count]);
								if (FAILED(hr)) {
									UE_LOG(LogTemp, Error, TEXT("Error : Opening model builder"));
									extractFaces = false;
									hr = S_OK;
									break;
								}
								else {
									float deformations[FaceShapeDeformations::FaceShapeDeformations_Count];
									hr = CreateFaceModel(1.0f, FaceShapeDeformations::FaceShapeDeformations_Count, &deformations[0], &m_pFaceModel[count]);
									if (FAILED(hr)) {
										UE_LOG(LogTemp, Error, TEXT("Error creating face deformation model"));
										extractFaces = false;
										hr = S_OK;
										break;
									}
									else {

										if (SUCCEEDED(hr) && vertexCount > 0) {
											m_pFaceVertices[count].SetNumZeroed(vertexCount);
										}


									}
								}

							}
						}
					}
				}

			}

		}

		if (!m_pKinectSensor || FAILED(hr)) {
			UE_LOG(LogTemp, Error, TEXT("No ready Kinect found!"));
			
		}

		
	}


	bRunning = true;
	//HACK (OS): should return status
	return bRunning;
}

/* Runs a looping thread for Kinect	*/
uint32 KinectDevice::Run()
{
	UE_LOG(LogTemp, Display, TEXT("Kinect Running: %d"), bRunning)
		while (bRunning)
		{
			IDepthFrame* pDepthFrame = NULL;
			IColorFrame* pColorFrame = NULL;
			IBodyIndexFrame* pBodyIndexFrame = NULL;
			IBodyFrame*		 pBodyFrame = NULL;
			IInfraredFrame*  pInfraredFrame = NULL;
			TIMESPAN		 depthTime = 0;
			TIMESPAN		 colorTime = 0;
			HRESULT hr = S_OK;

			IIsAvailableChangedEventArgs* args = { 0 };
			hr = m_pKinectSensor->GetIsAvailableChangedEventData(waitableHandle, &args);

			if (SUCCEEDED(hr)) {
				BOOLEAN isAvailable = FALSE;
				if (SUCCEEDED(args->get_IsAvailable(&isAvailable)) && lastAvailable != isAvailable )
				{
					lastAvailable = isAvailable;
					WCHAR uniqueId[1024];
					if (SUCCEEDED(m_pKinectSensor->get_UniqueKinectId(1024, uniqueId))) {


						if (isAvailable)
						{
							deviceId = uniqueId;
							UE_LOG(LogTemp, Display, TEXT("Kinect detected with ID %s"), *deviceId);

							HRESULT hr = m_pCoordinateMapper->GetDepthCameraIntrinsics(&cameraIntrinsics);
							if (FAILED(hr)) {
								cameraIntrinsics.FocalLengthX = -1;
							}

						}
						else {
							UE_LOG(LogTemp, Display, TEXT("Kinect with ID %s is now gone"), *deviceId);
							cameraIntrinsics.FocalLengthX = -1;
							if (extractSkeletonData)
							{
								frwLock.WriteLock();
								kinectData->clear();
								frwLock.WriteUnlock();
							}
						}

					}
				}
			}
			else if (hr == E_PENDING) {
				hr = S_OK;
				if (!lastAvailable) {
					WCHAR uniqueId[1024];
					if (SUCCEEDED(m_pKinectSensor->get_UniqueKinectId(1024, uniqueId))) {
						lastAvailable = true;
						deviceId =uniqueId;

						HRESULT hr = m_pCoordinateMapper->GetDepthCameraIntrinsics(&cameraIntrinsics);
						if (cameraIntrinsics.FocalLengthX < 0) {
							HRESULT hr = m_pCoordinateMapper->GetDepthCameraIntrinsics(&cameraIntrinsics);
							if (FAILED(hr)) {
								cameraIntrinsics.FocalLengthX = -1;
							}
						}
					}

				}
			}


			if (lastAvailable) {

				if (deviceId.IsEmpty()) {
					WCHAR uniqueId[1024];
					if (SUCCEEDED(m_pKinectSensor->get_UniqueKinectId(1024, uniqueId))) {
						deviceId = uniqueId;
						if (cameraIntrinsics.FocalLengthX < 0) {
							HRESULT hr = m_pCoordinateMapper->GetDepthCameraIntrinsics(&cameraIntrinsics);
							if (FAILED(hr)) {
								cameraIntrinsics.FocalLengthX = -1;
							}
						}
					}
				}

				if (extractColorData && m_pColorFrameReader) {
					if (SUCCEEDED(hr)) {

						IFrameDescription* pColorFrameDescription = NULL;
						int nColorWidth = 0;
						int nColorHeight = 0;
						ColorImageFormat imageFormat = ColorImageFormat_None;
						UINT nColorBufferSize = 0;
						RGBQUAD *pColorBuffer = NULL;

						hr = m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);
						if (SUCCEEDED(hr) && pColorFrame) {
							pColorFrame->get_RelativeTime(&colorTime);
						}

						if (extractColorData && pColorFrame && SUCCEEDED(hr)) {
							
							// get color frame data

							if (SUCCEEDED(hr)) {
								hr = pColorFrame->get_FrameDescription(&pColorFrameDescription);
							}

							if (SUCCEEDED(hr)) {
								hr = pColorFrameDescription->get_Width(&nColorWidth);
							}

							if (SUCCEEDED(hr)) {
								hr = pColorFrameDescription->get_Height(&nColorHeight);
							}

							if (SUCCEEDED(hr)) {
								hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
							}


							if (SUCCEEDED(hr))
							{
								if (imageFormat == ColorImageFormat_Rgba)
								{
									hr = pColorFrame->AccessRawUnderlyingBuffer(&nColorBufferSize, reinterpret_cast<BYTE**>(&pColorBuffer));
									nColorBufferSize = KinectDTO::cColorWidth * KinectDTO::cColorHeight * sizeof(RGBQUAD);
									memcpy(m_pColorRGBX, pColorBuffer, nColorBufferSize);
								}
								else if (m_pColorRGBX)
								{
									pColorBuffer = m_pColorRGBX;
									nColorBufferSize = KinectDTO::cColorWidth * KinectDTO::cColorHeight * sizeof(RGBQUAD);
									hr = pColorFrame->CopyConvertedFrameDataToArray(nColorBufferSize, reinterpret_cast<BYTE*>(pColorBuffer), ColorImageFormat_Rgba);
								}
								else
								{
									hr = E_FAIL;
								}
							}

							if (SUCCEEDED(hr) && isUpdatingColorFeed) {
								copyBufferToTexture(cameraTexture, (uint8*)pColorBuffer, KinectDTO::cColorWidth, KinectDTO::cColorHeight, 4);
							}

							SafeRelease(pColorFrameDescription);
							
						}

					}
					hr = S_OK;
				}

				if (m_pDepthFrameReader) {
					if (SUCCEEDED(hr)) {
						hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);
						if (SUCCEEDED(hr) && pDepthFrame) {
							pDepthFrame->get_RelativeTime(&depthTime);
						}
					}
				}

				if (extractIRData && m_pInfraredFrameReader) {
					if (SUCCEEDED(hr)) {
						hr = m_pInfraredFrameReader->AcquireLatestFrame(&pInfraredFrame);
					}
				}

				if (extractSkeletonData && m_pBodyIndexFrameReader) {
					if (SUCCEEDED(hr)) {
						hr = m_pBodyIndexFrameReader->AcquireLatestFrame(&pBodyIndexFrame);
					}
				}

				if (SUCCEEDED(hr)) {
					INT64 nDepthTime = 0;
					IFrameDescription* pDepthFrameDescription = NULL;
					int nDepthWidth = 0;
					int nDepthHeight = 0;
					UINT nDepthBufferSize = 0;
					UINT16 *pDepthBuffer = NULL;

					

					IFrameDescription* pInfraredFrameDescription = NULL;
					int nIRWidth = 0;
					int nIRHeight = 0;
					UINT nIRBufferSize = 0;
					UINT16 *pInfraredBuffer = NULL;

					IFrameDescription* pBodyIndexFrameDescription = NULL;
					int nBodyIndexWidth = 0;
					int nBodyIndexHeight = 0;
					UINT nBodyIndexBufferSize = 0;
					BYTE *pBodyIndexBuffer = NULL;

					// get depth frame data
					hr = pDepthFrame->get_RelativeTime(&nDepthTime);
					if (SUCCEEDED(hr)) {

						/*if (!requiresTimeSync || NtpClient::isSynchronized()) {
						if (!startTime) {
						utcStartTime = Poco::Timestamp().utcTime();
						startTime = nDepthTime;

						if (requiresTimeSync) {
						OFX_LOG(ofx_info, "Kinect clock sincronized");
						}
						}
						}*/

						hr = pDepthFrame->get_FrameDescription(&pDepthFrameDescription);
					}

					if (SUCCEEDED(hr)) {
						hr = pDepthFrameDescription->get_Width(&nDepthWidth);
					}

					if (SUCCEEDED(hr)) {
						hr = pDepthFrameDescription->get_Height(&nDepthHeight);
					}

					if (SUCCEEDED(hr)) {
						hr = pDepthFrame->AccessUnderlyingBuffer(&nDepthBufferSize, &pDepthBuffer);
					}


					

					if (extractIRData) {
						
						hr = pInfraredFrame->get_FrameDescription(&pInfraredFrameDescription);
							
						if (SUCCEEDED(hr)) {
							hr = pInfraredFrameDescription->get_Width(&nIRWidth);
						}

						if (SUCCEEDED(hr)) {
							hr = pInfraredFrameDescription->get_Height(&nIRHeight);
						}

						if (SUCCEEDED(hr)) {
							hr = pInfraredFrame->AccessUnderlyingBuffer(&nIRBufferSize, &pInfraredBuffer);
						}

						if (SUCCEEDED(hr)) {
							memcpy(m_pInfraRedBuffer, pInfraredBuffer, nIRWidth*nIRHeight * sizeof(UINT16));
							/*frwLock.WriteLock();
							Kinect20DataProcessor::processInfrared(m_pInfraRedBuffer, nIRWidth, nIRHeight, infraRedBuffer);
							frwLock.WriteUnlock();*/
						}
						
					}

					if (extractSkeletonData) {
						
						hr = pBodyIndexFrame->get_FrameDescription(&pBodyIndexFrameDescription);
						

						if (SUCCEEDED(hr)) {
							hr = pBodyIndexFrameDescription->get_Width(&nBodyIndexWidth);
						}

						if (SUCCEEDED(hr)) {
							hr = pBodyIndexFrameDescription->get_Height(&nBodyIndexHeight);
						}

						if (SUCCEEDED(hr)) {
							hr = pBodyIndexFrame->AccessUnderlyingBuffer(&nBodyIndexBufferSize, &pBodyIndexBuffer);
							if (FAILED(hr) && pBodyIndexBuffer) {
								pBodyIndexBuffer = NULL;
							}
						}

						if (SUCCEEDED(hr)) {
							hr = m_pBodyFrameReader->AcquireLatestFrame(&pBodyFrame);
						}

						IBody* ppBodies[BODY_COUNT] = { 0 };
						INT64 nTime = 0;
						if (SUCCEEDED(hr))
						{
							hr = pBodyFrame->get_RelativeTime(&nTime);

						}

						if (SUCCEEDED(hr))
						{
							hr = pBodyFrame->GetAndRefreshBodyData(_countof(ppBodies), ppBodies);
						}


						if (SUCCEEDED(hr))
						{
							frwLock.WriteLock();
							kinectData->setBodyFrameTime(nTime);
							for (int i = 0; i < _countof(ppBodies); ++i)
							{
								loadBody(kinectData->getBody(i), ppBodies[i], pDepthBuffer);
								if (extractFaces) {
									if (kinectData->getBody(i).tracked) {
										m_pHDFaceFrameSource[i]->put_TrackingId(kinectData->getBody(i).trackingId);
									}
								}
							}
							frwLock.WriteUnlock();

						}

						for (int i = 0; i < _countof(ppBodies); ++i)
						{
							SafeRelease(ppBodies[i]);
						}



						if (extractFaces) {
							for (int count = 0; count < KinectDTO::cTotalBodies; count++) {
								IHighDefinitionFaceFrame* pHDFaceFrame = nullptr;
								hr = m_pFaceFrameReader[count]->AcquireLatestFrame(&pHDFaceFrame);
								if (SUCCEEDED(hr) && pHDFaceFrame != nullptr) {
									frwLock.WriteLock();
									kinectData->getFace(count).tracked = false;
									BOOLEAN bFaceTracked = false;

									hr = pHDFaceFrame->get_IsFaceTracked(&bFaceTracked);
									if (SUCCEEDED(hr) && bFaceTracked) {
										HDFace20& face = kinectData->getFace(count);
										face.tracked = true;
										pHDFaceFrame->get_TrackingId(&kinectData->getFace(count).trackingId);
										hr = pHDFaceFrame->GetAndRefreshFaceAlignmentResult(m_pFaceAlignment[count]);
										if (SUCCEEDED(hr) && m_pFaceAlignment[count] != nullptr) {
											Vector4 orientation = { 0 };
											hr = m_pFaceAlignment[count]->get_FaceOrientation(&orientation);
											if (SUCCEEDED(hr)) {
												kinectData->getFace(count).orientation = FQuat(orientation.x, orientation.y, orientation.z, orientation.w);
											}

											CameraSpacePoint headPivot;
											hr = m_pFaceAlignment[count]->get_HeadPivotPoint(&headPivot);
											if (SUCCEEDED(hr)) {
												face.headPivot = FVector(headPivot.X, headPivot.Y, headPivot.Z);
											}

											RectI boundingBox;
											if (SUCCEEDED(m_pFaceAlignment[count]->get_FaceBoundingBox(&boundingBox))) {
												face.rect = FIntRect(boundingBox.Left, boundingBox.Top, boundingBox.Right, boundingBox.Bottom);
											}
											else {
												face.rect = FIntRect(0, 0, 0, 0);
											}

											HRESULT hr2 = m_pFaceModel[count]->CalculateVerticesForAlignment(m_pFaceAlignment[count], m_pFaceVertices[count].Num(), &m_pFaceVertices[count][0]);
											if (FAILED(hr2)) {
												UE_LOG(LogTemp, Warning, TEXT("Error calculating vertices algimnent"));
											}
											FQuat q(orientation.x, orientation.y, orientation.z, orientation.w);
											q = q.Inverse();
											FVector minPos(100), maxPos(-100);
											int nVertex = 0;
											for (CameraSpacePoint& p : m_pFaceVertices[count]) {

												FVector _p = q*FVector(p.X - headPivot.X, p.Y - headPivot.Y, p.Z - headPivot.Z);
												if (nVertex < 1500) {
													face.points[nVertex] = FVector(p.X, p.Y, p.Z);
												}
												else {
													UE_LOG(LogTemp, Warning, TEXT("Not enought vertices"));
												}

												if (_p.X < minPos.X) minPos.X = _p.X;
												if (_p.Y < minPos.Y) minPos.Y = _p.Y;
												if (_p.Z < minPos.Z) minPos.Z = _p.Z;

												if (_p.X > maxPos.X) maxPos.X = _p.X;
												if (_p.Y > maxPos.Y) maxPos.Y = _p.Y;
												if (_p.Z > maxPos.Z) maxPos.Z = _p.Z;
												nVertex++;
											}
											face.height = maxPos.Y - minPos.Y;;
										}

									}
									frwLock.WriteUnlock();
								}
								SafeRelease(pHDFaceFrame);
							}
						}
					}

					//Process the frame
					bool useCoordinates = false;
					if (m_pCoordinateMapper && m_pColorCoordinates) {

						HRESULT hr = m_pCoordinateMapper->MapDepthFrameToColorSpace(nDepthWidth * nDepthHeight, (UINT16*)pDepthBuffer, nDepthWidth * nDepthHeight, m_pColorCoordinates);
						if (SUCCEEDED(hr)) {
							useCoordinates = true;
						}
						if (extractDepthCoordinates) {
							hr = m_pCoordinateMapper->MapColorFrameToDepthSpace(nDepthWidth * nDepthHeight, (UINT16*)pDepthBuffer, KinectDTO::cColorWidth * KinectDTO::cColorHeight, m_pDepthCoordinates);
							if (SUCCEEDED(hr)) {
								int i = 0;
								i++;
							}
						}
					}
					if (extractDepthData) {

						/*frwLock.WriteLock();
						depthRawData = kinectData->getDepthRawData();
						kinectData->setFrameTime(nDepthTime - startTime);
						Kinect20DataProcessor::processData(pDepthBuffer, nDepthWidth, nDepthHeight, pColorBuffer, nColorWidth, nColorHeight, pBodyIndexBuffer, nBodyIndexWidth, nBodyIndexHeight, useCoordinates ? m_pColorCoordinates : NULL, depthRawData);
						memcpy(tempDepthData, depthRawData, sizeof(DepthRawData20) * Kinect20DTO::cDepthWidth * Kinect20DTO::cDepthHeight);

						memcpy(m_pDethRawBuffer, pDepthBuffer, sizeof(UINT16)* cDepthWidth * cDepthHeight);

						frwLock.WriteUnlock();*/
					}



					SafeRelease(pDepthFrameDescription);
					
					SafeRelease(pBodyIndexFrameDescription);
					SafeRelease(pBodyFrame);
					SafeRelease(pInfraredFrameDescription);
				}
			}


			SafeRelease(pDepthFrame);
			SafeRelease(pColorFrame);
			SafeRelease(pBodyIndexFrame);
			SafeRelease(pInfraredFrame);

			WaitForSingleObject(NULL, 15);
		}
	//TODO (OS): Should return status;
	return 0;
}

void KinectDevice::Stop()
{
	//TODO (OS - @MR): Implement this;
	UE_LOG(LogTemp, Display, TEXT("Kinect Stopped"))
		bRunning = false;
}

void KinectDevice::loadBody(Body20& body, IBody* pBody, UINT16* pDepthBuffer) {
	memset(&body, 0, sizeof(Body20));
	if (pBody)
	{

		BOOLEAN bTracked = false;
		HRESULT hr = pBody->get_IsTracked(&bTracked);

		if (SUCCEEDED(hr))
		{

			BOOLEAN bRestricted = false;
			pBody->get_IsRestricted(&bRestricted);
			pBody->get_ClippedEdges(&body.clippedEdges);
			pBody->get_HandLeftConfidence((TrackingConfidence*)&body.leftHandConfidence);
			pBody->get_HandRightConfidence((TrackingConfidence*)&body.rightHandConfidence);
			PointF lean;
			pBody->get_Lean(&lean);
			body.lean = FVector2D(lean.X, lean.Y);

			Joint joints[JointType_Count];
			JointOrientation orientations[JointType_Count];
			body.tracked = (bTracked == TRUE);
			body.restricted = (bRestricted == TRUE);


			HandState leftHandState = HandState_Unknown;
			HandState rightHandState = HandState_Unknown;
			DetectionResult result = DetectionResult_Unknown;
			::TrackingState state = TrackingState_NotTracked;
			pBody->get_LeanTrackingState(&state);
			UINT64	  id = 0;
			pBody->get_HandLeftState(&leftHandState);
			pBody->get_HandRightState(&rightHandState);
			pBody->get_TrackingId(&id);
			pBody->get_Engaged(&result);
			body.leftHandState = (HandTrackingState)leftHandState;
			body.rightHandState = (HandTrackingState)rightHandState;
			body.trackingId = id;
			body.trackingState = (KinectTrackingState)state;
			body.engaged = (DetectionState)result;

			hr = pBody->GetJoints(_countof(joints), joints);
			hr |= pBody->GetJointOrientations(_countof(joints), orientations);

			float avgDepth = 0.0f;
			int totalJoints = 0;

			DetectionResult activities[Activity_Count];
			pBody->GetActivityDetectionResults((UINT)Activity_Count, activities);
			memcpy(body.activities, activities, sizeof(DetectionResult)*Activity_Count);

			DetectionResult appareance[Appearance_Count];
			pBody->GetActivityDetectionResults((UINT)Appearance_Count, appareance);
			memcpy(body.appearance, appareance, sizeof(DetectionResult)*Appearance_Count);

			DetectionResult expressions[Appearance_Count];
			pBody->GetActivityDetectionResults((UINT)Expression_Count, expressions);
			memcpy(body.expressions, expressions, sizeof(DetectionResult)*Expression_Count);


			if (SUCCEEDED(hr))
			{

				TArray<UINT16> v;
				v.Reserve(9);

				for (int j = 0; j < _countof(joints); ++j)
				{
					body.joints[j].joinType = (JointId)joints[j].JointType;
					body.joints[j].trackingState = (KinectTrackingState)joints[j].TrackingState;
					
					body.joints[j].orientation = FQuat(orientations[j].Orientation.x, orientations[j].Orientation.y, orientations[j].Orientation.z, orientations[j].Orientation.w);
					body.joints[j].cameraSpacePos= FVector(joints[j].Position.X, joints[j].Position.Y, joints[j].Position.Z);

					DepthSpacePoint depthPoint = { 0 };
					m_pCoordinateMapper->MapCameraPointToDepthSpace(joints[j].Position, &depthPoint);
					body.joints[j].depthSpacePos = FVector(depthPoint.X, depthPoint.Y, 0);

					double zValue = -1;

					if (pDepthBuffer && joints[j].TrackingState != TrackingState_NotTracked)
					{
						int idx = depthPoint.Y*KinectDTO::cDepthWidth + depthPoint.X;
						

						for (int k = -1; k < 2; k++)
						{
							if ((idx + k) >= 0 && (idx + k) < KinectDTO::cTotalDepthPixels)
							{
								if (pDepthBuffer[idx + k] != 0) {
									v.Add(pDepthBuffer[idx + k]);
								}
							}

							if ((idx + k + KinectDTO::cDepthWidth) >= 0 && (idx + k + KinectDTO::cDepthWidth) < KinectDTO::cTotalDepthPixels)
							{
								if (pDepthBuffer[idx + k + KinectDTO::cDepthWidth] != 0) {
									v.Add(pDepthBuffer[idx + k + KinectDTO::cDepthWidth]);
								}
							}

							if ((idx + k - KinectDTO::cDepthWidth) >= 0 && (idx + k - KinectDTO::cDepthWidth) < KinectDTO::cTotalDepthPixels)
							{
								if (pDepthBuffer[idx + k - KinectDTO::cDepthWidth] != 0) {
									v.Add(pDepthBuffer[idx + k - KinectDTO::cDepthWidth]);
								}
							}
						}

						int size = v.Num();
						if (size > 0) {
							v.Sort();
							if (size % 2 == 0) {
								zValue = v[(size >> 1) - 1];
							}
							else {
								zValue = v[size >> 1];
							}
							body.joints[j].depth = zValue;
						}
					}




					if (body.joints[j].trackingState == Tracked) {
						if (pDepthBuffer && zValue >= 0) {
							avgDepth += zValue / 1000.0f;
						}
						else
						{
							avgDepth += body.joints[j].cameraSpacePos.Z;
						}
						totalJoints++;
					}
				}
			}
			if (totalJoints > 0) {
				avgDepth /= (float)totalJoints;
				body.zDepth = avgDepth;
				/*if (avgDepth < userMinimumDepth || avgDepth > userMaximumDepth) {
					body.tracked = false;
				}*/
			}

		}
	} 
}


void KinectDevice::copyBufferToTexture(TWeakObjectPtr<UTexture2D> Texture, unsigned char * pData, int TextureWidth, int TextureHeight, int numColors)
{

	if (Texture.IsValid() && Texture->Resource != NULL)
	{
		//Dispatch to render thread here
		struct FUpdateTextureRegionsData
		{
			FTexture2DResource* Texture2DResource;
			int32 MipIndex;
			FUpdateTextureRegion2D Region;
			uint32 SrcPitch;
			uint32 SrcBpp;
			uint8* SrcData;
		};

		FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;

		RegionData->Texture2DResource = (FTexture2DResource*)Texture->Resource;
		RegionData->MipIndex = 0;
		RegionData->Region = FUpdateTextureRegion2D(0, 0, 0, 0, TextureWidth, TextureHeight);;
		RegionData->SrcPitch = (uint32)(numColors * TextureWidth); //TODO: Clap to 32bits
		RegionData->SrcBpp = (uint32)numColors;
		RegionData->SrcData = pData;

	

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			UpdateTextureRegionsData,
			FUpdateTextureRegionsData*, RegionData, RegionData,
			{
				int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
		if (RegionData->MipIndex >= CurrentFirstMip)
		{
			RHIUpdateTexture2D(RegionData->Texture2DResource->GetTexture2DRHI(),
				RegionData->MipIndex - CurrentFirstMip,
				RegionData->Region,
				RegionData->SrcPitch,
				RegionData->SrcData + RegionData->Region.SrcY * RegionData->SrcPitch + RegionData->Region.SrcX * RegionData->SrcBpp);
		}

		delete RegionData;
			});
	}

}

DepthSpacePoint* KinectDevice::getDepthCoordinates() const {
	return m_pDepthCoordinates;
}

CameraIntrinsics KinectDevice::getCameraIntrinsics() const {
	return cameraIntrinsics;
}
TArray<Body20>	KinectDevice::getBodies() const {
	TArray<Body20> bodies;
	bodies.SetNumUninitialized(KinectDTO::cTotalBodies);
	((KinectDevice*)this)->frwLock.ReadLock();
	for ( int i = 0; i < KinectDTO::cTotalBodies; i++ ) {
		bodies[i] = kinectData->getBody(i);
	}
	((KinectDevice*)this)->frwLock.ReadUnlock();
	return bodies;

}

Body20	KinectDevice::getBody(int idx) const {
	((KinectDevice*)this)->frwLock.ReadLock();
	Body20 body = kinectData->getBody(idx);
	((KinectDevice*)this)->frwLock.ReadUnlock();
	return body;
}

void KinectDevice::setIsUpdatingColorCamera(bool value) {
	isUpdatingColorFeed = value;
}

UTexture2D*	KinectDevice::getColorTexture() {
	if (cameraTexture.IsValid()) {
		return cameraTexture.Get();
	}
	return NULL;
}

