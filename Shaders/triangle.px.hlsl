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

struct PixelInput
{
    float4 fragCoord : SV_Position;
};

struct PixelOutput
{
    float4 color : SV_Target0;
};

PixelOutput main(PixelInput pixelInput)
{
    PixelOutput output;
    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = pixelInput.fragCoord.xy/iResolution.xy;

    // Time varying pixel color
    float3 col = 0.5 + 0.5*cos(iTime+uv.xyx+float3(0,2,4));

    // Output to screen
    output.color = float4(col, 1.0);
    return output;
}
