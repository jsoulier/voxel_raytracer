#pragma once

#include <SDl3/SDL.h>
#include <glm/glm.hpp>

#include "buffer.hpp"

struct CameraState
{
    glm::vec3 Position;
    float AspectRatio;
    glm::vec3 Forward;
    float TanHalfFov;
    glm::vec3 Right;
    float Padding1;
    glm::vec3 Up;
    float Padding2;
};

class Camera
{
public:
    Camera();
    bool Init(SDL_GPUDevice* device);
    void Destroy(SDL_GPUDevice* device);
    void Resize(float width, float height);
    void Move(float dx, float dy, float dz);
    void Rotate(float dx, float dy, bool force = false);
    void SetFov(float fov);
    void Upload(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass);
    SDL_GPUBuffer* GetBuffer() const;
    void SetPosition(const glm::vec3& position);
    const glm::vec3& GetPosition() const;
    const glm::vec3& GetDirection() const;
    float GetWidth() const;
    float GetHeight() const;
    bool GetDirty() const;

private:
    StaticBuffer<CameraState> State;
    float Pitch;
    float Yaw;
    float Width;
    float Height;
};