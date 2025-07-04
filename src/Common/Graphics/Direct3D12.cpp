#include "../Config.h"
#include "../IGraphics.h"

#include <stdio.h>
#include <windows.h>
#include <wrl/client.h>
#include <cassert>

#include <d3dx12.h>
#include <dxcapi.h>
#include <vector>

using Microsoft::WRL::ComPtr;

#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

static IDxcCompiler3* pDxcCompiler = nullptr;
static IDxcUtils* pDxcUtils = nullptr;

#if (defined(D3D12))

template <typename T> inline void SafeRelease(T*& p) {
    if (p != nullptr) {
        p->Release();
        p = nullptr;
    }
}

// TODO seems trivial, remove
static bool is_depth_format(ImageFormat format) { return format == IMAGE_FORMAT_D32_FLOAT; }

static DXGI_FORMAT to_dx_format(ImageFormat format) {
    switch (format) {
        case IMAGE_FORMAT_UNDEFINED:
            return DXGI_FORMAT_UNKNOWN;
        case IMAGE_FORMAT_R32G32B32_FLOAT:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case IMAGE_FORMAT_R32G32B32A32_FLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case IMAGE_FORMAT_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case IMAGE_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;
        default:
            assert(false && "ImageFormat not supported");
            return DXGI_FORMAT_UNKNOWN;
    }
}
static D3D12_SHADER_VISIBILITY to_dx_shader_visibility(ShaderStage stages) {
    if (stages == SHADER_STAGE_ALL_GRAPHICS)
        return D3D12_SHADER_VISIBILITY_ALL;
    if (stages == SHADER_STAGE_VERTEX)
        return D3D12_SHADER_VISIBILITY_VERTEX;
    if (stages == SHADER_STAGE_FRAGMENT)
        return D3D12_SHADER_VISIBILITY_PIXEL;
    return D3D12_SHADER_VISIBILITY_ALL;
}

static D3D12_RESOURCE_STATES to_dx_resource_state(ResourceState state) {
    D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON;
    if (state & RESOURCE_STATE_RENDER_TARGET)
        result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    if (state & RESOURCE_STATE_PRESENT)
        result |= D3D12_RESOURCE_STATE_PRESENT;
    if (state & RESOURCE_STATE_SHADER_RESOURCE)
        result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if (state & RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
        result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    if (state & RESOURCE_STATE_GENERIC_READ)
        result |= D3D12_RESOURCE_STATE_GENERIC_READ;
    if (state & RESOURCE_STATE_DEPTH_WRITE)
        result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;

    return result;
}
static D3D12_DESCRIPTOR_RANGE_TYPE to_dx_descriptor_type(DescriptorType type) {
    if (type == DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    if (type == DESCRIPTOR_TYPE_TEXTURE)
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    if (type == DESCRIPTOR_TYPE_SAMPLER)
        return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
}

static D3D12_CULL_MODE to_dx_cull_mode(CullMode mode) {
    switch (mode) {
        case CULL_MODE_NONE:
            return D3D12_CULL_MODE_NONE;
        case CULL_MODE_FRONT:
            return D3D12_CULL_MODE_FRONT;
        case CULL_MODE_BACK:
            return D3D12_CULL_MODE_BACK;
        default:
            return D3D12_CULL_MODE_NONE;
    }
}

static D3D12_FILL_MODE to_dx_fill_mode(FillMode mode) {
    return (mode == FILL_MODE_WIREFRAME) ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
}

static D3D12_COMPARISON_FUNC to_dx_compare_mode(CompareMode mode) {
    return (D3D12_COMPARISON_FUNC) (mode + 1);
}

static D3D12_PRIMITIVE_TOPOLOGY_TYPE to_dx_primitive_topo_type(PrimitiveTopology topo) {
    if (topo == PRIMITIVE_TOPO_TRI_LIST)
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

static void init_shader_compiler() {
    if (!pDxcCompiler) {
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pDxcUtils));
        DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pDxcCompiler));
    }
}

static void exit_shader_compiler() {
    if (pDxcCompiler)
        pDxcCompiler->Release();
    if (pDxcUtils)
        pDxcUtils->Release();
}

static void load_shader(const char* pFileName, ShaderStage stage, ID3DBlob** ppBlob) {
    *ppBlob = nullptr;

    wchar_t unicodeFileName[1024];
    mbstowcs(unicodeFileName, pFileName, 1024);

    IDxcBlobEncoding* pSource = nullptr;
    HRESULT hr = pDxcUtils->LoadFile(unicodeFileName, nullptr, &pSource);
    if (FAILED(hr)) {
        printf("[ERROR] Could not find or open shader file: %s\n", pFileName);
        return;
    }

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = pSource->GetBufferPointer();
    sourceBuffer.Size = pSource->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_ACP;

    std::vector<LPCWSTR> arguments;
    arguments.push_back(L"-E");
    if (stage == SHADER_STAGE_VERTEX) {
        arguments.push_back(L"VS");
    } else if (stage == SHADER_STAGE_FRAGMENT) {
        arguments.push_back(L"PS");
    }

    arguments.push_back(L"-T");
    if (stage == SHADER_STAGE_VERTEX) {
        arguments.push_back(L"vs_6_5");
    } else if (stage == SHADER_STAGE_FRAGMENT) {
        arguments.push_back(L"ps_6_5");
    }

    arguments.push_back(DXC_ARG_DEBUG);

    IDxcResult* pCompileResult = nullptr;
    hr = pDxcCompiler->Compile(&sourceBuffer,
                               arguments.data(),
                               (UINT32) arguments.size(),
                               nullptr,
                               IID_PPV_ARGS(&pCompileResult));

    if (FAILED(hr)) {
        printf("[ERROR] Failed to invoke the DXC compiler.\n");
        if (pSource)
            pSource->Release();
        return;
    }

    IDxcBlobUtf8* pErrors = nullptr;
    pCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
    if (pErrors && pErrors->GetStringLength() > 0) {
        printf("Shader compilation fail: %s:\n\n%s\n", pFileName, pErrors->GetStringPointer());
    }
    if (pErrors) {
        pErrors->Release();
    }

    pCompileResult->GetStatus(&hr);
    if (SUCCEEDED(hr)) {
        pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(ppBlob), nullptr);
    }

    if (pCompileResult)
        pCompileResult->Release();
    if (pSource)
        pSource->Release();
}

