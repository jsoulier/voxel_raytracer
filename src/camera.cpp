#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

#include "camera.hpp"

static constexpr float kMaxPitch = glm::pi<float>() / 2.0f - 0.01f;
static constexpr float kEpsilon = glm::epsilon<float>();
static constexpr glm::vec3 kUp = glm::vec3{0.0f, 1.0f, 0.0f};

Camera::Camera()
    : State{}
    , Pitch{0.0f}
    , Yaw{0.0f}
    , Width{0}
    , Height{0}
{
}

bool Camera::Init(SDL_GPUDevice* device)
{
    if (!State.Init(device))
    {
        SDL_Log("Failed to initialize state");
        return false;
    }
    State.Get().Position.y = 32.0f;
    Rotate(0.0f, 0.0f, true);
    Resize(1, 1);
    SetFov(60.0f);
    return true;
}

void Camera::Destroy(SDL_GPUDevice* device)
{
    State.Destroy(device);
}

void Camera::Resize(int width, int height)
{
    Width = width;
    Height = height;
    State.Get().AspectRatio = float(Width) / Height;
}

void Camera::Move(float dx, float dy, float dz)
{
    if (glm::length(glm::vec3(dx, dy, dz)) < kEpsilon)
    {
        return;
    }
    State.Get().Position += State->Right * glm::vec3(dx);
    State.Get().Position += State->Forward * glm::vec3(dz);
    State.Get().Position += kUp * glm::vec3(dy);
}

void Camera::Rotate(float dx, float dy, bool force)
{
    if (!force && std::abs(dx) < kEpsilon && std::abs(dy) < kEpsilon)
    {
        return;
    }
    Yaw += dx;
    Pitch = std::clamp(Pitch - dy, -kMaxPitch, kMaxPitch);
    State.Get().Forward.x = std::cos(Pitch) * std::cos(Yaw);
    State.Get().Forward.y = std::sin(Pitch);
    State.Get().Forward.z = std::cos(Pitch) * std::sin(Yaw);
    State.Get().Forward = glm::normalize(State->Forward);
    State.Get().Right = glm::cross(State->Forward, kUp);
    State.Get().Right = glm::normalize(State->Right);
    State.Get().Up = glm::cross(State->Forward, State->Right);
    State.Get().Up = glm::normalize(State->Up);
}

void Camera::SetFov(float fov)
{
    State.Get().TanHalfFov = std::tan(glm::radians(fov) * 0.5f);
}

void Camera::Upload(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass)
{
    State.Upload(device, copyPass);
}

SDL_GPUBuffer* Camera::GetBuffer() const
{
    return State.GetBuffer();
}

void Camera::SetPosition(const glm::vec3& position)
{
    State.Get().Position = position;
}

const glm::vec3& Camera::GetPosition() const
{
    return State->Position;
}

const glm::vec3& Camera::GetDirection() const
{
    return State->Forward;
}

int Camera::GetWidth() const
{
    return Width;
}

int Camera::GetHeight() const
{
    return Height;
}

bool Camera::GetDirty() const
{
    return State.GetDirty();
}