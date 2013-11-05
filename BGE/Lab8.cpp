#include "Lab8.h"
#include "Content.h"
#include "VectorDrawer.h"
#include "LazerBeam.h"
#include "FountainEffect.h"

using namespace BGE;

Lab8::Lab8(void)
{
	elapsed = 10000;
}


Lab8::~Lab8(void)
{
}

bool Lab8::Initialise()
{
	std::shared_ptr<GameComponent> ground = make_shared<Ground>();
	Attach(ground);	

	ship1 = make_shared<GameComponent>();
	ship1->Attach(Content::LoadModel("cobramk3", glm::rotate(glm::mat4(1), 180.0f, glm::vec3(0,1,0))));
	ship1->position = glm::vec3(-10, 2, -10);
	ship1->Attach(make_shared<VectorDrawer>());
	Attach(ship1);

	
	
	riftEnabled = false;
	fullscreen = false;
	width = 800;
	height = 600;

	mass = 1.0f;
	ship1->velocity = glm::vec3(0,0,0);

	Game::Initialise();

	camera->GetController()->position = glm::vec3(0, 4, 20);
	return true;
}

void Lab8::Update(float timeDelta)
{	
	// Movement of ship2
	float newtons = 10.0f;
	if (keyState[SDL_SCANCODE_UP])
	{
		force += ship1->look * newtons;
	}
	if (keyState[SDL_SCANCODE_DOWN])
	{
		force -= ship1->look * newtons;
	}
	if (keyState[SDL_SCANCODE_LEFT])
	{
		force -= ship1->right * newtons;
	}
	if (keyState[SDL_SCANCODE_RIGHT])
	{
		force += ship1->right * newtons;
	}

	glm::vec3 accel = force / mass;
	ship1->velocity += accel * timeDelta;
	ship1->position += ship1->velocity * timeDelta;
	if (glm::length(ship1->velocity) > 0.01f)
	{
		ship1->look = glm::normalize(ship1->velocity);		
	}
	if (glm::length(ship1->look - GameComponent::basisLook) > 0.01f)
	{
		glm::vec3 axis = glm::cross(GameComponent::basisLook, ship1->look);
		axis = glm::normalize(axis);
		float theta = glm::acos(glm::dot(ship1->look, GameComponent::basisLook));
		ship1->orientation = glm::angleAxis(glm::degrees(theta), axis);
	}
	ship1->velocity *= 0.99f;
	force = glm::vec3(0,0,0);
	Game::Update(timeDelta);

}