// Resources
void addResource(Renderer* pRenderer, BufferLoadDesc* pBufferDesc) {
    assert(pRenderer);
    assert(pBufferDesc);
    assert(pBufferDesc->ppBuffer);

    Buffer* pBuffer = new Buffer();
    *pBufferDesc->ppBuffer = pBuffer;

    pBuffer->mSize = pBufferDesc->mDesc.mSize;
    pBuffer->mMemoryUsage = pBufferDesc->mDesc.mMemoryUsage;
    pBuffer->mFlags = pBufferDesc->mDesc.mFlags;
    pBuffer->mDescriptors = pBufferDesc->mDesc.mDescriptors;
    pBuffer->pCpuMappedAddress = nullptr;
    pBuffer->mStructStride = pBufferDesc->mDesc.mStructStride;

    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES initialState = to_dx_resource_state(pBufferDesc->mDesc.mStartState);

    if (pBuffer->mMemoryUsage == RESOURCE_MEMORY_USAGE_CPU_TO_GPU) {
        heapType = D3D12_HEAP_TYPE_UPLOAD;
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    if (pBufferDesc->pData != nullptr && pBuffer->mMemoryUsage == RESOURCE_MEMORY_USAGE_GPU_ONLY) {
        heapType = D3D12_HEAP_TYPE_UPLOAD;
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = heapType;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = pBuffer->mSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12MA::Allocation* pAllocation = nullptr;
    HRESULT hr = pRenderer->mDx.Allocator->CreateResource(&allocationDesc,
                                                          &resourceDesc,
                                                          initialState,
                                                          nullptr,
                                                          &pAllocation,
                                                          IID_PPV_ARGS(&pBuffer->mDx.pResource));

    if (FAILED(hr)) {
        delete pBuffer;
        *pBufferDesc->ppBuffer = nullptr;
        if (pAllocation)
            pAllocation->Release();
        return;
    }

    bool isPersistentlyMapped = (pBuffer->mFlags & BUFFER_CREATIONFLAG_PERSISTENT);

    if (isPersistentlyMapped) {
        pBuffer->mDx.pResource->Map(0, nullptr, &pBuffer->pCpuMappedAddress);

        if (pBufferDesc->pData) {
            memcpy(pBuffer->pCpuMappedAddress, pBufferDesc->pData, pBuffer->mSize);
        }
    } else if (pBufferDesc->pData) {
        void* pMappedData = nullptr;
        D3D12_RANGE readRange = { 0, 0 };
        hr = pBuffer->mDx.pResource->Map(0, &readRange, &pMappedData);
        if (SUCCEEDED(hr)) {
            memcpy(pMappedData, pBufferDesc->pData, pBuffer->mSize);
            pBuffer->mDx.pResource->Unmap(0, nullptr);
        }
    }

    pBuffer->mDx.mGpuAddress = pBuffer->mDx.pResource->GetGPUVirtualAddress();

    if (pAllocation) {
        pAllocation->Release();
    }
}

void beginUpdateResource(Renderer* pRenderer, BufferUpdateDesc* pDesc) {
    assert(pRenderer);
    assert(pDesc);
    assert(pDesc->pBuffer);

    Buffer* pBuffer = pDesc->pBuffer;

    if (pBuffer->mMemoryUsage == RESOURCE_MEMORY_USAGE_CPU_TO_GPU) {
        pDesc->mInternal.mNeedsUnmap = false;

        if (pBuffer->pCpuMappedAddress) {
            pDesc->pMappedData = (uint8_t*) pBuffer->pCpuMappedAddress + pDesc->mDstOffset;
        } else {
            void* mappedData = nullptr;
            D3D12_RANGE readRange = { 0, 0 };
            pBuffer->mDx.pResource->Map(0, &readRange, &mappedData);

            pDesc->pMappedData = (uint8_t*) mappedData + pDesc->mDstOffset;
            pDesc->mInternal.mNeedsUnmap = true;
        }
    } else if (pBuffer->mMemoryUsage == RESOURCE_MEMORY_USAGE_GPU_ONLY) {
        assert(false && "Updating a GPU_ONLY buffer is not yet implemented.");
        pDesc->pMappedData = nullptr;
    }
}

void endUpdateResource(Renderer* pRenderer, BufferUpdateDesc* pDesc) {
    assert(pRenderer);
    assert(pDesc);

    Buffer* pBuffer = pDesc->pBuffer;

    if (pBuffer->mMemoryUsage == RESOURCE_MEMORY_USAGE_CPU_TO_GPU) {
        if (pDesc->mInternal.mNeedsUnmap) {
            pBuffer->mDx.pResource->Unmap(0, NULL);
        }
    } else if (pBuffer->mMemoryUsage == RESOURCE_MEMORY_USAGE_GPU_ONLY) {
    }

    pDesc->pMappedData = NULL;
    pDesc->mInternal.mNeedsUnmap = false;
}

///////////////////////////////
/// Root Sig (PipelineLayout)
struct InternalPipelineLayout {
    ID3D12RootSignature* pRootSignature;
};

void initPipelineLayout(Renderer* pRenderer,
                        const PipelineLayoutDesc* pDesc,
                        PipelineLayout** ppPipelineLayout) {
    if (pDesc->pShaderFileName) {
        ID3DBlob* pShaderBlob = nullptr;
        load_shader(pDesc->pShaderFileName, SHADER_STAGE_VERTEX, &pShaderBlob);

        if (!pShaderBlob) {
            printf("[ERROR] Shader error:  %s.\n", pDesc->pShaderFileName);
            *ppPipelineLayout = nullptr;
            return;
        }

        InternalPipelineLayout* pInternal = new InternalPipelineLayout();
        HRESULT hr =
            pRenderer->mDx.pDevice->CreateRootSignature(0,
                                                        pShaderBlob->GetBufferPointer(),
                                                        pShaderBlob->GetBufferSize(),
                                                        IID_PPV_ARGS(&pInternal->pRootSignature));
        pShaderBlob->Release();

        if (FAILED(hr)) {
            printf("[ERROR] CreateRootSignature failed:  0x%lX\n", hr);
            delete pInternal;
            *ppPipelineLayout = nullptr;
        } else {
            *ppPipelineLayout = new PipelineLayout();
            (*ppPipelineLayout)->pHandle = pInternal;
        }
        return;
    }

    std::vector<CD3DX12_ROOT_PARAMETER> rootParameters;
    std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>> descriptorRanges;

    if (pDesc->parameterCount > 0) {
        rootParameters.resize(pDesc->parameterCount);
        descriptorRanges.resize(pDesc->parameterCount);

        for (uint32_t i = 0; i < pDesc->parameterCount; ++i) {
            const RootParameter* pParam = &pDesc->pParameters[i];
            D3D12_SHADER_VISIBILITY visibility = to_dx_shader_visibility(pParam->shaderVisibility);

            if (pParam->type == RESOURCE_TYPE_ROOT_CONSTANT_BUFFER) {
                rootParameters[i].InitAsConstantBufferView(pParam->rootConstantBuffer.binding,
                                                           0,
                                                           visibility);
            } else if (pParam->type == RESOURCE_TYPE_DESCRIPTOR_TABLE) {
                descriptorRanges[i].resize(pParam->descriptorTable.rangeCount);
                for (uint32_t r = 0; r < pParam->descriptorTable.rangeCount; ++r) {
                    const DescriptorRangeDesc* pRangeDesc = &pParam->descriptorTable.pRanges[r];
                    D3D12_DESCRIPTOR_RANGE_TYPE dxType = to_dx_descriptor_type(pRangeDesc->type);
                    descriptorRanges[i][r].Init(dxType,
                                                pRangeDesc->count,
                                                pRangeDesc->binding,
                                                0,
                                                D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                }
                rootParameters[i].InitAsDescriptorTable(pParam->descriptorTable.rangeCount,
                                                        descriptorRanges[i].data(),
                                                        visibility);
            }
        }
    }

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Init(pDesc->parameterCount,
                     pDesc->parameterCount > 0 ? rootParameters.data() : nullptr,
                     0,
                     nullptr,
                     D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ID3DBlob* signature = nullptr;
    ID3DBlob* error = nullptr;
    HRESULT hr =
        D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

    if (FAILED(hr)) {
        if (error)
            error->Release();
        *ppPipelineLayout = nullptr;
        return;
    }

    InternalPipelineLayout* pInternal = new InternalPipelineLayout();

    hr = pRenderer->mDx.pDevice->CreateRootSignature(0,
                                                     signature->GetBufferPointer(),
                                                     signature->GetBufferSize(),
                                                     IID_PPV_ARGS(&pInternal->pRootSignature));

    if (FAILED(hr)) {
        delete pInternal;
        *ppPipelineLayout = nullptr;
    } else {
        *ppPipelineLayout = new PipelineLayout();
        (*ppPipelineLayout)->pHandle = pInternal;
    }

    if (signature)
        signature->Release();
    if (error)
        error->Release();
}

void exitPipelineLayout(Renderer* pRenderer, PipelineLayout* pPipelineLayout) {
    if (pPipelineLayout && pPipelineLayout->pHandle) {
        InternalPipelineLayout* pInternal = (InternalPipelineLayout*) pPipelineLayout->pHandle;
        if (pInternal->pRootSignature) {
            pInternal->pRootSignature->Release();
        }
        delete pInternal;
    }
    delete pPipelineLayout;
}

VT_API void addShader(Renderer* pRenderer, const char* pFileName, Shader** ppShader) {
    Shader* pShader = new Shader();

    load_shader(pFileName, SHADER_STAGE_VERTEX, &pShader->mDx.pVertexShader);
    load_shader(pFileName, SHADER_STAGE_FRAGMENT, &pShader->mDx.pPixelShader);

    if (pShader->mDx.pVertexShader && pShader->mDx.pPixelShader) {
        pShader->mStages = SHADER_STAGE_ALL_GRAPHICS;
        *ppShader = pShader;
    } else {
        if (pShader->mDx.pVertexShader)
            pShader->mDx.pVertexShader->Release();
        if (pShader->mDx.pPixelShader)
            pShader->mDx.pPixelShader->Release();
        delete pShader;
        *ppShader = nullptr;
    }
}
///////////////////////////////
/// Pipeline
void addPipeline(Renderer* pRenderer, const GraphicsPipelineDesc* pDesc, Pipeline** ppPipeline) {
    assert(pRenderer);
    assert(pDesc);
    assert(pDesc->pPipelineLayout);
    assert(pDesc->pShaderProgram);
    assert(pDesc->pRasterizerState);
    assert(ppPipeline);

    InternalPipelineLayout* pLayout = (InternalPipelineLayout*) pDesc->pPipelineLayout->pHandle;
    assert(pLayout && pLayout->pRootSignature);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = pLayout->pRootSignature;

    psoDesc.VS = { pDesc->pShaderProgram->mDx.pVertexShader->GetBufferPointer(),
                   pDesc->pShaderProgram->mDx.pVertexShader->GetBufferSize() };
    psoDesc.PS = { pDesc->pShaderProgram->mDx.pPixelShader->GetBufferPointer(),
                   pDesc->pShaderProgram->mDx.pPixelShader->GetBufferSize() };

    CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    rasterizerDesc.CullMode = to_dx_cull_mode(pDesc->pRasterizerState->mCullMode);
    rasterizerDesc.FillMode = to_dx_fill_mode(pDesc->pRasterizerState->mFillMode);
    psoDesc.RasterizerState = rasterizerDesc;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    if (pDesc->pDepthState) {
        CD3DX12_DEPTH_STENCIL_DESC depthDesc(D3D12_DEFAULT);
        depthDesc.DepthEnable = pDesc->pDepthState->mDepthTest;
        depthDesc.DepthWriteMask = pDesc->pDepthState->mDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL
                                                                   : D3D12_DEPTH_WRITE_MASK_ZERO;
        depthDesc.DepthFunc = to_dx_compare_mode(pDesc->pDepthState->mDepthFunc);
        psoDesc.DepthStencilState = depthDesc;
    }

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
    if (pDesc->pVertexLayout && pDesc->pVertexLayout->mAttribCount > 0) {
        inputElementDescs.resize(pDesc->pVertexLayout->mAttribCount);
        for (uint32_t i = 0; i < pDesc->pVertexLayout->mAttribCount; ++i) {
            const VertexAttrib* pAttrib = &pDesc->pVertexLayout->pAttribs[i];
            inputElementDescs[i] = { pAttrib->pSemanticName,
                                     0,
                                     to_dx_format(pAttrib->mFormat),
                                     pAttrib->mBinding,
                                     pAttrib->mOffset,
                                     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     0 };
        }
    }
    psoDesc.InputLayout = { inputElementDescs.data(), (UINT) inputElementDescs.size() };

    psoDesc.PrimitiveTopologyType = to_dx_primitive_topo_type(pDesc->mPrimitiveTopo);

    psoDesc.NumRenderTargets = pDesc->mRenderTargetCount;
    for (uint32_t i = 0; i < pDesc->mRenderTargetCount; ++i) {
        psoDesc.RTVFormats[i] = to_dx_format(pDesc->pColorFormats[i]);
    }
    psoDesc.DSVFormat = to_dx_format(pDesc->mDepthStencilFormat);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    ID3D12PipelineState* pPipelineState = nullptr;
    HRESULT hr = pRenderer->mDx.pDevice->CreateGraphicsPipelineState(&psoDesc,
                                                                     IID_PPV_ARGS(&pPipelineState));

    if (FAILED(hr)) {
        *ppPipeline = nullptr;
        return;
    }

    Pipeline* pPipeline = (Pipeline*) calloc(1, sizeof(Pipeline));
    pPipeline->mDx.pPipelineState = pPipelineState;
    pPipeline->mDx.mType = PIPELINE_TYPE_GRAPHICS;
    pPipeline->mDx.pRootSignature = pLayout->pRootSignature;
    pPipeline->mDx.mPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    *ppPipeline = pPipeline;
}

void removeGraphicsPipeline(Renderer* pRenderer, Pipeline* pPipeline) {
    if (pPipeline && pPipeline->mDx.pPipelineState) {
        pPipeline->mDx.pPipelineState->Release();
        free(pPipeline);
    }
}

///////////////////////////////
/// Descriptor Heap
struct DescriptorHeap {
    ID3D12DescriptorHeap* pHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE mStartCpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE mStartGpuHandle;
    D3D12_DESCRIPTOR_HEAP_TYPE mType;
    uint32_t mNumDescriptors;
    uint32_t mDescriptorSize;
};

static void add_descriptor_heap(ID3D12Device* pDevice,
                                const D3D12_DESCRIPTOR_HEAP_DESC* pDesc,
                                DescriptorHeap** ppDescHeap) {
    DescriptorHeap* pDescHeap = new DescriptorHeap();
    if (!pDescHeap) {
        *ppDescHeap = nullptr;
        return;
    }

    HRESULT hr = pDevice->CreateDescriptorHeap(pDesc, IID_PPV_ARGS(&pDescHeap->pHeap));
    if (SUCCEEDED(hr)) {
        pDescHeap->mStartCpuHandle = pDescHeap->pHeap->GetCPUDescriptorHandleForHeapStart();
        if (pDesc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
            pDescHeap->mStartGpuHandle = pDescHeap->pHeap->GetGPUDescriptorHandleForHeapStart();
        }
        pDescHeap->mNumDescriptors = pDesc->NumDescriptors;
        pDescHeap->mType = pDesc->Type;
        pDescHeap->mDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(pDesc->Type);
        *ppDescHeap = pDescHeap;
    } else {
        delete pDescHeap;
        *ppDescHeap = nullptr;
    }
}
static void remove_descriptor_heap(DescriptorHeap* pDescHeap) {
    if (pDescHeap) {
        SafeRelease(pDescHeap->pHeap);
        delete pDescHeap;
    }
}

void addDescriptorSet(Renderer* pRenderer, const DescriptorSetDesc* pDesc, DescriptorSet** ppSet) {
    assert(pRenderer && pDesc && ppSet);

    uint32_t descriptorCount = 0;
    if (pDesc->mDescriptorCount > 0) {
        for (uint32_t i = 0; i < pDesc->mDescriptorCount; ++i) {
            descriptorCount =
                std::max(descriptorCount,
                         pDesc->pDescriptors[i].mOffset + pDesc->pDescriptors[i].mCount);
        }
    }

    DescriptorHeap* pHeap = pRenderer->mDx.pCbvSrvUavHeaps[pDesc->mNodeIndex];

    static uint32_t sCurrentDescriptorOffset = 0;

    uint32_t totalDescriptorsToAllocate = descriptorCount * pDesc->mMaxSets;
    assert(sCurrentDescriptorOffset + totalDescriptorsToAllocate < pHeap->mNumDescriptors);

    DescriptorSet* pSet = new DescriptorSet();
    pSet->mDx.pDescriptors = pDesc->pDescriptors;
    pSet->mDx.mCbvSrvUavHandle = sCurrentDescriptorOffset;
    pSet->mDx.mPipelineType = 0;
    pSet->mDx.mRootParamterIndex = pDesc->mIndex;

    pSet->mDx.mDescriptorCount = pDesc->mDescriptorCount;
    pSet->mDx.mDescriptorStride = descriptorCount;

    sCurrentDescriptorOffset += totalDescriptorsToAllocate;

    *ppSet = pSet;
}

void removeDescriptorSet(Renderer* pRenderer, DescriptorSet* pSet) {
    if (pSet) {
        delete pSet;
    }
}

void updateDescriptorSet(Renderer* pRenderer,
                         uint32_t setIndex,
                         DescriptorSet* pSet,
                         uint32_t dataCount,
                         const DescriptorData* pData) {
    assert(pRenderer && pSet && pData);

    DescriptorHeap* pHeap = pRenderer->mDx.pCbvSrvUavHeaps[0];

    uint32_t descriptorCountPerSet = pSet->mDx.mDescriptorStride;

    for (uint32_t i = 0; i < dataCount; ++i) {
        const DescriptorData* pDataItem = &pData[i];
        const Descriptor* pDesc = &pSet->mDx.pDescriptors[pDataItem->mIndex];

        assert(pDesc->mType == DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        for (uint32_t j = 0; j < pDataItem->mCount; ++j) {
            Buffer* pBuffer = pDataItem->ppBuffers[j];
            assert(pBuffer != nullptr);

            D3D12_CPU_DESCRIPTOR_HANDLE destHandle = pHeap->mStartCpuHandle;

            destHandle.ptr += pSet->mDx.mCbvSrvUavHandle * pHeap->mDescriptorSize;
            destHandle.ptr += setIndex * descriptorCountPerSet * pHeap->mDescriptorSize;
            destHandle.ptr +=
                (pDesc->mOffset + pDataItem->mArrayOffset + j) * pHeap->mDescriptorSize;

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = pBuffer->mDx.mGpuAddress;
            cbvDesc.SizeInBytes = (UINT) (pBuffer->mSize + 255) & ~255;

            pRenderer->mDx.pDevice->CreateConstantBufferView(&cbvDesc, destHandle);
        }
    }
}

void cmdBindDescriptorSet(Cmd* pCmd, DescriptorSet* pSet, uint32_t setIndex) {
    assert(pCmd && pSet);

    const uint32_t rootParameterIndex = pSet->mDx.mRootParamterIndex;
    DescriptorHeap* pHeap = pCmd->pRenderer->mDx.pCbvSrvUavHeaps[0];

    uint32_t descriptorCountPerSet = pSet->mDx.mDescriptorStride;

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = pHeap->mStartGpuHandle;
    gpuHandle.ptr += pSet->mDx.mCbvSrvUavHandle * pHeap->mDescriptorSize;
    gpuHandle.ptr += (setIndex * descriptorCountPerSet) * pHeap->mDescriptorSize;

    pCmd->mDx.pCmdList->SetGraphicsRootDescriptorTable(rootParameterIndex, gpuHandle);
}

/////////////////////////////////////////
// Implementation
static bool AddDevice(const RendererDesc* pDesc, Renderer* pRenderer) {
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_2;

    if (pDesc->pContext) {
        pRenderer->pGpuDesc = &pDesc->pContext->mGpu;
        featureLevel = pRenderer->pGpuDesc->mFeatureLevel;
    } else {
        pRenderer->pGpuDesc = &pRenderer->pContext->mGpu;
        featureLevel = pRenderer->pGpuDesc->mFeatureLevel;
    }

    HRESULT hr = D3D12CreateDevice(pRenderer->pGpuDesc->mDx.pAdapter,
                                   featureLevel,
                                   IID_PPV_ARGS(&pRenderer->mDx.pDevice));
    if (FAILED(hr)) {
        return false;
    }
    pRenderer->mDx.pDevice->SetName(L"Main Device");

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = pRenderer->mDx.pDevice;
    allocatorDesc.pAdapter = pRenderer->pGpuDesc->mDx.pAdapter;

    HRESULT allocatorHr = D3D12MA::CreateAllocator(&allocatorDesc, &pRenderer->mDx.Allocator);
    if (FAILED(allocatorHr)) {
        return false;
    }

#if defined(ENABLE_DEBUG)
    if (SUCCEEDED(pRenderer->mDx.pDevice->QueryInterface(
            IID_PPV_ARGS(&pRenderer->mDx.pDebugValidation)))) {
        pRenderer->mDx.pDebugValidation->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION,
                                                            TRUE);
        pRenderer->mDx.pDebugValidation->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    }
#endif
    return true;
}

static void RemoveDevice(Renderer* pRenderer) {
    // BUG lags
    // if (pRenderer->mDx.Allocator) {
    //     pRenderer->mDx.Allocator->Release();
    //     pRenderer->mDx.Allocator = nullptr;
    // }
#if defined(ENABLE_DEBUG)
    if (pRenderer->mDx.pDebugValidation) {
        SafeRelease(pRenderer->mDx.pDebugValidation);
    }
#endif
    SafeRelease(pRenderer->mDx.pDevice);
}

static bool AddDXGIFactory(RendererContext* pContext) {
    UINT dxgiFactoryFlags = 0;
#if defined(ENABLE_DEBUG)
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pContext->mDx.pDXGIFactory));
    return SUCCEEDED(hr);
}

void initRenderer(const char* name, RendererDesc* pDesc, Renderer** ppRenderer) {
#if defined(ENABLE_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    }
#endif

    Renderer* pRenderer = new Renderer();
    pRenderer->pName = name;
    pRenderer->mShaderTarget = pDesc->mShaderTarget;

    if (!pDesc->pContext) {
        RendererContextDesc contextDesc = {};
        contextDesc.mEnableGpuBasedValidation = true;
        // TODO make this flexible feature level
        contextDesc.mDx.mFeatureLevel = D3D_FEATURE_LEVEL_12_2;
        initRendererContext(name, &contextDesc, &pRenderer->pContext);
    } else {
        pRenderer->pContext = pDesc->pContext;
    }

    if (!AddDevice(pDesc, pRenderer)) {
        if (pRenderer->pContext && !pDesc->pContext) {
            SafeRelease(pRenderer->pContext->mDx.pDXGIFactory);
            SafeRelease(pRenderer->pContext->mGpu.mDx.pAdapter);
            delete pRenderer->pContext;
        }
        delete pRenderer;
        *ppRenderer = nullptr;
        return;
    }

    uint32_t nodeCount = 1;

    pRenderer->mDx.pCPUDescriptorHeaps = new DescriptorHeap*[4];
    pRenderer->mDx.pCbvSrvUavHeaps = new DescriptorHeap*[nodeCount];
    pRenderer->mDx.pSamplerHeaps = new DescriptorHeap*[nodeCount];

    for (uint32_t i = 0; i < nodeCount; ++i) {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 16384;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.NodeMask = 0;
        add_descriptor_heap(pRenderer->mDx.pDevice, &heapDesc, &pRenderer->mDx.pCbvSrvUavHeaps[i]);

        heapDesc.NumDescriptors = 2048;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        add_descriptor_heap(pRenderer->mDx.pDevice, &heapDesc, &pRenderer->mDx.pSamplerHeaps[i]);
    }

    // TODO remove this
    init_shader_compiler();

    *ppRenderer = pRenderer;
}

void initRendererContext(const char* appName,
                         const RendererContextDesc* pSettings,
                         RendererContext** ppContext) {
    *ppContext = new RendererContext();
    memset(*ppContext, 0, sizeof(RendererContext));

    if (!AddDXGIFactory(*ppContext)) {
        delete *ppContext;
        *ppContext = nullptr;
        return;
    }

    (*ppContext)
        ->mDx.pDXGIFactory->EnumAdapterByGpuPreference(
            0,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&(*ppContext)->mGpu.mDx.pAdapter));
    (*ppContext)->mGpu.mFeatureLevel = pSettings->mDx.mFeatureLevel;
}

void shutdownRenderer(Renderer* pRenderer) {
    if (pRenderer) {
        RemoveDevice(pRenderer);
        if (pRenderer->pContext) {
            SafeRelease(pRenderer->pContext->mDx.pDXGIFactory);
            SafeRelease(pRenderer->pContext->mGpu.mDx.pAdapter);
            delete pRenderer->pContext;
        }
        delete pRenderer;
    }
}

// Fence
void initFence(Renderer* pRenderer, Fence** ppFence) {
    Fence* pFence = new Fence();
    if (!pFence) {
        *ppFence = nullptr;
        return;
    }
    pFence->mDx.mFenceValue = 0;
    pRenderer->mDx.pDevice->CreateFence(0,
                                        D3D12_FENCE_FLAG_NONE,
                                        IID_PPV_ARGS(&pFence->mDx.pFence));
    pFence->mDx.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    *ppFence = pFence;
}

void exitFence(Renderer* pRenderer, Fence* pFence) {
    if (pFence) {
        if (pFence->mDx.hEvent)
            CloseHandle(pFence->mDx.hEvent);
        SafeRelease(pFence->mDx.pFence);
        delete pFence;
    }
}

void waitFence(Renderer* pRenderer, Fence* pFence) {
    if (pFence->mDx.pFence->GetCompletedValue() < pFence->mDx.mFenceValue) {
        pFence->mDx.pFence->SetEventOnCompletion(pFence->mDx.mFenceValue, pFence->mDx.hEvent);
        WaitForSingleObject(pFence->mDx.hEvent, INFINITE);
    }
}

// Queue
void initQueue(Renderer* pRenderer, QueueDesc* pQDesc, Queue** ppQueue) {
    Queue* pQueue = new Queue();
    if (!pQueue) {
        *ppQueue = nullptr;
        return;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    pRenderer->mDx.pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pQueue->mDx.pQueue));
    *ppQueue = pQueue;
}

