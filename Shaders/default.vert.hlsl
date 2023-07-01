cbuffer cb : register(b0)
{
    float3 iResolution : packoffset(c0.x);
    float iTime : packoffset(c0.w);
    float4 iMouse : packoffset(c1.x);
    float4 iDate : packoffset(c2.x);
    float iTimeDelta : packoffset(c3.x);
    float iFrameRate : packoffset(c3.y);
    float iFrame : packoffset(c3.z);
};

struct VertexInput
{
    float3 inPos : POSITION;
};

struct VertexOutput
{
    float4 fragCoord : SV_Position;
};

VertexOutput main(VertexInput vertexInput)
{
    VertexOutput output;
    output.fragCoord = float4(vertexInput.inPos, 1.0f);
    return output;
}
