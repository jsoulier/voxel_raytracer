#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    Camera();
    void Resize(float width, float height);
    void Move(float dx, float dy, float dz);
    void Rotate(float dx, float dy);
    void SetFov(float fov);
    const glm::vec3& GetPosition() const;
    const glm::vec3& GetForwardVector() const;
    const glm::vec3& GetRightVector() const;
    const glm::vec3& GetUpVector() const;
    float GetAspectRatio() const;
    float GetTanHalfFov() const;

private:
    glm::vec3 Position;
    glm::vec3 ForwardVector;
    glm::vec3 RightVector;
    float Pitch;
    float Yaw;
    float AspectRatio;
    float TanHalfFov;
};