void exitQueue(Renderer* pRenderer, Queue* pQueue) {
    if (pQueue) {
        SafeRelease(pQueue->mDx.pQueue);
        delete pQueue;
    }
}

void waitQueueIdle(Queue* pQueue, Fence* pFence) {
    const uint64_t fenceValue = ++pFence->mDx.mFenceValue;
    pQueue->mDx.pQueue->Signal(pFence->mDx.pFence, fenceValue);
    waitFence(NULL, pFence);
}

// Swapchain
void initSwapChain(Renderer* pRenderer, SwapChainDesc* pDesc, SwapChain** ppSwapChain) {
    SwapChain* pSwapChain = new SwapChain();
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = pDesc->mWidth;
    swapChainDesc.Height = pDesc->mHeight;
    swapChainDesc.Format = to_dx_format(pDesc->mColorFormat);
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = pDesc->mImageCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = pRenderer->pContext->mDx.pDXGIFactory->CreateSwapChainForHwnd(
        (*pDesc->ppPresentQueues)->mDx.pQueue,
        pDesc->mWindowHandle.window,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1);
    if (FAILED(hr)) {
        delete pSwapChain;
        *ppSwapChain = nullptr;
        return;
    }

    hr = swapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain->mDx.pSwapchain));
    if (FAILED(hr)) {
        delete pSwapChain;
        *ppSwapChain = nullptr;
        return;
    }

    pSwapChain->mDx.mImageCount = pDesc->mImageCount;
    pSwapChain->ppRenderTargets = new RenderTarget*[pDesc->mImageCount];
    pSwapChain->mEnableVSync = pDesc->mEnableVsync;

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = pDesc->mImageCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    add_descriptor_heap(pRenderer->mDx.pDevice, &rtvHeapDesc, &pSwapChain->mDx.pRtvHeap);

    for (uint32_t i = 0; i < pDesc->mImageCount; ++i) {
        pSwapChain->ppRenderTargets[i] = new RenderTarget();
        pSwapChain->ppRenderTargets[i]->pTexture = new Texture();
        hr = pSwapChain->mDx.pSwapchain->GetBuffer(
            i,
            IID_PPV_ARGS(&pSwapChain->ppRenderTargets[i]->pTexture->mDx.pResource));
        assert(SUCCEEDED(hr));

        pSwapChain->ppRenderTargets[i]->mWidth = pDesc->mWidth;
        pSwapChain->ppRenderTargets[i]->mHeight = pDesc->mHeight;

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pSwapChain->mDx.pRtvHeap->mStartCpuHandle;
        rtvHandle.ptr += (SIZE_T) i * pSwapChain->mDx.pRtvHeap->mDescriptorSize;
        pRenderer->mDx.pDevice->CreateRenderTargetView(
            pSwapChain->ppRenderTargets[i]->pTexture->mDx.pResource,
            nullptr,
            rtvHandle);
        pSwapChain->ppRenderTargets[i]->mDx.mDxRtvHandle = rtvHandle;
    }
    *ppSwapChain = pSwapChain;
}

