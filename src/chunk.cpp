#include <SDL3/SDL.h>

#include "chunk.hpp"
#include "noise.hpp"

Chunk::Chunk()
    : Flags{ChunkFlagsNone}
{
}

void Chunk::Generate(WorldProxy& proxy, int chunkX, int chunkZ)
{
    SDL_assert(Flags & ChunkFlagsGenerate);
    NoiseSetChunk(proxy, chunkX, chunkZ);
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