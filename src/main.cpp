#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdint>

#include "profile.hpp"

static SDL_Window* window;
static SDL_GPUDevice* device;
static uint32_t oldWidth;
static uint32_t oldHeight;

static bool Init()
{
    Profile();
#ifndef NDEBUG
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
#endif
    SDL_SetAppMetadata("Voxel Ray Tracer", nullptr, nullptr);
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }
    window = SDL_CreateWindow("Voxel Ray Tracer", 960, 720, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return false;
    }
    // Skipping d3d12 since there's issues with debug groups
#ifndef NDEBUG
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, true, nullptr);
#else
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, nullptr);
#endif
    if (!device)
    {
        SDL_Log("Failed to create device: %s", SDL_GetError());
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        SDL_Log("Failed to claim window: %s", SDL_GetError());
        return false;
    }
    return true;
}

static void Quit()
{
    Profile();
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static bool Poll()
{
    Profile();
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            return false;
        }
    }
    return true;
}

static void Update()
{
    Profile();
}

static void Render()
{
    Profile();
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!commandBuffer)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }
    SDL_GPUTexture* swapchainTexture;
    uint32_t newWidth;
    uint32_t newHeight;
    if (!SDL_AcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &newWidth, &newHeight))
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commandBuffer);
        return;
    }
    if (!swapchainTexture || !newWidth || !newHeight)
    {
        // Not an error
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return;
    }
    if (oldWidth != newWidth || oldHeight != newHeight)
    {
        // TODO:
    }
    SDL_SubmitGPUCommandBuffer(commandBuffer);
}

int main(int argc, char** argv)
{
    Profile();
    if (!Init())
    {
        return 1;
    }
    while (true)
    {
        if (!Poll())
        {
            break;
        }
        Update();
        Render();
    }
    Quit();
    return 0;
}