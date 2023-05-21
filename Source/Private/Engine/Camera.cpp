#include <Znpch.h>
#include <Engine/Camera.h>

#include <glm/ext.hpp>
#include <SDL.h>

using namespace Zn;

float camera_speed = 10.f;

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

            camera.Rotate(rotation.y, rotation.x, 0.f);

            SDL_SetRelativeMouseMode(SDL_TRUE);
        }
        else
        {
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
        break;
    case SDL_MOUSEWHEEL:
    {
        camera_speed = std::max(camera_speed + event.wheel.y, 0.1f);
    }
    default:
        break;
    }
    }
}

void Zn::camera_process_key_input(const i32& key, float deltaTime, Camera& camera)
{
    static const f32 kSpeed = 100.f;

    const f32 velocity = camera_speed * deltaTime;

    glm::vec3 delta_movement {0.f};

    const ViewInfo& view = camera.GetViewInfo();

    switch (key)
    {
    case SDLK_a:
        delta_movement = -(view.right * velocity);
        break;
    case SDLK_d:
        delta_movement = view.right * velocity;
        break;
    case SDLK_s:
        delta_movement = -(view.forward * velocity);
        break;
    case SDLK_w:
        delta_movement = view.forward * velocity;
        break;
    case SDLK_q:
        delta_movement = -(view.up * velocity);
        break;
    case SDLK_e:
        delta_movement = (view.up * velocity);
        break;
    }

    if (delta_movement.length() > 0.f)
    {
        camera.Move(delta_movement);
    }
}

void Zn::Camera::Rotate(f32 pitch_, f32 yaw_, f32 roll_)
{
    yaw += yaw_;
    pitch += pitch_;

    if (pitch > 89.0f)
    {
        pitch = 89.f;
    }

    if (pitch < -89.f)
    {
        pitch = -89.f;
    }

    view.forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    view.forward.y = sin(glm::radians(pitch));
    view.forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    view.forward   = glm::normalize(view.forward);

    view.right = glm::normalize(glm::cross(view.forward, kSceneUp));
    view.up    = glm::normalize(glm::cross(view.right, view.forward));

    // const glm::quat pitch_rotation = glm::angleAxis(camera.pitch, glm::vec3(1, 0, 0));
    // const glm::quat yaw_rotation   = glm::angleAxis(camera.yaw, glm::vec3(0, 1, 0));

    // const glm::quat orientation = glm::normalize(pitch_rotation * yaw_rotation);

    // const glm::mat4 rotate    = glm::mat4_cast(orientation);
    // const glm::mat4 translate = glm::translate(glm::mat4 {1.f}, -camera.position);

    // camera.view = rotate * translate;
}

Zn::Camera::Camera()
    : yaw(0.f)
    , pitch(0.f)
{
    view = {
        .position    = glm::vec3(0.f),
        .forward     = kSceneForward,
        .up          = kSceneUp,
        .right       = kSceneRight,
        .fov         = 60.f,
        .aspectRatio = 16.f / 9.f,
        .nearClip    = 0.1f,
        .farClip     = 10000.f,
    };
}

Zn::Camera& Zn::Camera::SetPosition(const glm::vec3& position_)
{
    view.position = position_;
    return *this;
}

Zn::Camera& Zn::Camera::SetYaw(f32 yaw_)
{
    yaw = yaw_;
    Rotate(0.f, 0.f, 0.f);
    return *this;
}

Zn::Camera& Zn::Camera::SetPitch(f32 pitch_)
{
    pitch = pitch_;
    Rotate(0.f, 0.f, 0.f);
    return *this;
}

Zn::Camera& Zn::Camera::SetRoll(f32 roll_)
{
    // TODO;
    return *this;
}

Zn::Camera& Zn::Camera::SetFOV(f32 fov_)
{
    view.fov = fov_;
    return *this;
}

Zn::Camera& Zn::Camera::SetAspectRatio(f32 aspectRatio_)
{
    view.aspectRatio = aspectRatio_;
    return *this;
}

Zn::Camera& Zn::Camera::SetNearClip(f32 nearClip_)
{
    view.nearClip = nearClip_;
    return *this;
}

Zn::Camera& Zn::Camera::SetFarClip(f32 farClip_)
{
    view.farClip = farClip_;
    return *this;
}

void Zn::Camera::Move(glm::vec3 offset_)
{
    view.position += offset_;
}

const ViewInfo& Zn::Camera::GetViewInfo() const
{
    return view;
}
