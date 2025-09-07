#pragma once

#include <SDL3/SDL.h>

#define DebugGroup(commandBuffer) DebugGroupInternal debugGroup(commandBuffer, SDL_FUNCTION)
#define DebugGroupBlock(commandBuffer, name) DebugGroupInternal debugGroup(commandBuffer, name)

class DebugGroupInternal
{
public:
    DebugGroupInternal(SDL_GPUCommandBuffer* commandBuffer, const char* name)
        : CommandBuffer{commandBuffer}
    {
        SDL_PushGPUDebugGroup(CommandBuffer, name);
    }

    ~DebugGroupInternal()
    {
        SDL_PopGPUDebugGroup(CommandBuffer);
    }

private:
    SDL_GPUCommandBuffer* CommandBuffer;
};

SDL_GPUShader* LoadShader(SDL_GPUDevice* device, const char* name);
SDL_GPUComputePipeline* LoadComputePipeline(SDL_GPUDevice* device, const char* name);