#include <SDL3/SDL.h>

#include "debug_group.hpp"

DebugGroupClass::DebugGroupClass(SDL_GPUCommandBuffer* commandBuffer, const char* name)
    : CommandBuffer{commandBuffer}
{
    SDL_PushGPUDebugGroup(CommandBuffer, name);
}

DebugGroupClass::~DebugGroupClass()
{
    SDL_PopGPUDebugGroup(CommandBuffer);
}
