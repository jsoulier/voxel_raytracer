#ifndef SHADER_HLSL
#define SHADER_HLSL

// https://github.com/libsdl-org/SDL_shadercross/issues/211
#include "../src/config.h"

struct WorldState
{
    float3 SkyBottom;
    int MaxSteps;
    float3 SkyTop;
    int MaxBounces;
    float3 SunColor;
    float SunIntensity;
    float3 SunDirection;
    float TimeOfDay;
    int2 Position;
};

struct CameraState
{
    float3 Position;
    float AspectRatio;
    float3 Forward;
    float TanHalfFov;
    float3 Right;
    float Padding1;
    float3 Up;
    float Padding2;
};

struct BlockState
{
    uint Color;
    float Light;
    float Roughness;
    float IOR;
};

static const uint kBlockAir = 0;
static const uint kBlockWater = 9;
static const float kEpsilon = 0.001f;

float3 GetColor(uint color)
{
    float red = (color >> 24) & 0xFFu;
    float green = (color >> 16) & 0xFFu;
    float blue = (color >> 8) & 0xFFu;
    return float3(red, green, blue) / 255.0f;
}

// https://gamedev.stackexchange.com/questions/92015/optimized-linear-to-srgb-glsl
float3 SRGBToLinear(float3 color)
{
    float3 low = color / 12.92f;
    float3 high = pow((color + 0.055f) / 1.055f, 2.4f);
    return lerp(high, low, step(color, 0.04045f));
}

float3 LinearToSRGB(float3 color)
{
    float3 low = color * 12.92f;
    float3 high = 1.055f * pow(color, 1.0f / 2.4f) - 0.055f;
    return lerp(high, low, step(color, 0.0031308f));
}

#endif