void exitSwapChain(Renderer* pRenderer, SwapChain* pSwapChain) {
    if (pSwapChain) {
        for (uint32_t i = 0; i < pSwapChain->mDx.mImageCount; ++i) {
            SafeRelease(pSwapChain->ppRenderTargets[i]->pTexture->mDx.pResource);
            delete pSwapChain->ppRenderTargets[i]->pTexture;
            delete pSwapChain->ppRenderTargets[i];
        }
        delete[] pSwapChain->ppRenderTargets;
        remove_descriptor_heap(pSwapChain->mDx.pRtvHeap);
        SafeRelease(pSwapChain->mDx.pSwapchain);
        delete pSwapChain;
    }
}

void acquireNextImage(Renderer* pRenderer,
                      SwapChain* pSwapChain,
                      Fence* pFence,
                      uint32_t* pImageIndex) {
    *pImageIndex = pSwapChain->mDx.pSwapchain->GetCurrentBackBufferIndex();
}

// Command Allocator
void initCmdPool(Renderer* pRenderer, CmdPoolDesc* pDesc, CmdPool** ppCmdPool) {
    CmdPool* pCmdPool = new CmdPool();
    if (!pCmdPool) {
        *ppCmdPool = nullptr;
        return;
    }
    pCmdPool->pQueue = pDesc->pQueue;
    pRenderer->mDx.pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                   IID_PPV_ARGS(&pCmdPool->mDx.pCmdAllocator));
    *ppCmdPool = pCmdPool;
}

