#pragma once
#include "Config.h"
#include "IOperatingSystem.h"

#if defined(D3D12)
#include <D3D12MemAlloc.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Enums and Structs
//

#if defined(D3D12)
typedef uint64_t DxDescriptorID;
#endif

typedef enum TextureCreationFlags {
    TEXTURE_CREATION_FLAG_NONE = 0,
    TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT = 0x01,
} TextureCreationFlags;

typedef enum ResourceState {
    RESOURCE_STATE_UNDEFINED = 0,
    RESOURCE_STATE_RENDER_TARGET = 1 << 0,
    RESOURCE_STATE_PRESENT = 1 << 1,
    RESOURCE_STATE_SHADER_RESOURCE = 1 << 2,
    RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER = 1 << 3,
    RESOURCE_STATE_GENERIC_READ = 1 << 4,
    RESOURCE_STATE_DEPTH_WRITE = 1 << 8,
} ResourceState;

typedef enum DescriptorType {
    DESCRIPTOR_TYPE_UNDEFINED = 0,
    DESCRIPTOR_TYPE_SAMPLER = 0x01,
    DESCRIPTOR_TYPE_TEXTURE = 0x02,
    DESCRIPTOR_TYPE_UNIFORM_BUFFER = 0x04,
    DESCRIPTOR_TYPE_VERTEX_BUFFER = 0x08,
} DescriptorType;

typedef enum ResourceMemoryUsage {
    RESOURCE_MEMORY_USAGE_GPU_ONLY,
    RESOURCE_MEMORY_USAGE_CPU_ONLY,
    RESOURCE_MEMORY_USAGE_CPU_TO_GPU
} ResourceMemoryUsage;

typedef enum ImageFormat {
    IMAGE_FORMAT_UNDEFINED = 0,
    IMAGE_FORMAT_R32G32B32_FLOAT,
    IMAGE_FORMAT_R32G32B32A32_FLOAT,
    IMAGE_FORMAT_R8G8B8A8_UNORM,
    IMAGE_FORMAT_D32_FLOAT,
} ImageFormat;

typedef enum SampleCount {
    SAMPLE_COUNT_1,
} SampleCount;

////////////////////////////////////////////
/// Shader
typedef enum ShaderStage {
    SHADER_STAGE_NONE = 0,
    SHADER_STAGE_VERTEX = 0x1,
    SHADER_STAGE_FRAGMENT = 0x2,
    SHADER_STAGE_COMPUTE = 0x4,
    SHADER_STAGE_ALL_GRAPHICS = SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT,
    SHADER_STAGE_ALL = 0xFF,
} ShaderStage;

typedef struct Shader Shader;
struct Shader {
    ShaderStage mStages;
#if defined(D3D12)
    struct {
        LPCWSTR* pEntryNames;
        ID3DBlob* pVertexShader;
        ID3DBlob* pPixelShader;
    } mDx;
#endif
};

///////////////////////////////
/// Render Target
typedef struct ClearValue {
    float r, g, b, a;

    struct {
        float depth;
        uint32_t stencil;
    };
} ClearValue;

#define ALIGN_Texture 64
ALIGNED_STRUCT(Texture, ALIGN_Texture) {
    uint32_t mWidth;
    uint32_t mHeight;
#if defined(D3D12)
    struct {
        ID3D12Resource* pResource;
    } mDx;
#endif
};

typedef struct RenderTargetDesc RenderTargetDesc;
struct RenderTargetDesc {
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mDepth;
    uint32_t mArraySize;

    ResourceState mStartState;
    ClearValue mClearValue;

    DescriptorType mDescriptors;
    const void* pNativeHandle;

    uint32_t mSampleQuality;
    uint32_t mSampleCount;

    const char* pName;

    TextureCreationFlags mFlags;

    ImageFormat mFormat;
};

#define ALIGN_RenderTarget 64
ALIGNED_STRUCT(RenderTarget, ALIGN_RenderTarget) {
    Texture* pTexture;
#if defined(D3D12)
    struct {
        D3D12_CPU_DESCRIPTOR_HANDLE mDxRtvHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE mDxDsvHandle;
        struct DescriptorHeap* pDsvHeap;
    } mDx;
#endif
    ClearValue mClearValue;
    uint32_t mWidth;
    uint32_t mHeight;
    ImageFormat mFormat;
};

