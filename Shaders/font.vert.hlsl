
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


struct VertexInput
{
    uint id : SV_VertexID;
    float3 inPos : POSITION;
};

struct VertexOutput
{
    float4 fragCoord : SV_Position;
    float2 uv : TEXCOORD;
};

VertexOutput main(VertexInput vertexInput)
{
    VertexOutput output;
	//output.fragCoord = float4(vertexInput.inPos, 1.0f);

    //if(vertexInput.id == 0)
	//	output.fragCoord = float4(-0.5f, 0.0f, 0.0f, 1.0f);
    //if(vertexInput.id == 1)
	//	output.fragCoord = float4(-0.5f, 0.5f, 0.0f, 1.0f);
    //if(vertexInput.id == 2)
	//	output.fragCoord = float4(0.0f, 0.0f, 0.0f, 1.0f);
    //if(vertexInput.id == 3)
	//	output.fragCoord = float4(0.0f, 0.5f, 0.0f, 1.0f);

    int2 bx = int2(Bearing.x, 0);
    int2 by = int2(0, Bearing.y);

    int2 w = int2(Size.x, 0);
    int2 h = int2(0, Size.y);
    
    if(vertexInput.id == 0)
    {
        output.uv = (AtlasPos + h) / float2(AtlasResolution);
        output.fragCoord = mul(Projection, float4(CanvasPos + bx + by - h, 0.0f, 1.0f));
    }
    if(vertexInput.id == 1)
    {
        output.uv = (AtlasPos) / float2(AtlasResolution);
        output.fragCoord = mul(Projection, float4(CanvasPos + bx + by, 0.0f, 1.0f));
    }
    if(vertexInput.id == 2)
    {
        output.uv = (AtlasPos + h + w) / float2(AtlasResolution);
		output.fragCoord = mul(Projection, float4(CanvasPos + bx + by + w - h, 0.0f, 1.0f));
    }
    if(vertexInput.id == 3)
    {
        output.uv = (AtlasPos + w) / float2(AtlasResolution);
		output.fragCoord = mul(Projection, float4(CanvasPos + bx + by + w, 0.0f, 1.0f));
    }
	    
    
    return output;
}
