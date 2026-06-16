#ifndef RHI_H
#define RHI_H

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Device & Queue                                                      */
/* ------------------------------------------------------------------ */

RHI_API RHI_Device *RHI_CreateDevice(const RHI_DeviceCreateInfo *info);
RHI_API void        RHI_DestroyDevice(RHI_Device *device);
RHI_API void        RHI_DeviceWaitIdle(RHI_Device *device);

RHI_API RHI_Queue  *RHI_GetQueue(RHI_Device *device, RHI_QueueType type, uint32_t index);
RHI_API void        RHI_QueueSubmit(RHI_Queue *queue, const RHI_SubmitInfo *info, RHI_Fence *fence);
RHI_API void        RHI_QueueWaitIdle(RHI_Queue *queue);

/* ------------------------------------------------------------------ */
/* Error handling                                                      */
/* ------------------------------------------------------------------ */

RHI_API void RHI_SetErrorCallback(RHI_Device *device, RHI_ErrorCallback callback, void *userData);

/* ------------------------------------------------------------------ */
/* Swapchain                                                           */
/* ------------------------------------------------------------------ */

RHI_API RHI_Swapchain    *RHI_CreateSwapchain(RHI_Device *device, const RHI_SwapchainCreateInfo *info);
RHI_API void              RHI_DestroySwapchain(RHI_Device *device, RHI_Swapchain *swapchain);
RHI_API RHI_TextureHandle RHI_AcquireSwapchainTexture(RHI_Swapchain *swapchain);
RHI_API RHI_Texture      *RHI_GetSwapchainTexture(RHI_Swapchain *swapchain);
RHI_API void              RHI_SetSwapchainCommandBuffer(RHI_Swapchain *swapchain, RHI_CommandBuffer *cmd);
RHI_API void              RHI_Present(RHI_Queue *queue, RHI_Swapchain *swapchain);
RHI_API void              RHI_ResizeSwapchain(RHI_Swapchain *swapchain, uint32_t width, uint32_t height);
RHI_API RHI_Format        RHI_GetSwapchainFormat(RHI_Swapchain *swapchain);

/* ------------------------------------------------------------------ */
/* Buffer                                                              */
/* ------------------------------------------------------------------ */

RHI_API RHI_Buffer *RHI_CreateBuffer(RHI_Device *device, const RHI_BufferCreateInfo *info);
RHI_API void        RHI_DestroyBuffer(RHI_Device *device, RHI_Buffer *buffer);
RHI_API void       *RHI_MapBuffer(RHI_Buffer *buffer, uint64_t offset, uint64_t size);
RHI_API void        RHI_UnmapBuffer(RHI_Buffer *buffer);

/* ------------------------------------------------------------------ */
/* Texture                                                             */
/* ------------------------------------------------------------------ */

RHI_API RHI_Texture *RHI_CreateTexture(RHI_Device *device, const RHI_TextureCreateInfo *info);
RHI_API void         RHI_DestroyTexture(RHI_Device *device, RHI_Texture *texture);

/* ------------------------------------------------------------------ */
/* Sampler                                                             */
/* ------------------------------------------------------------------ */

RHI_API RHI_Sampler *RHI_CreateSampler(RHI_Device *device, const RHI_SamplerCreateInfo *info);
RHI_API void         RHI_DestroySampler(RHI_Device *device, RHI_Sampler *sampler);

/* ------------------------------------------------------------------ */
/* Shader Module                                                       */
/* ------------------------------------------------------------------ */

RHI_API RHI_ShaderModule *RHI_CreateShaderModule(RHI_Device *device, const RHI_ShaderModuleCreateInfo *info);
RHI_API void              RHI_DestroyShaderModule(RHI_Device *device, RHI_ShaderModule *module);

/* ------------------------------------------------------------------ */
/* Graphics Pipeline                                                   */
/* ------------------------------------------------------------------ */

RHI_API RHI_GraphicsPipeline *RHI_CreateGraphicsPipeline(RHI_Device *device, const RHI_GraphicsPipelineCreateInfo *info);
RHI_API void                  RHI_DestroyGraphicsPipeline(RHI_Device *device, RHI_GraphicsPipeline *pipeline);

/* ------------------------------------------------------------------ */
/* Command Pool & Buffer                                               */
/* ------------------------------------------------------------------ */

