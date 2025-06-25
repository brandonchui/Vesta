#pragma once

#include "Config.h"
#include "IGraphics.h"

typedef struct BufferLoadDesc BufferLoadDesc;
struct BufferLoadDesc {
    Buffer** ppBuffer;
    const void* pData;
    BufferDesc mDesc;
};

typedef struct ShaderStageLoadDesc ShaderStageLoadDesc;
struct ShaderStageLoadDesc {
    const char* pFileName;
    const char* pEntryPointName;
};

typedef struct ShaderLoadDesc ShaderLoadDesc;
struct ShaderLoadDesc {
    ShaderStageLoadDesc mVert;
    ShaderStageLoadDesc mFrag;
};

typedef struct RootSignatureDesc RootSignatureDesc;
struct RootSignatureDesc {
    const char* pGraphicsFileName;
    const char* pComputeFileName;
};

VT_API void addResource(BufferLoadDesc* pBufferDesc);
VT_API void addShader(Renderer* pRenderer, const ShaderLoadDesc* pDesc, Shader** pShader);
VT_API void initRootSignature(Renderer* pRenderer, const RootSignatureDesc* pDesc);
