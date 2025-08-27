#include <SDL3/SDL.h>

#include "chunk.hpp"
#include "profile.hpp"
#include "world.hpp"

World::World()
    : Blocks{}
    , Heightmap{}
    , Chunks{}
    , Workers{}
    , BlockBuffer{}
    , HeightmapBuffer{}
    , State{}
    , BlockTexture{nullptr}
    , HeightmapTexture{nullptr}
    , ChunkTexture{nullptr}
{
}

bool World::Init(SDL_GPUDevice* device)
{
    Profile();
    {
        ProfileBlock("World::Init::Textures");
        SDL_GPUTextureCreateInfo info{};
        info.format = SDL_GPU_TEXTUREFORMAT_R8_UINT;
        info.type = SDL_GPU_TEXTURETYPE_3D;
        info.usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE;
        info.width = kWidth * Chunk::kWidth;
        info.height = Chunk::kHeight;
        info.layer_count_or_depth = kWidth * Chunk::kWidth;
        info.num_levels = 1;
        BlockTexture = SDL_CreateGPUTexture(device, &info);
        if (!BlockTexture)
        {
            SDL_Log("Failed to create block texture: %s", SDL_GetError());
            return false;
        }
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.height = kWidth * Chunk::kWidth;
        info.layer_count_or_depth = 1;
        HeightmapTexture = SDL_CreateGPUTexture(device, &info);
        if (!HeightmapTexture)
        {
            SDL_Log("Failed to create heightmap texture: %s", SDL_GetError());
            return false;
        }
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UINT;
        info.type = SDL_GPU_TEXTURETYPE_3D;
        info.usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE;
        info.width = kWidth;
        info.height = 1;
        info.layer_count_or_depth = kWidth;
        ChunkTexture = SDL_CreateGPUTexture(device, &info);
        if (!ChunkTexture)
        {
            SDL_Log("Failed to create chunk texture: %s", SDL_GetError());
            return false;
        }
    }
    if (!State.Init(device))
    {
        SDL_Log("Failed to initialize state");
        return false;
    }
    State->X = 0.0f;
    State->Z = 0.0f;
    return true;
}

void World::Quit(SDL_GPUDevice* device)
{
    Profile();
    State.Destroy(device);
    BlockBuffer.Destroy(device);
    HeightmapBuffer.Destroy(device);
    SDL_ReleaseGPUTexture(device, ChunkTexture);
    SDL_ReleaseGPUTexture(device, HeightmapTexture);
    SDL_ReleaseGPUTexture(device, BlockTexture);
}