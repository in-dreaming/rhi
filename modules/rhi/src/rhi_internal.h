#ifndef RHI_INTERNAL_H
#define RHI_INTERNAL_H

#include "rhi/defs.h"
#include "rhi/rhi.h"

#include <SDL3/SDL_gpu.h>

struct RHI_Device {
    SDL_GPUDevice       *sdlDevice;
    SDL_Window          *sdlWindow;
    RHI_Queue           *graphicsQueue;
    RHI_Queue           *computeQueue;
    RHI_Queue           *transferQueue;
    RHI_ErrorCallback    errorCallback;
    void                *errorCallbackUserData;
    uint64_t             frameCounter;
    RHI_Backend          backend;
    SDL_GPUShaderFormat  shaderFormat;
    bool                 validationEnabled;
};

struct RHI_Buffer {
    SDL_GPUBuffer       *sdlBuffer;
    SDL_GPUTransferBuffer *sdlTransferBuffer;
    RHI_Device         *device;
    uint64_t            size;
    RHI_MemoryType      memoryType;
};

struct RHI_Texture {
    SDL_GPUTexture      *sdlTexture;
    RHI_TextureType      type;
    RHI_Format           fmt;
    uint32_t             width;
    uint32_t             height;
    uint32_t             depth;
    uint32_t             mipLevels;
    uint32_t             arrayLayers;
};

struct RHI_Sampler {
    SDL_GPUSampler      *sdlSampler;
};

struct RHI_ShaderModule {
    SDL_GPUShader       *sdlShader;
    RHI_ShaderStage      stage;
};

struct RHI_GraphicsPipeline {
    SDL_GPUGraphicsPipeline *sdlPipeline;
};

struct RHI_CommandPool {
    RHI_Device          *device;
    RHI_QueueType        queueType;
};

struct RHI_CommandBuffer {
    SDL_GPUCommandBuffer *sdlCB;
    SDL_GPURenderPass   *sdlRenderPass;
    SDL_GPUCopyPass     *sdlCopyPass;
    RHI_Device          *device;
    bool                 recording;
    bool                 inRenderPass;
    bool                 inCopyPass;
};

struct RHI_Swapchain {
    RHI_Device          *device;
    SDL_Window          *sdlWindow;
    RHI_Texture         *currentTexture;
    RHI_CommandBuffer   *pendingCB;
    RHI_Format           format;
    uint32_t             width;
    uint32_t             height;
};

struct RHI_Fence {
    SDL_GPUFence        *sdlFence;
};

struct RHI_Semaphore {
    RHI_Device          *device;
    uint64_t             value;
};

struct RHI_Queue {
    SDL_GPUDevice       *sdlDevice;
    RHI_QueueType        type;
};

#endif
