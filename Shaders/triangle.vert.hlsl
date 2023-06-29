cbuffer cb : register(b0)
{
    //: packoffset(c0);
    float3 iResolution;
    float iTime;
    float iTimeDelta;
    float iFrameRate;
    int iFrame;
    float4 iMouse;
    float4 iDate;   
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
