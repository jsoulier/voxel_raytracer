#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

#include "camera.hpp"

static constexpr float kMaxPitch = glm::pi<float>() / 2.0f - 0.01f;
static constexpr float kEpsilon = glm::epsilon<float>();
static constexpr float kNear = 0.1f;
static constexpr float kFar = 10000.0f;
static constexpr float kReach = 10.0f;
static constexpr glm::vec3 kUp = glm::vec3{0.0f, 1.0f, 0.0f};

Camera::Camera()
    : State{}
    , Query{}
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
    State.Get().Position.y = 64.0f;
    Rotate(0.0f, -0.1f);
    Resize(1, 1);
    State.Get().Fov = glm::radians(60.0f);
    State.Get().TanHalfFov = std::tan(State->Fov * 0.5f);
    return true;
}

void Camera::Destroy(SDL_GPUDevice* device)
{
    State.Destroy(device);
}

void Camera::Upload(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass)
{
    State.Upload(device, copyPass);
}

void Camera::Update(const World& world)
{
    Query = world.Raycast(GetPosition(), GetForward(), kReach);
}

void Camera::Resize(int width, int height)
{
    if (Width == width && Height == height)
    {
        return;
    }
    Width = width;
    Height = height;
    State.Get().AspectRatio = float(Width) / Height;
    UpdateViewProjection();
}

void Camera::Move(float dx, float dy, float dz)
{
    if (glm::length(glm::vec3(dx, dy, dz)) < kEpsilon)
    {
        return;
    }
    State.Get().Position += State->Right * dx;
    State.Get().Position += State->Forward * dz;
    State.Get().Position += kUp * dy;
    UpdateViewProjection();
}

void Camera::Rotate(float dx, float dy)
{
    if (std::abs(dx) < kEpsilon && std::abs(dy) < kEpsilon)
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
    UpdateViewProjection();
}

SDL_GPUBuffer* Camera::GetBuffer() const
{
    return State.GetBuffer();
}

Block Camera::GetHitBlock() const
{
    return Query.HitBlock;
}

const glm::ivec3& Camera::GetHitPosition() const
{
    return Query.Position;
}

const glm::ivec3& Camera::GetPreviousHitPosition() const
{
    return Query.PreviousPosition;
}

const glm::vec3& Camera::GetPosition() const
{
    return State->Position;
}

const glm::vec3& Camera::GetForward() const
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

void Camera::UpdateViewProjection()
{
    CameraState& state = State.Get();
    glm::mat4 view = glm::lookAt(state.Position, state.Position + state.Forward, state.Up);
    glm::mat4 projection = glm::perspective(state.Fov, state.AspectRatio, kNear, kFar);
    state.ViewProjection = projection * view;
}