void exitCmdPool(Renderer* pRenderer, CmdPool* pCmdPool) {
    if (pCmdPool) {
        SafeRelease(pCmdPool->mDx.pCmdAllocator);
        delete pCmdPool;
    }
}

void initCmd(Renderer* pRenderer, CmdDesc* pDesc, Cmd** ppCmd) {
    Cmd* pCmd = new Cmd();
    if (!pCmd) {
        *ppCmd = nullptr;
        return;
    }
    pCmd->pRenderer = pRenderer;
    pCmd->pQueue = pDesc->pCmdPool->pQueue;
    pCmd->mDx.pCmdPool = pDesc->pCmdPool;
    pRenderer->mDx.pDevice->CreateCommandList(0,
                                              D3D12_COMMAND_LIST_TYPE_DIRECT,
                                              pDesc->pCmdPool->mDx.pCmdAllocator,
                                              nullptr,
                                              IID_PPV_ARGS(&pCmd->mDx.pCmdList));
    pCmd->mDx.pCmdList->Close();
    *ppCmd = pCmd;
}

void exitCmd(Renderer* pRenderer, Cmd* pCmd) {
    if (pCmd) {
        SafeRelease(pCmd->mDx.pCmdList);
        delete pCmd;
    }
}

void resetCmdPool(Renderer* pRenderer, CmdPool* pCmdPool) { pCmdPool->mDx.pCmdAllocator->Reset(); }

