#include "../Config.h"
#include "../IGraphics.h"

#include <stdio.h>
#include <windows.h>
#include <wrl/client.h>
#include <cassert>

using Microsoft::WRL::ComPtr;

#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#if (defined(D3D12))

template <typename T> inline void SafeRelease(T*& p) {
    if (p != nullptr) {
        p->Release();
        p = nullptr;
    }
}

static DXGI_FORMAT to_dx_format(ImageFormat format) {
    switch (format) {
        case IMAGE_FORMAT_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        default:
            assert(false && "Invalid Image Format");
            return DXGI_FORMAT_UNKNOWN;
    }
}

static D3D12_RESOURCE_STATES to_dx_resource_state(ResourceState state) {
    D3D12_RESOURCE_STATES res = D3D12_RESOURCE_STATE_COMMON;
    if (state & RESOURCE_STATE_RENDER_TARGET)
        res |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    if (state & RESOURCE_STATE_PRESENT)
        res |= D3D12_RESOURCE_STATE_PRESENT;
    if (state & RESOURCE_STATE_SHADER_RESOURCE)
        res |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if (state == RESOURCE_STATE_UNDEFINED) {
        res = D3D12_RESOURCE_STATE_COMMON;
    }
    return res;
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

//-----------------------------------------------------
// Implementation
//-----------------------------------------------------

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
    if (pRenderer->mDx.Allocator) {
        pRenderer->mDx.Allocator->Release();
        pRenderer->mDx.Allocator = nullptr;
    }
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

    // Get GPU
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

// Cmd (Command Allocator)
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
        // TODO: Add implementation for BufferBarriers
    }

    if (numTextureBarrier > 0) {
        // TODO: Add implementation for TextureBarriers
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

// Draw
void cmdBindRenderTargets(Cmd* pCmd, const BindRenderTargetsDesc* pDesc) {
    if (!pDesc) {
        pCmd->mDx.pCmdList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
        return;
    }

    const uint32_t renderTargetCount = pDesc->mRenderTargetCount;

    if (renderTargetCount == 0) {
        pCmd->mDx.pCmdList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandles = new D3D12_CPU_DESCRIPTOR_HANDLE[renderTargetCount];

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

    pCmd->mDx.pCmdList->OMSetRenderTargets(renderTargetCount, rtvHandles, FALSE, nullptr);

    delete[] rtvHandles;
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

#endif
