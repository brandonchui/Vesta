#pragma once
#include "IGraphics.h"

void initSlang();
void shutdownSlang();

void load_slang_shader(const char* pFileName, ShaderStage stage, ID3DBlob** ppBlob);
