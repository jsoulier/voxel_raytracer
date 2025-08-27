#pragma once

#include <SDL3/SDL.h>

#include "block.hpp"
#include "buffer.hpp"
#include "camera.hpp"
#include "chunk.hpp"
#include "worker.hpp"

struct WorldBlockJob
{
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
    uint8_t Value;
    uint8_t Padding;
};

struct WorldHeightmapJob
{
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
    uint16_t Padding;
};

struct WorldState
{
    float X;
    float Z;
};

class World
{
public:
    static constexpr int kWidth = 32;
    static constexpr int kWorkers = 1;

    World();
    World(const World& other) = delete;
    World& operator=(const World& other) = delete;
    World(World&& other) = delete;
    World& operator=(World&& other) = delete;
    bool Init(SDL_GPUDevice* device);
    void Quit(SDL_GPUDevice* device);

private:
    Block Blocks[kWidth * Chunk::kWidth][Chunk::kHeight][kWidth * Chunk::kWidth];
    uint8_t Heightmap[kWidth * Chunk::kWidth][kWidth * Chunk::kWidth];
    Chunk Chunks[kWidth][kWidth];
    Worker Workers[kWorkers];
    DynamicBuffer<WorldBlockJob> BlockBuffer;
    DynamicBuffer<WorldHeightmapJob> HeightmapBuffer;
    FixedBuffer<WorldState> State;

    // Contains all blocks
    SDL_GPUTexture* BlockTexture;

    // Contains height of all blocks
    SDL_GPUTexture* HeightmapTexture;

    // Maps world space chunk positions to the BlockTexture
    SDL_GPUTexture* ChunkTexture;
};