void beginCmd(Cmd* pCmd) {
    pCmd->mDx.pCmdList->Reset(pCmd->mDx.pCmdPool->mDx.pCmdAllocator, nullptr);

    ID3D12DescriptorHeap* heaps[] = { pCmd->pRenderer->mDx.pCbvSrvUavHeaps[0]->pHeap,
                                      pCmd->pRenderer->mDx.pSamplerHeaps[0]->pHeap };
    pCmd->mDx.pCmdList->SetDescriptorHeaps(_countof(heaps), heaps);
}

void endCmd(Cmd* pCmd) { pCmd->mDx.pCmdList->Close(); }

void cmdResourceBarrier(Cmd* pCmd,
                        uint32_t numBufferBarriers,
                        BufferBarrier* pBufferBarriers,
                        uint32_t numTextureBarrier,
                        TextureBarrier* pTextureBarrier,
                        uint32_t numRtBarriers,
                        RenderTargetBarrier* pRtBarriers) {
    if (numRtBarriers > 0 && pRtBarriers) {
        D3D12_RESOURCE_BARRIER* barriers = new D3D12_RESOURCE_BARRIER[numRtBarriers];
        for (uint32_t i = 0; i < numRtBarriers; ++i) {
            barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers[i].Transition.pResource =
                pRtBarriers[i].pRenderTarget->pTexture->mDx.pResource;
            barriers[i].Transition.StateBefore = to_dx_resource_state(pRtBarriers[i].mCurrentState);
            barriers[i].Transition.StateAfter = to_dx_resource_state(pRtBarriers[i].mNewState);
            barriers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        }
        pCmd->mDx.pCmdList->ResourceBarrier(numRtBarriers, barriers);
        delete[] barriers;
    }

    if (numBufferBarriers > 0) {
    }

    if (numTextureBarrier > 0) {
    }
}

