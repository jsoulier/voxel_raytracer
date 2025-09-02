#include <SDL3/SDL.h>

#include "chunk.hpp"
#include "noise.hpp"

Chunk::Chunk()
    : Flags{ChunkFlagsGenerate}
{
}

void Chunk::Generate(World& world, int chunkX, int chunkZ)
{
    SDL_assert(Flags & ChunkFlagsGenerate);
    NoiseGenerate(world, chunkX, chunkZ);
    Flags &= ~ChunkFlagsGenerate;
}

ChunkFlags Chunk::GetFlags() const
{
    return Flags;
}