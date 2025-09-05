#include <FastNoiseLite.h>
#include <SDL3/SDL.h>

#include "block.hpp"
#include "chunk.hpp"
#include "helpers.hpp"
#include "world.hpp"

Chunk::Chunk()
    : Flags{ChunkFlagsNone}
{
}

// void Chunk::Generate(WorldProxy& proxy, int chunkX, int chunkZ)
// {
//     Profile();
//     SDL_assert(Flags & ChunkFlagsGenerate);
//     FastNoiseLite noise;
//     noise.SetFrequency(0.02f);
//     for (int i = 0; i < Chunk::kWidth; i++)
//     for (int j = 0; j < Chunk::kWidth; j++)
//     {
//         static constexpr float kScale = 10.0f;
//         static constexpr int kWaterLevel = 5;
//         static constexpr int kSandLevel = 6;
//         float x = chunkX * Chunk::kWidth + i;
//         float z = chunkZ * Chunk::kWidth + j;
//         int height = (noise.GetNoise(x, z) + 1.0f) / 2.0f * kScale;
//         height = std::clamp(height, 1, Chunk::kHeight - 1);
//         for (int y = 0; y < std::min(height, kWaterLevel); y++)
//         {
//             proxy.SetBlock({i, y, j}, BlockWater);
//         }
//         for (int y = kWaterLevel; y < std::min(height, kSandLevel); y++)
//         {
//             proxy.SetBlock({i, y, j}, BlockSand);
//         }
//         for (int y = kSandLevel; y < height; y++)
//         {
//             proxy.SetBlock({i, y, j}, BlockDirt);
//         }
//         for (int y = height; y < Chunk::kHeight; y++)
//         {
//             proxy.SetBlock({i, y, j}, BlockAir);
//         }
//         if (height > kSandLevel)
//         {
//             proxy.SetBlock({i, height, j}, BlockGrass);
//         }
//     }
//     Flags &= ~ChunkFlagsGenerate;
// }

void Chunk::Generate(WorldProxy& proxy, int chunkX, int chunkZ)
{
    Profile();
    SDL_assert(Flags & ChunkFlagsGenerate);
    FastNoiseLite baseNoise;
    FastNoiseLite detailNoise;
    FastNoiseLite treeNoise;
    baseNoise.SetFrequency(0.01f);
    detailNoise.SetFrequency(0.05f);
    treeNoise.SetFrequency(0.08f);
    static constexpr int kWaterLevel = 20;
    static constexpr int kMaxHeight = Chunk::kHeight - 1;
    for (int i = 0; i < Chunk::kWidth; i++)
    for (int j = 0; j < Chunk::kWidth; j++)
    {
        for (int y = 0; y < Chunk::kHeight; y++)
        {
            proxy.SetBlock({i, y, j}, BlockAir);
        }
        float x = chunkX * Chunk::kWidth + i;
        float z = chunkZ * Chunk::kWidth + j;
        float base = (baseNoise.GetNoise(x, z) + 1.0f) * 0.5f * 20.0f;
        float detail = (detailNoise.GetNoise(x, z) + 1.0f) * 0.5f * 6.0f;
        int height = base + detail + 8;
        height = std::clamp(height, 1, kMaxHeight);
        for (int y = 0; y <= height; y++)
        {
            if (y < height - 3)
            {
                proxy.SetBlock({i, y, j}, BlockStone);
            }
            else if (y < height)
            {
                proxy.SetBlock({i, y, j}, BlockDirt);
            }
            else
            {
                proxy.SetBlock({i, y, j}, BlockGrass);
            }
        }
        if (height < kWaterLevel)
        {
            for (int y = height + 1; y <= kWaterLevel; y++)
            {
                proxy.SetBlock({i, y, j}, BlockWater);
            }
        }
    }
    Flags &= ~ChunkFlagsGenerate;
}

void Chunk::AddFlags(ChunkFlags flags)
{
    Flags |= flags;
}

ChunkFlags Chunk::GetFlags() const
{
    return Flags;
}