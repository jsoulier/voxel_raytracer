#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <execution>
#include <numeric>
#include <thread>
#include <unordered_set>
#include <vector>

#include "block.hpp"
#include "camera.hpp"
#include "chunk.hpp"
#include "config.h"
#include "helpers.hpp"
#include "world.hpp"

static int FloorChunkIndex(float index)
{
    return std::floor(index / Chunk::kWidth);
}

static void TestFloorChunkIndex()
{
    SDL_assert(FloorChunkIndex(0) == 0);
    SDL_assert(FloorChunkIndex(Chunk::kWidth - 1) == 0);
    SDL_assert(FloorChunkIndex(Chunk::kWidth) == 1);
    SDL_assert(FloorChunkIndex(Chunk::kWidth + 1) == 1);
    SDL_assert(FloorChunkIndex(-1) == -1);
    SDL_assert(FloorChunkIndex(-Chunk::kWidth + 1) == -1);
    SDL_assert(FloorChunkIndex(-Chunk::kWidth) == -1);
    SDL_assert(FloorChunkIndex(-Chunk::kWidth - 1) == -2);
}

WorldSetBlockJob::WorldSetBlockJob(const glm::ivec3& position, Block block)
    : X(position.x)
    , Y(position.y)
    , Z(position.z)
    , Value{block}
{
}

WorldSetChunkJob::WorldSetChunkJob(int inX, int inZ, int outX, int outZ)
    : InX(inX)
    , InZ(inZ)
    , OutX(outX)
    , OutZ(outZ)
{
}

WorldOptions::WorldOptions()
    : SkyBottom{1.0f, 1.0f, 1.0f}
    , MaxSteps{512}
    , SkyTop{0.5f, 0.7f, 1.0f}
    , MaxBounces{8}
{
}

WorldProxy::WorldProxy(World& handle, DynamicBuffer<WorldSetBlockJob>& buffer, int chunkX, int chunkZ)
    : Handle{handle}
    , Buffer{buffer}
    , X{chunkX * Chunk::kWidth}
    , Z{chunkZ * Chunk::kWidth}
{
    for (int i = 0; i < Chunk::kWidth; i++)
    for (int j = 0; j < Chunk::kWidth; j++)
    for (int y = 0; y < Chunk::kHeight; y++)
    {
        int x = X + i;
        int z = Z + j;
        Handle.Blocks[x][y][z] = BlockAir;
    }
}

void WorldProxy::SetBlock(glm::ivec3 position, Block block)
{
    SDL_assert(block != BlockAir);
    SDL_assert(position.x >= 0 && position.x < Chunk::kWidth);
    SDL_assert(position.y >= 0 && position.y < Chunk::kHeight);
    SDL_assert(position.z >= 0 && position.z < Chunk::kWidth);
    position.x += X;
    position.z += Z;
    Handle.Blocks[position.x][position.y][position.z] = block;
    Buffer.Emplace(Handle.Device, position, block);
}

World::World()
    : Device{nullptr}
    , Blocks{}
    , Chunks{}
    , ChunkMap{}
    , SetBlocksBuffers{}
    , SetBlocksBufferCount{0}
    , UpdateGroups{}
    , SetChunksBuffer{}
    , ClearChunks{}
    , WorldStateBuffer{}
    , BlockStateBuffer{}
    , BlockTexture{nullptr}
    , GroupTexture{nullptr}
    , ChunkTexture{nullptr}
    , ColorTexture{nullptr}
    , SetBlocksPipeline{nullptr}
    , SetChunksPipeline{nullptr}
    , ClearBlocksPipeline{nullptr}
    , RaytracePipeline{nullptr}
    , ClearTexturePipeline{nullptr}
    , SampleTexturePipeline{nullptr}
    , ClearGroupsPipeline{nullptr}
    , SetGroupsPipeline{nullptr}
    , Width{0}
    , Height{0}
    , Dirty{true}
    , Sample{0}
{
}

