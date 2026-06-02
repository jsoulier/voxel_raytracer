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
    // birch grass
    {
        .Color = 0x82AD21FF,
        .Light = 0.0f,
        .Roughness = 0.8f,
        .IOR = 0.0f,
    },
    // jungle grass
    {
        .Color = 0x1B4D03FF,
        .Light = 0.0f,
        .Roughness = 0.8f,
        .IOR = 0.0f,
    },
    // cherry grass
    {
        .Color = 0xF5F5F5FF,
        .Light = 0.0f,
        .Roughness = 0.8f,
        .IOR = 0.0f,
    },
    // Autumn grass
    {
        .Color = 0x8B7D3AFF,
        .Light = 0.0f,
        .Roughness = 0.8f,
        .IOR = 0.0f,
    },
    // blue grass
    {
        .Color = 0x3F76E4FF,
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
        .Color = 0x0055FFFF,
        .Light = 0.0f,
        .Roughness = 0.01f,
        .IOR = 0.33f,
    },
    // glass
    {
        .Color = 0x99FFFFFF,
        .Light = 0.0f,
        .Roughness = 1.0f,
        .IOR = 0.5f,
    },
    // stone
    {
        .Color = 0x777777FF,
        .Light = 0.0f,
        .Roughness = 0.9f,
        .IOR = 0.0f,
    },
    // oak wood
    {
        .Color = 0x8B4513FF,
        .Light = 0.0f,
        .Roughness = 0.7f,
        .IOR = 0.0f,
    },
    // oak leaves
    {
        .Color = 0x228B22FF,
        .Light = 0.0f,
        .Roughness = 0.9f,
        .IOR = 0.0f,
    },
    // snow
    {
        .Color = 0xFFFFFFFF,
        .Light = 0.0f,
        .Roughness = 0.9f,
        .IOR = 0.0f,
    },
    // pine wood
    {
        .Color = 0x3D2B1FFF,
        .Light = 0.0f,
        .Roughness = 0.7f,
        .IOR = 0.0f,
    },
    // pine leaves
    {
        .Color = 0x014421FF,
        .Light = 0.0f,
        .Roughness = 0.9f,
        .IOR = 0.0f,
    },
    // birch wood
    {
        .Color = 0xDDD6C1FF,
        .Light = 0.0f,
        .Roughness = 0.7f,
        .IOR = 0.0f,
    },
    // birch leaves
    {
        .Color = 0x82AD21FF,
        .Light = 0.0f,
        .Roughness = 0.9f,
        .IOR = 0.0f,
    },
    // jungle wood
    {
        .Color = 0x563012FF,
        .Light = 0.0f,
        .Roughness = 0.7f,
        .IOR = 0.0f,
    },
    // jungle leaves
    {
        .Color = 0x1B4D03FF,
        .Light = 0.0f,
        .Roughness = 0.9f,
        .IOR = 0.0f,
    },
    // cherry wood
    {
        .Color = 0x5C4033FF,
        .Light = 0.0f,
        .Roughness = 0.7f,
        .IOR = 0.0f,
    },
    // cherry leaves
    {
        .Color = 0xFFB7C5FF,
        .Light = 0.0f,
        .Roughness = 0.9f,
        .IOR = 0.0f,
    },
    // maple wood
    {
        .Color = 0x4B2D1FFF,
        .Light = 0.0f,
        .Roughness = 0.7f,
        .IOR = 0.0f,
    },
    // maple leaves 
    {
        .Color = 0xB22222FF,
        .Light = 0.0f,
        .Roughness = 0.9f,
        .IOR = 0.0f,
    },
    // blue wood 
    {
        .Color = 0x3B2F2FFF,
        .Light = 0.0f,
        .Roughness = 0.7f,
        .IOR = 0.0f,
    },
    // blue leaves
    {
        .Color = 0x006994FF,
        .Light = 0.0f,
        .Roughness = 0.9f,
        .IOR = 0.0f,
    },
    // clay
    {
        .Color = 0x5D4037FF,
        .Light = 0.0f,
        .Roughness = 1.0f,
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
