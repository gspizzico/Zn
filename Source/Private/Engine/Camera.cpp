#include <Znpch.h>
#include <Engine/Camera.h>

#include <glm/ext.hpp>

using namespace Zn;

static const glm::vec3 kUpVector{ 0.0f, 1.f, 0.f };

void Zn::move_camera(glm::vec3 direction, Camera& camera)
{
	camera.direction = glm::normalize(camera.direction);
	glm::vec3 right = glm::normalize(glm::cross(camera.direction, kUpVector));

	camera.position += (direction.z * camera.direction);
	camera.position += (direction.y * kUpVector);
	camera.position += (direction.x * right);
}

void Zn::rotate_camera(glm::vec2 rotation, Camera & camera)
{
	glm::vec3 direction = glm::normalize(camera.direction);
	glm::vec3 right = glm::normalize(glm::cross(direction, kUpVector));
	glm::vec3 up = glm::normalize(glm::cross(right, direction));

	glm::mat4 rotationMatrix = glm::mat4(1.f);
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.y), right);
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(-rotation.x), up);

	camera.direction = glm::vec3(rotationMatrix * glm::vec4(camera.direction, 0.f));
}
