#include <SDL3/SDL.h>

#include <cmath>
#include <cstring>
#include <vector>

#include "block.hpp"
#include "camera.hpp"
#include "chunk.hpp"
#include "config.h"
#include "debug_group.hpp"
#include "helpers.hpp"
#include "profile.hpp"
#include "world.hpp"

static int FloorChunkIndex(int index)
{
    return std::floorf(float(index) / float(Chunk::kWidth));
}

static void TestFloorChunkIndex()
{
    SDL_assert(FloorChunkIndex(0) == 0);
    SDL_assert(FloorChunkIndex(Chunk::kWidth - 1) == 0);
    SDL_assert(FloorChunkIndex(Chunk::kWidth) == 1);
    SDL_assert(FloorChunkIndex(Chunk::kWidth + 1) == 1);
    SDL_assert(FloorChunkIndex(-1) == -1);
    SDL_assert(FloorChunkIndex(-Chunk::kWidth + 1) == -1);
    SDL_assert(FloorChunkIndex(-Chunk::kWidth) == -1);
    SDL_assert(FloorChunkIndex(-Chunk::kWidth - 1) == -2);
}

WorldSetBlockJob::WorldSetBlockJob(const glm::ivec3& position, Block block)
    : X(position.x)
    , Y(position.y)
    , Z(position.z)
    , Value{block}
{
}

WorldSetChunkJob::WorldSetChunkJob(int inX, int inZ, int outX, int outZ)
    : InX(inX)
    , InZ(inZ)
    , OutX(outX)
    , OutZ(outZ)
{
}

World::World()
    : Blocks{}
    , Chunks{}
    , SetBlocksBuffer{}
    , WorldStateBuffer{}
    , BlockTexture{nullptr}
    , ChunkTexture{nullptr}
    , WorldSetBlocksPipeline{nullptr}
    , WorldSetChunksPipeline{nullptr}
    , RayTracePipeline{nullptr}
{
}

bool World::Init(SDL_GPUDevice* device)
{
    Profile();
    Device = device;
    {
        // TODO: need to zero out the textures
        ProfileBlock("World::Init::Textures");
        SDL_GPUTextureCreateInfo info{};
        info.format = SDL_GPU_TEXTUREFORMAT_R8_UINT;
        info.type = SDL_GPU_TEXTURETYPE_3D;
        info.usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE;
        info.width = kWidth * Chunk::kWidth;
        info.height = Chunk::kHeight;
        info.layer_count_or_depth = kWidth * Chunk::kWidth;
        info.num_levels = 1;
        BlockTexture = SDL_CreateGPUTexture(Device, &info);
        if (!BlockTexture)
        {
            SDL_Log("Failed to create block texture: %s", SDL_GetError());
            return false;
        }
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8_UINT;
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.width = kWidth;
        info.height = kWidth;
        info.layer_count_or_depth = 1;
        ChunkTexture = SDL_CreateGPUTexture(Device, &info);
        if (!ChunkTexture)
        {
            SDL_Log("Failed to create chunk texture: %s", SDL_GetError());
            return false;
        }
    }
    {
        ProfileBlock("World::Init::Pipelines");
        WorldSetBlocksPipeline = LoadComputePipeline(Device, "world_set_blocks.comp");
        if (!WorldSetBlocksPipeline)
        {
            SDL_Log("Failed to load world set blocks pipeline");
            return false;
        }
        WorldSetChunksPipeline = LoadComputePipeline(Device, "world_set_chunks.comp");
        if (!WorldSetChunksPipeline)
        {
            SDL_Log("Failed to load world set chunks pipeline");
            return false;
        }
        WorldClearBlocksPipeline = LoadComputePipeline(Device, "clear_blocks.comp");
        if (!WorldClearBlocksPipeline)
        {
            SDL_Log("Failed to load world clear blocks pipeline");
            return false;
        }
        RayTracePipeline = LoadComputePipeline(Device, "ray_trace.comp");
        if (!RayTracePipeline)
        {
            SDL_Log("Failed to load ray trace pipeline");
            return false;
        }
    }
    {
        ProfileBlock("World::Init::Other");
        if (!WorldStateBuffer.Init(Device))
        {
            SDL_Log("Failed to initialize world state");
            return false;
        }
        if (!BlockStateBuffer.Init(Device))
        {
            SDL_Log("Failed to initialize block state");
            return false;
        }
        BlockStateBuffer.Get() = BlockGetState();
        WorldStateBuffer.Get().X = 0;
        WorldStateBuffer.Get().Z = 0;
        for (int x = 0; x < kWidth; x++)
        for (int z = 0; z < kWidth; z++)
        {
            Chunks[x][z].AddFlags(ChunkFlagsGenerate);
            ChunkMap[x][z] = {x, z};
            SetChunksBuffer.Emplace(device, x, z, x, z);
        }
        AtlasTexture = LoadTexture(Device, "atlas.png");
        if (!AtlasTexture)
        {
            SDL_Log("Failed to load atlas texture");
            return false;
        }
        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        if (!commandBuffer)
        {
            SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
            return false;
        }
        Dispatch(commandBuffer);
        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }
    return true;
}

