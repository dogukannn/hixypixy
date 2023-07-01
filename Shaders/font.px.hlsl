
cbuffer cb : register(b0)
{
    float4x4 Projection : packoffset(c0.x);
	int2 AtlasPos : packoffset(c4.x);
	int2 AtlasResolution : packoffset(c4.z);
    int2 Size : packoffset(c5.x);
    int2 Bearing : packoffset(c5.z);
    int2 CanvasPos : packoffset(c6.x);
    int Advance : packoffset(c6.z);
};

struct PixelInput
{
    float4 fragCoord : SV_Position;
    float2 uv : TEXCOORD;
};

struct PixelOutput
{
    float4 color : SV_Target0;
};

Texture2D g_texture : register(t0);
SamplerState s1 : register(s0);

PixelOutput main(PixelInput pixelInput)
{
    PixelOutput output;
    //output.color = mul(Projection, float4(1.0f, 1.0f, 1.0f, 1.0f));
    output.color = g_texture.Sample(s1, pixelInput.uv);

    return output;
}
