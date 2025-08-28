#pragma once

#include <SDL3/SDL.h>

#include "block.hpp"
#include "buffer.hpp"
#include "camera.hpp"
#include "chunk.hpp"
#include "worker.hpp"

struct WorldBlockJob
{
    WorldBlockJob(int x, int y, int z, Block block)
        : X(x), Y(y), Z(z), Value(block) {}

    uint16_t X;
    uint16_t Y;
    uint16_t Z;
    Block Value;
    uint8_t Padding;
};

struct WorldHeightmapJob
{
    WorldHeightmapJob(int x, int y, int z)
        : X(x), Y(y), Z(z) {}

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

static_assert(sizeof(WorldBlockJob) == 8);;
static_assert(sizeof(WorldHeightmapJob) == 8);;

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
    void Destroy();
    void Update(Camera& camera);
    void Dispatch(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* colorTexture, Camera& camera);
    void SetBlock(int x, int y, int z, Block block);

private:
    SDL_GPUDevice* Device;
    Block Blocks[kWidth * Chunk::kWidth][Chunk::kHeight][kWidth * Chunk::kWidth];
    uint8_t Heightmap[kWidth * Chunk::kWidth][kWidth * Chunk::kWidth];
    Chunk Chunks[kWidth][kWidth];
    Worker Workers[kWorkers];
    DynamicBuffer<WorldBlockJob> BlockBuffer;
    DynamicBuffer<WorldHeightmapJob> HeightmapBuffer;
    FixedBuffer<WorldState> State;
    SDL_GPUTexture* BlockTexture;
    SDL_GPUTexture* HeightmapTexture;
    SDL_GPUTexture* ChunkTexture;
    SDL_GPUComputePipeline* WorldSetBlocksPipeline;
    SDL_GPUComputePipeline* RayTracePipeline;
};