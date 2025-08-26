#include <SDL3/SDL.h>

#include "chunk.hpp"
#include "profile.hpp"
#include "world.hpp"

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
        info.height = kHeight * Chunk::kHeight;
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
        info.width = Chunk::kWidth;
        info.height = Chunk::kHeight;
        info.layer_count_or_depth = Chunk::kWidth;
        ChunkTexture = SDL_CreateGPUTexture(device, &info);
        if (!ChunkTexture)
        {
            SDL_Log("Failed to create chunk texture: %s", SDL_GetError());
            return false;
        }
    }
    return true;
}

void World::Quit(SDL_GPUDevice* device)
{
    Profile();
    SDL_ReleaseGPUTexture(device, ChunkTexture);
    SDL_ReleaseGPUTexture(device, HeightmapTexture);
    SDL_ReleaseGPUTexture(device, BlockTexture);
}