typedef enum LoadActionType {
    LOAD_ACTION_ANY,
    LOAD_ACTION_LOAD,
    LOAD_ACTION_CLEAR,
    MAX_LOAD_ACTION
} LoadActionType;

typedef enum StoreActionType {
    STORE_ACTION_STORE,
    STORE_ACTION_ANY,
    STORE_ACTION_NONE,
    MAX_STORE_ACTION
} StoreActionType;

typedef struct BindRenderTargetDesc BindRenderTargetDesc;
struct BindRenderTargetDesc {
    RenderTarget* pRenderTarget;
    LoadActionType mLoadAction;
    StoreActionType mStoreAction;
    // Default to gray
    ClearValue mClearValue { 0.5f, 0.5f, 0.5f, 1.0f };
};

typedef struct BindRenderTargetsDesc BindRenderTargetsDesc;
struct BindRenderTargetsDesc {
    uint32_t mRenderTargetCount;
    BindRenderTargetDesc mDepthStencil;
    // TODO max render target attachments in array?
    BindRenderTargetDesc mRenderTarget[1];
};

///////////////////////////////
/// Resource Barrier

typedef struct RenderTargetBarrier {
    RenderTarget* pRenderTarget;
    ResourceState mCurrentState;
    ResourceState mNewState;
} RenderTargetBarrier;

typedef struct BufferBarrier BufferBarrier;
struct BufferBarrier {
    // TODO
    uint32_t id;
};

typedef struct TextureBarrier TextureBarrier;
struct TextureBarrier {
    // TODO
    uint32_t id;
};

///////////////////////////////
/// Fence
typedef struct Fence {
#if defined(D3D12)
    struct {
        ID3D12Fence* pFence;
        HANDLE hEvent;
        uint64_t mFenceValue;
    } mDx;
#endif
} Fence;

typedef enum ShaderTarget {
    SHADER_TARGET_6_5,
} ShaderTarget;

typedef struct GpuDesc {
    struct {
        IDXGIAdapter4* pAdapter;
    } mDx;
    uint32_t id;
#if defined(D3D12)
    D3D_FEATURE_LEVEL mFeatureLevel;
#endif
} GpuDesc;

typedef struct RendererContextDesc {
#if defined(D3D12)
    struct {
        D3D_FEATURE_LEVEL mFeatureLevel;
    } mDx;
#endif
    bool mEnableGpuBasedValidation;
} RendererContextDesc;

typedef struct RendererContext {
#if defined(D3D12)
    struct {
        IDXGIFactory6* pDXGIFactory;
        ID3D12Debug* pDebug;
    } mDx;
#endif
    GpuDesc mGpu;
} RendererContext;

typedef struct RendererDesc {
#if defined(D3D12)
    struct {
        D3D_FEATURE_LEVEL mFeatureLevel;
    } mDx;
#endif
    ShaderTarget mShaderTarget;
    RendererContext* pContext;
} RendererDesc;

#define ALIGN_Renderer 64
ALIGNED_STRUCT(Renderer, ALIGN_Renderer) {
#if defined(D3D12)
    struct {
        // using ThirdParty/D3D12MemoryAllocator
        // TODO convert to C
        Microsoft::WRL::ComPtr<D3D12MA::Allocator> Allocator;

        struct DescriptorHeap** pCPUDescriptorHeaps;
        struct DescriptorHeap** pCbvSrvUavHeaps;
        struct DescriptorHeap** pSamplerHeaps;
        ID3D12Device* pDevice;
#if defined(D3D12) && defined(ENABLE_DEBUG)
        ID3D12InfoQueue1* pDebugValidation;
#endif
    } mDx;
#endif
    struct RendererContext* pContext;
    uint32_t mShaderTarget;
    const struct GpuDesc* pGpuDesc;
    const char* pName;
};

