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

enum Block : uint8_t
{
#define X(block) Block##block,
    BLOCKS
#undef X
    BlockCount,
};

struct BlockProperty
{
    uint32_t Color;
};

using BlockState = std::array<BlockProperty, BlockCount>;

BlockState BlockGetState();
const char* BlockToString(Block block);