bool World::Init(SDL_GPUDevice* device)
{
    Device = device;
    // https://en.cppreference.com/cpp/17
    // Somehow Apple Clang doesn't support execution policies yet (even with -fexperimental-library)
#if !SDL_PLATFORM_APPLE
    SetBlocksBuffers.resize(std::max(1u, std::thread::hardware_concurrency()));
#else
    SetBlocksBuffers.resize(1);
#endif
    {
        SDL_GPUTextureCreateInfo info{};
        info.format = SDL_GPU_TEXTUREFORMAT_R8_UINT;
        info.type = SDL_GPU_TEXTURETYPE_3D;
        info.usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE;
        info.width = kWidth * Chunk::kWidth;
        info.height = Chunk::kHeight;
        info.layer_count_or_depth = kWidth * Chunk::kWidth;
        info.num_levels = 1;
        BlockTexture = SDL_CreateGPUTexture(Device, &info);
        if (!BlockTexture)
        {
            SDL_Log("Failed to create block texture: %s", SDL_GetError());
            return false;
        }
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8_UINT;
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.width = kWidth;
        info.height = kWidth;
        info.layer_count_or_depth = 1;
        ChunkTexture = SDL_CreateGPUTexture(Device, &info);
        if (!ChunkTexture)
        {
            SDL_Log("Failed to create chunk texture: %s", SDL_GetError());
            return false;
        }
        info.format = SDL_GPU_TEXTUREFORMAT_R8_UINT;
        info.type = SDL_GPU_TEXTURETYPE_3D;
        info.width = GROUP_WIDTH;
        info.height = GROUP_HEIGHT;
        info.layer_count_or_depth = GROUP_WIDTH;
        GroupTexture = SDL_CreateGPUTexture(Device, &info);
        if (!GroupTexture)
        {
            SDL_Log("Failed to create group texture: %s", SDL_GetError());
            return false;
        }
    }
    {
        SetBlocksPipeline = LoadComputePipeline(Device, "set_blocks.comp");
        if (!SetBlocksPipeline)
        {
            SDL_Log("Failed to load set blocks pipeline");
            return false;
        }
        SetChunksPipeline = LoadComputePipeline(Device, "set_chunks.comp");
        if (!SetChunksPipeline)
        {
            SDL_Log("Failed to load set chunks pipeline");
            return false;
        }
        ClearBlocksPipeline = LoadComputePipeline(Device, "clear_blocks.comp");
        if (!ClearBlocksPipeline)
        {
            SDL_Log("Failed to load clear blocks pipeline");
            return false;
        }
        RaytracePipeline = LoadComputePipeline(Device, "raytrace.comp");
        if (!RaytracePipeline)
        {
            SDL_Log("Failed to load raytrace pipeline");
            return false;
        }
        ClearTexturePipeline = LoadComputePipeline(Device, "clear_texture.comp");
        if (!ClearTexturePipeline)
        {
            SDL_Log("Failed to load clear texture pipeline");
            return false;
        }
        SampleTexturePipeline = LoadComputePipeline(Device, "sample_texture.comp");
        if (!SampleTexturePipeline)
        {
            SDL_Log("Failed to load sample texture pipeline");
            return false;
        }
        ClearGroupsPipeline = LoadComputePipeline(Device, "clear_groups.comp");
        if (!ClearGroupsPipeline)
        {
            SDL_Log("Failed to load clear groups pipeline");
            return false;
        }
        SetGroupsPipeline = LoadComputePipeline(Device, "set_groups.comp");
        if (!SetGroupsPipeline)
        {
            SDL_Log("Failed to load set groups pipeline");
            return false;
        }
    }
    {
        if (!WorldStateBuffer.Init(Device))
        {
            SDL_Log("Failed to initialize world state");
            return false;
        }
        if (!BlockStateBuffer.Init(Device))
        {
            SDL_Log("Failed to initialize block state");
            return false;
        }
        BlockStateBuffer.Get() = BlockGetState();
        WorldStateBuffer.Get().X = 0;
        WorldStateBuffer.Get().Z = 0;
        for (int x = 0; x < kWidth; x++)
        for (int z = 0; z < kWidth; z++)
        {
            Chunks[x][z].AddFlags(ChunkFlagsGenerate);
            ChunkMap[x][z] = {x, z};
            SetChunksBuffer.Emplace(device, x, z, x, z);
        }
        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        if (!commandBuffer)
        {
            SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
            return false;
        }
        Dispatch(commandBuffer);
        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }
    return true;
}

