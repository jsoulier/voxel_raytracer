#pragma once

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include "profile.hpp"

template<typename T, SDL_GPUBufferUsageFlags U = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ>
class DynamicBuffer
{
public:
    static constexpr int kStartingCapacity = 4096;
    static constexpr int kGrowthRate = 2;

    DynamicBuffer()
        : Buffer{nullptr}
        , TransferBuffer{nullptr}
        , BufferSize{0}
        , TransferBufferSize{0}
        , BufferCapacity{0}
        , TransferBufferCapacity{0}
        , Data{nullptr}
    {
    }

    void Destroy(SDL_GPUDevice* device)
    {
        if (Data)
        {
            SDL_UnmapGPUTransferBuffer(device, TransferBuffer);
            Data = nullptr;
        }
        SDL_ReleaseGPUBuffer(device, Buffer);
        SDL_ReleaseGPUTransferBuffer(device, TransferBuffer);
        Buffer = nullptr;
        TransferBuffer = nullptr;
    }

    template<typename... Args>
    void Emplace(SDL_GPUDevice* device, Args&&... args)
    {
        if (!Data && TransferBuffer)
        {
            ProfileBlock("Buffer::Emplace::Map");
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
            ProfileBlock("Buffer::Emplace::Realloc");
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
                ProfileBlock("Buffer::Emplace::Realloc::Copy");
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
        if (TransferBufferCapacity > BufferCapacity)
        {
            ProfileBlock("Buffer::Upload::Reallocate");
            SDL_ReleaseGPUBuffer(device, Buffer);
            Buffer = nullptr;
            BufferCapacity = 0;
            SDL_GPUBufferCreateInfo info{};
            info.usage = U;
            info.size = TransferBufferCapacity * sizeof(T);
            Buffer = SDL_CreateGPUBuffer(device, &info);
            if (!Buffer)
            {
                SDL_Log("Failed to create buffer: %s", SDL_GetError());
                return;
            }
            BufferCapacity = TransferBufferCapacity;
        }
        SDL_GPUTransferBufferLocation location{};
        SDL_GPUBufferRegion region{};
        location.transfer_buffer = TransferBuffer;
        region.buffer = Buffer;
        region.size = size * sizeof(T);
        SDL_UploadToGPUBuffer(copyPass, &location, &region, true);
        BufferSize = size;
    }

    SDL_GPUBuffer* GetBuffer() const
    {
        SDL_assert(!Data);
        return Buffer;
    }

    uint32_t GetSize() const
    {
        SDL_assert(!Data);
        return BufferSize;
    }

private:
    SDL_GPUBuffer* Buffer;
    SDL_GPUTransferBuffer* TransferBuffer;
    int BufferSize;
    int TransferBufferSize;
    int BufferCapacity;
    int TransferBufferCapacity;
    T* Data;
};

template<typename T, SDL_GPUBufferUsageFlags U = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ>
class StaticBuffer
{
public:
    StaticBuffer()
        : Buffer{nullptr}
        , TransferBuffer{nullptr}
        , Data{}
        , Dirty{false}
    {
    }

    bool Init(SDL_GPUDevice* device)
    {
        Profile();
        {
            SDL_GPUTransferBufferCreateInfo info{};
            info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            info.size = sizeof(T);
            TransferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
            if (!TransferBuffer)
            {
                SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
                return false;
            }
        }
        {
            SDL_GPUBufferCreateInfo info{};
            info.usage = U;
            info.size = sizeof(T);
            Buffer = SDL_CreateGPUBuffer(device, &info);
            if (!Buffer)
            {
                SDL_Log("Failed to create buffer: %s", SDL_GetError());
                return false;
            }
        }
        return true;
    }

    void Destroy(SDL_GPUDevice* device)
    {
        SDL_ReleaseGPUBuffer(device, Buffer);
        SDL_ReleaseGPUTransferBuffer(device, TransferBuffer);
        Buffer = nullptr;
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
        region.buffer = Buffer;
        region.size = sizeof(T);
        SDL_UploadToGPUBuffer(copyPass, &location, &region, true);
        Dirty = false;
    }

    SDL_GPUBuffer* GetBuffer() const
    {
        return Buffer;
    }

    T* GetPointer()
    {
        Dirty = true;
        return &Data;
    }

    const T* operator->() const
    {
        return &Data;
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

private:
    SDL_GPUBuffer* Buffer;
    SDL_GPUTransferBuffer* TransferBuffer;
    T Data;
    bool Dirty;
};