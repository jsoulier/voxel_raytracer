struct Input
{
    float4 Position : SV_Position;
    float4 Color : COLOR0;
};

float4 main(Input input) : SV_Target
{
    return input.Color;
}
