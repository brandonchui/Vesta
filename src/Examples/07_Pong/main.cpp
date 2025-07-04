#include "Common/IApp.h"
#include "Common/IGraphics.h"
#include "Common/RingBuffer.hpp"
#include "Common/Util/Logger.h"
#include "Common/Util/Time.h"
#include "VTMath.h"

#include <cmath>
#include <cstdlib>

const float PADDLE_SPEED = 8.0f;
const float BALL_SPEED_X = 10.0f;
const float BALL_SPEED_Y = 8.0f;
const float GAME_WIDTH = 20.0f;
const float GAME_HEIGHT = 10.0f;
const float WALL_THICKNESS = 0.5f;

// game objects
struct Paddle {
    Vector2 pos;
    Vector2 size;
};
struct Ball {
    Vector2 pos;
    Vector2 size;
    Vector2 velocity;
};
struct Wall {
    Vector2 pos;
    Vector2 size;
};

const uint32_t gObjectCount = 5;
Paddle gPlayerPaddle;
Ball gBall;
Wall gTopWall, gBottomWall, gRightWall;

inline uint64_t AlignUp(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

struct ObjectConstants {
    Matrix4x4 worldMatrix;
    Matrix4x4 viewProjMatrix;
    Vector4 color;
    uint32_t useObjectColor;
    Vector3 padding;
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

PipelineLayout* pPipelineLayout = NULL;
Pipeline* pSolidPipeline = NULL;
Pipeline* pWireframePipeline = NULL;
Buffer* pQuadVertexBuffer = NULL;

GpuCmdRing gCmdRing = {};

struct VertexPosColor {
    Vector3 pos;
    Vector4 color;
};

VertexPosColor gQuadVertices[] = { { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
                                   { { 0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
                                   { { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
                                   { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
                                   { { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
                                   { { -0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } } };

//////////////////////////////////////////////
// Application
class Example : public IApp {
  public:
    bool mIsBallMoving = false;
    float mBallWaitTimer = 1.0f;

    void addPipelines() {
        VertexLayout vertexLayout = {};
        vertexLayout.mAttribCount = 2;
        VertexAttrib attribs[2] = {
            { "POSITION", IMAGE_FORMAT_R32G32B32_FLOAT, 0, 0, 0 },
            { "COLOR", IMAGE_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(Vector3), 1 }
        };
        vertexLayout.pAttribs = attribs;

        Shader* pShader = NULL;
        addShader(pRenderer, "Shaders/SimplePong.hlsl", &pShader);
        if (!pShader) {
            VT_ERROR("Failed to create shader.");
            return;
        }

        RasterizerStateDesc solidRasterizerStateDesc = { CULL_MODE_NONE, FILL_MODE_SOLID };
        GraphicsPipelineDesc pipelineDesc = {};
        pipelineDesc.pPipelineLayout = pPipelineLayout;
        pipelineDesc.pShaderProgram = pShader;
        pipelineDesc.pRasterizerState = &solidRasterizerStateDesc;
        pipelineDesc.pVertexLayout = &vertexLayout;
        pipelineDesc.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineDesc.mRenderTargetCount = 1;
        ImageFormat colorFormat = IMAGE_FORMAT_R8G8B8A8_UNORM;
        pipelineDesc.pColorFormats = &colorFormat;
        pipelineDesc.mDepthStencilFormat = IMAGE_FORMAT_UNDEFINED;
        pipelineDesc.mSampleCount = SAMPLE_COUNT_1;
        addPipeline(pRenderer, &pipelineDesc, &pSolidPipeline);

        RasterizerStateDesc wireframeRasterizerStateDesc = { CULL_MODE_NONE, FILL_MODE_WIREFRAME };
        pipelineDesc.pRasterizerState = &wireframeRasterizerStateDesc;
        addPipeline(pRenderer, &pipelineDesc, &pWireframePipeline);
    }

    bool Init() override {
        mSettings.pTitle = "07_Pong";
        srand(static_cast<unsigned int>(Time::GetTotalTime()));

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
        initPipelineLayout(pRenderer, &pipelineLayoutDesc, &pPipelineLayout);

        addPipelines();

        BufferLoadDesc vbLoadDesc = {};
        vbLoadDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        vbLoadDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        vbLoadDesc.mDesc.mSize = sizeof(gQuadVertices);
        vbLoadDesc.mDesc.mStructStride = sizeof(VertexPosColor);
        vbLoadDesc.pData = gQuadVertices;
        vbLoadDesc.ppBuffer = &pQuadVertexBuffer;
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
        setDesc.pPipelineLayout = pPipelineLayout;
        setDesc.mIndex = 0;
        setDesc.mMaxSets = gFrameCount;
        setDesc.mDescriptorCount = 1;
        setDesc.pDescriptors = gUniformDescriptorLayout;
        for (uint32_t i = 0; i < gObjectCount; ++i) {
            addDescriptorSet(pRenderer, &setDesc, &pDescriptorSet[i]);
        }

        gViewMatrix =
            Matrix4x4::CreateLookAt(Vector3(0.0f, 0.0f, -1.0f), Vector3(0, 0, 0), Vector3(0, 1, 0));
        gProjectionMatrix = Matrix4x4::CreateOrthographicOffCenter(-GAME_WIDTH / 2.0f,
                                                                   GAME_WIDTH / 2.0f,
                                                                   -GAME_HEIGHT / 2.0f,
                                                                   GAME_HEIGHT / 2.0f,
                                                                   0.1f,
                                                                   100.0f);

        // init objects
        for (uint32_t i = 0; i < gObjectCount; ++i) {
            gObjectConstants[i].useObjectColor = 0;
        }
        gObjectConstants[1].useObjectColor = 1;

        gPlayerPaddle.pos = { -GAME_WIDTH / 2.0f + 1.0f, 0.0f };
        gPlayerPaddle.size = { 1.0f / 3.5, 5.0f / 3 };
        gBall.pos = { 0.0f, 0.0f };
        gBall.size = { 0.35f, 0.35f };
        gBall.velocity = { 0.0f, 0.0f };
        gTopWall.pos = { 0.0f, GAME_HEIGHT / 2.0f };
        gTopWall.size = { GAME_WIDTH, WALL_THICKNESS };
        gBottomWall.pos = { 0.0f, -GAME_HEIGHT / 2.0f };
        gBottomWall.size = { GAME_WIDTH, WALL_THICKNESS };
        gRightWall.pos = { GAME_WIDTH / 2.0f, 0.0f };
        gRightWall.size = { WALL_THICKNESS, GAME_HEIGHT + WALL_THICKNESS };

        if (!pInput)
            return false;
        InitLogging(GetName());
        VT_INFO("App Initialized.");
        return true;
    }

    void ShutDown() override {
        waitQueueIdle(pQueue, gCmdRing.pFences[0][0]);
        exitGpuCmdRing(pRenderer, &gCmdRing);

        for (uint32_t i = 0; i < gObjectCount; ++i) {
            removeDescriptorSet(pRenderer, pDescriptorSet[i]);
        }
    }

    void Update(float deltaTime) override {
        if (pInput->WasPressed(Key::F)) {
            isWireframe = !isWireframe;
        }
        if (pInput->IsDown(Key::W)) {
            gPlayerPaddle.pos.y += PADDLE_SPEED * deltaTime;
        }
        if (pInput->IsDown(Key::S)) {
            gPlayerPaddle.pos.y -= PADDLE_SPEED * deltaTime;
        }

        float paddleHalfHeight = gPlayerPaddle.size.y / 2.0f;
        if (gPlayerPaddle.pos.y + paddleHalfHeight > GAME_HEIGHT / 2.0f) {
            gPlayerPaddle.pos.y = GAME_HEIGHT / 2.0f - paddleHalfHeight;
        }
        if (gPlayerPaddle.pos.y - paddleHalfHeight < -GAME_HEIGHT / 2.0f) {
            gPlayerPaddle.pos.y = -GAME_HEIGHT / 2.0f + paddleHalfHeight;
        }

        if (mIsBallMoving) {
            gObjectConstants[1].color = { 1.0f, 1.0f, 1.0f, 1.0f };
            gBall.pos.x += gBall.velocity.x * deltaTime;
            gBall.pos.y += gBall.velocity.y * deltaTime;

            float ballHalfWidth = gBall.size.x / 2.0f;
            float ballHalfHeight = gBall.size.y / 2.0f;

            if (gBall.velocity.x < 0.0f) {
                float paddleHalfWidth = gPlayerPaddle.size.x / 2.0f;
                if (gBall.pos.x - ballHalfWidth < gPlayerPaddle.pos.x + paddleHalfWidth &&
                    gBall.pos.x + ballHalfWidth > gPlayerPaddle.pos.x - paddleHalfWidth &&
                    gBall.pos.y - ballHalfHeight < gPlayerPaddle.pos.y + paddleHalfHeight &&
                    gBall.pos.y + ballHalfHeight > gPlayerPaddle.pos.y - paddleHalfHeight) {
                    gBall.velocity.x *= -1;
                    gBall.pos.x = gPlayerPaddle.pos.x + paddleHalfWidth + ballHalfWidth;
                }
            }

            if ((gBall.pos.y + ballHalfHeight) > (GAME_HEIGHT / 2.0f - WALL_THICKNESS / 2.0f)) {
                gBall.velocity.y *= -1;
                gBall.pos.y = GAME_HEIGHT / 2.0f - WALL_THICKNESS / 2.0f - ballHalfHeight;
            } else if ((gBall.pos.y - ballHalfHeight) <
                       (-GAME_HEIGHT / 2.0f + WALL_THICKNESS / 2.0f)) {
                gBall.velocity.y *= -1;
                gBall.pos.y = -GAME_HEIGHT / 2.0f + WALL_THICKNESS / 2.0f + ballHalfHeight;
            }

            if ((gBall.pos.x + ballHalfWidth) > (GAME_WIDTH / 2.0f - WALL_THICKNESS / 2.0f)) {
                gBall.velocity.x *= -1;
                gBall.pos.x = GAME_WIDTH / 2.0f - WALL_THICKNESS / 2.0f - ballHalfWidth;
            }

            if (gBall.pos.x < -GAME_WIDTH / 2.0f - ballHalfWidth) {
                gBall.pos = { 0.0f, 0.0f };
                gBall.velocity = { 0.0f, 0.0f };
                mIsBallMoving = false;
                mBallWaitTimer = 1.0f;
            }
        } else {
            float blink = fmod(Time::GetTotalTime(), 0.2f);
            if (blink > 0.1f) {
                gObjectConstants[1].color = { 1.0f, 1.0f, 1.0f, 1.0f };
            } else {
                gObjectConstants[1].color = { 0.1f, 0.1f, 0.1f, 1.0f };
            }

            mBallWaitTimer -= deltaTime;
            if (mBallWaitTimer <= 0.0f) {
                mIsBallMoving = true;
                float randomY =
                    -1.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 2.0f));
                int randomXDir = (rand() % 2) == 0 ? -1 : 1;
                gBall.velocity.x = randomXDir * BALL_SPEED_X;
                gBall.velocity.y = randomY * BALL_SPEED_Y;
            }
        }
    }

    void DrawRect(Cmd* pCurrentCmd, uint32_t objectIndex, const Vector2& pos, const Vector2& size) {
        Matrix4x4 scale = Matrix4x4::CreateScale(size.x, size.y, 1.0f);
        Matrix4x4 translation = Matrix4x4::CreateTranslation(pos.x, pos.y, 0.0f);
        Matrix4x4 world = scale * translation;
        world.Transpose();

        Matrix4x4 vp = gViewMatrix * gProjectionMatrix;
        vp.Transpose();

        gObjectConstants[objectIndex].worldMatrix = world;
        gObjectConstants[objectIndex].viewProjMatrix = vp;

        BufferUpdateDesc cbUpdateDesc = { pConstantBuffer[objectIndex][gFrameIndex] };
        beginUpdateResource(pRenderer, &cbUpdateDesc);
        memcpy(cbUpdateDesc.pMappedData, &gObjectConstants[objectIndex], sizeof(ObjectConstants));
        endUpdateResource(pRenderer, &cbUpdateDesc);

        DescriptorData data = {};
        data.mIndex = 0;
        data.mCount = 1;
        data.ppBuffers = &pConstantBuffer[objectIndex][gFrameIndex];
        updateDescriptorSet(pRenderer, gFrameIndex, pDescriptorSet[objectIndex], 1, &data);
        cmdBindDescriptorSet(pCurrentCmd, pDescriptorSet[objectIndex], gFrameIndex);

        cmdDraw(pCurrentCmd, 6, 0);
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

        cmdBindPipeline(pCurrentCmd, isWireframe ? pWireframePipeline : pSolidPipeline);
        cmdBindVertexBuffer(pCurrentCmd, 1, &pQuadVertexBuffer);

        DrawRect(pCurrentCmd, 0, gPlayerPaddle.pos, gPlayerPaddle.size);
        DrawRect(pCurrentCmd, 1, gBall.pos, gBall.size);
        DrawRect(pCurrentCmd, 2, gTopWall.pos, gTopWall.size);
        DrawRect(pCurrentCmd, 3, gBottomWall.pos, gBottomWall.size);
        DrawRect(pCurrentCmd, 4, gRightWall.pos, gRightWall.size);

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