///////////////////////////////
/// Queue Structs
typedef struct Queue Queue;
struct Queue {
#if defined(D3D12)
    struct {
        ID3D12CommandQueue* pQueue;
    } mDx;
#endif
    uint32_t mType;
};

typedef struct QueueDesc QueueDesc;
struct QueueDesc {
    // empty for now
    uint32_t id;
};

typedef enum QueueType {
    QUEUE_TYPE_GRAPHICS = 0,
    QUEUE_TYPE_TRANSFER,
    QUEUE_TYPE_COMPUTE,
    MAX_QUEUE_TYPE
} QueueType;

///////////////////////////////
/// Command Pool
typedef struct CmdPoolDesc {
    Queue* pQueue;
} CmdPoolDesc;

typedef struct CmdPool {
#if defined(D3D12)
    struct {
        ID3D12CommandAllocator* pCmdAllocator;
    } mDx;
#endif
    Queue* pQueue;
} CmdPool;

typedef struct CmdDesc {
    CmdPool* pCmdPool;
#if defined(ENABLE_DEBUG)
    const char* pName = "";
#endif
} CmdDesc;

#define ALIGN_Cmd 64
ALIGNED_STRUCT(Cmd, ALIGN_Cmd) {
#if defined(D3D12)
    struct {
        ID3D12GraphicsCommandList1* pCmdList;
#if defined(ENABLE_DEBUG)
        ID3D12DebugCommandList* pDebugCmdList;
#endif
        CmdPool* pCmdPool;
    } mDx;
#endif
    Renderer* pRenderer;
    Queue* pQueue;
};

///////////////////////////////////
/// Swapchain
typedef struct SwapChainDesc {
    WindowHandle mWindowHandle;
    Queue** ppPresentQueues;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mImageCount;
    ImageFormat mColorFormat;
    SampleCount mSampleCount;
    bool mEnableVsync;
} SwapChainDesc;

typedef struct SwapChain {
    RenderTarget** ppRenderTargets;
#if defined(D3D12)
    struct {
        IDXGISwapChain4* pSwapchain;
        uint32_t mImageCount;
        struct DescriptorHeap* pRtvHeap;
    } mDx;
#endif
    bool mEnableVSync;
} SwapChain;

/////////////////////////////////////
/// Pipeline
typedef enum PipelineType {
    PIPELINE_TYPE_UNDEFINED = 0,
    PIPELINE_TYPE_COMPUTE,
    PIPELINE_TYPE_GRAPHICS,
    PIPELINE_TYPE_COUNT,
} PipelineType;

#define ALIGN_Pipeline 64
ALIGNED_STRUCT(Pipeline, ALIGN_Pipeline) {
#if defined(D3D12)
    struct {
        ID3D12PipelineState* pPipelineState;
        ID3D12RootSignature* pRootSignature;
        PipelineType mType;
        D3D_PRIMITIVE_TOPOLOGY mPrimitiveTopology;
    } mDx;
#endif
};

/////////////////////////////////
/// Desc
typedef struct QueueSubmitDesc QueueSubmitDesc;
struct QueueSubmitDesc {
    Cmd** ppCmd;
    Fence* pSignalFence;
    uint32_t mCmdCount;
    bool mSubmitDone;
};

typedef struct QueuePresentDesc QueuePresentDesc;
struct QueuePresentDesc {
    SwapChain* pSwapChain;
    uint32_t mIndex;
    bool mSubmitDone;
};

///////////////////////////////
/// Buffer
#define ALIGN_Buffer 64
ALIGNED_STRUCT(Buffer, ALIGN_Buffer) {
    // NOTE might move this later but good enough for now
    void* pCpuMappedAddress;

#if defined(D3D12)
    struct {
        D3D12_GPU_VIRTUAL_ADDRESS mGpuAddress;
        DxDescriptorID mDescriptors;
        uint8_t mSrvDescriptorOffset;
        uint8_t mUavDescriptorOffset;

        ID3D12Resource* pResource;

        // TODO
        uint8_t mMarkerBuffer;
        union {
            ID3D12Heap* pMarkerBufferHeap;
        };
    } mDx;
#endif

    uint64_t mSize;
    uint64_t mStructStride;
    uint64_t mDescriptors;
    ResourceMemoryUsage mMemoryUsage;
    uint32_t mFlags;
};

