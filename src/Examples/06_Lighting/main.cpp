#include "Common/IApp.h"
#include "Common/IGraphics.h"
#include "Common/Util/CameraController.h"
#include "Common/Util/Logger.h"
#include "Common/Util/Time.h"
#include "VTMath.h"

inline uint64_t AlignUp(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

struct ObjectConstants {
    Matrix4x4 worldMatrix;
    Matrix4x4 viewProjMatrix;
};

struct SceneConstants {
    Vector3 lightDirection;
    float _pad0;
    Vector4 lightColor;
    Vector3 cameraPosition;
    float _pad1;
};

const uint32_t gFrameCount = 2;
Buffer* pConstantBuffer[gFrameCount] = { nullptr };
Buffer* pSceneConstantBuffer[gFrameCount] = { nullptr };

DescriptorSet* pObjectDescriptorSet = { NULL };
DescriptorSet* pSceneDescriptorSet = { NULL };

Descriptor gObjectUniformDescriptorLayout[] = { { DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 0 } };
Descriptor gSceneUniformDescriptorLayout[] = { { DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 0 } };

ObjectConstants gObjectConstants;
SceneConstants gSceneConstants;
uint32_t gFrameIndex = 0;
RenderTarget* pDepthBuffer = NULL;

Matrix4x4 gViewMatrix;
Matrix4x4 gProjectionMatrix;
ICameraController* pCameraController = nullptr;

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
Buffer* pCubeVertexBuffer = NULL;
DepthStateDesc gDepthStateDesc = {};

struct VertexPosNormalColor {
    Vector3 pos;
    Vector3 normal;
    Vector4 color;
};

VertexPosNormalColor gCubeVertices[] = {
    // Front face
    { { -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    // Back face
    { { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    // Left face
    { { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, 0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, -0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    // Right face
    { { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    // Top face
    { { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    // Bottom face
    { { -0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
    { { -0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.8f, 0.8f, 0.8f, 1.0f } },
};

//////////////////////////////////////////////
// Application
class Example : public IApp {
  public:
    void addPipelines() {
        VertexLayout vertexLayout = {};
        vertexLayout.mAttribCount = 3;
        VertexAttrib attribs[3] = {
            { "POSITION", IMAGE_FORMAT_R32G32B32_FLOAT, 0, 0, 0 },
            { "NORMAL", IMAGE_FORMAT_R32G32B32_FLOAT, 0, sizeof(Vector3), 0 },
            { "COLOR", IMAGE_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(Vector3) * 2, 0 }
        };
        vertexLayout.pAttribs = attribs;

        Shader* pTriangleShader = NULL;
        addShader(pRenderer, "Shaders/Light.hlsl", &pTriangleShader);
        if (!pTriangleShader) {
            VT_ERROR("Failed to create shader.");
            return;
        }

        gDepthStateDesc.mDepthTest = true;
        gDepthStateDesc.mDepthWrite = true;
        gDepthStateDesc.mDepthFunc = COMPARE_MODE_LESS;

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
        pipelineDesc.mDepthStencilFormat = IMAGE_FORMAT_D32_FLOAT;
        pipelineDesc.pDepthState = &gDepthStateDesc;
        pipelineDesc.mSampleCount = SAMPLE_COUNT_1;
        addPipeline(pRenderer, &pipelineDesc, &pTrianglePipeline);

        RasterizerStateDesc wireframeRasterizerStateDesc = { CULL_MODE_NONE, FILL_MODE_WIREFRAME };
        pipelineDesc.pRasterizerState = &wireframeRasterizerStateDesc;
        addPipeline(pRenderer, &pipelineDesc, &pWireframePipeline);
    }

    bool Init() override {
        mSettings.pTitle = "06_Lighting";

        Time::Initialize();

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
        pipelineLayoutDesc.pShaderFileName = "Shaders/Light.hlsl";
        initPipelineLayout(pRenderer, &pipelineLayoutDesc, &pTrianglePipelineLayout);

        addPipelines();

        BufferLoadDesc vbLoadDesc = {};
        vbLoadDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        vbLoadDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        vbLoadDesc.mDesc.mStructStride = sizeof(VertexPosNormalColor);
        vbLoadDesc.mDesc.mSize = sizeof(gCubeVertices);
        vbLoadDesc.pData = gCubeVertices;
        vbLoadDesc.ppBuffer = &pCubeVertexBuffer;
        addResource(pRenderer, &vbLoadDesc);

        for (uint32_t i = 0; i < gFrameCount; ++i) {
            BufferDesc cbDesc = {};
            cbDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            cbDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
            cbDesc.mFlags = BUFFER_CREATIONFLAG_PERSISTENT;
            cbDesc.mSize = AlignUp(sizeof(ObjectConstants), 256);
            cbDesc.pName = "Object Constant Buffer";
            BufferLoadDesc cbLoadDesc = { &pConstantBuffer[i], nullptr, cbDesc };
            addResource(pRenderer, &cbLoadDesc);

            cbDesc.mSize = AlignUp(sizeof(SceneConstants), 256);
            cbDesc.pName = "Scene Constant Buffer";
            BufferLoadDesc sceneCbLoadDesc = { &pSceneConstantBuffer[i], nullptr, cbDesc };
            addResource(pRenderer, &sceneCbLoadDesc);
        }

        DescriptorSetDesc setDesc = {};

        setDesc.pPipelineLayout = pTrianglePipelineLayout;
        setDesc.mMaxSets = gFrameCount;
        setDesc.mDescriptorCount = 1;

        setDesc.mIndex = 0;
        setDesc.pDescriptors = gObjectUniformDescriptorLayout;
        addDescriptorSet(pRenderer, &setDesc, &pObjectDescriptorSet);

        setDesc.mIndex = 1;
        setDesc.pDescriptors = gSceneUniformDescriptorLayout;
        addDescriptorSet(pRenderer, &setDesc, &pSceneDescriptorSet);

        pCameraController = new OrbitCameraController(Vector3(0.0f, 0.0f, 0.0f), 5.0f);

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
        delete pCameraController;

        removeDescriptorSet(pRenderer, pObjectDescriptorSet);
        removeDescriptorSet(pRenderer, pSceneDescriptorSet);
    }

    void Update(float deltaTime) override {
        if (pInput->WasPressed(Key::F))
            isWireframe = !isWireframe;

        pCameraController->Update(pInput, deltaTime);
        gViewMatrix = pCameraController->GetViewMatrix();

        gObjectConstants.worldMatrix = IdentityMatrix();
        Matrix4x4 vp = gViewMatrix * gProjectionMatrix;
        vp.Transpose();
        gObjectConstants.viewProjMatrix = vp;

        // rotate light
        Vector3 baseLightDir = Vector3(0.7f, -1.0f, 0.0f);
        baseLightDir.Normalize();

        float rotationAngle = Time::GetTotalTime() * 0.8f;
        Matrix4x4 lightRotation = Matrix4x4::CreateRotationY(rotationAngle);

        gSceneConstants.lightDirection = Vector3::TransformVector(baseLightDir, lightRotation);

        gSceneConstants.lightColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        gSceneConstants.cameraPosition = pCameraController->GetPosition();
    }

    void Draw() override {
        if (!pSwapChain) {
            if (!addSwapChain())
                return;
        }

        if (!pDepthBuffer) {
            if (!addDepthBuffer())
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

        BufferUpdateDesc sceneCbUpdateDesc = { pSceneConstantBuffer[gFrameIndex] };
        beginUpdateResource(pRenderer, &sceneCbUpdateDesc);
        memcpy(sceneCbUpdateDesc.pMappedData, &gSceneConstants, sizeof(gSceneConstants));
        endUpdateResource(pRenderer, &sceneCbUpdateDesc);

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

        bindRTs.mDepthStencil = { pDepthBuffer,
                                  LOAD_ACTION_CLEAR,
                                  STORE_ACTION_STORE,
                                  { .depth = 1.0f, .stencil = 0 } };

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

        DescriptorData objectData = {};
        objectData.mIndex = 0;
        objectData.mCount = 1;
        objectData.ppBuffers = &pConstantBuffer[gFrameIndex];
        updateDescriptorSet(pRenderer, gFrameIndex, pObjectDescriptorSet, 1, &objectData);

        DescriptorData sceneData = {};
        sceneData.mIndex = 0;
        sceneData.mCount = 1;
        sceneData.ppBuffers = &pSceneConstantBuffer[gFrameIndex];
        updateDescriptorSet(pRenderer, gFrameIndex, pSceneDescriptorSet, 1, &sceneData);

        cmdBindDescriptorSet(pCurrentCmd, pObjectDescriptorSet, gFrameIndex);
        cmdBindDescriptorSet(pCurrentCmd, pSceneDescriptorSet, gFrameIndex);

        cmdBindVertexBuffer(pCurrentCmd, 1, &pCubeVertexBuffer);
        cmdDraw(pCurrentCmd, 36, 0);

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

    bool addDepthBuffer() {
        RenderTargetDesc depthRT = {};
        depthRT.mWidth = pWindow->width;
        depthRT.mHeight = pWindow->height;
        depthRT.mFormat = IMAGE_FORMAT_D32_FLOAT;
        depthRT.mStartState = RESOURCE_STATE_DEPTH_WRITE;
        depthRT.mClearValue.depth = 1.0f;
        depthRT.mClearValue.stencil = 0;
        depthRT.mSampleCount = SAMPLE_COUNT_1;
        depthRT.pName = "Depth Buffer";
        addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);

        if (!pDepthBuffer)
            VT_ERROR("NO DEPTH BUFFER");

        return pDepthBuffer != NULL;
    }
};

DEFINE_APPLICATION_MAIN(Example)