RHI_API RHI_CommandPool   *RHI_CreateCommandPool(RHI_Device *device, const RHI_CommandPoolCreateInfo *info);
RHI_API void               RHI_DestroyCommandPool(RHI_Device *device, RHI_CommandPool *pool);
RHI_API void               RHI_ResetCommandPool(RHI_CommandPool *pool);

RHI_API RHI_CommandBuffer *RHI_AllocateCommandBuffer(RHI_CommandPool *pool);
RHI_API RHI_CommandBuffer *RHI_AcquireCommandBuffer(RHI_Device *device, RHI_QueueType queue);

RHI_API void RHI_BeginCommandBuffer(RHI_CommandBuffer *cmd);
RHI_API void RHI_EndCommandBuffer(RHI_CommandBuffer *cmd);

/* ------------------------------------------------------------------ */
/* Command Buffer recording                                           */
/* ------------------------------------------------------------------ */

RHI_API void RHI_CmdBeginRenderPass(RHI_CommandBuffer *cmd,
                                    const RHI_ColorTargetInfo *colorTargets,
                                    uint32_t colorTargetCount,
                                    const RHI_DepthStencilTargetInfo *depthStencil);
RHI_API void RHI_CmdEndRenderPass(RHI_CommandBuffer *cmd);

RHI_API void RHI_CmdBindGraphicsPipeline(RHI_CommandBuffer *cmd, RHI_GraphicsPipeline *pipeline);
RHI_API void RHI_CmdBindVertexBuffer(RHI_CommandBuffer *cmd, uint32_t slot, RHI_Buffer *buffer, uint64_t offset);
RHI_API void RHI_CmdBindIndexBuffer(RHI_CommandBuffer *cmd, RHI_Buffer *buffer, uint64_t offset, RHI_IndexFormat format);
RHI_API void RHI_CmdPushConstants(RHI_CommandBuffer *cmd, uint32_t offset, uint32_t size, const void *data);

RHI_API void RHI_CmdDraw(RHI_CommandBuffer *cmd,
                          uint32_t vertexCount, uint32_t instanceCount,
                          uint32_t firstVertex, uint32_t firstInstance);
RHI_API void RHI_CmdDrawIndexed(RHI_CommandBuffer *cmd,
                                 uint32_t indexCount, uint32_t instanceCount,
                                 uint32_t firstIndex, int32_t vertexOffset,
                                 uint32_t firstInstance);

/* ------------------------------------------------------------------ */
/* Copy / Upload                                                       */
/* ------------------------------------------------------------------ */

typedef struct RHI_TransferBufferLocation {
    RHI_Buffer *transferBuffer;
    uint32_t    offset;
} RHI_TransferBufferLocation;

typedef struct RHI_BufferRegion {
    RHI_Buffer *buffer;
    uint32_t    offset;
    uint32_t    size;
} RHI_BufferRegion;

RHI_API void RHI_CmdUploadBuffer(RHI_CommandBuffer *cmd,
                                  const RHI_TransferBufferLocation *src,
                                  const RHI_BufferRegion *dst);

/* ------------------------------------------------------------------ */
/* Sync                                                                */
/* ------------------------------------------------------------------ */

RHI_API RHI_Fence *RHI_CreateFence(RHI_Device *device, bool signaled);
RHI_API void       RHI_WaitForFence(RHI_Device *device, RHI_Fence *fence, uint64_t timeoutNs);
RHI_API void       RHI_ResetFence(RHI_Device *device, RHI_Fence *fence);
RHI_API void       RHI_DestroyFence(RHI_Device *device, RHI_Fence *fence);

RHI_API RHI_Semaphore *RHI_CreateTimelineSemaphore(RHI_Device *device, uint64_t initialValue);
RHI_API uint64_t       RHI_GetSemaphoreValue(RHI_Semaphore *semaphore);
RHI_API void           RHI_WaitSemaphore(RHI_Device *device, RHI_Semaphore *semaphore, uint64_t value, uint64_t timeoutNs);
RHI_API void           RHI_SignalSemaphore(RHI_Device *device, RHI_Semaphore *semaphore, uint64_t value);
RHI_API void           RHI_DestroySemaphore(RHI_Device *device, RHI_Semaphore *semaphore);

#ifdef __cplusplus
}
#endif

#endif
