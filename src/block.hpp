#pragma once

#include <array>
#include <cstdint>

#include "buffer.hpp"

enum Block : uint8_t
{
    BlockAir,
    BlockGrass,
    BlockDirt,
    BlockSand,
    BlockWater,
    BlockCount,
};

struct BlockProperty
{
    uint32_t Color;
};

using BlockState = std::array<BlockProperty, BlockCount>;

BlockState BlockGetState();