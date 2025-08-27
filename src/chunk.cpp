#include "chunk.hpp"

Chunk::Chunk()
    : Flags{ChunkFlags::None}
{
}

ChunkFlags Chunk::GetFlags() const
{
    return Flags;
}