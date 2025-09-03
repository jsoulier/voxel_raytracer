#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "block.hpp"
#include "buffer.hpp"
#include "camera.hpp"
#include "chunk.hpp"
#include "config.h"

struct WorldBlockJob
{
    WorldBlockJob(const glm::ivec3& position, Block block);

    uint16_t X;
    uint16_t Y;
    uint16_t Z;
    Block Value;
    uint8_t Padding;
};

struct WorldChunkJob
{
    WorldChunkJob(int inX, int inZ, int outX, int outZ);

    uint8_t InX;
    uint8_t InZ;
    uint8_t OutX;
    uint8_t OutZ;
};

struct WorldState
{
    int32_t X;
    int32_t Z;
};

static_assert(sizeof(WorldBlockJob) == 8);;
static_assert(sizeof(WorldChunkJob) == 4);;

class World
{
public:
    static constexpr int kWidth = WORLD_WIDTH;

    World();
    World(const World& other) = delete;
    World& operator=(const World& other) = delete;
    World(World&& other) = delete;
    World& operator=(World&& other) = delete;
    bool Init(SDL_GPUDevice* device);
    void Destroy();
    void Update(Camera& camera);
    void Dispatch(SDL_GPUCommandBuffer* commandBuffer);
    void Render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* colorTexture, Camera& camera);
    void SetBlock(glm::ivec3 position, Block block);

private:
    SDL_GPUDevice* Device;
    Block Blocks[kWidth * Chunk::kWidth][Chunk::kHeight][kWidth * Chunk::kWidth];
    Chunk Chunks[kWidth][kWidth];
    glm::ivec2 ChunkMap[kWidth][kWidth];
    DynamicBuffer<WorldBlockJob> BlockBuffer;
    DynamicBuffer<WorldChunkJob> ChunkBuffer;
    StaticBuffer<WorldState> State;
    SDL_GPUTexture* BlockTexture;
    SDL_GPUTexture* ChunkTexture;
    SDL_GPUComputePipeline* WorldSetBlocksPipeline;
    SDL_GPUComputePipeline* WorldSetChunksPipeline;
    SDL_GPUComputePipeline* RayTracePipeline;
};