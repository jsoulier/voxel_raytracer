#include <SDL3/SDL.h>

#include "debug_group.hpp"

DebugGroupInternal::DebugGroupInternal(SDL_GPUCommandBuffer* commandBuffer, const char* name)
    : CommandBuffer{commandBuffer}
{
    SDL_PushGPUDebugGroup(CommandBuffer, name);
}

DebugGroupInternal::~DebugGroupInternal()
{
    SDL_PopGPUDebugGroup(CommandBuffer);
}