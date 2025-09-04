#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>

#include <cstdint>

#include "camera.hpp"
#include "debug_group.hpp"
#include "noise.hpp"
#include "profile.hpp"
#include "world.hpp"

static constexpr float kSpeed = 0.035f;
static constexpr float kSensitivity = 0.001f;
static constexpr float kWidth = 640.0f;

static SDL_Window* window;
static SDL_GPUDevice* device;
static SDL_GPUTexture* colorTexture;
static uint32_t swapchainWidth;
static uint32_t swapchainHeight;
static Camera camera;
static World world;
static uint64_t time1;
static uint64_t time2;
static float dt;
static bool focus;

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
    window = SDL_CreateWindow("Voxel Ray Tracer", 960, 720, SDL_WINDOW_HIDDEN);
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
    if (!world.Init(device))
    {
        SDL_Log("Failed to initialize world");
        return false;
    }
    if (!camera.Init(device))
    {
        SDL_Log("Failed to initialize camera");
        return false;
    }
    if (SDL_WindowSupportsGPUPresentMode(device, window, SDL_GPU_PRESENTMODE_MAILBOX))
    {
        SDL_SetGPUSwapchainParameters(device, window,
            SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_MAILBOX);
    }
    SDL_ShowWindow(window);
    SDL_SetWindowResizable(window, true);
    SDL_FlashWindow(window, SDL_FLASH_BRIEFLY);
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplSDL3_InitForSDLGPU(window);
        ImGui_ImplSDLGPU3_InitInfo info{};
        info.Device = device;
        info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
        ImGui_ImplSDLGPU3_Init(&info);
    }
    return true;
}

static void Quit()
{
    Profile();
    SDL_HideWindow(window);
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    camera.Destroy(device);
    world.Destroy();
    SDL_ReleaseGPUTexture(device, colorTexture);
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
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            return false;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button & SDL_BUTTON_LMASK)
            {
                if (!SDL_GetWindowRelativeMouseMode(window))
                {
                    focus = true;
                }
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (SDL_GetWindowRelativeMouseMode(window))
            {
                camera.Rotate(event.motion.xrel * kSensitivity, event.motion.yrel * kSensitivity);
            }
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.scancode == SDL_SCANCODE_ESCAPE)
            {
                SDL_SetWindowRelativeMouseMode(window, false);
                if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)
                {
                    SDL_SetWindowFullscreen(window, false);
                }
            }
            else if (event.key.scancode == SDL_SCANCODE_F11)
            {
                if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)
                {
                    SDL_SetWindowFullscreen(window, false);
                }
                else
                {
                    SDL_SetWindowFullscreen(window, true);
                    if (!SDL_GetWindowRelativeMouseMode(window))
                    {
                        focus = true;
                    }
                }
            }
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            if (SDL_GetWindowRelativeMouseMode(window))
            {
                SDL_SetWindowRelativeMouseMode(window, false);
            }
            break;
        }
    }
    return true;
}

static void Update()
{
    Profile();
    if (SDL_GetWindowRelativeMouseMode(window))
    {
        float dx = 0.0f;
        float dy = 0.0f;
        float dz = 0.0f;
        float speed = kSpeed * dt;
        const bool* keys = SDL_GetKeyboardState(nullptr);
        dx += keys[SDL_SCANCODE_D];
        dx -= keys[SDL_SCANCODE_A];
        dy -= keys[SDL_SCANCODE_Q];
        dy += keys[SDL_SCANCODE_E];
        dz += keys[SDL_SCANCODE_W];
        dz -= keys[SDL_SCANCODE_S];
        if (keys[SDL_SCANCODE_LCTRL])
        {
            speed *= 10.0f;
        }
        else if (keys[SDL_SCANCODE_LSHIFT])
        {
            speed /= 10.0f;
        }
        dx *= speed;
        dy *= speed;
        dz *= speed;
        camera.Move(dx, dy, dz);
    }
    world.Update(camera);
}

static bool Resize(uint32_t width, uint32_t height)
{
    Profile();
    float aspectRatio = float(width) / float(height);
    camera.Resize(kWidth, kWidth / aspectRatio);
    SDL_ReleaseGPUTexture(device, colorTexture);
    SDL_GPUTextureCreateInfo info{};
    info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = camera.GetWidth();
    info.height = camera.GetHeight();
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    colorTexture = SDL_CreateGPUTexture(device, &info);
    if (!colorTexture)
    {
        return false;
    }
    swapchainWidth = width;
    swapchainHeight = height;
    return true;
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
    uint32_t width;
    uint32_t height;
    {
        ProfileBlock("Render::AcquireSwapchainTexture");
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &width, &height))
        {
            SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
            SDL_CancelGPUCommandBuffer(commandBuffer);
            return;
        }
    }
    if (!swapchainTexture || !width || !height)
    {
        // Not an error
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return;
    }
    if ((swapchainWidth != width || swapchainHeight != height) && !Resize(width, height))
    {
        SDL_Log("Failed to create color texture: %s", SDL_GetError());
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return;
    }
    {
        ProfileBlock("Render::PrepareImGui");
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize.x = width;
        io.DisplaySize.y = height;
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        if (focus && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        {
            SDL_SetWindowRelativeMouseMode(window, true);
        }
        focus = false;
        ImGui::Render();
        ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), commandBuffer);
    }
    camera.Upload(device, commandBuffer);
    world.Dispatch(commandBuffer);
    world.Render(commandBuffer, colorTexture, camera);
    {
        ProfileBlock("Render::Blit");
        DebugGroup(commandBuffer);
        SDL_GPUBlitInfo info{};
        info.source.texture = colorTexture;
        info.source.w = camera.GetWidth();
        info.source.h = camera.GetHeight();
        info.destination.texture = swapchainTexture;
        info.destination.w = swapchainWidth;
        info.destination.h = swapchainHeight;
        SDL_BlitGPUTexture(commandBuffer, &info);
    }
    {
        ProfileBlock("Render::RenderImGui");
        SDL_GPUColorTargetInfo info{};
        info.texture = swapchainTexture;
        info.load_op = SDL_GPU_LOADOP_LOAD;
        info.store_op = SDL_GPU_STOREOP_STORE;
        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &info, 1, nullptr);
        if (!renderPass)
        {
            SDL_Log("Failed to begin render pass: %s", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(commandBuffer);
            return;
        }
        ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderPass);
        SDL_EndGPURenderPass(renderPass);
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
    time2 = SDL_GetTicks();
    time1 = time2;
    while (true)
    {
        ProfileBlock("main::Loop");
        time2 = SDL_GetTicks();
        dt = time2 - time1;
        time1 = time2;
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