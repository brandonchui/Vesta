#define ROOT_SIG "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
                 "DescriptorTable(CBV(b0))"                
                
struct ObjectConstants
{
 float colorChangeValue;
};
ConstantBuffer<ObjectConstants> ObjConstants : register(b0);

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

[RootSignature(ROOT_SIG)]
VOut VS(VIn vIn)
{
    VOut output;

    output.position = vIn.position;
	
	output.color = vIn.color * ObjConstants.colorChangeValue;
	
    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
    return pIn.color;
}