void World::Destroy()
{
    Profile();
    BlockStateBuffer.Destroy(Device);
    WorldStateBuffer.Destroy(Device);
    SetChunksBuffer.Destroy(Device);
    SetBlocksBuffer.Destroy(Device);
    SDL_ReleaseGPUComputePipeline(Device, RayTracePipeline);
    SDL_ReleaseGPUComputePipeline(Device, WorldSetBlocksPipeline);
    SDL_ReleaseGPUComputePipeline(Device, WorldSetChunksPipeline);
    SDL_ReleaseGPUComputePipeline(Device, WorldClearBlocksPipeline);
    SDL_ReleaseGPUTexture(Device, ChunkTexture);
    SDL_ReleaseGPUTexture(Device, BlockTexture);
    SDL_ReleaseGPUTexture(Device, AtlasTexture);
}

void World::Update(Camera& camera)
{
    Profile();
    int cameraX = camera.GetPosition().x / Chunk::kWidth - kWidth / 2;
    int cameraZ = camera.GetPosition().z / Chunk::kWidth - kWidth / 2;
    int offsetX = cameraX - WorldStateBuffer->X;
    int offsetZ = cameraZ - WorldStateBuffer->Z;
    if (offsetX || offsetZ)
    {
        // TODO: refactor
        WorldStateBuffer.Get().X = cameraX;
        WorldStateBuffer.Get().Z = cameraZ;
        static constexpr int kNull = -1;
        glm::ivec2 chunkMap[kWidth][kWidth];
        std::vector<glm::ivec2> outOfBoundsChunks;
        outOfBoundsChunks.reserve(kWidth * kWidth);
        for (int x = 0; x < kWidth; x++)
        for (int z = 0; z < kWidth; z++)
        {
            chunkMap[x][z].x = kNull;
        }
        for (int x = 0; x < kWidth; x++)
        for (int z = 0; z < kWidth; z++)
        {
            int newX = x - offsetX;
            int newZ = z - offsetZ;
            if (newX < 0 || newZ < 0 || newX >= kWidth || newZ >= kWidth)
            {
                outOfBoundsChunks.push_back(ChunkMap[x][z]);
            }
            else
            {
                chunkMap[newX][newZ] = ChunkMap[x][z];
            }
        }
        std::memcpy(ChunkMap, chunkMap, sizeof(ChunkMap));
        for (int x = 0; x < kWidth; x++)
        for (int z = 0; z < kWidth; z++)
        {
            if (ChunkMap[x][z].x != kNull)
            {
                SetChunksBuffer.Emplace(Device, x, z, ChunkMap[x][z].x, ChunkMap[x][z].y);
                continue;
            }
            glm::ivec2 position = outOfBoundsChunks.back();
            outOfBoundsChunks.pop_back();
            ChunkMap[x][z] = position;
            Chunk& chunk = Chunks[position.x][position.y];
            chunk.AddFlags(ChunkFlagsGenerate);
            SetChunksBuffer.Emplace(Device, x, z, position.x, position.y);
        }
        SDL_assert(outOfBoundsChunks.empty());
    }
    // TODO:
    // 1. sort chunk indices from the center outwards
    // 2. hold onto current index and reset to zero when e.g. chunks move, chunks modified, etc
    for (int inX = 0; inX < kWidth; inX++)
    for (int inZ = 0; inZ < kWidth; inZ++)
    {
        int outX = ChunkMap[inX][inZ].x;
        int outZ = ChunkMap[inX][inZ].y;
        Chunk& chunk = Chunks[outX][outZ];
        if (chunk.GetFlags() & ChunkFlagsGenerate)
        {
            ClearChunks.emplace_back(inX, inZ);
            chunk.Generate(*this, WorldStateBuffer->X + inX, WorldStateBuffer->Z + inZ);
            return;
        }
    }
}

