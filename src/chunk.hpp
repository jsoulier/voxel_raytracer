#pragma once

#include <cstdint>

class World;

using ChunkFlags = uint32_t;
static constexpr ChunkFlags ChunkFlagsNone = 0;
static constexpr ChunkFlags ChunkFlagsGenerate = 0x01;

class Chunk
{
public:
    static constexpr int kWidth = 32;
    static constexpr int kHeight = 128;

    Chunk();
    void Generate(World& world, int chunkX, int chunkY);
    ChunkFlags GetFlags() const;

private:
    ChunkFlags Flags;
};