#define ROOT_SIG "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
                 "DescriptorTable(CBV(b0)), " \
                 "DescriptorTable(CBV(b1))"

struct ObjectConstants
{
    float4x4 worldMatrix;
    float4x4 viewProjMatrix;
};
ConstantBuffer<ObjectConstants> ObjConstants : register(b0);

struct SceneConstants
{
    float3 lightDirection;
    float4 lightColor;
    float3 cameraPosition;
};
ConstantBuffer<SceneConstants> SceneConsts : register(b1);

struct VIn
{
    float4 position : POSITION0;
    float3 normal   : NORMAL0;
    float4 color    : COLOR0;
};

struct VOut
{
    float4 position     : SV_POSITION;
    float3 worldNormal  : NORMAL0;
    float4 color        : COLOR0;
    float4 worldPos     : TEXCOORD0;
};


[RootSignature(ROOT_SIG)]
VOut VS(VIn vIn)
{
    VOut output;
    
    output.worldPos = mul(vIn.position, ObjConstants.worldMatrix);
    output.position = mul(output.worldPos, ObjConstants.viewProjMatrix);
    
    output.worldNormal = normalize(mul((float3)vIn.normal, (float3x3)ObjConstants.worldMatrix));
    output.color = vIn.color;

    return output;
}

float4 PS(VOut pIn) : SV_TARGET
{
	//diffuse 
    float3 objectColor = pIn.color.rgb;
    float3 normal = normalize(pIn.worldNormal);
    float3 lightVec = normalize(-SceneConsts.lightDirection);
    
    float NdotL = dot(normal, lightVec);
    float diffuseFactor = (NdotL * 0.5f) + 0.5f;
    float3 diffuseColor = objectColor * SceneConsts.lightColor.rgb * diffuseFactor;
    
	//specular
    float3 viewDir = normalize(SceneConsts.cameraPosition - pIn.worldPos.xyz);
    float3 halfVec = normalize(lightVec + viewDir);
    float shininess = 32.0f;
    
    float NdotH = saturate(dot(normal, halfVec));
    float specularFactor = pow(NdotH, shininess);
    
    float3 specularColor = specularFactor * SceneConsts.lightColor.rgb;
    
    float3 finalColor = diffuseColor + specularColor;
    
    return float4(finalColor, pIn.color.a);
}
