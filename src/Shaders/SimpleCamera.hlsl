struct ObjectConstants {
    float4x4 World;
    float4x4 WorldViewProj;
};
ConstantBuffer<ObjectConstants> objectConstants : register(b0);

struct VIn
{
    float4 position : POSITION0;
    float4 color : COLOR0;
};

struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

VOut VS(VIn vIn)
{
    VOut output;

    output.position = mul(vIn.position, objectConstants.WorldViewProj);
    output.color = vIn.color;

    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    return pIn.color;
}

