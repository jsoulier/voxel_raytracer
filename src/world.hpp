#pragma once

#include <SDL3/SDL.h>

#include "block.hpp"
#include "buffer.hpp"
#include "camera.hpp"
#include "chunk.hpp"

struct WorldBlockJob
{
    WorldBlockJob(int x, int y, int z, Block block);

    uint16_t X;
    uint16_t Y;
    uint16_t Z;
    Block Value;
    uint8_t Padding;
};

struct WorldState
{
    float X;
    float Z;
};

static_assert(sizeof(WorldBlockJob) == 8);;

class World
{
public:
    static constexpr int kWidth = 32;

    World();
    World(const World& other) = delete;
    World& operator=(const World& other) = delete;
    World(World&& other) = delete;
    World& operator=(World&& other) = delete;
    bool Init(SDL_GPUDevice* device);
    void Destroy();
    void Update(Camera& camera);
    void Dispatch(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* colorTexture, Camera& camera);
    void SetBlock(int x, int y, int z, Block block);

private:
    SDL_GPUDevice* Device;
    Block Blocks[kWidth * Chunk::kWidth][Chunk::kHeight][kWidth * Chunk::kWidth];
    Chunk Chunks[kWidth][kWidth];
    DynamicBuffer<WorldBlockJob> BlockBuffer;
    StaticBuffer<WorldState> State;
    SDL_GPUTexture* BlockTexture;
    SDL_GPUTexture* ChunkTexture;
    SDL_GPUComputePipeline* WorldSetBlocksPipeline;
    SDL_GPUComputePipeline* RayTracePipeline;
};