typedef enum BufferCreationFlags {
    BUFFER_CREATION_FLAG_NONE = 0x0,
    BUFFER_CREATIONFLAG_PERSISTENT = 0x1,
} BufferCreationFlags;

typedef struct BufferDesc BufferDesc;
struct BufferDesc {
    uint64_t mSize;

    uint32_t mFirstElement;

    uint32_t mElementCount;

    uint32_t mStructStride;

    uint32_t mAlignment;

    const char* pName;

    BufferCreationFlags mFlags;

    QueueType mQueueType;

    ResourceState mStartState;

    DescriptorType mDescriptors;

    ResourceMemoryUsage mMemoryUsage;
};

typedef struct BufferUpdateDesc BufferUpdateDesc;
struct BufferUpdateDesc {
    Buffer* pBuffer;

    uint64_t mDstOffset;
    uint64_t mSize;

    void* pMappedData;

    struct {
        void* pStagingBuffer;
        bool mNeedsUnmap; // dirty
    } mInternal;
};

/////////////////////////////////////////
// PIPELINE LAYOUT
typedef enum ResourceType {
    RESOURCE_TYPE_ROOT_CONSTANT_BUFFER,
    RESOURCE_TYPE_DESCRIPTOR_TABLE,
} ResourceType;

typedef struct RootSignatureDesc RootSignatureDesc;
struct RootSignatureDesc {
    const char* pGraphicsFileName;
    const char* pComputeFileName;
};

typedef struct DescriptorRangeDesc DescriptorRangeDesc;
struct DescriptorRangeDesc {
    uint32_t binding;
    uint32_t count;
    DescriptorType type;
};

typedef struct RootParameter RootParameter;
struct RootParameter {
    ResourceType type;
    ShaderStage shaderVisibility;
    union {
        // For RESOURCE_TYPE_ROOT_CONSTANT_BUFFER
        struct {
            uint32_t binding;
        } rootConstantBuffer;

        // For RESOURCE_TYPE_DESCRIPTOR_TABLE
        struct {
            uint32_t rangeCount;
            const DescriptorRangeDesc* pRanges;
        } descriptorTable;
    };
};

typedef struct PipelineLayout PipelineLayout;
struct PipelineLayout {
    void* pHandle;
};

typedef struct StaticSamplerDesc StaticSamplerDesc;
struct StaticSamplerDesc {
    uint32_t binding;
    ShaderStage shaderVisibility;
};

typedef struct PipelineLayoutDesc PipelineLayoutDesc;
struct PipelineLayoutDesc {
    uint32_t parameterCount;
    const RootParameter* pParameters;
    uint32_t staticSamplerCount;
    const StaticSamplerDesc* pStaticSamplers;
    const char* pShaderFileName;
};

typedef struct VertexAttrib VertexAttrib;
struct VertexAttrib {
    const char* pSemanticName;

    ImageFormat mFormat;

    uint32_t mBinding;

    uint32_t mOffset;

    uint32_t mLocation;
};

typedef struct VertexLayout VertexLayout;
struct VertexLayout {
    uint32_t mAttribCount;
    VertexAttrib* pAttribs;
    uint32_t mStride;
};

typedef enum CullMode { CULL_MODE_NONE = 0, CULL_MODE_FRONT, CULL_MODE_BACK } CullMode;
typedef enum FillMode { FILL_MODE_SOLID = 0, FILL_MODE_WIREFRAME } FillMode;

typedef struct RasterizerStateDesc RasterizerStateDesc;
struct RasterizerStateDesc {
    CullMode mCullMode;
    FillMode mFillMode;
};

typedef enum CompareMode {
    COMPARE_MODE_NEVER,
    COMPARE_MODE_LESS,
    COMPARE_MODE_EQUAL,
    COMPARE_MODE_LESS_EQUAL,
    COMPARE_MODE_GREATER,
    COMPARE_MODE_NOT_EQUAL,
    COMPARE_MODE_GREATER_EQUAL,
    COMPARE_MODE_ALWAYS,
} CompareMode;

