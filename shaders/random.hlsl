#ifndef RANDOM_HLSL
#define RANDOM_HLSL

uint Hash(uint x)
{
    x += (x << 10);
    x ^= (x >> 6);
    x += (x << 3);
    x ^= (x >> 11);
    x += (x << 15);
    return x;
}

uint Hash(uint2 v)
{
    return Hash(v.x ^ Hash(v.y));
}

uint Hash(uint3 v)
{
    return Hash(v.x ^ Hash(v.y) ^ Hash(v.z));
}

uint Hash(uint4 v)
{
    return Hash(v.x ^ Hash(v.y) ^ Hash(v.z) ^ Hash(v.w));
}

float FloatConstruct(uint m)
{
    const uint ieeeMantissa = 0x007FFFFF;
    const uint ieeeOne = 0x3F800000;
    m &= ieeeMantissa;
    m |= ieeeOne;
    float f = asfloat(m);
    return f - 1.0;
}

float Random(float x)
{
    return FloatConstruct(Hash(asuint(x)));
}

float Random(float2 v)
{
    return FloatConstruct(Hash(asuint(v)));
}

float Random(float3 v)
{
    return FloatConstruct(Hash(asuint(v)));
}

float Random(float4 v)
{
    return FloatConstruct(Hash(asuint(v)));
}

float Random(float v, float a, float b)
{
    return a + (b - a) * Random(v);
}

float Random(float2 v, float a, float b)
{
    return a + (b - a) * Random(v);
}

float Random(float3 v, float a, float b)
{
    return a + (b - a) * Random(v);
}

float Random(float4 v, float a, float b)
{
    return a + (b - a) * Random(v);
}

float3 RandomHemisphere(float3 normal, float2 id, uint sample, uint bounce)
{
    float u = Random(float4(id, sample * 1337.0f, bounce));
    float v = Random(float4(id, sample, bounce * 19.0f));
    float theta = acos(sqrt(1.0f - u));
    float phi = 2.0f * 3.14159265f * v;
    float3 direction;
    direction.x = sin(theta) * cos(phi);
    direction.y = cos(theta);
    direction.z = sin(theta) * sin(phi);
    float3 up = abs(normal.y) < 0.999f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);
    return tangent * direction.x + normal * direction.y + bitangent * direction.z;
}

#endif