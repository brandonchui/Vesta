#include "Common/Util/Time.h"
#define ENABLE_DEBUG

#include "Common/IApp.h"
#include "Common/IGraphics.h"
#include "Common/RingBuffer.hpp"
#include "Common/Util/Logger.h"

// Global
const uint32_t gFrameCount = 2;
uint32_t gFrameIndex = 0;

Renderer* pRenderer = NULL;
Queue* pQueue = NULL;
SwapChain* pSwapChain = NULL;

// Handles fence, cmd pool, and cmd list
GpuCmdRing gCmdRing = {};

ClearValue color { 0.2f, 0.2f, 0.2f, 1.0f };

//////////////////////////////////////////////
// Application
class Example : public IApp {
  public:
    bool Init() override {
        mSettings.pTitle = "00_Init";

        RendererDesc settings = {};
        initRenderer(mSettings.pTitle, &settings, &pRenderer);
        if (!pRenderer) {
            VT_ERROR("Failed to initialize renderer.");
            return false;
        }

        QueueDesc queueDesc = {};
        initQueue(pRenderer, &queueDesc, &pQueue);
        if (!pQueue) {
            VT_ERROR("Failed to initialize queue.");
            return false;
        }

        GpuCmdRingDesc cmdRingDesc = {};
        cmdRingDesc.pQueue = pQueue;
        cmdRingDesc.mPoolCount = gFrameCount;
        cmdRingDesc.mCmdPerPoolCount = 1;
        initGpuCmdRing(pRenderer, &cmdRingDesc, &gCmdRing);

        if (!pInput) {
            return false;
        }

        InitLogging(GetName());
        VT_INFO("App Initialized.");
        return true;
    }

    void ShutDown() override {
        waitQueueIdle(pQueue, gCmdRing.pFences[0][0]);
        exitGpuCmdRing(pRenderer, &gCmdRing);

        exitSwapChain(pRenderer, pSwapChain);
        exitQueue(pRenderer, pQueue);
        shutdownRenderer(pRenderer);

        ShutdownLogging();
    }

    // Non-Rendering Logic in here
    void Update(float deltaTime) override {
        // Change color based on time
        static float time = 0.0f;
        time += deltaTime;
        color.r = (sin(time * 2.0f) + 1.0f) * 0.5f;
        color.g = (sin(time * 3.0f) + 1.0f) * 0.5f;
        color.b = (sin(time * 4.0f) + 1.0f) * 0.5f;

        if (pInput->WasPressed(Key::Space)) {
            VT_TRACE("Time: {}", Time::GetTotalTime());
        }
    }

    // Rendering stuff here
    void Draw() override {
        if (!pSwapChain) {
            if (!addSwapChain()) {
                return;
            }
        }

        // Grab pool,cmd,fence for this frame
        GpuCmdRingElement elem = getNextGpuCmdRingElement(&gCmdRing, true, 1);
        Cmd* pCmd = elem.pCmds[0];

        waitFence(pRenderer, elem.pFence);

        uint32_t swapChainImageIndex;
        acquireNextImage(pRenderer, pSwapChain, NULL, &swapChainImageIndex);
        RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapChainImageIndex];

        resetCmdPool(pRenderer, elem.pCmdPool);
        beginCmd(pCmd);

        // Transition Present -> Render Target
        RenderTargetBarrier rtBarrier = { pRenderTarget,
                                          RESOURCE_STATE_PRESENT,
                                          RESOURCE_STATE_RENDER_TARGET };
        cmdResourceBarrier(pCmd, 0, NULL, 0, NULL, 1, &rtBarrier);

        // Set the render target
        BindRenderTargetsDesc bindRenderTargets = {};
        bindRenderTargets.mRenderTargetCount = 1;
        bindRenderTargets.mRenderTarget[0] = { pRenderTarget,
                                               LOAD_ACTION_CLEAR,
                                               STORE_ACTION_STORE,
                                               color };
        cmdBindRenderTargets(pCmd, &bindRenderTargets);

        cmdSetViewport(pCmd,
                       0.0f,
                       0.0f,
                       (float) pRenderTarget->mWidth,
                       (float) pRenderTarget->mHeight,
                       0.0f,
                       1.0f);

        // Transition Target -> Present
        rtBarrier = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
        cmdResourceBarrier(pCmd, 0, NULL, 0, NULL, 1, &rtBarrier);

        endCmd(pCmd);

        // Submit Commands
        QueueSubmitDesc submitDesc = {};
        submitDesc.mCmdCount = 1;
        submitDesc.ppCmd = &pCmd;
        submitDesc.pSignalFence = elem.pFence;
        queueSubmit(pQueue, &submitDesc);

        QueuePresentDesc presentDesc = {};
        presentDesc.pSwapChain = pSwapChain;
        presentDesc.mIndex = swapChainImageIndex;
        queuePresent(pQueue, &presentDesc);

        gFrameIndex = (gFrameIndex + 1) % gFrameCount;
    }

    const char* GetName() override { return mSettings.pTitle; }

    bool addSwapChain() {
        if (!pWindow)
            return false;

        waitQueueIdle(pQueue, gCmdRing.pFences[0][0]);

        SwapChainDesc swapChainDesc = {};
        swapChainDesc.mWindowHandle = pWindow->handle;
        swapChainDesc.mWidth = pWindow->width;
        swapChainDesc.mHeight = pWindow->height;
        swapChainDesc.ppPresentQueues = &pQueue;
        swapChainDesc.mImageCount = 2;
        swapChainDesc.mSampleCount = SAMPLE_COUNT_1;
        swapChainDesc.mColorFormat = IMAGE_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.mEnableVsync = true;
        initSwapChain(pRenderer, &swapChainDesc, &pSwapChain);
        return pSwapChain != NULL;
    }
};

DEFINE_APPLICATION_MAIN(Example)
