#pragma once

#include <cstdint>
#include <limits>

#include "block.hpp"
#include "buffer.hpp"
#include "chunk.hpp"

struct WorkerBlockJob
{
    uint8_t X;
    uint8_t Y;
    uint8_t Z;
    Block Value;
};

struct WorkerHeightmapJob
{
    uint8_t X;
    uint8_t Y;
    uint8_t Z;
    uint8_t Padding;
};

static_assert(sizeof(WorkerBlockJob) == 4);;
static_assert(sizeof(WorkerHeightmapJob) == 4);;
static_assert(Chunk::kWidth < std::numeric_limits<uint8_t>::max());
static_assert(Chunk::kHeight < std::numeric_limits<uint8_t>::max());
static_assert(BlockCount < std::numeric_limits<uint8_t>::max());

class Worker
{
public:
    void Destroy(SDL_GPUDevice* device);

private:
    DynamicBuffer<WorkerBlockJob> BlockBuffer;
    DynamicBuffer<WorkerHeightmapJob> HeightmapBuffer;
};