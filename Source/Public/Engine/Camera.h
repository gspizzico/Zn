#pragma once

#include <Core/HAL/BasicTypes.h>
#include <Rendering/RendererTypes.h>
#include <glm/glm.hpp>

union SDL_Event;

namespace Zn
{
class Camera
{
  public:
    Camera();

    void Rotate(f32 pitch_, f32 yaw_, f32 roll_);

    void Move(glm::vec3 offset_);

    Camera& SetPosition(const glm::vec3& position_);
    Camera& SetYaw(f32 yaw_);
    Camera& SetPitch(f32 pitch_);
    Camera& SetRoll(f32 roll_);
    Camera& SetFOV(f32 fov_);
    Camera& SetAspectRatio(f32 aspectRatio_);
    Camera& SetNearClip(f32 nearClip_);
    Camera& SetFarClip(f32 farClip_);

    const ViewInfo& GetViewInfo() const;

  private:
    f32 yaw;
    f32 pitch;

    ViewInfo view;

    static constexpr glm::vec3 kSceneUp      = glm::vec3(0.0f, 1.f, 0.f);
    static constexpr glm::vec3 kSceneForward = glm::vec3(1.0f, 0.f, 0.f);
    static constexpr glm::vec3 kSceneRight   = glm::vec3(0.0f, 0.f, 1.f);
};

void camera_process_input(const SDL_Event& event, f32 deltaTime, Camera& camera);

void camera_process_key_input(const i32& key, float deltaTime, Camera& camera);
} // namespace Zn
