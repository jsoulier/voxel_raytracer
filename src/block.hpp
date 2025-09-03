#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>

#include "buffer.hpp"

enum Block : uint8_t
{
    BlockAir,
    BlockGrass,
    BlockDirt,
    BlockCount,
};

struct BlockProperty
{
    glm::vec2 TexCoord;
};

using BlockState = std::array<BlockProperty, BlockCount>;

BlockState BlockGetState(SDL_GPUDevice* device);