void World::Destroy()
{
    BlockStateBuffer.Destroy(Device);
    WorldStateBuffer.Destroy(Device);
    SetChunksBuffer.Destroy(Device);
    for (int i = 0; i < SetBlocksBuffers.size(); i++)
    {
        SetBlocksBuffers[i].Destroy(Device);
    }
    SDL_ReleaseGPUComputePipeline(Device, SampleTexturePipeline);
    SDL_ReleaseGPUComputePipeline(Device, ClearTexturePipeline);
    SDL_ReleaseGPUComputePipeline(Device, RaytracePipeline);
    SDL_ReleaseGPUComputePipeline(Device, SetBlocksPipeline);
    SDL_ReleaseGPUComputePipeline(Device, SetChunksPipeline);
    SDL_ReleaseGPUComputePipeline(Device, ClearBlocksPipeline);
    SDL_ReleaseGPUComputePipeline(Device, ClearGroupsPipeline);
    SDL_ReleaseGPUComputePipeline(Device, SetGroupsPipeline);
    SDL_ReleaseGPUTexture(Device, GroupTexture);
    SDL_ReleaseGPUTexture(Device, ChunkTexture);
    SDL_ReleaseGPUTexture(Device, BlockTexture);
    SDL_ReleaseGPUTexture(Device, ColorTexture);
}

void World::Update(Camera& camera)
{
    int cameraX = camera.GetPosition().x / Chunk::kWidth - kWidth / 2;
    int cameraZ = camera.GetPosition().z / Chunk::kWidth - kWidth / 2;
    int offsetX = cameraX - WorldStateBuffer->X;
    int offsetZ = cameraZ - WorldStateBuffer->Z;
    if (offsetX || offsetZ)
    {
        WorldStateBuffer.Get().X = cameraX;
        WorldStateBuffer.Get().Z = cameraZ;
        static constexpr int kNull = -1;
        glm::ivec2 chunkMap[kWidth][kWidth];
        std::vector<glm::ivec2> outOfBoundsChunks;
        outOfBoundsChunks.reserve(kWidth * kWidth);
        for (int x = 0; x < kWidth; x++)
        for (int z = 0; z < kWidth; z++)
        {
            chunkMap[x][z].x = kNull;
        }
        for (int x = 0; x < kWidth; x++)
        for (int z = 0; z < kWidth; z++)
        {
            int newX = x - offsetX;
            int newZ = z - offsetZ;
            if (newX < 0 || newZ < 0 || newX >= kWidth || newZ >= kWidth)
            {
                outOfBoundsChunks.push_back(ChunkMap[x][z]);
            }
            else
            {
                chunkMap[newX][newZ] = ChunkMap[x][z];
            }
        }
        std::memcpy(ChunkMap, chunkMap, sizeof(ChunkMap));
        for (int x = 0; x < kWidth; x++)
        for (int z = 0; z < kWidth; z++)
        {
            if (ChunkMap[x][z].x != kNull)
            {
                SetChunksBuffer.Emplace(Device, x, z, ChunkMap[x][z].x, ChunkMap[x][z].y);
                continue;
            }
            glm::ivec2 position = outOfBoundsChunks.back();
            outOfBoundsChunks.pop_back();
            ChunkMap[x][z] = position;
            Chunk& chunk = Chunks[position.x][position.y];
            chunk.AddFlags(ChunkFlagsGenerate);
            SetChunksBuffer.Emplace(Device, x, z, position.x, position.y);
            ClearChunks.emplace_back(position.x, position.y);
            UpdateGroups.insert(position);
        }
        SDL_assert(outOfBoundsChunks.empty());
    }
    std::vector<glm::ivec2> jobs;
    for (int inX = 0; inX < kWidth; inX++)
    for (int inZ = 0; inZ < kWidth; inZ++)
    {
        int outX = ChunkMap[inX][inZ].x;
        int outZ = ChunkMap[inX][inZ].y;
        Chunk& chunk = Chunks[outX][outZ];
        if (chunk.GetFlags() & ChunkFlagsGenerate)
        {
            jobs.push_back({inX, inZ});
        }
    }
    if (!jobs.empty())
    {
        int maxJobs = std::min(jobs.size(), SetBlocksBuffers.size() - SetBlocksBufferCount);
        std::vector<int> jobIndices(maxJobs);
        std::iota(jobIndices.begin(), jobIndices.end(), 0);
        // https://en.cppreference.com/cpp/17
        // Somehow Apple Clang doesn't support execution policies yet (even with -fexperimental-library)
#if !SDL_PLATFORM_APPLE
        std::for_each(std::execution::par, jobIndices.begin(), jobIndices.end(), [this, &jobs](int i)
#else
        for (int i : jobIndices)
#endif
        {
            int bufferIndex = SetBlocksBufferCount + i;
            int inX = jobs[i].x;
            int inZ = jobs[i].y;
            int outX = ChunkMap[inX][inZ].x;
            int outZ = ChunkMap[inX][inZ].y;
            Chunk& chunk = Chunks[outX][outZ];
            WorldProxy proxy{*this, SetBlocksBuffers[bufferIndex], outX, outZ};
            chunk.Generate(proxy, WorldStateBuffer->X + inX, WorldStateBuffer->Z + inZ);
        }
#if !SDL_PLATFORM_APPLE
        );
#endif
        for (int i = 0; i < maxJobs; i++)
        {
            int inX = jobs[i].x;
            int inZ = jobs[i].y;
            int outX = ChunkMap[inX][inZ].x;
            int outZ = ChunkMap[inX][inZ].y;
            UpdateGroups.insert({outX, outZ});
        }
        SetBlocksBufferCount += maxJobs;
        Dirty = true;
    }
}

