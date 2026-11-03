#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "profile.hpp"

template<typename T, SDL_GPUBufferUsageFlags U>
class GPUBuffer
{
public:
    GPUBuffer()
        : Buffer{nullptr}
        , Capacity{0}
    {
    }

    void Destroy(SDL_GPUDevice* device)
    {
        Profile();
        SDL_ReleaseGPUBuffer(device, Buffer);
        Buffer = nullptr;
        Capacity = 0;
    }

    bool Resize(SDL_GPUDevice* device, int capacity, bool copy)
    {
        Profile();
        if (capacity <= Capacity)
        {
            return true;
        }
        SDL_GPUBufferCreateInfo info{};
        info.usage = U;
        info.size = capacity * sizeof(T);
        SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(device, &info);
        if (!buffer)
        {
            SDL_Log("Failed to create buffer: %s", SDL_GetError());
            return false;
        }
        if (copy && Buffer && Capacity > 0)
        {
            SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
            if (!commandBuffer)
            {
                SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
                SDL_ReleaseGPUBuffer(device, buffer);
                return false;
            }
            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
            if (!copyPass)
            {
                SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
                SDL_CancelGPUCommandBuffer(commandBuffer);
                SDL_ReleaseGPUBuffer(device, buffer);
                return false;
            }
            SDL_GPUBufferLocation source{};
            source.buffer = Buffer;
            SDL_GPUBufferLocation destination{};
            destination.buffer = buffer;
            SDL_CopyGPUBufferToBuffer(copyPass, &source, &destination, Capacity * sizeof(T), false);
            SDL_EndGPUCopyPass(copyPass);
            SDL_SubmitGPUCommandBuffer(commandBuffer);
        }
        SDL_ReleaseGPUBuffer(device, Buffer);
        Buffer = buffer;
        Capacity = capacity;
        return true;
    }

    SDL_GPUBuffer* GetBuffer() const
    {
        return Buffer;
    }

    int GetCapacity() const
    {
        return Capacity;
    }

private:
    SDL_GPUBuffer* Buffer;
    int Capacity;
};

template<typename T, SDL_GPUBufferUsageFlags U = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ>
class DynamicBuffer
{
    static constexpr int kStartingCapacity = 4096;
    static constexpr int kGrowthRate = 2;

public:
    DynamicBuffer()
        : Buffer{}
        , TransferBuffer{nullptr}
        , TransferBufferCapacity{0}
        , Data{nullptr}
    {
        Reset();
    }

    void Destroy(SDL_GPUDevice* device)
    {
        Profile();
        if (Data)
        {
            SDL_UnmapGPUTransferBuffer(device, TransferBuffer);
            Data = nullptr;
        }
        Buffer.Destroy(device);
        SDL_ReleaseGPUTransferBuffer(device, TransferBuffer);
        TransferBuffer = nullptr;
    }

    void Reset()
    {
        SDL_assert(!Data);
        BufferSize = 0;
        TransferBufferSize = 0;
    }

    template<typename... Args>
    void Emplace(SDL_GPUDevice* device, Args&&... args)
    {
        if (!Data && TransferBuffer)
        {
            BufferSize = 0;
            SDL_assert(!TransferBufferSize);
            Data = static_cast<T*>(SDL_MapGPUTransferBuffer(device, TransferBuffer, true));
            if (!Data)
            {
                SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
                return;
            }
        }
        SDL_assert(TransferBufferSize <= TransferBufferCapacity);
        if (TransferBufferSize == TransferBufferCapacity)
        {
            ProfileBlock("DynamicBuffer::Emplace::Grow");
            int capacity = std::max(kStartingCapacity, TransferBufferSize * kGrowthRate);
            SDL_GPUTransferBufferCreateInfo info{};
            info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            info.size = capacity * sizeof(T);
            SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
            if (!transferBuffer)
            {
                SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
                return;
            }
            T* data = static_cast<T*>(SDL_MapGPUTransferBuffer(device, transferBuffer, false));
            if (!data)
            {
                SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
                SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
                return;
            }
            if (Data)
            {
                std::copy(Data, Data + TransferBufferSize, data);
                SDL_UnmapGPUTransferBuffer(device, TransferBuffer);
            }
            SDL_ReleaseGPUTransferBuffer(device, TransferBuffer);
            TransferBufferCapacity = capacity;
            TransferBuffer = transferBuffer;
            Data = data;
        }
        SDL_assert(Data);
        Data[TransferBufferSize++] = T{std::forward<Args>(args)...};
    }

