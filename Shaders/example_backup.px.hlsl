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

static float3 camEye =   float3(0.f,0.f,5.f);
static float3 camDir =   float3(0.f,0.f,-1.f);
static float3 camUp =    float3(0.f,1.f,0.f);
static float3 camRight = float3(1.f,0.f,0.f);
static float near = 1.f;
static float far = 15.f;

float sdSphere( float3 p, float s )
{
    return length(p)-s;
}

float3 GenerateRay(float2 coord)
{
    return camDir * near + coord.y * camUp + coord.x * camRight;
}

PixelOutput main(PixelInput pixelInput)
{
    
    PixelOutput output;
    // Normalized pixel coordinates (from 0 to 1)
    //float2 uv = pixelInput.fragCoord.xy/iResolution.xy;

    // Time varying pixel color
    //float3 col = 0.5 + 0.5*cos(iTime+uv.xyx+float3(0,2,4));
    
    // Output to screen
    //output.color = float4(col, 1.0);
    
    float2 uv = (pixelInput.fragCoord.xy * 2.0 - iResolution.xy) / iResolution.y;
    uv.y *= -1.f;
    
    float3 rayDir = GenerateRay(uv);
    rayDir = normalize(rayDir);
    float3 rayStart = camEye;
    float3 col = float3(0.f, 0.f, 0.f);
    
    for(int i = 0; i < 10; i++)
    {
        float d = sdSphere(rayStart, 2.99f);
        rayStart = rayStart + rayDir * d;
        if(d <= 0.001f)
        {   
            col = float3(rayStart);
            break;
        }
    }
    
    
    
    output.color = float4(col, 1.0f);
    
    return output;
}
