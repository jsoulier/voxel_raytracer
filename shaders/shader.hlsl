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
    float3 ForwardVector;
    float TanHalfFov;
    float3 RightVector;
    float Padding;
};

#endif
