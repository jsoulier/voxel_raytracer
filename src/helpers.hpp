#pragma once

#include <SDL3/SDL.h>

#include <string_view>

SDL_GPUShader* LoadShader(SDL_GPUDevice* device, const std::string_view& name);
SDL_GPUComputePipeline* LoadComputePipeline(SDL_GPUDevice* device, const std::string_view& name);