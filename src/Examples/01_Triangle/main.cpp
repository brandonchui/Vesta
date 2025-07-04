#include "Common/IApp.h"
#include "Common/IGraphics.h"
#include "Common/Util/Logger.h"

// Globals
bool isWireframe = false;
Renderer* pRenderer = NULL;
Queue* pQueue = NULL;
SwapChain* pSwapChain = NULL;
Fence* pFence = NULL;
CmdPool* pCmdPool = NULL;
Cmd* pCmd = NULL;

PipelineLayout* pTrianglePipelineLayout = NULL;

Pipeline* pTrianglePipeline = NULL;
Pipeline* pWireframePipeline = NULL;

Buffer* pTriangleVertexBuffer = NULL;
Buffer* pQuadVertexBuffer = NULL;

enum Shape { SHAPE_TRIANGLE = 0, SHAPE_QUAD, SHAPE_COUNT };
Shape gCurrentShape = SHAPE_TRIANGLE;

struct Vector3 {
    float x, y, z;
};

struct Vector4 {
    float x, y, z, w;
};

struct VertexPosColor {
    Vector3 pos;
    Vector4 color;
};

struct ObjectConstants {
    float colorChangeValue;
};

VertexPosColor gTriangleVertices[] = { { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
                                       { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                                       { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } } };

VertexPosColor gQuadVertices[] = { { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
                                   { { 0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 1.0f, 1.0f } },
                                   { { 0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },

                                   { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
                                   { { 0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },
                                   { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } } };

//////////////////////////////////////////////
// Application
class Example : public IApp {
  public:
    void addPipelines() {
        VertexLayout vertexLayout = {};
        vertexLayout.mAttribCount = 2;
        VertexAttrib attribs[2] = {
            { "POSITION", IMAGE_FORMAT_R32G32B32_FLOAT, 0, 0, 0 },
            { "COLOR", IMAGE_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(Vector3), 1 }
        };
        vertexLayout.pAttribs = attribs;

        Shader* pTriangleShader = NULL;
        addShader(pRenderer, "Shaders/Simple.hlsl", &pTriangleShader);
        if (!pTriangleShader) {
            VT_ERROR("Failed to create shader.");
            return;
        }

        RasterizerStateDesc solidRasterizerStateDesc = {};
        solidRasterizerStateDesc.mCullMode = CULL_MODE_NONE;
        solidRasterizerStateDesc.mFillMode = FILL_MODE_SOLID;

        GraphicsPipelineDesc pipelineDesc = {};
        pipelineDesc.pPipelineLayout = pTrianglePipelineLayout;
        pipelineDesc.pShaderProgram = pTriangleShader;
        pipelineDesc.pRasterizerState = &solidRasterizerStateDesc;
        pipelineDesc.pVertexLayout = &vertexLayout;
        pipelineDesc.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineDesc.mRenderTargetCount = 1;
        ImageFormat colorFormat = IMAGE_FORMAT_R8G8B8A8_UNORM;
        pipelineDesc.pColorFormats = &colorFormat;
        pipelineDesc.mDepthStencilFormat = IMAGE_FORMAT_UNDEFINED;
        pipelineDesc.mSampleCount = SAMPLE_COUNT_1;

        addPipeline(pRenderer, &pipelineDesc, &pTrianglePipeline);

        RasterizerStateDesc wireframeRasterizerStateDesc = {};
        wireframeRasterizerStateDesc.mCullMode = CULL_MODE_NONE;
        wireframeRasterizerStateDesc.mFillMode = FILL_MODE_WIREFRAME;

        pipelineDesc.pRasterizerState = &wireframeRasterizerStateDesc;

        addPipeline(pRenderer, &pipelineDesc, &pWireframePipeline);
    }

    bool Init() override {
        mSettings.pTitle = "01_Triangle";

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

        PipelineLayoutDesc pipelineLayoutDesc = {};
        pipelineLayoutDesc.pShaderFileName = "Shaders/Simple2.hlsl";
        initPipelineLayout(pRenderer, &pipelineLayoutDesc, &pTrianglePipelineLayout);

        addPipelines();

        BufferLoadDesc vbLoadDesc = {};

        vbLoadDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        vbLoadDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        vbLoadDesc.mDesc.mSize = sizeof(gTriangleVertices);
        vbLoadDesc.mDesc.mStructStride = sizeof(VertexPosColor);
        vbLoadDesc.pData = gTriangleVertices;
        vbLoadDesc.ppBuffer = &pTriangleVertexBuffer;

        addResource(pRenderer, &vbLoadDesc);
        if (!pTriangleVertexBuffer) {
            return false;
        }

        vbLoadDesc.mDesc.mSize = sizeof(gQuadVertices);
        vbLoadDesc.pData = gQuadVertices;
        vbLoadDesc.ppBuffer = &pQuadVertexBuffer;

        addResource(pRenderer, &vbLoadDesc);
        if (!pQuadVertexBuffer) {
            return false;
        }

        if (!pInput) {
            return false;
        }

        InitLogging(GetName());
        VT_INFO("App Initialized.");
        return true;
    }

    void ShutDown() override {
        waitQueueIdle(pQueue, pFence);

        removeGraphicsPipeline(pRenderer, pTrianglePipeline);
        exitPipelineLayout(pRenderer, pTrianglePipelineLayout);

        exitCmd(pRenderer, pCmd);
        exitCmdPool(pRenderer, pCmdPool);
        exitFence(pRenderer, pFence);
        exitSwapChain(pRenderer, pSwapChain);
        exitQueue(pRenderer, pQueue);
        shutdownRenderer(pRenderer);

        ShutdownLogging();
    }

    void Update(float deltaTime) override {
        if (pInput->WasPressed(Key::F)) {
            isWireframe = !isWireframe;
        }
        if (pInput->WasPressed(Key::Space)) {
            gCurrentShape = (Shape) ((gCurrentShape + 1) % SHAPE_COUNT);
        }
    }

    void Draw() override {
        if (!pSwapChain) {
            if (!addSwapChain()) {
                return;
            }
        }

        uint32_t frameIndex = 0;
        acquireNextImage(pRenderer, pSwapChain, pFence, &frameIndex);
        RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[frameIndex];

        resetCmdPool(pRenderer, pCmdPool);
        beginCmd(pCmd);

        RenderTargetBarrier rtBarrier = { pRenderTarget,
                                          RESOURCE_STATE_PRESENT,
                                          RESOURCE_STATE_RENDER_TARGET };
        cmdResourceBarrier(pCmd, 0, NULL, 0, NULL, 1, &rtBarrier);

        BindRenderTargetsDesc bindRenderTargets = {};
        bindRenderTargets.mRenderTargetCount = 1;
        bindRenderTargets.mRenderTarget[0] = { pRenderTarget,
                                               LOAD_ACTION_CLEAR,
                                               STORE_ACTION_STORE,
                                               { 0.1f, 0.1f, 0.1f, 1.0f } };
        cmdBindRenderTargets(pCmd, &bindRenderTargets);

        cmdSetViewport(pCmd,
                       0.0f,
                       0.0f,
                       (float) pRenderTarget->mWidth,
                       (float) pRenderTarget->mHeight,
                       0.0f,
                       1.0f);
        cmdSetScissor(pCmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

        if (isWireframe) {
            cmdBindPipeline(pCmd, pWireframePipeline);
        } else {
            cmdBindPipeline(pCmd, pTrianglePipeline);
        }

        switch (gCurrentShape) {
            case SHAPE_TRIANGLE:
                cmdBindVertexBuffer(pCmd, 1, &pTriangleVertexBuffer);
                cmdDraw(pCmd, 3, 0);
                break;

            case SHAPE_QUAD:
                cmdBindVertexBuffer(pCmd, 1, &pQuadVertexBuffer);
                cmdDraw(pCmd, 6, 0);
                break;
            case SHAPE_COUNT:
                break;
        }

        rtBarrier = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
        cmdResourceBarrier(pCmd, 0, NULL, 0, NULL, 1, &rtBarrier);

        endCmd(pCmd);

        QueueSubmitDesc submitDesc = { &pCmd, pFence, 1, false };
        queueSubmit(pQueue, &submitDesc);

        QueuePresentDesc presentDesc = { pSwapChain, frameIndex, false };
        queuePresent(pQueue, &presentDesc);

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
