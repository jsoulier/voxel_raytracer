#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

#include "camera.hpp"

static constexpr float kMaxPitch = glm::pi<float>() / 2.0f - 0.01f;
static constexpr glm::vec3 kUp = glm::vec3{0.0f, 1.0f, 0.0f};

Camera::Camera()
    : State{}
    , Pitch{0.0f}
    , Yaw{0.0f}
    , Width{0.0f}
    , Height{0.0f}
{
}

bool Camera::Init(SDL_GPUDevice* device)
{
    if (!State.Init(device))
    {
        SDL_Log("Failed to initialize state");
        return false;
    }
    State->Up = kUp;
    Rotate(0.0f, 0.0f);
    Resize(1.0f, 1.0f);
    SetFov(60.0f);
    return true;
}

void Camera::Destroy(SDL_GPUDevice* device)
{
    State.Destroy(device);
}

void Camera::Resize(float width, float height)
{
    Width = width;
    Height = height;
    State->AspectRatio = Width / Height;
}

void Camera::Move(float dx, float dy, float dz)
{
    State->Position += State->Right * glm::vec3(dx);
    State->Position += State->Forward * glm::vec3(dz);
    State->Position += State->Up * glm::vec3(dy);
}

void Camera::Rotate(float dx, float dy)
{
    Yaw += dx;
    Pitch = std::clamp(Pitch + dy, -kMaxPitch, kMaxPitch);
    State->Forward.x = std::cos(Pitch) * std::cos(Yaw);
    State->Forward.y = std::sin(Pitch);
    State->Forward.z = std::cos(Pitch) * std::sin(Yaw);
    State->Forward = glm::normalize(State->Forward);
    State->Right = glm::cross(State->Forward, State->Up);
    State->Right = glm::normalize(State->Right);
}

void Camera::SetFov(float fov)
{
    State->TanHalfFov = std::tan(glm::radians(fov) * 0.5f);
}

void Camera::Upload(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass)
{
    State.Upload(device, copyPass);
}

SDL_GPUBuffer* Camera::GetBuffer() const
{
    return State.GetBuffer();
}

float Camera::GetWidth() const
{
    return Width;
}

float Camera::GetHeight() const
{
    return Height;
}