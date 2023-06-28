cbuffer cb : register(b0)
{
    float iTime : packoffset(c0);
};

struct VertexInput
{
    float3 inPos : POSITION;
};

struct VertexOutput
{
    float4 position : SV_Position;
};

VertexOutput main(VertexInput vertexInput)
{
    VertexOutput output;
    output.position = float4(vertexInput.inPos, 1.0f);
    return output;
}
