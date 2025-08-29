#include <SDL3/SDL.h>

#include "chunk.hpp"
#include "noise.hpp"

Chunk::Chunk()
    : Flags{ChunkFlagsGenerate}
{
}

void Chunk::Generate(World& world, int chunkX, int chunkY)
{
    SDL_assert(Flags & ChunkFlagsGenerate);
    NoiseGenerate(world, chunkX, chunkY);
    Flags &= ~ChunkFlagsGenerate;
}

ChunkFlags Chunk::GetFlags() const
{
    return Flags;
}