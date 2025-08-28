#ifndef SHADER_HLSL
#define SHADER_HLSL

struct WorldState
{
    float2 Position;
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

#endif