void queueSubmit(Queue* pQueue, const QueueSubmitDesc* pDesc) {
    ID3D12CommandList** cmdLists = new ID3D12CommandList*[pDesc->mCmdCount];
    for (uint32_t i = 0; i < pDesc->mCmdCount; ++i) {
        cmdLists[i] = pDesc->ppCmd[i]->mDx.pCmdList;
    }

    pQueue->mDx.pQueue->ExecuteCommandLists(pDesc->mCmdCount, cmdLists);

    if (pDesc->pSignalFence)
        pQueue->mDx.pQueue->Signal(pDesc->pSignalFence->mDx.pFence,
                                   ++pDesc->pSignalFence->mDx.mFenceValue);

    delete[] cmdLists;
}

void queuePresent(Queue* pQueue, const QueuePresentDesc* pDesc) {
    pDesc->pSwapChain->mDx.pSwapchain->Present(pDesc->pSwapChain->mEnableVSync ? 1 : 0, 0);
}

void addRenderTarget(Renderer* pRenderer,
                     const RenderTargetDesc* pDesc,
                     RenderTarget** ppRenderTarget) {
    assert(pRenderer);
    assert(pDesc);
    assert(ppRenderTarget);

    bool isDepth = is_depth_format(pDesc->mFormat);
    if (!isDepth) {
        assert(false && "addRenderTarget is only for depthstencil targets");
        *ppRenderTarget = nullptr;
        return;
    }

    RenderTarget* pRenderTarget = new RenderTarget();
    pRenderTarget->pTexture = new Texture();
    pRenderTarget->mWidth = pDesc->mWidth;
    pRenderTarget->mHeight = pDesc->mHeight;
    pRenderTarget->mFormat = pDesc->mFormat;
    pRenderTarget->mClearValue = pDesc->mClearValue;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = pDesc->mWidth;
    resourceDesc.Height = pDesc->mHeight;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = to_dx_format(pDesc->mFormat);
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = to_dx_format(pDesc->mFormat);
    clearValue.DepthStencil.Depth = pDesc->mClearValue.depth;
    clearValue.DepthStencil.Stencil = pDesc->mClearValue.stencil;

    D3D12MA::Allocation* pAllocation = nullptr;
    HRESULT hr = pRenderer->mDx.Allocator->CreateResource(
        &allocationDesc,
        &resourceDesc,
        to_dx_resource_state(pDesc->mStartState),
        &clearValue,
        &pAllocation,
        IID_PPV_ARGS(&pRenderTarget->pTexture->mDx.pResource));

    if (FAILED(hr)) {
        delete pRenderTarget->pTexture;
        delete pRenderTarget;
        *ppRenderTarget = nullptr;
        if (pAllocation)
            pAllocation->Release();
        return;
    }
    pRenderTarget->pTexture->mDx.pResource->SetName(L"Depth Buffer Resource");

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    add_descriptor_heap(pRenderer->mDx.pDevice, &dsvHeapDesc, &pRenderTarget->mDx.pDsvHeap);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = to_dx_format(pDesc->mFormat);
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    pRenderTarget->mDx.mDxDsvHandle = pRenderTarget->mDx.pDsvHeap->mStartCpuHandle;
    pRenderer->mDx.pDevice->CreateDepthStencilView(pRenderTarget->pTexture->mDx.pResource,
                                                   &dsvDesc,
                                                   pRenderTarget->mDx.mDxDsvHandle);

    *ppRenderTarget = pRenderTarget;
    if (pAllocation)
        pAllocation->Release();
}

