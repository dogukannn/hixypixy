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


float rand(float3 p) 
{
    return frac(sin(dot(p, float3(12.345, 67.89, 412.12))) * 42123.45) * 2.0f - 1.0f;
    //return frac(sin(dot(p,
    //                     float3(12.9898,78.233,24.52345)))*
    //    43758.5453123) * 2.0f - .9f;
}

float valueNoise(float3 p)
{
    float3 u = floor(p);
    float3 v = frac(p);
    float3 s = smoothstep(0.0, 1.0, v);
    
    
    float a = rand(u);
    float b = rand(u + float3(1.0, 0.0, 0.0));
    float c = rand(u + float3(0.0, 1.0, 0.0));
    float d = rand(u + float3(1.0, 1.0, 0.0));
    float e = rand(u + float3(0.0, 0.0, 1.0));
    float f = rand(u + float3(1.0, 0.0, 1.0));
    float g = rand(u + float3(0.0, 1.0, 1.0));
    float h = rand(u + float3(1.0, 1.0, 1.0));
    
    return lerp(lerp(lerp(a, b, s.x), lerp(c, d, s.x), s.y),
               lerp(lerp(e, f, s.x), lerp(g, h, s.x), s.y),
               s.z);
}

float fbm(float3 p)
{
    float3 q = p - float3(0.1, 0.0, 0.0) * iTime;
    int numOctaves = 8;
    float weight = 0.7;
    float ret = 0.0;
    
    for (int i = 0; i < numOctaves; i++)
    {
        ret += weight * valueNoise(q);
        q *= 2.0;
        weight *= 0.5;
    }
    
    return clamp(ret - p.y, 0.0, 1.0);
}

float4 volumetricMarch(float3 ro, float3 rd)
{
    float depth = 0.0;
    float4 color = float4(0., 0., 0., 0.);
    
    for (int i = 0; i < 150; i++)
    {
        float3 p = ro + depth * rd;
        float density = fbm(p * 0.9);
        density *= valueNoise(p * 0.4);
        
        if(density > 1e-3)
        {
            float4 c = float4(lerp(float3(1.0, 1.0, 1.0), float3(0.0, 0.0, 0.0), density), density);
            c.a *= 0.5;
            c.rgb *= c.a;
            color += c * (1.0 - color.a);
        }
        
        depth += max(0.05, 0.02 * depth);
    }
    
    return float4(clamp(color.rgb, 0.0, 1.0), color.a);
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
    //float2 uv = (pixelInput.fragCoord.xy / iResolution.xy) * 2.0 - 1.0;
    uv.y *= -1.f;
    
    float3 bgColor = lerp(float3(0.902, 0.463, 0.059), float3(0.059, 0.675, 0.902),  uv.y * 0.5  + 0.3);
    
    
    float3 ro = float3(0.0, 1.0, iTime);
    float3 front = normalize(float3(0.0, -0.1, 1.0));
    float3 right = normalize(cross(front, float3(0.0, 1.0, 0.0)));
    float3 up = normalize(cross(right, front));
    float3x3 lookAt = float3x3(right, up, front);
    float3 rd = mul(lookAt, normalize(float3(uv, 1.0f)));
    float4 cloudColor = volumetricMarch(ro, rd);
    
    cloudColor = float4(pow(cloudColor.xyz, 0.4545), cloudColor.a);
    
    //cloudColor.a -= uv.y / 5;
    
    cloudColor = float4(lerp(bgColor, cloudColor.rgb, cloudColor.a), 1.0f);
    //float3 col = fbm(float3(uv, iTime));
    //float3 col = fbm(float3(uv, iTime));
    //col = normalize(col);
    
    output.color = float4(cloudColor.xyz, 1.0f);
    
    return output;
}
