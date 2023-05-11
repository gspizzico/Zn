#pragma once

#include <Core/HAL/BasicTypes.h>
#include <glm/glm.hpp>

union SDL_Event;

namespace Zn
{
struct Camera
{
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;

    glm::vec3 worldUp;

    f32 yaw;
    f32 pitch;
};

void camera_rotate(glm::vec2 rotation, Camera& camera);

void camera_process_input(const SDL_Event& event, f32 deltaTime, Camera& camera);

void camera_process_key_input(const i32& key, float deltaTime, Camera& camera);
} // namespace Zn
