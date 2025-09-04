#include <FastNoiseLite.h>

#include <algorithm>
#include <limits>

#include "block.hpp"
#include "camera.hpp"
#include "noise.hpp"
#include "profile.hpp"
#include "world.hpp"

void NoiseSetChunk(WorldProxy& proxy, int chunkX, int chunkZ)
{
    // NOTE: air is only explicitly set on the CPU 
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
            proxy.SetBlock({i, y, j}, BlockWater);
        }
        for (int y = kWaterLevel; y < std::min(height, kSandLevel); y++)
        {
            proxy.SetBlock({i, y, j}, BlockSand);
        }
        for (int y = kSandLevel; y < height; y++)
        {
            proxy.SetBlock({i, y, j}, BlockDirt);
        }
        for (int y = height + 1; y < Chunk::kHeight; y++)
        {
            proxy.SetBlock({i, height, j}, BlockAir);
        }
        if (height > kSandLevel)
        {
            proxy.SetBlock({i, height, j}, BlockGrass);
        }
    }
}