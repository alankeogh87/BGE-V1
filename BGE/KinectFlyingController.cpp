#include "KinectFlyingController.h"
#include "Utils.h"
#include "VRGame.h"

using namespace BGE;

void CALLBACK BGE::StatusProc1( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName, void * pUserData)
{      
	KinectFlyingController * kfc = (KinectFlyingController *) pUserData;
	if ( SUCCEEDED( hrStatus ) )      
	{   kfc->CreateFirstConnected();       
	kfc->connected = true;      
	}      
	else      
	{          
		kfc->tracked = false;
		kfc->connected = false;       
	}
}


KinectFlyingController::KinectFlyingController(shared_ptr<Model> model)
{
	this->steerable = make_shared<Steerable3DController>(model);

	connected = false;
	tracked = false;
	m_pNuiSensor = NULL;
	scale = 20.0f;
	footHeight = 0.0f;
	myRoll = 0.0f;
	worldMode = world_modes::to_parent;

	//Attach(this->steerable);
}

KinectFlyingController::~KinectFlyingController(void)
{
}

void KinectFlyingController::Update(float timeDelta)
{
	this->timeDelta = timeDelta;
	if (connected)
	{
		SetStatusMessage("Kinect is connected");
		SetStatusMessage("Press C to toggle the head camera");
		if (tracked)
		{
			SetStatusMessage("Kinect is tracking");
		}
		else
		{
			SetStatusMessage("Kinect is not tracking");
		}
		// Wait for 0ms, just quickly test if it is time to process a skeleton
		if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextSkeletonEvent, 0) )
		{
			NUI_SKELETON_FRAME skeletonFrame = {0};

			// Get the skeleton frame that is ready
			if (SUCCEEDED(m_pNuiSensor->NuiSkeletonGetNextFrame(0, &skeletonFrame)))
			{
				m_pNuiSensor->NuiTransformSmooth(&skeletonFrame, &verySmoothParams);
				// Process the skeleton frame				
				SkeletonFrameReady(&skeletonFrame);
			}
		}
	}
	else
	{
		SetStatusMessage("Kinect is not connected");
	}
	steerable->Update(timeDelta);
	position = steerable->position;
	look = steerable->look;
	up = steerable->up;
	right = steerable->right;

	orientation= steerable->orientation;

	Game::Instance()->PrintVector("Steerable Position: ", position);
	GameComponent::Update(timeDelta);
}

void KinectFlyingController::SkeletonFrameReady( NUI_SKELETON_FRAME* pSkeletonFrame )
{
	tracked = false;
	for (int i = 0; i < NUI_SKELETON_COUNT; i++)
	{
		const NUI_SKELETON_DATA & skeleton = pSkeletonFrame->SkeletonData[i];

		switch (skeleton.eTrackingState)
		{
		case NUI_SKELETON_TRACKED:
			UpdateSkeleton(skeleton);
			tracked = true;
			// Just draw the first skeleton I find
			return;
		case NUI_SKELETON_NOT_TRACKED:
		case NUI_SKELETON_POSITION_ONLY:
			break;
		}
	}

}

bool KinectFlyingController::Initialise()
{
	CreateFirstConnected();
	NuiSetDeviceStatusCallback(&StatusProc1, this);

	VRGame * game = (VRGame *) Game::Instance();

	this->steerable->worldMode = world_modes::from_self;
	this->steerable->position = position;
	game->camFollower = make_shared<GameComponent>();
	shared_ptr<SteeringController> camController = make_shared<SteeringController>();
	camController->offset = glm::vec3(0,0,50);
	camController->leader = this->steerable;
	camController->position = game->camFollower->position = this->steerable->position + camController->offset;

	camController->TurnOffAll();
	camController->TurnOn(SteeringController::behaviour_type::offset_pursuit);
	game->Attach(game->camFollower);
	game->camFollower->Attach(camController);

	steerable->position = position;
	steerable->look = look;
	steerable->up = up;
	steerable->right = right;
	return GameComponent::Initialise();
}

