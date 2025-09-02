#pragma once

#include <cstdint>

#include "config.h"

class World;

using ChunkFlags = uint32_t;
static constexpr ChunkFlags ChunkFlagsNone = 0;
static constexpr ChunkFlags ChunkFlagsGenerate = 0x01;

class Chunk
{
public:
    static constexpr int kWidth = CHUNK_WIDTH;
    static constexpr int kHeight = CHUNK_HEIGHT;

    Chunk();
    void Generate(World& world, int chunkX, int chunkZ);
    ChunkFlags GetFlags() const;

private:
    ChunkFlags Flags;
};