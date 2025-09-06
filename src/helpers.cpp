#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include <stb_image.h>

#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <string>

#include "helpers.hpp"

static void* LoadShaderInternal(SDL_GPUDevice* device, const char* name)
{
    SDL_GPUShaderFormat shaderFormat = SDL_GetGPUShaderFormats(device);
    const char* entrypoint;
    const char* fileExtension;
    if (shaderFormat & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        shaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
        fileExtension = "spv";
    }
    else if (shaderFormat & SDL_GPU_SHADERFORMAT_DXIL)
    {
        shaderFormat = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
        fileExtension = "dxil";
    }
    else if (shaderFormat & SDL_GPU_SHADERFORMAT_MSL)
    {
        shaderFormat = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
        fileExtension = "msl";
    }
    else
    {
        SDL_assert(false);
    }
    std::filesystem::path path = SDL_GetBasePath();
    path /= name;
    std::string shaderPath = std::format("{}.{}", path.string(), fileExtension);
    std::ifstream shaderFile(shaderPath, std::ios::binary);
    if (shaderFile.fail())
    {
        SDL_Log("Failed to open shader: %s", shaderPath.data());
        return nullptr;
    }
    std::string jsonPath = std::format("{}.json", path.string());
    std::ifstream jsonFile(jsonPath, std::ios::binary);
    if (jsonFile.fail())
    {
        SDL_Log("Failed to open json: %s", jsonPath.data());
        return nullptr;
    }
    std::string shaderData(std::istreambuf_iterator<char>(shaderFile), {});
    nlohmann::json json;
    try
    {
        jsonFile >> json;
    }
    catch (const std::exception& exception)
    {
        SDL_Log("Failed to parse json: %s, %s", jsonPath.data(), exception.what());
        return nullptr;
    }
    void* shader = nullptr;
    if (std::strstr(name, ".comp"))
    {
        SDL_GPUComputePipelineCreateInfo info{};
        info.num_samplers = json["samplers"];
        info.num_readonly_storage_textures = json["readonly_storage_textures"];
        info.num_readonly_storage_buffers = json["readonly_storage_buffers"];
        info.num_readwrite_storage_textures = json["readwrite_storage_textures"];
        info.num_readwrite_storage_buffers = json["readwrite_storage_buffers"];
        info.num_uniform_buffers = json["uniform_buffers"];
        info.threadcount_x = json["threadcount_x"];
        info.threadcount_y = json["threadcount_y"];
        info.threadcount_z = json["threadcount_z"];
        info.code = reinterpret_cast<Uint8*>(shaderData.data());
        info.code_size = shaderData.size();
        info.entrypoint = entrypoint;
        info.format = shaderFormat;
        shader = SDL_CreateGPUComputePipeline(device, &info);
    }
    else
    {
        SDL_GPUShaderCreateInfo info{};
        info.num_samplers = json["samplers"];
        info.num_storage_textures = json["storage_textures"];
        info.num_storage_buffers = json["storage_buffers"];
        info.num_uniform_buffers = json["uniform_buffers"];
        info.code = reinterpret_cast<Uint8*>(shaderData.data());
        info.code_size = shaderData.size();
        info.entrypoint = entrypoint;
        info.format = shaderFormat;
        if (std::strstr(name, ".frag"))
        {
            info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        }
        else
        {
            info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
        }
        shader = SDL_CreateGPUShader(device, &info);
    }
    if (!shader)
    {
        SDL_Log("Failed to create shader: %s, %s", name, SDL_GetError());
        return nullptr;
    }
    return shader;
}

SDL_GPUShader* LoadShader(SDL_GPUDevice* device, const char* name)
{
    return static_cast<SDL_GPUShader*>(LoadShaderInternal(device, name));
}

SDL_GPUComputePipeline* LoadComputePipeline(SDL_GPUDevice* device, const char* name)
{
    return static_cast<SDL_GPUComputePipeline*>(LoadShaderInternal(device, name));
}

SDL_GPUTexture* LoadTexture(SDL_GPUDevice* device, const char* name)
{
    std::filesystem::path path = SDL_GetBasePath();
    path /= name;
    int width;
    int height;
    int channels;
    void* srcData = stbi_load(path.string().data(), &width, &height, &channels, 4);
    if (!srcData)
    {
        SDL_Log("Failed to load image: %s, %s", path.string().data(), stbi_failure_reason());
        return nullptr;
    }
    SDL_GPUTexture* texture;
    SDL_GPUTransferBuffer* transferBuffer;
    {
        SDL_GPUTextureCreateInfo info{};
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        texture = SDL_CreateGPUTexture(device, &info);
        if (!texture)
        {
            SDL_Log("Failed to create texture: %s", SDL_GetError());
            stbi_image_free(srcData);
            return nullptr;
        }
    }
    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = width * height * 4;
        transferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!transferBuffer)
        {
            SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
            stbi_image_free(srcData);
            SDL_ReleaseGPUTexture(device, texture);
            return nullptr;
        }
    }
    void* dstData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    if (!dstData)
    {
        SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
        stbi_image_free(srcData);
        SDL_ReleaseGPUTexture(device, texture);
        return nullptr;
    }
    std::memcpy(dstData, srcData, width * height * 4);
    stbi_image_free(srcData);
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!commandBuffer)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTexture(device, texture);
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        return nullptr;
    }
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (!copyPass)
    {
        SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
        SDL_ReleaseGPUTexture(device, texture);
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        SDL_CancelGPUCommandBuffer(commandBuffer);
        return nullptr;
    }
    SDL_GPUTextureTransferInfo info{};
    SDL_GPUTextureRegion region{};
    info.transfer_buffer = transferBuffer;
    region.texture = texture;
    region.w = width;
    region.h = height;
    region.d = 1;
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);
    SDL_UploadToGPUTexture(copyPass, &info, &region, true);
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    return texture;
}