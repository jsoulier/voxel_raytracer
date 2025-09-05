#include <SDL3/SDL.h>

#include "block.hpp"

static constexpr BlockState kBlocks =
{
    BlockProperty
    // air
    {
        .Color = 0,
        .Light = 0.0f,
        .Roughness = 1.0f,
    },
    // grass
    {
        .Color = 0x00FF00FF,
        .Light = 0.1f,
        .Roughness = 1.0f,
    },
    // dirt
    {
        .Color = 0xA52A2AFF,
        .Light = 0.1f,
        .Roughness = 1.0f,
    },
    // sand
    {
        .Color = 0xFFFF00FF,
        .Light = 0.1f,
        .Roughness = 1.0f,
    },
    // water
    {
        .Color = 0x0000FFFF,
        .Light = 0.1f,
        .Roughness = 1.0f,
    },
    // red light
    {
        .Color = 0xFF0000FF,
        .Light = 1.0f,
        .Roughness = 0.0f,
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

const char** BlockGetStrings()
{
    static const char* kStrings[] =
    {
#define X(block) #block,
    BLOCKS
#undef X
    };
    return kStrings;
}