typedef struct DepthStateDesc DepthStateDesc;
struct DepthStateDesc {
    bool mDepthTest;
    bool mDepthWrite;
    CompareMode mDepthFunc;
};

typedef enum PrimitiveTopology {
    PRIMITIVE_TOPO_TRI_LIST,
} PrimitiveTopology;

typedef struct GraphicsPipelineDesc GraphicsPipelineDesc;
struct GraphicsPipelineDesc {
    PipelineLayout* pPipelineLayout;

    Shader* pShaderProgram;

    VertexLayout* pVertexLayout;

    RasterizerStateDesc* pRasterizerState;
    DepthStateDesc* pDepthState;

    PrimitiveTopology mPrimitiveTopo;
    uint32_t mRenderTargetCount;
    ImageFormat* pColorFormats;
    ImageFormat mDepthStencilFormat;
    SampleCount mSampleCount;
};

typedef struct BufferLoadDesc BufferLoadDesc;
struct BufferLoadDesc {
    Buffer** ppBuffer;
    const void* pData;
    BufferDesc mDesc;
};

///////////////////////////////////////////////
/// Descriptor Set
typedef struct Descriptor {
    DescriptorType mType;
    uint32_t mCount;
    uint32_t mOffset;
} Descriptor;

typedef struct DescriptorDataRange {
    uint32_t mOffset;
    uint32_t mSize;

    uint32_t mStructStride;
} DescriptorDataRange;

typedef struct DescriptorData {
    uint32_t mCount : 31;
    uint32_t mBindStencilResource : 1;
    uint32_t mArrayOffset : 20;
    uint32_t mIndex : 12;
    DescriptorDataRange* pRanges;
    union {
        struct {
            uint16_t mUAVMipSlice;
            bool mBindMipChain;
        };
    };
    union {
        Texture** ppTextures;
        /// TODO add sampler Array of sampler descriptors
        Buffer** ppBuffers;
    };
} DescriptorData;

typedef struct DescriptorSetDesc {
    PipelineLayout* pPipelineLayout;
    uint32_t rootParameterIndex;
    uint32_t mIndex;
    uint32_t mMaxSets;
    uint32_t mNodeIndex;
    uint32_t mDescriptorCount;
    const Descriptor* pDescriptors;
} DescriptorSetDesc;

#define ALIGN_DescriptorSet 64
ALIGNED_STRUCT(DescriptorSet, ALIGN_DescriptorSet) {
#if defined(D3D12)
    struct {
        DxDescriptorID mCbvSrvUavHandle;
        const struct Descriptor* pDescriptors;
        uint32_t mPipelineType;

        uint32_t mDescriptorCount;
        uint32_t mDescriptorStride;
        uint32_t mRootParamterIndex;
    } mDx;
#endif
};

// --------------------------------------------------------------
// API Function Declarations
// --------------------------------------------------------------

VT_API void addResource(Renderer* pRenderer, BufferLoadDesc* pBufferDesc);

VT_API void initRenderer(const char* name, RendererDesc* pDesc, Renderer** ppRenderer);
VT_API void initRendererContext(const char* appName,
                                const RendererContextDesc* pSettings,
                                RendererContext** ppContext);
VT_API void shutdownRenderer(Renderer* pRenderer);

// Fence
VT_API void initFence(Renderer* pRenderer, Fence** ppFence);
VT_API void exitFence(Renderer* pRenderer, Fence* pFence);
VT_API void waitFence(Renderer* pRenderer, Fence* pFence);

// Queue
VT_API void initQueue(Renderer* pRenderer, QueueDesc* pQDesc, Queue** ppQueue);
VT_API void exitQueue(Renderer* pRenderer, Queue* pQueue);
VT_API void waitQueueIdle(Queue* pQueue, Fence* pFence);

