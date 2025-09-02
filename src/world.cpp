#include <SDL3/SDL.h>

#include "block.hpp"
#include "camera.hpp"
#include "chunk.hpp"
#include "config.h"
#include "debug_group.hpp"
#include "helpers.hpp"
#include "profile.hpp"
#include "world.hpp"

WorldBlockJob::WorldBlockJob(const glm::ivec3& position, Block block)
    : X(position.x)
    , Y(position.y)
    , Z(position.z)
    , Value{block}
{
}

WorldChunkJob::WorldChunkJob(int inX, int inZ, int outX, int outZ)
    : InX(inX)
    , InZ(inZ)
    , OutX(outX)
    , OutZ(outZ)
{
}

World::World()
    : Blocks{}
    , Chunks{}
    , BlockBuffer{}
    , State{}
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
        RayTracePipeline = LoadComputePipeline(Device, "ray_trace.comp");
        if (!RayTracePipeline)
        {
            SDL_Log("Failed to load ray trace pipeline");
            return false;
        }
    }
    {
        ProfileBlock("World::Init::Other");
        if (!State.Init(Device))
        {
            SDL_Log("Failed to initialize state");
            return false;
        }
        State->X = 0;
        State->Z = 0;
        for (int x = 0; x < kWidth; x++)
        for (int z = 0; z < kWidth; z++)
        {
            ChunkMap[x][z] = {x, z};
            ChunkBuffer.Emplace(device, x, z, x, z);
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
    State.Destroy(Device);
    ChunkBuffer.Destroy(Device);
    BlockBuffer.Destroy(Device);
    SDL_ReleaseGPUComputePipeline(Device, RayTracePipeline);
    SDL_ReleaseGPUComputePipeline(Device, WorldSetBlocksPipeline);
    SDL_ReleaseGPUComputePipeline(Device, WorldSetChunksPipeline);
    SDL_ReleaseGPUTexture(Device, ChunkTexture);
    SDL_ReleaseGPUTexture(Device, BlockTexture);
}

void World::Update(Camera& camera)
{
    Profile();
    int cameraX = camera.GetPosition().x / Chunk::kWidth - kWidth / 2;
    int cameraZ = camera.GetPosition().z / Chunk::kWidth - kWidth / 2;
    if (cameraX != State->X || cameraZ != State->Z)
    {
        Chunk chunks[kWidth][kWidth];
        int offsetX = cameraX - State->X;
        int offsetZ = cameraZ - State->Z;
        State->X = cameraX;
        State->Z = cameraZ;
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
            chunk.Generate(*this, inX, inZ);
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
        State.Upload(Device, copyPass);
        BlockBuffer.Upload(Device, copyPass);
        ChunkBuffer.Upload(Device, copyPass);
        SDL_EndGPUCopyPass(copyPass);
    }
    if (ChunkBuffer.GetSize())
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
        int groupsX = (ChunkBuffer.GetSize() + WORLD_SET_CHUNKS_THREADS_X - 1) / WORLD_SET_CHUNKS_THREADS_X;
        SDL_GPUBuffer* readBuffers[1]{};
        readBuffers[0] = ChunkBuffer.GetBuffer();
        SDL_BindGPUComputePipeline(computePass, WorldSetChunksPipeline);
        SDL_BindGPUComputeStorageBuffers(computePass, 0, readBuffers, 1);
        SDL_DispatchGPUCompute(computePass, groupsX, 1, 1);
        SDL_EndGPUComputePass(computePass);
    }
    if (BlockBuffer.GetSize())
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
        int groupsX = (BlockBuffer.GetSize() + WORLD_SET_BLOCKS_THREADS_X - 1) / WORLD_SET_BLOCKS_THREADS_X;
        SDL_GPUBuffer* readBuffers[1]{};
        readBuffers[0] = BlockBuffer.GetBuffer();
        SDL_BindGPUComputePipeline(computePass, WorldSetBlocksPipeline);
        SDL_BindGPUComputeStorageBuffers(computePass, 0, readBuffers, 1);
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
    SDL_GPUBuffer* readBuffers[2]{};
    readTextures[0] = BlockTexture;
    readTextures[1] = ChunkTexture;
    readBuffers[0] = camera.GetBuffer();
    readBuffers[1] = State.GetBuffer();
    SDL_BindGPUComputePipeline(computePass, RayTracePipeline);
    SDL_BindGPUComputeStorageTextures(computePass, 0, readTextures, 2);
    SDL_BindGPUComputeStorageBuffers(computePass, 0, readBuffers, 2);
    SDL_DispatchGPUCompute(computePass, groupsX, groupsY, 1);
    SDL_EndGPUComputePass(computePass);
}

void World::SetBlock(const glm::ivec3& position, Block block)
{
    BlockBuffer.Emplace(Device, position, block);
}