void World::Dispatch(SDL_GPUCommandBuffer* commandBuffer)
{
    {
        DebugGroupBlock(commandBuffer, "World::Render::Upload");
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        if (!copyPass)
        {
            SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
            return;
        }
        WorldStateBuffer.Upload(Device, copyPass);
        BlockStateBuffer.Upload(Device, copyPass);
        for (int i = 0; i < SetBlocksBufferCount; i++)
        {
            SetBlocksBuffers[i].Upload(Device, copyPass);
        }
        SetChunksBuffer.Upload(Device, copyPass);
        SDL_EndGPUCopyPass(copyPass);
    }
    if (SetChunksBuffer.GetSize())
    {
        DebugGroupBlock(commandBuffer, "World::Render::SetChunks");
        SDL_GPUStorageTextureReadWriteBinding writeTexture{};
        writeTexture.texture = ChunkTexture;
        SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &writeTexture, 1, nullptr, 0);
        if (!computePass)
        {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            return;
        }
        int numJobs = SetChunksBuffer.GetSize();
        int groupsX = (numJobs + SET_CHUNKS_THREADS_X - 1) / SET_CHUNKS_THREADS_X;
        SDL_GPUBuffer* readBuffers[1]{};
        readBuffers[0] = SetChunksBuffer.GetBuffer();
        SDL_BindGPUComputePipeline(computePass, SetChunksPipeline);
        SDL_BindGPUComputeStorageBuffers(computePass, 0, readBuffers, 1);
        SDL_PushGPUComputeUniformData(commandBuffer, 0, &numJobs, sizeof(numJobs));
        SDL_DispatchGPUCompute(computePass, groupsX, 1, 1);
        SDL_EndGPUComputePass(computePass);
    }
    if (!ClearChunks.empty())
    {
        DebugGroupBlock(commandBuffer, "World::Render::ClearBlocks");
        SDL_GPUStorageTextureReadWriteBinding writeTexture{};
        writeTexture.texture = BlockTexture;
        SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &writeTexture, 1, nullptr, 0);
        if (!computePass)
        {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            return;
        }
        int groupsX = (Chunk::kWidth + CLEAR_BLOCKS_THREADS_X - 1) / CLEAR_BLOCKS_THREADS_X;
        int groupsY = (Chunk::kHeight + CLEAR_BLOCKS_THREADS_Y - 1) / CLEAR_BLOCKS_THREADS_Y;
        SDL_BindGPUComputePipeline(computePass, ClearBlocksPipeline);
        for (glm::ivec2 position : ClearChunks)
        {
            SDL_PushGPUComputeUniformData(commandBuffer, 0, &position, sizeof(position));
            SDL_DispatchGPUCompute(computePass, groupsX, groupsY, groupsX);
        }
        ClearChunks.clear();
        SDL_EndGPUComputePass(computePass);
    }
    if (SetBlocksBufferCount > 0)
    {
        DebugGroupBlock(commandBuffer, "World::Render::SetBlocks");
        SDL_GPUStorageTextureReadWriteBinding writeTexture{};
        writeTexture.texture = BlockTexture;
        SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &writeTexture, 1, nullptr, 0);
        if (!computePass)
        {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            return;
        }
        SDL_BindGPUComputePipeline(computePass, SetBlocksPipeline);
        for (int i = 0; i < SetBlocksBufferCount; i++)
        {
            int numJobs = SetBlocksBuffers[i].GetSize();
            if (numJobs > 0)
            {
                int groupsX = (numJobs + SET_BLOCKS_THREADS_X - 1) / SET_BLOCKS_THREADS_X;
                SDL_GPUBuffer* readBuffers[1]{};
                readBuffers[0] = SetBlocksBuffers[i].GetBuffer();
                SDL_BindGPUComputeStorageBuffers(computePass, 0, readBuffers, 1);
                SDL_PushGPUComputeUniformData(commandBuffer, 0, &numJobs, sizeof(numJobs));
                SDL_DispatchGPUCompute(computePass, groupsX, 1, 1);
            }
        }
        SDL_EndGPUComputePass(computePass);
        SetBlocksBufferCount = 0;
    }
    if (!UpdateGroups.empty())
    {
        DebugGroupBlock(commandBuffer, "World::Render::ClearGroups");
        SDL_GPUStorageTextureReadWriteBinding writeTexture{};
        writeTexture.texture = GroupTexture;
        SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &writeTexture, 1, nullptr, 0);
        if (!computePass)
        {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            return;
        }
        SDL_BindGPUComputePipeline(computePass, ClearGroupsPipeline);
        for (const glm::ivec2& position : UpdateGroups)
        {
            int groupsX = (CHUNK_WIDTH / GROUP_SIZE + CLEAR_GROUPS_THREADS_X - 1) / CLEAR_GROUPS_THREADS_X;
            int groupsY = (CHUNK_HEIGHT / GROUP_SIZE + CLEAR_GROUPS_THREADS_Y - 1) / CLEAR_GROUPS_THREADS_Y;
            int groupsZ = (CHUNK_WIDTH / GROUP_SIZE + CLEAR_GROUPS_THREADS_Z - 1) / CLEAR_GROUPS_THREADS_Z;
            SDL_PushGPUComputeUniformData(commandBuffer, 0, &position, sizeof(position));
            SDL_DispatchGPUCompute(computePass, groupsX, groupsY, groupsZ);
        }
        SDL_EndGPUComputePass(computePass);
    }
    if (!UpdateGroups.empty())
    {
        DebugGroupBlock(commandBuffer, "World::Render::UpdateGroups");
        SDL_GPUStorageTextureReadWriteBinding writeTexture{};
        writeTexture.texture = GroupTexture;
        SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &writeTexture, 1, nullptr, 0);
        if (!computePass)
        {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            return;
        }
        SDL_GPUTexture* readTextures[1]{};
        readTextures[0] = BlockTexture;
        SDL_BindGPUComputePipeline(computePass, SetGroupsPipeline);
        SDL_BindGPUComputeStorageTextures(computePass, 0, readTextures, 1);
        for (const glm::ivec2& position : UpdateGroups)
        {
            int groupsX = (CHUNK_WIDTH + UPDATE_GROUPS_THREADS_X - 1) / UPDATE_GROUPS_THREADS_X;
            int groupsY = (CHUNK_HEIGHT + UPDATE_GROUPS_THREADS_Y - 1) / UPDATE_GROUPS_THREADS_Y;
            int groupsZ = (CHUNK_WIDTH + UPDATE_GROUPS_THREADS_Z - 1) / UPDATE_GROUPS_THREADS_Z;
            SDL_PushGPUComputeUniformData(commandBuffer, 0, &position, sizeof(position));
            SDL_DispatchGPUCompute(computePass, groupsX, groupsY, groupsZ);
        }
        SDL_EndGPUComputePass(computePass);
    }
    UpdateGroups.clear();
}

