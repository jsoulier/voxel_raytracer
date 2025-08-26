#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

#include "camera.hpp"

static constexpr float kMaxPitch = glm::pi<float>() / 2.0f - 0.01f;
static constexpr glm::vec3 kUpVector = glm::vec3{0.0f, 1.0f, 0.0f};

Camera::Camera()
    : Position{}
    , ForwardVector{}
    , RightVector{}
    , Pitch{0.0f}
    , Yaw{0.0f}
    , AspectRatio{}
    , TanHalfFov{}
{
    Rotate(0.0f, 0.0f);
    Resize(1.0f, 1.0f);
    SetFov(60.0f);
}

void Camera::Resize(float width, float height)
{
    AspectRatio = width / height;
}

void Camera::Move(float dx, float dy, float dz)
{
    Position += RightVector * glm::vec3(dx);
    Position += ForwardVector * glm::vec3(dz);
    Position += kUpVector * glm::vec3(dy);
}

void Camera::Rotate(float dx, float dy)
{
    Yaw += dx;
    Pitch = std::clamp(Pitch + dy, -kMaxPitch, kMaxPitch);
    ForwardVector.x = std::cos(Pitch) * std::cos(Yaw);
    ForwardVector.y = std::sin(Pitch);
    ForwardVector.z = std::cos(Pitch) * std::sin(Yaw);
    ForwardVector = glm::normalize(ForwardVector);
    RightVector = glm::cross(ForwardVector, kUpVector);
    RightVector = glm::normalize(RightVector);
}

void Camera::SetFov(float fov)
{
    TanHalfFov = std::tan(glm::radians(fov) * 0.5f);
}

const glm::vec3& Camera::GetPosition() const
{
    return Position;
}

const glm::vec3& Camera::GetForwardVector() const
{
    return ForwardVector;
}

const glm::vec3& Camera::GetRightVector() const
{
    return RightVector;
}

const glm::vec3& Camera::GetUpVector() const
{
    return kUpVector;
}

float Camera::GetAspectRatio() const
{
    return AspectRatio;
}

float Camera::GetTanHalfFov() const
{
    return TanHalfFov;
}