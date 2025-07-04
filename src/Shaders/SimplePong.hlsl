#define ROOT_SIG "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
                 "DescriptorTable(CBV(b0))"

struct ObjectConstants
{
    float4x4 worldMatrix;
    float4x4 viewProjMatrix;
    float4 objectColor;
    uint   useObjectColor;
};

ConstantBuffer<ObjectConstants> ObjConstants : register(b0);

struct VIn
{
    float4 position : POSITION0;
    float4 color    : COLOR0;
};

struct VOut
{
    float4 position : SV_POSITION;
    float4 color    : COLOR0;
};


[RootSignature(ROOT_SIG)]
VOut VS(VIn vIn)
{
    VOut output;

    //to clip space
    float4 worldPos = mul(vIn.position, ObjConstants.worldMatrix);
    output.position = mul(worldPos, ObjConstants.viewProjMatrix);
    
    output.color = vIn.color;

    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    if (ObjConstants.useObjectColor)
    {
        return ObjConstants.objectColor;
    }
    else
    {
        return pIn.color;
    }
}
