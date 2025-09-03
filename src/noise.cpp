#include <FastNoiseLite.h>

#include <algorithm>
#include <limits>

#include "block.hpp"
#include "camera.hpp"
#include "noise.hpp"
#include "profile.hpp"
#include "world.hpp"

// TODO: avoid using SetBlock and have a fast path that precalculates the chunk
void NoiseSetChunk(World& world, int chunkX, int chunkZ)
{
    Profile();
    FastNoiseLite noise;
    noise.SetFrequency(0.02f);
    for (int i = 0; i < Chunk::kWidth; i++)
    for (int j = 0; j < Chunk::kWidth; j++)
    {
        static constexpr float kScale = 10.0f;
        static constexpr int kWaterLevel = 5;
        static constexpr int kSandLevel = 6;
        float x = chunkX * Chunk::kWidth + i;
        float z = chunkZ * Chunk::kWidth + j;
        int height = (noise.GetNoise(x, z) + 1.0f) / 2.0f * kScale;
        height = std::clamp(height, 1, Chunk::kHeight);
        for (int y = 0; y < std::min(height, kWaterLevel); y++)
        {
            world.SetBlock({x, y, z}, BlockWater);
        }
        for (int y = kWaterLevel; y < std::min(height, kSandLevel); y++)
        {
            world.SetBlock({x, y, z}, BlockSand);
        }
        for (int y = kSandLevel; y < height; y++)
        {
            world.SetBlock({x, y, z}, BlockDirt);
        }
        if (height > kSandLevel)
        {
            world.SetBlock({x, height, z}, BlockGrass);
        }
    }
}