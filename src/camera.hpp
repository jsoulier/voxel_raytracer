#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "world.hpp"

struct CameraState
{
    glm::vec3 Position;
    float AspectRatio;
    glm::vec3 Forward;
    float Fov;
    glm::vec3 Right;
    float TanHalfFov;
    glm::vec3 Up;
    float Padding;
    glm::mat4 ViewProjection;
};

class Camera
{
public:
    Camera();
    bool Init(SDL_GPUDevice* device);
    void Destroy(SDL_GPUDevice* device);
    void Upload(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass);
    void Update(const World& world);
    void Resize(int width, int height);
    void Move(float dx, float dy, float dz);
    void Rotate(float dx, float dy);
    SDL_GPUBuffer* GetBuffer() const;
    Block GetHitBlock() const;
    const glm::ivec3& GetHitPosition() const;
    const glm::ivec3& GetPreviousHitPosition() const;
    const glm::vec3& GetPosition() const;
    const glm::vec3& GetForward() const;
    int GetWidth() const;
    int GetHeight() const;
    bool GetDirty() const;

private:
    void UpdateViewProjection();

    StaticBuffer<CameraState> State;
    WorldQuery Query;
    float Pitch;
    float Yaw;
    int Width;
    int Height;
};
