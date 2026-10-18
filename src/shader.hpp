#pragma once

#include <SDL3/SDL.h>

SDL_GPUShader* LoadShader(SDL_GPUDevice* device, const char* name);
SDL_GPUComputePipeline* LoadComputePipeline(SDL_GPUDevice* device, const char* name);