HRESULT KinectFlyingController::CreateFirstConnected()
{
	INuiSensor * pNuiSensor = NULL;

	int iSensorCount = 0;
	HRESULT hr = NuiGetSensorCount(&iSensorCount);
	if (FAILED(hr))
	{
		return hr;
	}

	// Look at each Kinect sensor
	for (int i = 0; i < iSensorCount; ++i)
	{
		// Create the sensor so we can check status, if we can't create it, move on to the next
		hr = NuiCreateSensorByIndex(i, &pNuiSensor);
		if (FAILED(hr))
		{
			continue;
		}

		// Get the status of the sensor, and if connected, then we can initialize it
		hr = pNuiSensor->NuiStatus();
		if (S_OK == hr)
		{
			m_pNuiSensor = pNuiSensor;
			break;
		}

		// This sensor wasn't OK, so release it since we're not using it
		pNuiSensor->Release();
		return hr;
	}

	if (NULL != m_pNuiSensor)
	{
		// Initialize the Kinect and specify that we'll be using skeleton
		hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_SKELETON); 
		if (SUCCEEDED(hr))
		{
			// Create an event that will be signaled when skeleton data is available
			m_hNextSkeletonEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
			// Open a skeleton stream to receive skeleton data
			hr = m_pNuiSensor->NuiSkeletonTrackingEnable(m_hNextSkeletonEvent, 0); 
		}
	}

	if (NULL == m_pNuiSensor || FAILED(hr))
	{
		SetStatusMessage("No ready Kinect found!");
		return E_FAIL;
	}
	connected = true;
	SetStatusMessage("Kinect Connected");
	return hr;
}



void KinectFlyingController::SetStatusMessage( std::string message )
{
	Game::Instance()->PrintText(message);
}


void KinectFlyingController::UpdateSkeleton(const NUI_SKELETON_DATA & skeleton)
{
	const Uint8 * keyState = Game::Instance()->GetKeyState();
	static bool lastPressed = false;
	//if (footHeight == 0.0f)
	{
		footHeight = glm::min<float>(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_FOOT_RIGHT].y, skeleton.SkeletonPositions[NUI_SKELETON_POSITION_FOOT_LEFT].y);
	}	

	leftHandPos = NUIToGLVector(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT], false);
	rightHandPos = NUIToGLVector(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT], false);
	shoulderPos = NUIToGLVector(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER], false);

	float leftDiff = leftHandPos.y - shoulderPos.y;
	float rightDiff = rightHandPos.y - shoulderPos.y;
	float forceScale = 1000.0f;
	//steerable->AddForce(glm::vec3(0.0f, -9.8f, 0.0f) * timeDelta * forceScale);
	if ((leftDiff > 0.0f) && (rightDiff > 0.0f))
	{
		float forceAmount = glm::max(leftDiff, rightDiff) * 20.0f;
		steerable->AddForce(glm::vec3(0, forceAmount * forceScale * timeDelta * 7.0f, 0));
		steerable->gravity = glm::vec3(0.0f, -9.0f, 0.0f);
		return;		
	}
	if ((leftDiff > 0.0f))
	{
		float forceAmount = leftHandPos.y - rightHandPos.y;
		//steerable->AddForce(steerable->right * forceAmount * forceScale * timeDelta);
		steerable->AddTorque(up * forceAmount * forceScale * timeDelta * 0.5f);
		myRoll = forceAmount;
		//steerable->gravity = glm::vec3(0.0f, -1.0f, 0.0f);
		return;
	}
	if ((rightDiff > 0.0f))
	{
		float forceAmount = - (rightHandPos.y - leftHandPos.y);
		myRoll = forceAmount;
		
		//steerable->AddForce(steerable->right * forceAmount * forceScale * timeDelta);

		steerable->AddTorque(up * forceAmount * forceScale * timeDelta * 0.5f);
		//steerable->gravity = glm::vec3(0.0f, -1.0f, 0.0f);
	}
}
