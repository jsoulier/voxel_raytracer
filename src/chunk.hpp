#pragma once

#include <cstdint>
#include <functional>

#include "config.h"

class WorldProxy;

using ChunkFlags = uint32_t;
static constexpr ChunkFlags ChunkFlagsNone = 0;
static constexpr ChunkFlags ChunkFlagsGenerate = 0x01;

class Chunk
{
public:
    static constexpr int kWidth = CHUNK_WIDTH;
    static constexpr int kHeight = CHUNK_HEIGHT;

    Chunk();
    void Generate(WorldProxy& proxy, int chunkX, int chunkZ);
    void AddFlags(ChunkFlags flags);
    ChunkFlags GetFlags() const;

private:
    ChunkFlags Flags;
};