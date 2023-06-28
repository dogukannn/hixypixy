struct PixelInput
{
    float4 position : SV_Position;
};

struct PixelOutput
{
    float4 attachment0 : SV_Target0;
};

PixelOutput main(PixelInput pixelInput)
{
    PixelOutput output;
    if(pixelInput.position.x > 700.f)
        output.attachment0 = float4(1.0f, 0.0f, 0.0f, 1.0f);
    else
        output.attachment0 = float4(1.0f, 0.3f, 1.0f, 1.0f);
    return output;
}