    void AssignRange(SDL_GPUDevice* device, const T* values, int count, int start, bool copy)
    {
        Profile();
        int end = start + count;
        if (!Buffer.Resize(device, end, copy))
        {
            return;
        }
        if (end > TransferBufferCapacity)
        {
            if (Data)
            {
                SDL_UnmapGPUTransferBuffer(device, TransferBuffer);
                Data = nullptr;
            }
            int capacity = std::max(kStartingCapacity, end);
            SDL_GPUTransferBufferCreateInfo info{};
            info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            info.size = capacity * sizeof(T);
            SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
            if (!transferBuffer)
            {
                SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
                return;
            }
            SDL_ReleaseGPUTransferBuffer(device, TransferBuffer);
            TransferBuffer = transferBuffer;
            TransferBufferCapacity = capacity;
        }
        if (!Data)
        {
            Data = static_cast<T*>(SDL_MapGPUTransferBuffer(device, TransferBuffer, true));
            if (!Data)
            {
                SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
                return;
            }
        }
        std::copy(values, values + count, Data + start);
        TransferBufferSize = std::max(TransferBufferSize, end);
    }

    void Assign(SDL_GPUDevice* device, const T* values, int count)
    {
        AssignRange(device, values, count, 0, false);
    }

    bool UploadRange(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass, int count, int start, bool cycle)
    {
        Profile();
        if (!Buffer.Resize(device, start + count, false))
        {
            return false;
        }
        if (Data)
        {
            SDL_UnmapGPUTransferBuffer(device, TransferBuffer);
            Data = nullptr;
        }
        SDL_GPUTransferBufferLocation location{};
        location.transfer_buffer = TransferBuffer;
        location.offset = start * sizeof(T);
        SDL_GPUBufferRegion region{};
        region.buffer = Buffer.GetBuffer();
        region.offset = start * sizeof(T);
        region.size = count * sizeof(T);
        SDL_UploadToGPUBuffer(copyPass, &location, &region, cycle);
        BufferSize = std::max(BufferSize, start + count);
        return true;
    }

    void Upload(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass)
    {
        Profile();
        if (Data)
        {
            SDL_UnmapGPUTransferBuffer(device, TransferBuffer);
            Data = nullptr;
        }
        int size = TransferBufferSize;
        TransferBufferSize = 0;
        if (!size)
        {
            BufferSize = 0;
            return;
        }
        BufferSize = 0;
        UploadRange(device, copyPass, size, 0, true);
    }

    void Upload(SDL_GPUDevice* device, SDL_GPUCommandBuffer* commandBuffer)
    {
        Profile();
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        if (!copyPass)
        {
            SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
            return;
        }
        Upload(device, copyPass);
        SDL_EndGPUCopyPass(copyPass);
    }

    void Upload(SDL_GPUDevice* device)
    {
        Profile();
        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        if (!commandBuffer)
        {
            SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
            return;
        }
        Upload(device, commandBuffer);
        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    SDL_GPUBuffer* GetBuffer() const
    {
        SDL_assert(!Data);
        return Buffer.GetBuffer();
    }

    int GetSize() const
    {
        SDL_assert(!Data);
        return BufferSize;
    }

private:
    GPUBuffer<T, U> Buffer;
    SDL_GPUTransferBuffer* TransferBuffer;
    int BufferSize;
    int TransferBufferSize;
    int TransferBufferCapacity;
    T* Data;
};

template<typename T, SDL_GPUBufferUsageFlags U = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ>
class StaticBuffer
{
public:
    StaticBuffer()
        : Buffer{}
        , TransferBuffer{nullptr}
        , Data{}
        , Dirty{true}
    {
    }

    bool Init(SDL_GPUDevice* device)
    {
        Profile();
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = sizeof(T);
        TransferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!TransferBuffer)
        {
            SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
            return false;
        }
        return Buffer.Resize(device, 1, false);
    }

    void Destroy(SDL_GPUDevice* device)
    {
        Profile();
        Buffer.Destroy(device);
        SDL_ReleaseGPUTransferBuffer(device, TransferBuffer);
        TransferBuffer = nullptr;
    }

    void Upload(SDL_GPUDevice* device, SDL_GPUCopyPass* copyPass)
    {
        Profile();
        if (!Dirty)
        {
            return;
        }
        T* data = static_cast<T*>(SDL_MapGPUTransferBuffer(device, TransferBuffer, false));
        if (!data)
        {
            SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
            return;
        }
        *data = Data;
        SDL_UnmapGPUTransferBuffer(device, TransferBuffer);
        SDL_GPUTransferBufferLocation location{};
        SDL_GPUBufferRegion region{};
        location.transfer_buffer = TransferBuffer;
        region.buffer = Buffer.GetBuffer();
        region.size = sizeof(T);
        SDL_UploadToGPUBuffer(copyPass, &location, &region, true);
        Dirty = false;
    }

    void Upload(SDL_GPUDevice* device, SDL_GPUCommandBuffer* commandBuffer)
    {
        Profile();
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        if (!copyPass)
        {
            SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
            return;
        }
        Upload(device, copyPass);
        SDL_EndGPUCopyPass(copyPass);
    }

