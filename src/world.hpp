#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

#include "block.hpp"
#include "buffer.hpp"
#include "camera.hpp"
#include "chunk.hpp"
#include "config.h"

class World;

struct WorldSetBlockJob
{
    WorldSetBlockJob(const glm::ivec3& position, Block block);

    uint16_t X;
    uint16_t Y;
    uint16_t Z;
    Block Value;
    uint8_t Padding;
};

struct WorldSetChunkJob
{
    WorldSetChunkJob(int inX, int inZ, int outX, int outZ);

    uint8_t InX;
    uint8_t InZ;
    uint8_t OutX;
    uint8_t OutZ;
};

static_assert(sizeof(WorldSetBlockJob) == 8);
static_assert(sizeof(WorldSetChunkJob) == 4);

struct WorldOptions
{
    WorldOptions();

    glm::vec3 SkyBottom;
    int32_t MaxSteps;
    glm::vec3 SkyTop;
    int32_t MaxBounces;
};

struct WorldState
{
    WorldOptions Options;
    int32_t X;
    int32_t Z;
};

class WorldProxy
{
public:
    WorldProxy(World& handle, DynamicBuffer<WorldSetBlockJob>& buffer, int chunkX, int chunkZ);
    void Clear();
    void SetBlock(glm::ivec3 position, Block block);

private:
    World& Handle;
    DynamicBuffer<WorldSetBlockJob>& Buffer;
    int X;
    int Z;
};

struct WorldQuery
{
    Block HitBlock;
    glm::ivec3 Position;
    glm::ivec3 PreviousPosition;
};

class World
{
private:
    friend class WorldProxy;

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
    Block GetBlock(glm::ivec3 position) const;
    WorldQuery Raycast(const glm::vec3& position, const glm::vec3& direction, float length);
    void SetOptions(const WorldOptions& options);

private:
    bool WorldToLocalPosition(glm::ivec3& position) const;

private:
    SDL_GPUDevice* Device;
    Block Blocks[kWidth * Chunk::kWidth][Chunk::kHeight][kWidth * Chunk::kWidth];
    Chunk Chunks[kWidth][kWidth];
    glm::ivec2 ChunkMap[kWidth][kWidth];
    DynamicBuffer<WorldSetBlockJob> SetBlocksBuffer;
    DynamicBuffer<WorldSetChunkJob> SetChunksBuffer;
    std::vector<glm::ivec2> ClearChunks;
    StaticBuffer<WorldState> WorldStateBuffer;
    StaticBuffer<BlockState> BlockStateBuffer;
    SDL_GPUTexture* BlockTexture;
    SDL_GPUTexture* ChunkTexture;
    SDL_GPUTexture* ColorTexture;
    SDL_GPUComputePipeline* SetBlocksPipeline;
    SDL_GPUComputePipeline* SetChunksPipeline;
    SDL_GPUComputePipeline* ClearBlocksPipeline;
    SDL_GPUComputePipeline* RaytracePipeline;
    SDL_GPUComputePipeline* ClearTexturePipeline;
    SDL_GPUComputePipeline* SampleTexturePipeline;
    int Width;
    int Height;
    bool Dirty;
    int Sample;
};