void World::Dispatch(SDL_GPUCommandBuffer* commandBuffer)
{
    Profile();
    {
        ProfileBlock("World::Render::Upload");
        DebugGroupBlock(commandBuffer, "World::Render::Upload");
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        if (!copyPass)
        {
            SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
            return;
        }
        WorldStateBuffer.Upload(Device, copyPass);
        BlockStateBuffer.Upload(Device, copyPass);
        SetBlocksBuffer.Upload(Device, copyPass);
        SetChunksBuffer.Upload(Device, copyPass);
        SDL_EndGPUCopyPass(copyPass);
    }
    if (SetChunksBuffer.GetSize())
    {
        ProfileBlock("World::Render::SetChunks");
        DebugGroupBlock(commandBuffer, "World::Render::SetChunks");
        SDL_GPUStorageTextureReadWriteBinding writeTexture{};
        writeTexture.texture = ChunkTexture;
        SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &writeTexture, 1, nullptr, 0);
        if (!computePass)
        {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            return;
        }
        int numJobs = SetChunksBuffer.GetSize();
        int groupsX = (numJobs + WORLD_SET_CHUNKS_THREADS_X - 1) / WORLD_SET_CHUNKS_THREADS_X;
        SDL_GPUBuffer* readBuffers[1]{};
        readBuffers[0] = SetChunksBuffer.GetBuffer();
        SDL_BindGPUComputePipeline(computePass, WorldSetChunksPipeline);
        SDL_BindGPUComputeStorageBuffers(computePass, 0, readBuffers, 1);
        SDL_PushGPUComputeUniformData(commandBuffer, 0, &numJobs, sizeof(numJobs));
        SDL_DispatchGPUCompute(computePass, groupsX, 1, 1);
        SDL_EndGPUComputePass(computePass);
    }
    if (!ClearChunks.empty())
    {
        ProfileBlock("World::Render::ClearBlocks");
        DebugGroupBlock(commandBuffer, "World::Render::ClearBlocks");
        SDL_GPUStorageTextureReadWriteBinding writeTexture{};
        writeTexture.texture = BlockTexture;
        SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &writeTexture, 1, nullptr, 0);
        if (!computePass)
        {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            return;
        }
        int groupsX = (Chunk::kWidth + CLEAR_BLOCKS_THREADS_X - 1) / CLEAR_BLOCKS_THREADS_X;
        int groupsY = (Chunk::kHeight + CLEAR_BLOCKS_THREADS_Y - 1) / CLEAR_BLOCKS_THREADS_Y;
        SDL_GPUTexture* readTextures[1]{};
        readTextures[0] = ChunkTexture;
        SDL_BindGPUComputePipeline(computePass, WorldClearBlocksPipeline);
        SDL_BindGPUComputeStorageTextures(computePass, 0, readTextures, 1);
        for (glm::ivec2 position : ClearChunks)
        {
            SDL_PushGPUComputeUniformData(commandBuffer, 0, &position, sizeof(position));
            SDL_DispatchGPUCompute(computePass, groupsX, groupsY, groupsX);
        }
        ClearChunks.clear();
        SDL_EndGPUComputePass(computePass);
    }
    if (SetBlocksBuffer.GetSize())
    {
        ProfileBlock("World::Render::SetBlocks");
        DebugGroupBlock(commandBuffer, "World::Render::SetBlocks");
        SDL_GPUStorageTextureReadWriteBinding writeTexture{};
        writeTexture.texture = BlockTexture;
        SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &writeTexture, 1, nullptr, 0);
        if (!computePass)
        {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            return;
        }
        int numJobs = SetBlocksBuffer.GetSize();
        int groupsX = (numJobs + WORLD_SET_BLOCKS_THREADS_X - 1) / WORLD_SET_BLOCKS_THREADS_X;
        SDL_GPUBuffer* readBuffers[1]{};
        readBuffers[0] = SetBlocksBuffer.GetBuffer();
        SDL_BindGPUComputePipeline(computePass, WorldSetBlocksPipeline);
        SDL_BindGPUComputeStorageBuffers(computePass, 0, readBuffers, 1);
        SDL_PushGPUComputeUniformData(commandBuffer, 0, &numJobs, sizeof(numJobs));
        SDL_DispatchGPUCompute(computePass, groupsX, 1, 1);
        SDL_EndGPUComputePass(computePass);
    }
}

