#include "shader.hlsl"

cbuffer UniformBuffer : register(b0, space1)
{
    int4 Position;
};

StructuredBuffer<CameraState> cameraState : register(t0, space0);

static const float3 kPositions[8] =
{
    float3(-0.5f,-0.5f,-0.5f ),
    float3( 0.5f,-0.5f,-0.5f ),
    float3( 0.5f, 0.5f,-0.5f ),
    float3(-0.5f, 0.5f,-0.5f ),
    float3(-0.5f,-0.5f, 0.5f ),
    float3( 0.5f,-0.5f, 0.5f ),
    float3( 0.5f, 0.5f, 0.5f ),
    float3(-0.5f, 0.5f, 0.5f ),
};

static const uint kIndices[36] =
{
    0, 1, 2, 0, 2, 3,
    5, 4, 7, 5, 7, 6,
    4, 0, 3, 4, 3, 7,
    1, 5, 6, 1, 6, 2,
    3, 2, 6, 3, 6, 7,
    4, 5, 1, 4, 1, 0,
};

static const float4 kColor = float4(1.0f, 1.0f, 1.0f, 0.1f);

struct Output
{
    float4 Position : SV_Position;
    float4 Color : COLOR0;
};

Output main(uint VertexID : SV_VertexID)
{
    float3 position = kPositions[kIndices[VertexID]] * 1.025f;
    position += Position.xyz + 0.5f;
    Output output;
    output.Position = mul(cameraState[0].ViewProjection, float4(position, 1.0f));
    output.Position.x *= -1.0f;
    output.Color = kColor;
    return output;
}
