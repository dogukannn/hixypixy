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
    //float2 uv = pixelInput.fragCoord.xy/iResolution.xy;
    //float2 uv = iMouse.xy/iResolution.xy;

    // Time varying pixel color
    //float3 col = 0.5 + 0.5*cos(iTime+uv.xyx+float3(0,2,4));
    float3 col = float3(0., 0., 0.);
    if(iMouse.z < 0.0f && iMouse.w < 0.0f)
    {
        col = float3(iMouse.x / 800.0f, 1., 1.);
    }
    
    // Output to screen
    output.color = float4(col, 1.0);
    return output;
}
