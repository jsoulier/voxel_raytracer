#include <FastNoiseLite.h>

#include "block.hpp"
#include "noise.hpp"
#include "profile.hpp"
#include "world.hpp"

void NoiseGenerate(World& world, int chunkX, int chunkY)
{
    Profile();
    // TODO: zero out chunk
    FastNoiseLite noise;
    noise.SetFrequency(0.02f);
    for (int i = 0; i < Chunk::kWidth; i++)
    for (int j = 0; j < Chunk::kWidth; j++)
    {
        float x = chunkX * Chunk::kWidth + i;
        float z = chunkY * Chunk::kWidth + j;
        static constexpr float kScale = 10.0f;
        int height = (noise.GetNoise(x, z) + 1.0f) / 2.0f * kScale;
        height = std::min(height, Chunk::kHeight);
        for (int y = 0; y < height; y++)
        {
            world.SetBlock(x, y, z, BlockDirt);
        }
        world.SetBlock(x, height, z, BlockGrass);
    }
}