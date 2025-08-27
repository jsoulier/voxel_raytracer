#pragma once

#ifndef NDEBUG
#include <SDL3/SDL.h>
#define DebugGroup(commandBuffer) DebugGroupInternal debugGroup(commandBuffer, __func__)
#define DebugGroupBlock(commandBuffer, name) DebugGroupInternal debugGroup(commandBuffer, name)
#else
#define DebugGroup(commandBuffer)
#define DebugGroupBlock(commandBuffer, name)
#endif

class DebugGroupInternal
{
public:
    DebugGroupInternal(SDL_GPUCommandBuffer* commandBuffer, const char* name);
    ~DebugGroupInternal();

private:
    SDL_GPUCommandBuffer* CommandBuffer;
};