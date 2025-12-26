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

void Chunk::Generate(WorldProxy& proxy, int chunkX, int chunkZ)
{
    SDL_assert(Flags & ChunkFlagsGenerate);
    FastNoiseLite baseNoise;
    FastNoiseLite detailNoise;
    FastNoiseLite treeNoise;
    baseNoise.SetFrequency(0.01f);
    detailNoise.SetFrequency(0.05f);
    treeNoise.SetFrequency(0.1f);
    static constexpr int kWaterLevel = 20;
    static constexpr int kMaxHeight = Chunk::kHeight - 1;
    for (int i = 0; i < Chunk::kWidth; i++)
    for (int j = 0; j < Chunk::kWidth; j++)
    {
        float x = chunkX * Chunk::kWidth + i;
        float z = chunkZ * Chunk::kWidth + j;
        float base = (baseNoise.GetNoise(x, z) + 1.0f) * 0.5f * 20.0f;
        float detail = (detailNoise.GetNoise(x, z) + 1.0f) * 0.5f;
        int height = base + detail * 6.0f + 8;
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
            else if (height >= kWaterLevel + 2)
            {
                proxy.SetBlock({i, y, j}, BlockGrass);
                if (i < 2 || i >= Chunk::kWidth - 2 || j < 2 || j >= Chunk::kWidth - 2)
                {
                    continue;
                }
                if (treeNoise.GetNoise(x, z) < 0.4f)
                {
                    continue;
                }
                int offset = 3 + detail * 2.0f;
                for (int y = 1; y <= offset; y++)
                {
                    proxy.SetBlock({i, height + y, j}, BlockWood);
                }
                for (int dx = -2; dx <= 2; dx++)
                for (int dy = -2; dy <= 2; dy++)
                for (int dz = -2; dz <= 2; dz++)
                {
                    if (dy <= 0 && dx == 0 && dz == 0)
                    {
                        continue;
                    }
                    if (dx * dx + dy * dy + dz * dz <= 4)
                    {
                        proxy.SetBlock({i + dx, height + offset + dy, j + dz}, BlockLeaves);
                    }
                }
            }
            else
            {
                proxy.SetBlock({i, y, j}, BlockSand);
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