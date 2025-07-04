#include "Common/IApp.h"
#include "Common/IGraphics.h"
#include "Common/RingBuffer.hpp"
#include "Common/Util/Logger.h"
#include "VTMath.h"

const uint32_t gObjectCount = 3;

inline uint64_t AlignUp(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

struct ObjectConstants {
    Matrix4x4 worldMatrix;
    Matrix4x4 viewProjMatrix;
};

const uint32_t gFrameCount = 2;
uint32_t gFrameIndex = 0;

Buffer* pConstantBuffer[gObjectCount][gFrameCount] = { { nullptr } };
DescriptorSet* pDescriptorSet[gObjectCount] = { nullptr };

Descriptor gUniformDescriptorLayout[] = { { DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 0 } };
ObjectConstants gObjectConstants[gObjectCount];

Matrix4x4 gViewMatrix;
Matrix4x4 gProjectionMatrix;

bool isWireframe = false;
Renderer* pRenderer = NULL;
Queue* pQueue = NULL;
SwapChain* pSwapChain = NULL;

PipelineLayout* pTrianglePipelineLayout = NULL;
Pipeline* pTrianglePipeline = NULL;
Pipeline* pWireframePipeline = NULL;
Buffer* pTriangleVertexBuffer = NULL;

// Declare the GpuCmdRing
GpuCmdRing gCmdRing = {};

struct VertexPosColor {
    Vector3 pos;
    Vector4 color;
};

VertexPosColor gTriangleVertices[] = { { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
                                       { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                                       { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } } };

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
        addShader(pRenderer, "Shaders/SimpleMovable.hlsl", &pTriangleShader);
        if (!pTriangleShader) {
            VT_ERROR("Failed to create shader.");
            return;
        }

        RasterizerStateDesc solidRasterizerStateDesc = { CULL_MODE_NONE, FILL_MODE_SOLID };
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

        RasterizerStateDesc wireframeRasterizerStateDesc = { CULL_MODE_NONE, FILL_MODE_WIREFRAME };
        pipelineDesc.pRasterizerState = &wireframeRasterizerStateDesc;
        addPipeline(pRenderer, &pipelineDesc, &pWireframePipeline);
    }

    bool Init() override {
        mSettings.pTitle = "05_MultiObjects";

        RendererDesc settings = {};
        initRenderer(mSettings.pTitle, &settings, &pRenderer);
        if (!pRenderer)
            return false;

        QueueDesc queueDesc = {};
        initQueue(pRenderer, &queueDesc, &pQueue);
        if (!pQueue)
            return false;

        GpuCmdRingDesc cmdRingDesc = {};
        cmdRingDesc.pQueue = pQueue;
        cmdRingDesc.mPoolCount = gFrameCount;
        cmdRingDesc.mCmdPerPoolCount = 1;
        initGpuCmdRing(pRenderer, &cmdRingDesc, &gCmdRing);

        PipelineLayoutDesc pipelineLayoutDesc = {};
        pipelineLayoutDesc.pShaderFileName = "Shaders/SimpleMovable.hlsl";
        initPipelineLayout(pRenderer, &pipelineLayoutDesc, &pTrianglePipelineLayout);

        addPipelines();

        BufferLoadDesc vbLoadDesc = {};
        vbLoadDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        vbLoadDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        vbLoadDesc.mDesc.mSize = sizeof(gTriangleVertices);
        vbLoadDesc.mDesc.mStructStride = sizeof(VertexPosColor);
        vbLoadDesc.pData = gTriangleVertices;
        vbLoadDesc.ppBuffer = &pTriangleVertexBuffer;
        addResource(pRenderer, &vbLoadDesc);

        for (uint32_t i = 0; i < gObjectCount; ++i) {
            for (uint32_t j = 0; j < gFrameCount; ++j) {
                BufferDesc cbDesc = {};
                cbDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                cbDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
                cbDesc.mFlags = BUFFER_CREATIONFLAG_PERSISTENT;
                cbDesc.mSize = AlignUp(sizeof(ObjectConstants), 256);
                BufferLoadDesc cbLoadDesc = { &pConstantBuffer[i][j], nullptr, cbDesc };
                addResource(pRenderer, &cbLoadDesc);
            }
        }

        DescriptorSetDesc setDesc = {};
        setDesc.pPipelineLayout = pTrianglePipelineLayout;
        setDesc.mIndex = 0;
        setDesc.mMaxSets = gFrameCount;
        setDesc.mDescriptorCount = 1;
        setDesc.pDescriptors = gUniformDescriptorLayout;
        for (uint32_t i = 0; i < gObjectCount; ++i) {
            addDescriptorSet(pRenderer, &setDesc, &pDescriptorSet[i]);
        }

        gViewMatrix =
            Matrix4x4::CreateLookAt(Vector3(0.0f, 0.0f, -5.0f), Vector3(0, 0, 0), Vector3(0, 1, 0));
        gProjectionMatrix = Matrix4x4::CreatePerspectiveFieldOfView(ToRadians(60.0f),
                                                                    (float) mSettings.mWidth /
                                                                        (float) mSettings.mHeight,
                                                                    0.1f,
                                                                    100.0f);

        if (!pInput)
            return false;
        InitLogging(GetName());
        VT_INFO("App Initialized.");
        return true;
    }

    void ShutDown() override {
        // Correctly call waitQueueIdle with a valid fence from the command ring
        waitQueueIdle(pQueue, gCmdRing.pFences[0][0]);

        exitGpuCmdRing(pRenderer, &gCmdRing);

        for (uint32_t i = 0; i < gObjectCount; ++i) {
            removeDescriptorSet(pRenderer, pDescriptorSet[i]);
        }
    }

    void Update(float deltaTime) override {
        if (pInput->WasPressed(Key::F))
            isWireframe = !isWireframe;

        static float totalTime = 0.0f;
        totalTime += deltaTime;

        Matrix4x4 vp = gViewMatrix * gProjectionMatrix;
        vp.Transpose();

        for (int32_t i = 0; i < gObjectCount; ++i) {
            float x_offset = (float) (i - 1) * 2.0f;
            Matrix4x4 translation = Matrix4x4::CreateTranslation(x_offset, 0.0f, 0.0f);

            float scale_val = (i == 1) ? 2.0f : 1.0f;
            Matrix4x4 scale = Matrix4x4::CreateScale(scale_val);

            Matrix4x4 rotation = Matrix4x4::CreateRotationY(totalTime * (i * 0.5f + 0.5f));

            Matrix4x4 world = scale * rotation * translation;
            world.Transpose();

            gObjectConstants[i].worldMatrix = world;
            gObjectConstants[i].viewProjMatrix = vp;
        }
    }

    void Draw() override {
        if (!pSwapChain) {
            if (!addSwapChain())
                return;
        }

        uint32_t swapChainImageIndex;
        acquireNextImage(pRenderer, pSwapChain, NULL, &swapChainImageIndex);
        RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapChainImageIndex];

        GpuCmdRingElement elem = getNextGpuCmdRingElement(&gCmdRing, true, 1);
        Cmd* pCurrentCmd = elem.pCmds[0];

        waitFence(pRenderer, elem.pFence);

        resetCmdPool(pRenderer, elem.pCmdPool);
        beginCmd(pCurrentCmd);

        RenderTargetBarrier rtBarrier = { pRenderTarget,
                                          RESOURCE_STATE_PRESENT,
                                          RESOURCE_STATE_RENDER_TARGET };
        cmdResourceBarrier(pCurrentCmd, 0, NULL, 0, NULL, 1, &rtBarrier);

        BindRenderTargetsDesc bindRTs = {};
        bindRTs.mRenderTargetCount = 1;
        bindRTs.mRenderTarget[0] = { pRenderTarget,
                                     LOAD_ACTION_CLEAR,
                                     STORE_ACTION_STORE,
                                     { 0.1f, 0.1f, 0.1f, 1.0f } };
        cmdBindRenderTargets(pCurrentCmd, &bindRTs);
        cmdSetViewport(pCurrentCmd,
                       0.0f,
                       0.0f,
                       (float) pRenderTarget->mWidth,
                       (float) pRenderTarget->mHeight,
                       0.0f,
                       1.0f);

        cmdSetScissor(pCurrentCmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

        cmdBindPipeline(pCurrentCmd, isWireframe ? pWireframePipeline : pTrianglePipeline);
        cmdBindVertexBuffer(pCurrentCmd, 1, &pTriangleVertexBuffer);

        for (uint32_t i = 0; i < gObjectCount; ++i) {
            BufferUpdateDesc cbUpdateDesc = { pConstantBuffer[i][gFrameIndex] };
            beginUpdateResource(pRenderer, &cbUpdateDesc);
            memcpy(cbUpdateDesc.pMappedData, &gObjectConstants[i], sizeof(ObjectConstants));
            endUpdateResource(pRenderer, &cbUpdateDesc);

            DescriptorData data = {};
            data.mIndex = 0;
            data.mCount = 1;
            data.ppBuffers = &pConstantBuffer[i][gFrameIndex];
            updateDescriptorSet(pRenderer, gFrameIndex, pDescriptorSet[i], 1, &data);
            cmdBindDescriptorSet(pCurrentCmd, pDescriptorSet[i], gFrameIndex);
            cmdDraw(pCurrentCmd, 3, 0);
        }

        rtBarrier = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
        cmdResourceBarrier(pCurrentCmd, 0, NULL, 0, NULL, 1, &rtBarrier);

        endCmd(pCurrentCmd);

        QueueSubmitDesc submitDesc = { &pCurrentCmd, elem.pFence, 1, false };
        queueSubmit(pQueue, &submitDesc);

        QueuePresentDesc presentDesc = { pSwapChain, swapChainImageIndex, false };
        queuePresent(pQueue, &presentDesc);

        gFrameIndex = (gFrameIndex + 1) % gFrameCount;
    }

    const char* GetName() override { return mSettings.pTitle; }

    bool addSwapChain() {
        if (!pWindow)
            return false;

        // Correctly call waitQueueIdle with a valid fence from the command ring
        waitQueueIdle(pQueue, gCmdRing.pFences[0][0]);

        SwapChainDesc swapChainDesc = {};
        swapChainDesc.mWindowHandle = pWindow->handle;
        swapChainDesc.mWidth = pWindow->width;
        swapChainDesc.mHeight = pWindow->height;
        swapChainDesc.ppPresentQueues = &pQueue;
        swapChainDesc.mImageCount = gFrameCount;
        swapChainDesc.mSampleCount = SAMPLE_COUNT_1;
        swapChainDesc.mColorFormat = IMAGE_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.mEnableVsync = true;
        initSwapChain(pRenderer, &swapChainDesc, &pSwapChain);
        return pSwapChain != NULL;
    }
};

DEFINE_APPLICATION_MAIN(Example)
