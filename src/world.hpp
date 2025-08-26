#pragma once

#include <SDL3/SDL.h>

#include "buffer.hpp"
#include "chunk.hpp"

class World
{
public:
    static constexpr int kWidth = 32;
    static constexpr int kHeight = 8;

    World() = default;
    World(const World& other) = delete;
    World& operator=(const World& other) = delete;
    World(World&& other) = delete;
    World& operator=(World&& other) = delete;
    bool Init(SDL_GPUDevice* device);
    void Quit(SDL_GPUDevice* device);

private:
    // Contains all blocks
    SDL_GPUTexture* BlockTexture;

    // Contains height of all blocks
    SDL_GPUTexture* HeightmapTexture;

    // Maps world space chunk positions to the BlockTexture
    SDL_GPUTexture* ChunkTexture;
};