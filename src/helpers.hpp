#pragma once

#include <SDL3/SDL.h>

#ifndef NDEBUG
#include <tracy/Tracy.hpp>
#define DebugGroup(commandBuffer) DebugGroupInternal debugGroup(commandBuffer, SDL_FUNCTION)
#define DebugGroupBlock(commandBuffer, name) DebugGroupInternal debugGroup(commandBuffer, name)
#define Profile() ZoneScoped
#define ProfileBlock(name) ZoneScopedN(name)
#else
#define DebugGroup(commandBuffer)
#define DebugGroupBlock(commandBuffer, name)
#define Profile()
#define ProfileBlock(name)
#endif

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
SDL_GPUTexture* LoadTexture(SDL_GPUDevice* device, const char* name);