#pragma once

enum class ChunkFlags
{
    None = 0,
    Generate = 0x01,
};

class Chunk
{
public:
    static constexpr int kWidth = 32;
    static constexpr int kHeight = 128;

    Chunk();
    ChunkFlags GetFlags() const;

private:
    ChunkFlags Flags;
};