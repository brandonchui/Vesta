#pragma once

#include "IGraphics.h"

typedef struct GPURingBuffer {
    Renderer* pRenderer;
    Buffer* pBuffer;

    uint32_t mBufferAlignment;
    uint64_t mMaxBufferSize;
    uint64_t mCurrentBufferOffset;
} GPURingBuffer;

typedef struct GPURingBufferOffset {
    Buffer* pBuffer;
    uint64_t mOffset;
} GPURingBufferOffset;

#ifndef MAX_GPU_CMD_POOLS_PER_RING
#define MAX_GPU_CMD_POOLS_PER_RING 64u
#endif
#ifndef MAX_GPU_CMDS_PER_POOL
#define MAX_GPU_CMDS_PER_POOL 4u
#endif

typedef struct GpuCmdRingDesc {
    Queue* pQueue;
    uint32_t mPoolCount;
    uint32_t mCmdPerPoolCount;
} GpuCmdRingDesc;

// Contains the resources for the ring buffer, like the things you actually need to render
typedef struct GpuCmdRingElement {
    CmdPool* pCmdPool;
    Cmd** pCmds;
    Fence* pFence;
} GpuCmdRingElement;

// Ring buffer containg the resources
typedef struct GpuCmdRing {
    CmdPool* pCmdPools[MAX_GPU_CMD_POOLS_PER_RING];
    Cmd* pCmds[MAX_GPU_CMD_POOLS_PER_RING][MAX_GPU_CMDS_PER_POOL];
    Fence* pFences[MAX_GPU_CMD_POOLS_PER_RING][MAX_GPU_CMDS_PER_POOL];
    uint32_t mPoolIndex;
    uint32_t mCmdIndex;
    uint32_t mFenceIndex;
    uint32_t mPoolCount;
    uint32_t mCmdPerPoolCount;
} GpuCmdRing;

static inline void addUniformGPURingBuffer(Renderer* pRenderer,
                                           uint32_t requiredUniformBufferSize,
                                           GPURingBuffer* pRingBuffer) {
    *pRingBuffer = {};
    pRingBuffer->pRenderer = pRenderer;

    // NOTE hard coded for now, might differ for other api
    const uint32_t uniformBufferAlignment = 256;

    const uint32_t maxUniformBufferSize = requiredUniformBufferSize;
    pRingBuffer->mBufferAlignment = uniformBufferAlignment;
    pRingBuffer->mMaxBufferSize = maxUniformBufferSize;

    BufferDesc ubDesc = {};
    ubDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    ubDesc.mFlags = BUFFER_CREATIONFLAG_PERSISTENT;
    ubDesc.mSize = maxUniformBufferSize;
    ubDesc.pName = "UniformRingBuffer";

    BufferLoadDesc loadDesc = {};
    loadDesc.mDesc = ubDesc;
    loadDesc.ppBuffer = &pRingBuffer->pBuffer;

    addResource(pRenderer, &loadDesc);
}

static inline void removeGPURingBuffer(GPURingBuffer* pRingBuffer) {
    // TODO implement
}

static inline void resetGPURingBuffer(GPURingBuffer* pRingBuffer) {
    pRingBuffer->mCurrentBufferOffset = 0;
}

static inline GPURingBufferOffset getGPURingBufferOffset(GPURingBuffer* pRingBuffer,
                                                         uint32_t memoryRequirement,
                                                         uint32_t alignment = 0) {
    uint32_t alignedSize = (memoryRequirement + (alignment - 1)) & ~(alignment - 1);
    if (alignment == 0) {
        alignedSize = (memoryRequirement + (pRingBuffer->mBufferAlignment - 1)) &
                      ~(pRingBuffer->mBufferAlignment - 1);
    }

    if (alignedSize > pRingBuffer->mMaxBufferSize) {
        return { NULL, 0 };
    }

    if (pRingBuffer->mCurrentBufferOffset + alignedSize >= pRingBuffer->mMaxBufferSize) {
        pRingBuffer->mCurrentBufferOffset = 0;
    }

    GPURingBufferOffset ret = { pRingBuffer->pBuffer, pRingBuffer->mCurrentBufferOffset };
    pRingBuffer->mCurrentBufferOffset += alignedSize;

    return ret;
}

static inline void
initGpuCmdRing(Renderer* pRenderer, const GpuCmdRingDesc* pDesc, GpuCmdRing* pOut) {
    pOut->mPoolCount = pDesc->mPoolCount;
    pOut->mCmdPerPoolCount = pDesc->mCmdPerPoolCount;

    CmdPoolDesc poolDesc = {};
    poolDesc.pQueue = pDesc->pQueue;

    for (uint32_t pool = 0; pool < pDesc->mPoolCount; ++pool) {
        initCmdPool(pRenderer, &poolDesc, &pOut->pCmdPools[pool]);
        CmdDesc cmdDesc = {};
        cmdDesc.pCmdPool = pOut->pCmdPools[pool];
        for (uint32_t cmd = 0; cmd < pDesc->mCmdPerPoolCount; ++cmd) {
            initCmd(pRenderer, &cmdDesc, &pOut->pCmds[pool][cmd]);
            initFence(pRenderer, &pOut->pFences[pool][cmd]);
        }
    }

    pOut->mPoolIndex = UINT32_MAX;
    pOut->mCmdIndex = UINT32_MAX;
    pOut->mFenceIndex = UINT32_MAX;
}

static inline void exitGpuCmdRing(Renderer* pRenderer, GpuCmdRing* pRing) {
    for (uint32_t pool = 0; pool < pRing->mPoolCount; ++pool) {
        for (uint32_t cmd = 0; cmd < pRing->mCmdPerPoolCount; ++cmd) {
            exitCmd(pRenderer, pRing->pCmds[pool][cmd]);
            if (pRing->pFences[pool][cmd]) {
                exitFence(pRenderer, pRing->pFences[pool][cmd]);
            }
        }
        exitCmdPool(pRenderer, pRing->pCmdPools[pool]);
    }
    *pRing = {};
}

static inline GpuCmdRingElement
getNextGpuCmdRingElement(GpuCmdRing* pRing, bool cyclePool, uint32_t cmdCount) {
    if (cyclePool) {
        pRing->mPoolIndex = (pRing->mPoolIndex + 1) % pRing->mPoolCount;
        pRing->mCmdIndex = 0;
        pRing->mFenceIndex = 0;
    }

    if (pRing->mCmdIndex + cmdCount > pRing->mCmdPerPoolCount) {
        return GpuCmdRingElement {};
    }

    GpuCmdRingElement ret = {};
    ret.pCmdPool = pRing->pCmdPools[pRing->mPoolIndex];
    ret.pCmds = &pRing->pCmds[pRing->mPoolIndex][pRing->mCmdIndex];
    ret.pFence = pRing->pFences[pRing->mPoolIndex][pRing->mFenceIndex];

    pRing->mCmdIndex += cmdCount;
    ++pRing->mFenceIndex;

    return ret;
}
