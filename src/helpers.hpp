#pragma once

#include <SDL3/SDL.h>

#define DebugGroup(commandBuffer) DebugGroupClass debugGroup(commandBuffer, SDL_FUNCTION)
#define DebugGroupBlock(commandBuffer, name) DebugGroupClass debugGroup(commandBuffer, name)

class DebugGroupClass
{
public:
    DebugGroupClass(SDL_GPUCommandBuffer* commandBuffer, const char* name);
    ~DebugGroupClass();

private:
    SDL_GPUCommandBuffer* CommandBuffer;
};

SDL_GPUShader* LoadShader(SDL_GPUDevice* device, const char* name);
SDL_GPUComputePipeline* LoadComputePipeline(SDL_GPUDevice* device, const char* name);