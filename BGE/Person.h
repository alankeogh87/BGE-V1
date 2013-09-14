#pragma once
#include "GameComponent.h"
#include "PhysicsController.h"
#include "NuiApi.h"
#include <string>
#include <map>


using namespace std;

namespace BGE
{
	void CALLBACK StatusProc( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName, void* pUserData);

	struct Hand
	{
		glm::vec3 pos;
		glm::vec3 look;
	};

	class Person :
		public GameComponent
	{
	private:
		bool m_bSeatedMode;
		INuiSensor* m_pNuiSensor;
		HANDLE m_hNextSkeletonEvent;
		void SetStatusMessage(std::string message);
		void UpdateSkeleton(const NUI_SKELETON_DATA & skeleton);
		void SkeletonFrameReady( NUI_SKELETON_FRAME* skeletonFrame );
		void UpdateBone( const NUI_SKELETON_DATA & skeleton, NUI_SKELETON_POSITION_INDEX jointFrom, NUI_SKELETON_POSITION_INDEX jointTo);
		void UpdateHead( const NUI_SKELETON_DATA & skeleton, NUI_SKELETON_POSITION_INDEX joint);
		
		void UpdateHand( const NUI_SKELETON_DATA & skeleton, NUI_SKELETON_POSITION_INDEX jointFrom, NUI_SKELETON_POSITION_INDEX jointTo, int handIndex);
		
		map<string, std::shared_ptr<PhysicsController>> boneComponents;
		bool connected;
		bool tracked;
		float footHeight;
		float scale;
	public:
		Person(void);
		~Person(void);

		void Update(float timeDelta);
		bool Initialise();
		HRESULT CreateFirstConnected();
		bool headCamera;
		friend void CALLBACK StatusProc( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName, void* pUserData);

		Hand hands[2];
		glm::vec3 shoulderPos;
	};
}

