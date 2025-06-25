#include "Common/Util/Time.h"
#define ENABLE_DEBUG

#include "Common/IApp.h"
#include "Common/IGraphics.h"
#include "Common/Util/Logger.h"

// Global
Renderer* pRenderer = NULL;
Queue* pQueue = NULL;

SwapChain* pSwapChain = NULL;
Fence* pFence = NULL;
CmdPool* pCmdPool = NULL;
Cmd* pCmd = NULL;

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

        initFence(pRenderer, &pFence);
        if (!pFence) {
            VT_ERROR("Failed to initialize fence.");
            return false;
        }

        CmdPoolDesc cmdPoolDesc = { pQueue };
        initCmdPool(pRenderer, &cmdPoolDesc, &pCmdPool);

        CmdDesc cmdDesc = { pCmdPool };
        initCmd(pRenderer, &cmdDesc, &pCmd);

        if (!pInput) {
            return false;
        }

        InitLogging(GetName());
        VT_INFO("App Initialized.");
        return true;
    }

    void ShutDown() override {
        waitQueueIdle(pQueue, pFence);

        exitCmd(pRenderer, pCmd);
        exitCmdPool(pRenderer, pCmdPool);
        exitFence(pRenderer, pFence);
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

        uint32_t mFrameIndex = 0; // Used for ring buffer later

        acquireNextImage(pRenderer, pSwapChain, pFence, &mFrameIndex);
        RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[mFrameIndex];

        resetCmdPool(pRenderer, pCmdPool);

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
        submitDesc.pSignalFence = pFence;
        queueSubmit(pQueue, &submitDesc);

        QueuePresentDesc presentDesc = {};
        presentDesc.pSwapChain = pSwapChain;
        presentDesc.mIndex = mFrameIndex;
        queuePresent(pQueue, &presentDesc);

        // Wait for the frame to finish
        waitFence(pRenderer, pFence);
    }

    const char* GetName() override { return mSettings.pTitle; }

    bool addSwapChain() {
        if (!pWindow)
            return false;

        waitQueueIdle(pQueue, pFence);

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