void World::Render(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* colorTexture, Camera& camera)
{
    if (Width != camera.GetWidth() || Height != camera.GetHeight())
    {
        DebugGroupBlock(commandBuffer, "World::Render::Resize");
        SDL_ReleaseGPUTexture(Device, ColorTexture);
        SDL_GPUTextureCreateInfo info{};
        info.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ |
            SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE;
        info.width = camera.GetWidth();
        info.height = camera.GetHeight();
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        ColorTexture = SDL_CreateGPUTexture(Device, &info);
        if (!ColorTexture)
        {
            SDL_Log("Failed to create texture: %s", SDL_GetError());
            return;
        }
        Width = camera.GetWidth();
        Height = camera.GetHeight();
        Dirty = true;
    }
    if (Dirty || camera.GetDirty())
    {
        Sample = 0;
        DebugGroupBlock(commandBuffer, "World::Render::ClearTexture");
        SDL_GPUStorageTextureReadWriteBinding writeTexture{};
        writeTexture.texture = ColorTexture;
        SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &writeTexture, 1, nullptr, 0);
        if (!computePass)
        {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            return;
        }
        int groupsX = (Width + CLEAR_TEXTURE_THREADS_X - 1) / CLEAR_TEXTURE_THREADS_X;
        int groupsY = (Height + CLEAR_TEXTURE_THREADS_Y - 1) / CLEAR_TEXTURE_THREADS_Y;
        SDL_BindGPUComputePipeline(computePass, ClearTexturePipeline);
        SDL_DispatchGPUCompute(computePass, groupsX, groupsY, 1);
        SDL_EndGPUComputePass(computePass);
        Dirty = false;
    }
    Sample++;
    {
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        if (!copyPass)
        {
            SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
            return;
        }
        camera.Upload(Device, copyPass);
        SDL_EndGPUCopyPass(copyPass);
    }
    {
        DebugGroupBlock(commandBuffer, "World::Render::Raytrace");
        SDL_GPUStorageTextureReadWriteBinding writeTexture{};
        writeTexture.texture = ColorTexture;
        SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &writeTexture, 1, nullptr, 0);
        if (!computePass)
        {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            return;
        }
        int groupsX = (Width + RAYTRACE_THREADS_X - 1) / RAYTRACE_THREADS_X;
        int groupsY = (Height + RAYTRACE_THREADS_Y - 1) / RAYTRACE_THREADS_Y;
        SDL_GPUTexture* readTextures[3]{};
        SDL_GPUBuffer* readBuffers[3]{};
        readTextures[0] = BlockTexture;
        readTextures[1] = GroupTexture;
        readTextures[2] = ChunkTexture;
        readBuffers[0] = camera.GetBuffer();
        readBuffers[1] = WorldStateBuffer.GetBuffer();
        readBuffers[2] = BlockStateBuffer.GetBuffer();
        SDL_BindGPUComputePipeline(computePass, RaytracePipeline);
        SDL_PushGPUComputeUniformData(commandBuffer, 0, &Sample, sizeof(Sample));
        SDL_BindGPUComputeStorageTextures(computePass, 0, readTextures, 3);
        SDL_BindGPUComputeStorageBuffers(computePass, 0, readBuffers, 3);
        SDL_DispatchGPUCompute(computePass, groupsX, groupsY, 1);
        SDL_EndGPUComputePass(computePass);
    }
    {
        DebugGroupBlock(commandBuffer, "World::Render::SampleTexture");
        SDL_GPUStorageTextureReadWriteBinding writeTexture{};
        writeTexture.texture = colorTexture;
        SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(commandBuffer, &writeTexture, 1, nullptr, 0);
        if (!computePass)
        {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            return;
        }
        int groupsX = (Width + SAMPLE_TEXTURE_THREADS_X - 1) / SAMPLE_TEXTURE_THREADS_X;
        int groupsY = (Height + SAMPLE_TEXTURE_THREADS_Y - 1) / SAMPLE_TEXTURE_THREADS_Y;
        SDL_GPUTexture* readTextures[1]{};
        readTextures[0] = ColorTexture;
        SDL_BindGPUComputePipeline(computePass, SampleTexturePipeline);
        SDL_PushGPUComputeUniformData(commandBuffer, 0, &Sample, sizeof(Sample));
        SDL_BindGPUComputeStorageTextures(computePass, 0, readTextures, 1);
        SDL_DispatchGPUCompute(computePass, groupsX, groupsY, 1);
        SDL_EndGPUComputePass(computePass);
    }
}