void World::Render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* colorTexture, Camera& camera)
{
    Profile();
    DebugGroup(commandBuffer);
    SDL_GPUStorageTextureReadWriteBinding writeTexture{};
    writeTexture.texture = colorTexture;
    SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &writeTexture, 1, nullptr, 0);
    if (!computePass)
    {
        SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
        return;
    }
    int groupsX = (camera.GetWidth() + RAY_TRACE_THREADS_X - 1) / RAY_TRACE_THREADS_X;
    int groupsY = (camera.GetHeight() + RAY_TRACE_THREADS_Y - 1) / RAY_TRACE_THREADS_Y;
    SDL_GPUTexture* readTextures[2]{};
    SDL_GPUBuffer* readBuffers[3]{};
    readTextures[0] = BlockTexture;
    readTextures[1] = ChunkTexture;
    readBuffers[0] = camera.GetBuffer();
    readBuffers[1] = WorldStateBuffer.GetBuffer();
    readBuffers[2] = BlockStateBuffer.GetBuffer();
    SDL_BindGPUComputePipeline(computePass, RayTracePipeline);
    SDL_BindGPUComputeStorageTextures(computePass, 0, readTextures, 2);
    SDL_BindGPUComputeStorageBuffers(computePass, 0, readBuffers, 3);
    SDL_DispatchGPUCompute(computePass, groupsX, groupsY, 1);
    SDL_EndGPUComputePass(computePass);
}

void World::SetBlock(glm::ivec3 position, Block block)
{
    position.x -= WorldStateBuffer->X * Chunk::kWidth;
    position.z -= WorldStateBuffer->Z * Chunk::kWidth;
    glm::ivec2 chunk = {position.x, position.z};
    chunk.x = FloorChunkIndex(chunk.x);
    chunk.y = FloorChunkIndex(chunk.y);
    position.x -= chunk.x * Chunk::kWidth;
    position.z -= chunk.y * Chunk::kWidth;
    chunk = ChunkMap[chunk.x][chunk.y];
    position.x += chunk.x * Chunk::kWidth;
    position.z += chunk.y * Chunk::kWidth;
    SetBlocksBuffer.Emplace(Device, position, block);
    // TODO: set CPU-side blocks
}