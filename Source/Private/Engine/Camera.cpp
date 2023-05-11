#include <Znpch.h>
#include <Engine/Camera.h>

#include <glm/ext.hpp>
#include <SDL.h>

using namespace Zn;

void Zn::camera_rotate(glm::vec2 rotation, Camera& camera)
{
    camera.yaw += rotation.x;
    camera.pitch += rotation.y;

    if (camera.pitch > 89.0f)
    {
        camera.pitch = 89.f;
    }

    if (camera.pitch < -89.f)
    {
        camera.pitch = -89.f;
    }

    glm::vec3 front;
    front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    front.y = sin(glm::radians(camera.pitch));
    front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));

    camera.front = glm::normalize(front);

    camera.right = glm::normalize(glm::cross(camera.front, camera.worldUp));
    camera.up    = glm::normalize(glm::cross(camera.right, camera.front));
}

void Zn::camera_process_input(const SDL_Event& event, f32 deltaTime, Camera& camera)
{
    switch (event.type)
    {
    case SDL_MOUSEMOTION:
    {
        if (event.motion.state & SDL_BUTTON_RMASK)
        {
            static constexpr f32 kSensitivity = 0.1f;

            glm::vec2 rotation {event.motion.xrel * kSensitivity, -event.motion.yrel * kSensitivity};

            camera_rotate(rotation, camera);

            SDL_SetRelativeMouseMode(SDL_TRUE);
        }
        else
        {
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
        break;
    default:
        break;
    }
    }
}

void Zn::camera_process_key_input(const i32& key, float deltaTime, Camera& camera)
{
    static const f32 kSpeed = 10.f;

    const f32 velocity = kSpeed * deltaTime;

    glm::vec3 delta_movement {0.f};

    switch (key)
    {
    case SDLK_a:
        delta_movement = -(camera.right * velocity);
        break;
    case SDLK_d:
        delta_movement = camera.right * velocity;
        break;
    case SDLK_s:
        delta_movement = -(camera.front * velocity);
        break;
    case SDLK_w:
        delta_movement = camera.front * velocity;
        break;
    case SDLK_q:
        delta_movement = -(camera.up * velocity);
        break;
    case SDLK_e:
        delta_movement = (camera.up * velocity);
        break;
    }

    if (delta_movement.length() > 0.f)
    {
        camera.position += delta_movement;
    }
}
