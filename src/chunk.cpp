#include <FastNoiseLite.h>
#include <SDL3/SDL.h>

#include "block.hpp"
#include "chunk.hpp"
#include "helpers.hpp"
#include "world.hpp"

static constexpr int kWaterLevel = 8;
static constexpr int kSnowThreshold = 85;
static constexpr int kMaxHeight = Chunk::kHeight - 1;

enum Biome
{
    BiomeForest,
    BiomeBirchForest,
    BiomeJungle,
    BiomeCherryBlossom,
    BiomeAutumnalForest,
    BiomeClay,
    BiomeBlueForest,
    BiomeMountain,
    BiomeOcean,
    BiomeInvalid,
};

static void GenerateTree(WorldProxy& proxy, int x, int y, int z, Block wood, Block leaves, float detail)
{
    int offset = 3 + detail * 2.0f;
    for (int i = 1; i <= offset; i++)
    {
        if (y + i < Chunk::kHeight)
        {
            proxy.SetBlock({x, y + i, z}, wood);
        }
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
            int lx = x + dx;
            int ly = y + offset + dy;
            int lz = z + dz;
            if (lx >= 0 && lx < Chunk::kWidth && ly >= 0 && ly < Chunk::kHeight && lz >= 0 && lz < Chunk::kWidth)
            {
                proxy.SetBlock({lx, ly, lz}, leaves);
            }
        }
    }
}

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
    FastNoiseLite biomeNoise;
    FastNoiseLite ridgeNoise;
    FastNoiseLite mountainNoise;
    baseNoise.SetFrequency(0.01f);
    detailNoise.SetFrequency(0.05f);
    treeNoise.SetFrequency(0.1f);
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    biomeNoise.SetFrequency(0.01f);
    biomeNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
    ridgeNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    ridgeNoise.SetFrequency(0.004f);
    ridgeNoise.SetFractalOctaves(6);
    mountainNoise.SetFrequency(0.002f);
    mountainNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    mountainNoise.SetFractalOctaves(3);
    for (int i = 0; i < Chunk::kWidth; i++)
    for (int j = 0; j < Chunk::kWidth; j++)
    {
        Biome biome = BiomeInvalid;
        float x = chunkX * Chunk::kWidth + i;
        float z = chunkZ * Chunk::kWidth + j;
        float ridge = 0.0f;
        float mountain = (mountainNoise.GetNoise(x, z) + 1.0f) * 0.5f;
        float detail = (detailNoise.GetNoise(x, z) + 1.0f) * 0.5f;
        if (mountain > 0.45f + (detail - 0.5f) * 0.05f)
        {
            biome = BiomeMountain;
            float weight = (mountain - 0.45f) / 0.55f;
            weight = std::pow(weight, 0.8f);
            ridge = (ridgeNoise.GetNoise(x, z) + 1.0f) * 0.5f * 120.0f * weight;
        }
        float base = (baseNoise.GetNoise(x, z) + 1.0f) * 0.5f * 15.0f;
        int height = base + detail * 4.0f + ridge;
        height = std::clamp(height, 1, kMaxHeight);
        if (height < kWaterLevel)
        {
            biome = BiomeOcean;
        }
        else if (biome == BiomeMountain && height > 35)
        {
            // Noop
        }
        else
        {
            float bv = biomeNoise.GetNoise(x, z);
            if (bv < -0.7f) biome = BiomeForest;
            else if (bv < -0.4f) biome = BiomeBirchForest;
            else if (bv < -0.1f) biome = BiomeJungle;
            else if (bv < 0.2f) biome = BiomeCherryBlossom;
            else if (bv < 0.5f) biome = BiomeAutumnalForest;
            else if (bv < 0.6f) biome = BiomeClay;
            else biome = BiomeBlueForest;
        }
        SDL_assert(biome != BiomeInvalid);
        for (int y = 0; y <= height; y++)
        {
            if (biome == BiomeMountain)
            {
                if (y >= height - 1 && height > kSnowThreshold + (detail - 0.5f) * 6.0f)
                {
                    proxy.SetBlock({i, y, j}, BlockSnow);
                }
                else
                {
                    proxy.SetBlock({i, y, j}, BlockStone);
                }
            }
            else if (biome == BiomeClay)
            {
                proxy.SetBlock({i, y, j}, BlockClay);
            }
            else
            {
                if (y < height - 3)
                {
                    proxy.SetBlock({i, y, j}, BlockStone);
                }
                else if (y < height)
                {
                    proxy.SetBlock({i, y, j}, BlockDirt);
                }
                else if (biome == BiomeOcean)
                {
                    proxy.SetBlock({i, y, j}, BlockSand);
                }
                else
                {
                    Block surfaceBlock = BlockGrass;
                    if (biome == BiomeBirchForest) surfaceBlock = BlockBirchGrass;
                    else if (biome == BiomeJungle) surfaceBlock = BlockJungleGrass;
                    else if (biome == BiomeCherryBlossom) surfaceBlock = BlockCherryGrass;
                    else if (biome == BiomeAutumnalForest) surfaceBlock = BlockAutumnGrass;
                    else if (biome == BiomeBlueForest) surfaceBlock = BlockBlueGrass;
                    proxy.SetBlock({i, y, j}, surfaceBlock);
                    if (i < 2 || i >= Chunk::kWidth - 2 || j < 2 || j >= Chunk::kWidth - 2)
                    {
                        continue;
                    }
                    if (treeNoise.GetNoise(x, z) > 0.4f)
                    {
                        Block wood = BlockAir;
                        Block leaves = BlockAir;
                        if (biome == BiomeForest) { wood = BlockOakWood; leaves = BlockOakLeaves; }
                        else if (biome == BiomeBirchForest) { wood = BlockBirchWood; leaves = BlockBirchLeaves; }
                        else if (biome == BiomeJungle) { wood = BlockJungleWood; leaves = BlockJungleLeaves; }
                        else if (biome == BiomeCherryBlossom) { wood = BlockCherryWood; leaves = BlockCherryLeaves; }
                        else if (biome == BiomeAutumnalForest) { wood = BlockMapleWood; leaves = BlockMapleLeaves; }
                        else if (biome == BiomeBlueForest) { wood = BlockBlueWood; leaves = BlockBlueLeaves; }
                        SDL_assert(wood != BlockAir);
                        SDL_assert(leaves != BlockAir);
                        GenerateTree(proxy, i, height, j, wood, leaves, detail);
                    }
                }
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