    void Upload(SDL_GPUDevice* device)
    {
        Profile();
        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        if (!commandBuffer)
        {
            SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
            return;
        }
        Upload(device, commandBuffer);
        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    SDL_GPUBuffer* GetBuffer() const
    {
        return Buffer.GetBuffer();
    }

    T& Get()
    {
        Dirty = true;
        return Data;
    }

    const T& operator*() const
    {
        return Data;
    }

    const T* operator->() const
    {
        return &Data;
    }

    bool GetDirty() const
    {
        return Dirty;
    }

private:
    GPUBuffer<T, U> Buffer;
    SDL_GPUTransferBuffer* TransferBuffer;
    T Data;
    bool Dirty;
};

template<typename T, typename TraitsT, SDL_GPUBufferUsageFlags U = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ>
class SpatialBuffer
{
    static constexpr int kMaxUploads = 16;
    static constexpr int kMaxTombstones = 16;

    struct Upload
    {
        int Index;
        T Value;
    };

public:
    SpatialBuffer()
        : Buffer{}
        , Values{}
        , Indices{}
        , FreeIndices{}
        , Uploads{}
    {
    }

    bool Init(SDL_GPUDevice* device)
    {
        Profile();
        Values.clear();
        Values.push_back(TraitsT::kInvalid);
        Indices.clear();
        FreeIndices.assign(1, 0);
        Uploads.assign(1, {0, TraitsT::kInvalid});
        return true;
    }

    void Destroy(SDL_GPUDevice* device)
    {
        Profile();
        Buffer.Destroy(device);
    }

    void Add(const glm::ivec3& position, const T& value)
    {
        Profile();
        auto it = Indices.find(position);
        if (it != Indices.end())
        {
            Values[it->second] = value;
            Uploads.push_back({it->second, value});
            return;
        }
        int index;
        if (FreeIndices.empty())
        {
            index = Values.size();
            Values.push_back(value);
        }
        else
        {
            index = FreeIndices.back();
            FreeIndices.pop_back();
            Values[index] = value;
        }
        Indices.emplace(position, index);
        Uploads.push_back({index, value});
    }

    void Remove(const glm::ivec3& position)
    {
        Profile();
        auto it = Indices.find(position);
        if (it != Indices.end())
        {
            int index = it->second;
            Values[index] = TraitsT::kInvalid;
            Uploads.push_back({index, TraitsT::kInvalid});
            FreeIndices.push_back(index);
            Indices.erase(it);
        }
    }

    void Resize(const glm::ivec2& min, const glm::ivec2& max)
    {
        Profile();
        for (auto it = Indices.begin(); it != Indices.end();)
        {
            glm::ivec3 position = it->first;
            it++;
            if (position.x < min.x || position.z < min.y || position.x >= max.x || position.z >= max.y)
            {
                Remove(position);
            }
        }
    }

    void ShrinkToFit()
    {
        Profile();
        Uploads.clear();
        int lastIndex = Values.size() - 1;
        while (!FreeIndices.empty() && lastIndex >= Indices.size())
        {
            int freeIndex = FreeIndices.back();
            if (freeIndex >= Indices.size())
            {
                FreeIndices.pop_back();
                continue;
            }
            if (!TraitsT::IsValid(Values[lastIndex]))
            {
                lastIndex--;
                continue;
            }
            FreeIndices.pop_back();
            Values[freeIndex] = Values[lastIndex];
            auto it = Indices.find(TraitsT::GetPosition(Values[freeIndex]));
            SDL_assert(it != Indices.end());
            it->second = freeIndex;
            Uploads.push_back({freeIndex, Values[freeIndex]});
            lastIndex--;
        }
        FreeIndices.clear();
        Values.resize(Indices.size());
        Buffer.Reset();
    }

    void Update(SDL_GPUDevice* device, SDL_GPUCommandBuffer* commandBuffer)
    {
        Profile();
        if (FreeIndices.size() > kMaxTombstones)
        {
            ShrinkToFit();
        }
        if (Uploads.empty())
        {
            return;
        }
        if (Uploads.size() >= kMaxUploads)
        {
            Buffer.Assign(device, Values.data(), Values.size());
            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
            if (!copyPass)
            {
                SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
                return;
            }
            Buffer.Upload(device, copyPass);
            SDL_EndGPUCopyPass(copyPass);
            Uploads.clear();
            return;
        }
        for (const Upload& upload : Uploads)
        {
            Buffer.AssignRange(device, &upload.Value, 1, upload.Index, true);
        }
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        if (!copyPass)
        {
            SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
            return;
        }
        for (const Upload& upload : Uploads)
        {
            Buffer.UploadRange(device, copyPass, 1, upload.Index, false);
        }
        SDL_EndGPUCopyPass(copyPass);
        Uploads.clear();
    }

    SDL_GPUBuffer* GetBuffer() const
    {
        return Buffer.GetBuffer();
    }

    int GetSize() const
    {
        return Values.size();
    }

private:
    DynamicBuffer<T, U> Buffer;
    std::vector<T> Values;
    std::vector<Upload> Uploads;
    std::unordered_map<glm::ivec3, int> Indices;
    std::vector<int> FreeIndices;
};
