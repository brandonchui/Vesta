#pragma once
#include "Config.h"
#include "IOperatingSystem.h"

#if defined(D3D12)
#include <D3D12MemAlloc.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Enums and Structs
typedef enum ImageFormat {
    IMAGE_FORMAT_R8G8B8A8_UNORM,
} ImageFormat;

typedef enum SampleCount {
    SAMPLE_COUNT_1,
} SampleCount;

////////////////////////////////////////////
/// Shader
typedef enum ShaderStage {
    SHADER_STAGE_NONE = 0,
    SHADER_STAGE_VERT = 0x1,
    SHADER_STAGE_FRAG = 0x2,
} ShaderStage;

typedef struct Shader Shader;
struct Shader {
    ShaderStage mStages;
#if defined(D3D12)
    struct {
        LPCWSTR* pEntryNames;
        // TODO blob here?
    } mDx;
#endif
};

///////////////////////////////////////////////
/// Descriptor Set
#if defined(D3D12)
typedef uint64_t DxDescriptorID;
#endif

typedef enum DescriptorType {
    DESCRIPTOR_TYPE_UNDEFINED = 0,
    DESCRIPTOR_TYPE_SAMPLER = 0x01,
} DescriptorType;

typedef struct Descriptor {
    DescriptorType mType;
    uint32_t mCount;
    uint32_t mOffSet;
} Descriptor;

#define ALIGN_DescriptorSet 64
ALIGNED_STRUCT(DescriptorSet, ALIGN_DescriptorSet) {
#if defined(D3D12)
    struct {
        DxDescriptorID mCbvSrvUavHandle;
        const struct Descriptor* pDescriptors;
    } mDx;
#endif
};

///////////////////////////////
/// Render Target
typedef struct ClearValue {
    float r, g, b, a;
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

#define ALIGN_RenderTarget 64
ALIGNED_STRUCT(RenderTarget, ALIGN_RenderTarget) {
    Texture* pTexture;
#if defined(D3D12)
    struct {
        D3D12_CPU_DESCRIPTOR_HANDLE mDxRtvHandle;
    } mDx;
#endif
    ClearValue mClearValue;
    uint32_t mWidth;
    uint32_t mHeight;
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
    // Default to a gray
    ClearValue mClearValue { 0.5f, 0.5f, 0.5f, 1.0f };
};

typedef struct BindRenderTargetsDesc BindRenderTargetsDesc;
struct BindRenderTargetsDesc {
    uint32_t mRenderTargetCount;
    // TODO max render target attachments in array?
    BindRenderTargetDesc mRenderTarget[1];
};

///////////////////////////////
/// Resource Barrier
typedef enum ResourceState {
    RESOURCE_STATE_UNDEFINED = 0,
    RESOURCE_STATE_RENDER_TARGET = 1 << 0,
    RESOURCE_STATE_PRESENT = 1 << 1,
    RESOURCE_STATE_SHADER_RESOURCE = 1 << 2,
} ResourceState;

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
#if defined(D3D12)
    struct {
        D3D12_GPU_VIRTUAL_ADDRESS mGpuAddress;
        DxDescriptorID mDescriptors;
        uint8_t mSrvDescriptorOffset;
        uint8_t mUavDescriptorOffset;

        // Resource handle
        ID3D12Resource* pResource;
        uint8_t mMarkerBuffer;

        union {
            ID3D12Heap* pMarkerBufferHeap;
        };
    } mDx;
#endif

    uint64_t mSize;
    uint64_t mDescriptors;
};

typedef enum BufferCreationFlags {
    BUFFER_CREATION_FLAG_NONE = 0x0,
} BufferCreationFlags;

typedef struct BufferDesc BufferDesc;
struct BufferDesc {
    uint64_t mSize;

    uint32_t mFirstElement;

    uint32_t mElementCount;

    uint32_t mStructStride;

    uint32_t mAlignment;

    // Debug purposes
    const char* pName;

    BufferCreationFlags mFlags;

    QueueType mQueueType;
    /// What state will the buffer get created in
    ResourceState mStartState;

    DescriptorType mDescriptors;
};

// --------------------------------------------------------------
// API Function Declarations
// --------------------------------------------------------------

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

VT_API void cmdResourceBarrier(Cmd* pCmd,
                               uint32_t numBufferBarriers,
                               BufferBarrier* pBufferBarriers,
                               uint32_t numTextureBarrier,
                               TextureBarrier* pTextureBarrier,
                               uint32_t numRtBarriers,
                               RenderTargetBarrier* pRtBarriers);

#ifdef __cplusplus
}
#endif
