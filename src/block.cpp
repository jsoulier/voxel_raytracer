#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "block.hpp"

static constexpr int kAtlasWidth = 256;
static constexpr int kAtlasHeight = 256;
static constexpr int kBlockWidth = 16;

struct
{
    glm::ivec2 AtlasIndex;
}
static constexpr kBlocks[] =
{
    // air
    {

    },
    // grass
    {

    },
    // dirt
    {

    }
};

static_assert(BlockAir == 0);
static_assert(SDL_arraysize(kBlocks) == BlockCount);

BlockState BlockGetState(SDL_GPUDevice* device)
{
    return {};
}