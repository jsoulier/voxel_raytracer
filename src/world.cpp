#include <SDL3/SDL.h>

#include "block.hpp"
#include "camera.hpp"
#include "chunk.hpp"
#include "debug_group.hpp"
#include "helpers.hpp"
#include "profile.hpp"
#include "threads.h"
#include "world.hpp"

WorldBlockJob::WorldBlockJob(int x, int y, int z, Block block)
    : X(x)
    , Y(y)
    , Z(z)
    , Value{block}
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
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UINT;
        info.width = kWidth;
        info.height = 1;
        info.layer_count_or_depth = kWidth;
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
        RayTracePipeline = LoadComputePipeline(Device, "ray_trace.comp");
        if (!RayTracePipeline)
        {
            SDL_Log("Failed to load ray trace pipeline");
            return false;
        }
    }
    if (!State.Init(Device))
    {
        SDL_Log("Failed to initialize state");
        return false;
    }
    State->X = 0.0f;
    State->Z = 0.0f;
    return true;
}

void World::Destroy()
{
    Profile();
    State.Destroy(Device);
    BlockBuffer.Destroy(Device);
    SDL_ReleaseGPUComputePipeline(Device, RayTracePipeline);
    SDL_ReleaseGPUComputePipeline(Device, WorldSetBlocksPipeline);
    SDL_ReleaseGPUTexture(Device, ChunkTexture);
    SDL_ReleaseGPUTexture(Device, BlockTexture);
}

void World::Update(Camera& camera)
{
    Profile();
    // TODO:
    // 1. sort chunk indices from the center outwards
    // 2. hold onto current index and reset to zero when e.g. chunks move, chunks modified, etc
    for (int i = 0; i < kWidth; i++)
    for (int j = 0; j < kWidth; j++)
    {
        Chunk& chunk = Chunks[i][j];
        if (chunk.GetFlags() & ChunkFlagsGenerate)
        {
            chunk.Generate(*this, i, j);
            return;
        }
    }
}

void World::Dispatch(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* colorTexture, Camera& camera)
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
        camera.Upload(Device, copyPass);
        SDL_EndGPUCopyPass(copyPass);
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
        SDL_GPUBuffer* readBuffers[2]{};
        readBuffers[0] = BlockBuffer.GetBuffer();
        readBuffers[1] = State.GetBuffer();
        SDL_BindGPUComputePipeline(computePass, WorldSetBlocksPipeline);
        SDL_BindGPUComputeStorageBuffers(computePass, 0, readBuffers, 2);
        SDL_DispatchGPUCompute(computePass, groupsX, 1, 1);
        SDL_EndGPUComputePass(computePass);
    }
    {
        ProfileBlock("World::Render::RayTrace");
        DebugGroupBlock(commandBuffer, "World::Render::RayTrace");
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
        SDL_GPUBuffer* readBuffers[2]{};
        readBuffers[0] = camera.GetBuffer();
        readBuffers[1] = State.GetBuffer();
        SDL_BindGPUComputePipeline(computePass, RayTracePipeline);
        SDL_BindGPUComputeStorageTextures(computePass, 0, &BlockTexture, 1);
        SDL_BindGPUComputeStorageBuffers(computePass, 0, readBuffers, 2);
        SDL_DispatchGPUCompute(computePass, groupsX, groupsY, 1);
        SDL_EndGPUComputePass(computePass);
    }
}

void World::SetBlock(int x, int y, int z, Block block)
{
    BlockBuffer.Emplace(Device, x, y, z, block);
}