// Draw
void cmdBindRenderTargets(Cmd* pCmd, const BindRenderTargetsDesc* pDesc) {
    if (!pDesc) {
        pCmd->mDx.pCmdList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
        return;
    }

    const uint32_t renderTargetCount = pDesc->mRenderTargetCount;

    if (renderTargetCount == 0 && !pDesc->mDepthStencil.pRenderTarget) {
        pCmd->mDx.pCmdList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
        return;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[1];

    for (uint32_t i = 0; i < renderTargetCount; ++i) {
        const BindRenderTargetDesc& rtBindDesc = pDesc->mRenderTarget[i];
        RenderTarget* pRenderTarget = rtBindDesc.pRenderTarget;

        assert(pRenderTarget && "Render target is null");
        if (!pRenderTarget)
            continue;

        rtvHandles[i] = pRenderTarget->mDx.mDxRtvHandle;

        if (rtBindDesc.mLoadAction == LOAD_ACTION_CLEAR) {
            pCmd->mDx.pCmdList->ClearRenderTargetView(rtvHandles[i],
                                                      &rtBindDesc.mClearValue.r,
                                                      0,
                                                      nullptr);
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE* pDsvHandle = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
    if (pDesc->mDepthStencil.pRenderTarget) {
        RenderTarget* pDepthTarget = pDesc->mDepthStencil.pRenderTarget;
        dsvHandle = pDepthTarget->mDx.mDxDsvHandle;
        pDsvHandle = &dsvHandle;

        if (pDesc->mDepthStencil.mLoadAction == LOAD_ACTION_CLEAR) {
            pCmd->mDx.pCmdList->ClearDepthStencilView(dsvHandle,
                                                      D3D12_CLEAR_FLAG_DEPTH,
                                                      pDesc->mDepthStencil.mClearValue.depth,
                                                      0,
                                                      0,
                                                      nullptr);
        }
    }

    pCmd->mDx.pCmdList->OMSetRenderTargets(renderTargetCount,
                                           renderTargetCount > 0 ? rtvHandles : nullptr,
                                           FALSE,
                                           pDsvHandle);
}

void cmdClearRenderTarget(Cmd* pCmd, RenderTarget* pRenderTarget, const float* pClearColor) {
    pCmd->mDx.pCmdList->ClearRenderTargetView(pRenderTarget->mDx.mDxRtvHandle,
                                              pClearColor,
                                              0,
                                              nullptr);
}

void cmdSetViewport(Cmd* pCmd,
                    float x,
                    float y,
                    float width,
                    float height,
                    float minDepth,
                    float maxDepth) {
    D3D12_VIEWPORT viewport = { x, y, width, height, minDepth, maxDepth };
    pCmd->mDx.pCmdList->RSSetViewports(1, &viewport);
}

void cmdSetScissor(Cmd* pCmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    D3D12_RECT scissorRect = { (LONG) x, (LONG) y, (LONG) (x + width), (LONG) (y + height) };
    pCmd->mDx.pCmdList->RSSetScissorRects(1, &scissorRect);
}

void cmdBindPipeline(Cmd* pCmd, Pipeline* pPipeline) {
    assert(pCmd && pPipeline);
    ID3D12PipelineState* pso = pPipeline->mDx.pPipelineState;
    ID3D12RootSignature* rs = pPipeline->mDx.pRootSignature;
    D3D_PRIMITIVE_TOPOLOGY topo = pPipeline->mDx.mPrimitiveTopology;

    pCmd->mDx.pCmdList->SetPipelineState(pso);
    pCmd->mDx.pCmdList->SetGraphicsRootSignature(rs);
    pCmd->mDx.pCmdList->IASetPrimitiveTopology(topo);
}

void cmdBindVertexBuffer(Cmd* pCmd, uint32_t bufferCount, Buffer** ppVertexBuffers) {
    assert(pCmd && bufferCount > 0 && ppVertexBuffers);

    D3D12_VERTEX_BUFFER_VIEW* vbvs =
        (D3D12_VERTEX_BUFFER_VIEW*) _alloca(bufferCount * sizeof(D3D12_VERTEX_BUFFER_VIEW));

    for (uint32_t i = 0; i < bufferCount; ++i) {
        Buffer* pBuffer = ppVertexBuffers[i];
        assert(pBuffer);
        assert(pBuffer->mStructStride > 0);

        vbvs[i].BufferLocation = pBuffer->mDx.mGpuAddress;
        vbvs[i].StrideInBytes = pBuffer->mStructStride;
        vbvs[i].SizeInBytes = (UINT) pBuffer->mSize;
    }

    pCmd->mDx.pCmdList->IASetVertexBuffers(0, bufferCount, vbvs);
}

void cmdDraw(Cmd* pCmd, uint32_t vertexCount, uint32_t firstVertex) {
    assert(pCmd);
    pCmd->mDx.pCmdList->DrawInstanced(vertexCount, 1, firstVertex, 0);
}

#endif
