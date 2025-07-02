#include "Common/IApp.h"
#include "Common/IGraphics.h"
#include "Common/Util/Logger.h"
#include "VTMath.h"

// TODO move
inline uint64_t AlignUp(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

struct ObjectConstants {
    Matrix4x4 worldMatrix;
    Matrix4x4 viewProjMatrix;
};

const uint32_t gFrameCount = 2;
Buffer* pConstantBuffer[gFrameCount] = { nullptr };
DescriptorSet* pDescriptorSet = { NULL };
Descriptor gUniformDescriptorLayout[] = { { DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 0 } };
ObjectConstants gObjectConstants;
uint32_t gFrameIndex = 0;

Matrix4x4 gViewMatrix;
Matrix4x4 gProjectionMatrix;

Vector3 gCameraPosition = Vector3(0.0f, 0.0f, -3.0f);
Vector3 gCameraFront = Vector3(0.0f, 0.0f, 1.0f);
Vector3 gCameraUp = Vector3(0.0f, 1.0f, 0.0f);

bool isWireframe = false;
Renderer* pRenderer = NULL;
Queue* pQueue = NULL;
SwapChain* pSwapChain = NULL;
Fence* pFrameFences[gFrameCount] = { nullptr };
CmdPool* pCmdPools[gFrameCount] = { nullptr };
Cmd* pCmds[gFrameCount] = { nullptr };
PipelineLayout* pTrianglePipelineLayout = NULL;
Pipeline* pTrianglePipeline = NULL;
Pipeline* pWireframePipeline = NULL;
Buffer* pTriangleVertexBuffer = NULL;
Buffer* pQuadVertexBuffer = NULL;
enum Shape { SHAPE_TRIANGLE = 0, SHAPE_QUAD, SHAPE_COUNT };
Shape gCurrentShape = SHAPE_TRIANGLE;

struct VertexPosColor {
    Vector3 pos;
    Vector4 color;
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
        mSettings.pTitle = "03_MovableObject";

        RendererDesc settings = {};
        initRenderer(mSettings.pTitle, &settings, &pRenderer);
        if (!pRenderer)
            return false;

        QueueDesc queueDesc = {};
        initQueue(pRenderer, &queueDesc, &pQueue);
        if (!pQueue)
            return false;

        for (uint32_t i = 0; i < gFrameCount; ++i) {
            initFence(pRenderer, &pFrameFences[i]);
            CmdPoolDesc cmdPoolDesc = { pQueue };
            initCmdPool(pRenderer, &cmdPoolDesc, &pCmdPools[i]);
            CmdDesc cmdDesc = { pCmdPools[i] };
            initCmd(pRenderer, &cmdDesc, &pCmds[i]);
        }

        PipelineLayoutDesc pipelineLayoutDesc = {};
        /*
        DescriptorRangeDesc range = { 0, 1, DESCRIPTOR_TYPE_UNIFORM_BUFFER };
        RootParameter rootParam = {};
        rootParam.type = RESOURCE_TYPE_DESCRIPTOR_TABLE;
        rootParam.shaderVisibility = SHADER_STAGE_VERTEX;
        rootParam.descriptorTable.rangeCount = 1;
        rootParam.descriptorTable.pRanges = &range;
        pipelineLayoutDesc.parameterCount = 1;
        pipelineLayoutDesc.pParameters = &rootParam;
        */
        pipelineLayoutDesc.pShaderFileName = "Shaders/SimpleMovable.hlsl";
        initPipelineLayout(pRenderer, &pipelineLayoutDesc, &pTrianglePipelineLayout);

        addPipelines();

        BufferLoadDesc vbLoadDesc = {};
        vbLoadDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        vbLoadDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        vbLoadDesc.mDesc.mSize = sizeof(gTriangleVertices);
        vbLoadDesc.pData = gTriangleVertices;
        vbLoadDesc.mDesc.mStructStride = sizeof(VertexPosColor);
        vbLoadDesc.ppBuffer = &pTriangleVertexBuffer;
        addResource(pRenderer, &vbLoadDesc);

        vbLoadDesc.mDesc.mSize = sizeof(gQuadVertices);
        vbLoadDesc.pData = gQuadVertices;
        vbLoadDesc.ppBuffer = &pQuadVertexBuffer;
        addResource(pRenderer, &vbLoadDesc);

        for (uint32_t i = 0; i < gFrameCount; ++i) {
            BufferDesc cbDesc = {};
            cbDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            cbDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
            cbDesc.mFlags = BUFFER_CREATIONFLAG_PERSISTENT;
            cbDesc.mSize = AlignUp(sizeof(ObjectConstants), 256);
            cbDesc.pName = "Constant Buffer";
            BufferLoadDesc cbLoadDesc = { &pConstantBuffer[i], nullptr, cbDesc };
            addResource(pRenderer, &cbLoadDesc);
        }

        DescriptorSetDesc setDesc = {};
        setDesc.pPipelineLayout = pTrianglePipelineLayout;
        setDesc.mIndex = 0;
        setDesc.mMaxSets = gFrameCount;
        setDesc.mDescriptorCount = 1;
        setDesc.pDescriptors = gUniformDescriptorLayout;
        addDescriptorSet(pRenderer, &setDesc, &pDescriptorSet);

        gViewMatrix = Matrix4x4::CreateLookAt(gCameraPosition, Vector3(0, 0, 0), Vector3(0, 1, 0));
        gProjectionMatrix = Matrix4x4::CreatePerspectiveFieldOfView(ToRadians(60.0f),
                                                                    (float) mSettings.mWidth /
                                                                        (float) mSettings.mHeight,
                                                                    0.1f,
                                                                    100.0f);

        if (!pInput)
            return false;
        InitLogging(GetName());
        return true;
    }

    void ShutDown() override {
        waitQueueIdle(pQueue, pFrameFences[0]);

        removeDescriptorSet(pRenderer, pDescriptorSet);
    }

    void Update(float deltaTime) override {
        if (pInput->WasPressed(Key::F))
            isWireframe = !isWireframe;
        if (pInput->WasPressed(Key::Space))
            gCurrentShape = (Shape) ((gCurrentShape + 1) % SHAPE_COUNT);

        const float moveSpeed = 4.0f * deltaTime;

        if (pInput->IsDown(Key::W))
            gCameraPosition = gCameraPosition - gCameraUp * moveSpeed;
        if (pInput->IsDown(Key::S))
            gCameraPosition = gCameraPosition + gCameraUp * moveSpeed;

        Vector3 rightVector = Vector3::Cross(gCameraFront, gCameraUp).Normalized();
        if (pInput->IsDown(Key::A))
            gCameraPosition = gCameraPosition - rightVector * moveSpeed;
        if (pInput->IsDown(Key::D))
            gCameraPosition = gCameraPosition + rightVector * moveSpeed;

        gViewMatrix =
            Matrix4x4::CreateLookAt(gCameraPosition, gCameraPosition + gCameraFront, gCameraUp);

        gObjectConstants.worldMatrix = IdentityMatrix();
        Matrix4x4 vp = gViewMatrix * gProjectionMatrix;
        vp.Transpose();
        gObjectConstants.viewProjMatrix = vp;
    }

    void Draw() override {
        if (!pSwapChain) {
            if (!addSwapChain())
                return;
        }

        uint32_t swapChainImageIndex;
        acquireNextImage(pRenderer, pSwapChain, NULL, &swapChainImageIndex);
        RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapChainImageIndex];

        CmdPool* pCurrentCmdPool = pCmdPools[gFrameIndex];
        Cmd* pCurrentCmd = pCmds[gFrameIndex];
        Fence* pCurrentFence = pFrameFences[gFrameIndex];

        waitFence(pRenderer, pCurrentFence);
        resetCmdPool(pRenderer, pCurrentCmdPool);
        beginCmd(pCurrentCmd);

        BufferUpdateDesc cbUpdateDesc = { pConstantBuffer[gFrameIndex] };
        beginUpdateResource(pRenderer, &cbUpdateDesc);
        memcpy(cbUpdateDesc.pMappedData, &gObjectConstants, sizeof(gObjectConstants));
        endUpdateResource(pRenderer, &cbUpdateDesc);

        DescriptorData data = {};
        data.mIndex = 0;
        data.mCount = 1;
        data.ppBuffers = &pConstantBuffer[gFrameIndex];
        updateDescriptorSet(pRenderer, gFrameIndex, pDescriptorSet, 1, &data);

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
        cmdBindDescriptorSet(pCurrentCmd, pDescriptorSet, gFrameIndex);

        if (gCurrentShape == SHAPE_TRIANGLE) {
            cmdBindVertexBuffer(pCurrentCmd, 1, &pTriangleVertexBuffer);
            cmdDraw(pCurrentCmd, 3, 0);
        } else {
            cmdBindVertexBuffer(pCurrentCmd, 1, &pQuadVertexBuffer);
            cmdDraw(pCurrentCmd, 6, 0);
        }

        rtBarrier = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
        cmdResourceBarrier(pCurrentCmd, 0, NULL, 0, NULL, 1, &rtBarrier);

        endCmd(pCurrentCmd);

        QueueSubmitDesc submitDesc = { &pCurrentCmd, pCurrentFence, 1, false };
        queueSubmit(pQueue, &submitDesc);

        QueuePresentDesc presentDesc = { pSwapChain, swapChainImageIndex, false };
        queuePresent(pQueue, &presentDesc);

        gFrameIndex = (gFrameIndex + 1) % gFrameCount;
    }

    const char* GetName() override { return mSettings.pTitle; }

    bool addSwapChain() {
        if (!pWindow)
            return false;
        waitQueueIdle(pQueue, pFrameFences[gFrameIndex]);

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
