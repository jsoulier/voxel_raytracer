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
        .IOR = 0.0f,
    },
    // grass
    {
        .Color = 0x33CC33FF,
        .Light = 0.0f,
        .Roughness = 0.8f,
        .IOR = 0.0f,
    },
    // dirt
    {
        .Color = 0x8B5A2BFF,
        .Light = 0.0f,
        .Roughness = 1.0f,
        .IOR = 0.0f,
    },
    // sand
    {
        .Color = 0xFFF5B1FF,
        .Light = 0.0f,
        .Roughness = 1.0f,
        .IOR = 0.0f,
    },
    // water
    {
        .Color = 0x3399FFFF,
        .Light = 0.0f,
        .Roughness = 0.01f,
        .IOR = 0.33f,
    },
    // glass
    {
        .Color = 0x99FFFFFF,
        .Light = 0.0f,
        .Roughness = 0.0f,
        .IOR = 0.5f,
    },
    // stone
    {
        .Color = 0x777777FF,
        .Light = 0.0f,
        .Roughness = 0.9f,
        .IOR = 0.0f,
    },
    // wood
    {
        .Color = 0x8B4513FF,
        .Light = 0.0f,
        .Roughness = 0.7f,
        .IOR = 0.0f,
    },
    // leaves
    {
        .Color = 0x228B22FF,
        .Light = 0.0f,
        .Roughness = 0.9f,
        .IOR = 0.0f,
    },
    // steel
    {
        .Color = 0xCCCCCCFF,
        .Light = 0.0f,
        .Roughness = 0.0f,
        .IOR = 0.0f,
    },
    // red light
    {
        .Color = 0xFF0000FF,
        .Light = 5.0f,
        .Roughness = 0.0f,
        .IOR = 0.0f,
    },
    // green light
    {
        .Color = 0x00FF00FF,
        .Light = 5.0f,
        .Roughness = 0.0f,
        .IOR = 0.0f,
    },
    // blue light
    {
        .Color = 0x0000FFFF,
        .Light = 5.0f,
        .Roughness = 0.0f,
        .IOR = 0.0f,
    },
    // cyan light
    {
        .Color = 0x00FFFFFF,
        .Light = 5.0f,
        .Roughness = 0.0f,
        .IOR = 0.0f,
    },
    // magenta light
    {
        .Color = 0xFF00FFFF,
        .Light = 5.0f,
        .Roughness = 0.0f,
        .IOR = 0.0f,
    },
    // yellow light
    {
        .Color = 0xFFFF00FF,
        .Light = 5.0f,
        .Roughness = 0.0f,
        .IOR = 0.0f,
    },
    // white light
    {
        .Color = 0xFFFFFFFF,
        .Light = 5.0f,
        .Roughness = 0.0f,
        .IOR = 0.0f,
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