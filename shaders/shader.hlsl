#ifndef SHADER_HLSL
#define SHADER_HLSL

struct WorldState
{
    float3 SkyBottom;
    int MaxSteps;
    float3 SkyTop;
    int MaxBounces;
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
static const float kEpsilon = 0.001f;

#endif