bool World::WorldToLocalPosition(glm::ivec3& position) const
{
    position.x -= WorldStateBuffer->X * Chunk::kWidth;
    position.z -= WorldStateBuffer->Z * Chunk::kWidth;
    glm::ivec2 chunk;
    chunk.x = FloorChunkIndex(position.x);
    chunk.y = FloorChunkIndex(position.z);
    position.x -= chunk.x * Chunk::kWidth;
    position.z -= chunk.y * Chunk::kWidth;
    chunk = ChunkMap[chunk.x][chunk.y];
    position.x += chunk.x * Chunk::kWidth;
    position.z += chunk.y * Chunk::kWidth;
    return position.x >= 0 && position.y >= 0 && position.z >= 0 &&
        position.x < Chunk::kWidth * World::kWidth &&
        position.y < Chunk::kHeight &&
        position.z < Chunk::kWidth * World::kWidth;
}

void World::SetBlock(glm::ivec3 position, Block block)
{
    if (WorldToLocalPosition(position))
    {
        int chunkX = position.x / Chunk::kWidth;
        int chunkZ = position.z / Chunk::kWidth;
        // TODO: While I don't know if it can ever happen in game, this is technically not safe. We reserve the first
        // SetBlocksBuffer for SetBlock calls. If a chunk generates in the same chunk that this block is placed in,
        // the chunk generation will overwrite the SetBlock call. However, since we trigger all dispatches to update_blocks.comp
        // in the same compute pass, the order on the GPU is undefined since there's no barrier
        SetBlocksBuffers[0].Emplace(Device, position, block);
        SetBlocksBufferCount = std::max(SetBlocksBufferCount, 1);
        UpdateGroups.insert({chunkX, chunkZ});
        Blocks[position.x][position.y][position.z] = block;
        Dirty = true;
    }
    else
    {
        SDL_Log("Bad block position: %d, %d, %d", position.x, position.y, position.z);
    }
}