// Swapchain
VT_API void initSwapChain(Renderer* pRenderer, SwapChainDesc* pDesc, SwapChain** ppSwapChain);
VT_API void exitSwapChain(Renderer* pRenderer, SwapChain* pSwapChain);
VT_API void
acquireNextImage(Renderer* pRenderer, SwapChain* pSwapChain, Fence* pFence, uint32_t* pImageIndex);

// Pipeline
VT_API void
addPipeline(Renderer* pRenderer, const GraphicsPipelineDesc* pDesc, Pipeline** ppPipeline);
VT_API void removeGraphicsPipeline(Renderer* pRenderer, Pipeline* pPipeline);

// Cmd
VT_API void initCmdPool(Renderer* pRenderer, CmdPoolDesc* pDesc, CmdPool** ppCmdPool);
VT_API void exitCmdPool(Renderer* pRenderer, CmdPool* pCmdPool);
VT_API void initCmd(Renderer* pRenderer, CmdDesc* pDesc, Cmd** ppCmd);
VT_API void exitCmd(Renderer* pRenderer, Cmd* pCmd);

VT_API void resetCmdPool(Renderer* pRenderer, CmdPool* pCmdPool);
VT_API void beginCmd(Cmd* pCmd);
VT_API void endCmd(Cmd* pCmd);

// Command Submission
VT_API void queueSubmit(Queue* pQueue, const QueueSubmitDesc* pDesc);
VT_API void queuePresent(Queue* pQueue, const QueuePresentDesc* pDesc);

// Drawing Commands
VT_API void cmdBindRenderTargets(Cmd* pCmd, const BindRenderTargetsDesc* pDesc);
VT_API void cmdClearRenderTarget(Cmd* pCmd, RenderTarget* pRenderTarget, const float* pClearColor);
VT_API void cmdSetViewport(Cmd* pCmd,
                           float x,
                           float y,
                           float width,
                           float height,
                           float minDepth,
                           float maxDepth);
VT_API void cmdSetScissor(Cmd* pCmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

VT_API void cmdResourceBarrier(Cmd* pCmd,
                               uint32_t numBufferBarriers,
                               BufferBarrier* pBufferBarriers,
                               uint32_t numTextureBarrier,
                               TextureBarrier* pTextureBarrier,
                               uint32_t numRtBarriers,
                               RenderTargetBarrier* pRtBarriers);

VT_API void cmdSetPipelineLayout(Cmd* pCmd, PipelineLayout* pPipelineLayout);
VT_API void cmdBindPipeline(Cmd* pCmd, Pipeline* pPipeline);
VT_API
void cmdBindVertexBuffer(Cmd* pCmd, uint32_t bufferCount, Buffer** ppVertexBuffers);

VT_API void cmdDraw(Cmd* pCmd, uint32_t vertexCount, uint32_t firstVertex);
// Pipeline layout

VT_API void initPipelineLayout(Renderer* pRenderer,
                               const PipelineLayoutDesc* pDesc,
                               PipelineLayout** ppPipelineLayout);
VT_API void exitPipelineLayout(Renderer* pRenderer, PipelineLayout* pPipelineLayout);

// Shader
VT_API void addShader(Renderer* pRenderer, const char* pFileName, Shader** ppShader);

// Render target
VT_API void
addRenderTarget(Renderer* pRenderer, const RenderTargetDesc* pDesc, RenderTarget** ppRenderTarget);

// Resources
VT_API void beginUpdateResource(Renderer* pRenderer, BufferUpdateDesc* pDesc);
VT_API void endUpdateResource(Renderer* pRenderer, BufferUpdateDesc* pDesc);

// Descripotor sets
VT_API void
addDescriptorSet(Renderer* pRenderer, const DescriptorSetDesc* pDesc, DescriptorSet** ppSet);
VT_API void removeDescriptorSet(Renderer* pRenderer, DescriptorSet* pSet);
VT_API void updateDescriptorSet(Renderer* pRenderer,
                                uint32_t setIndex,
                                DescriptorSet* pSet,
                                uint32_t dataCount,
                                const DescriptorData* pData);
VT_API void cmdBindDescriptorSet(Cmd* pCmd, DescriptorSet* pSet, uint32_t setIndex);

#ifdef __cplusplus
}
#endif
