#pragma once

#include "Config.h"
#include "IGraphics.h"

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

VT_API void initRootSignature(Renderer* pRenderer, const RootSignatureDesc* pDesc);