Block World::GetBlock(glm::ivec3 position) const
{
    if (WorldToLocalPosition(position))
    {
        return Blocks[position.x][position.y][position.z];
    }
    else
    {
        return BlockAir;
    }
}

WorldQuery World::Raycast(const glm::vec3& position, const glm::vec3& direction, float length)
{
    WorldQuery query;
    query.Position = glm::floor(position);
    query.PreviousPosition = query.Position;
    glm::vec3 delta = glm::abs(1.0f / direction);
    glm::ivec3 step;
    glm::vec3 distance;
    for (int i = 0; i < 3; i++)
    {
        if (direction[i] < 0.0f)
        {
            step[i] = -1;
            distance[i] = (position[i] - query.Position[i]) * delta[i];
        }
        else
        {
            step[i] = 1;
            distance[i] = (query.Position[i] + 1.0f - position[i]) * delta[i];
        }
    }
    float traveled = 0.0f;
    while (traveled <= length)
    {
        Block block = GetBlock(query.Position);
        if (block != BlockAir)
        {
            query.HitBlock = block;
            return query;
        }
        query.PreviousPosition = query.Position;
        if (distance.x < distance.y)
        {
            if (distance.x < distance.z)
            {
                traveled = distance.x;
                distance.x += delta.x;
                query.Position.x += step.x;
            }
            else
            {
                traveled = distance.z;
                distance.z += delta.z;
                query.Position.z += step.z;
            }
        }
        else
        {
            if (distance.y < distance.z)
            {
                traveled = distance.y;
                distance.y += delta.y;
                query.Position.y += step.y;
            }
            else
            {
                traveled = distance.z;
                distance.z += delta.z;
                query.Position.z += step.z;
            }
        }
    }
    query.HitBlock = BlockAir;
    return query;
}

void World::SetOptions(const WorldOptions& options)
{
    WorldStateBuffer.Get().Options = options;
    Dirty = true;
}
