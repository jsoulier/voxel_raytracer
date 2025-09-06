#pragma once

#include <array>
#include <cstdint>

#include "buffer.hpp"

#define BLOCKS \
    X(Air) \
    X(Grass) \
    X(Dirt) \
    X(Sand) \
    X(Water) \
    X(Stone) \
    X(Wood) \
    X(Leaves) \
    X(Steel) \
    X(RedLight) \
    X(GreenLight) \
    X(BlueLight) \
    X(CyanLight) \
    X(MagentaLight) \
    X(YellowLight) \
    X(WhiteLight) \

enum Block : uint8_t
{
#define X(block) Block##block,
    BLOCKS
#undef X
    BlockCount,
    BlockFirst = BlockAir + 1,
    BlockLast = BlockCount - 1,
};

struct BlockProperty
{
    uint32_t Color;
    float Light;
    float Roughness;
    float IOR;
};

using BlockState = std::array<BlockProperty, BlockCount>;

BlockState BlockGetState();
const char* BlockToString(Block block);
const char** BlockGetStrings();