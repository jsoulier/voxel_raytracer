#include <SDL3/SDL.h>

#include "debug_group.hpp"
#include "profile.hpp"

DebugGroupClass::DebugGroupClass(SDL_GPUCommandBuffer* commandBuffer, const char* name)
    : CommandBuffer{commandBuffer}
{
    Profile();
    SDL_PushGPUDebugGroup(CommandBuffer, name);
}

DebugGroupClass::~DebugGroupClass()
{
    Profile();
    SDL_PopGPUDebugGroup(CommandBuffer);
}
