#include <SDL3/SDL.h>

#include "block.hpp"

static constexpr BlockState kBlocks =
{
    // TODO: need at least 1 member with BlockProperty to deduce the initializer?
    BlockProperty
    // air
    {
        .Color = 0,
    },
    // grass
    {
        .Color = 0x00FF00FF,
    },
    // dirt
    {
        .Color = 0xA52A2AFF,
    },
    // sand
    {
        .Color = 0xFFFF00FF,
    },
    // water
    {
        .Color = 0x0000FFFF,
    },
};

static_assert(BlockAir == 0);
static_assert(SDL_arraysize(kBlocks) == BlockCount);

BlockState BlockGetState()
{
    return kBlocks;
}

const char* BlockToString(Block block)
{
    switch (block)
    {
#define X(block) case Block##block: return #block;
    BLOCKS
#undef X
    }
    return "?";
}