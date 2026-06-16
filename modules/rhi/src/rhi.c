#include "rhi_internal.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void rhiReportError(RHI_Device *device, RHI_ErrorType type, const char *msg)
{
    if (device && device->errorCallback) {
        device->errorCallback(type, msg, device->errorCallbackUserData);
    }
}

static SDL_GPUPresentMode rhiToSDLPresentMode(bool vsync)
{
    return vsync ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE;
}

static SDL_GPUTextureFormat rhiToSDLTextureFormat(RHI_Format fmt)
{
    switch (fmt) {
    case RHI_FORMAT_R8_UNORM:           return SDL_GPU_TEXTUREFORMAT_R8_UNORM;
    case RHI_FORMAT_RGBA8_UNORM:        return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    case RHI_FORMAT_RGBA8_SRGB:         return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
    case RHI_FORMAT_BGRA8_UNORM:        return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    case RHI_FORMAT_BGRA8_SRGB:         return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM_SRGB;
    case RHI_FORMAT_RGBA16_FLOAT:       return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    case RHI_FORMAT_RGBA32_FLOAT:       return SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
    case RHI_FORMAT_R32_FLOAT:          return SDL_GPU_TEXTUREFORMAT_R32_FLOAT;
    case RHI_FORMAT_D16_UNORM:          return SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    case RHI_FORMAT_D24_UNORM_S8_UINT:  return SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
    case RHI_FORMAT_D32_FLOAT:          return SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    case RHI_FORMAT_BC7_UNORM:          return SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM;
    case RHI_FORMAT_ASTC_4x4_UNORM:     return SDL_GPU_TEXTUREFORMAT_ASTC_4x4_UNORM;
    default:                            return SDL_GPU_TEXTUREFORMAT_INVALID;
    }
}

static SDL_GPUVertexElementFormat rhiToSDLVertexFmt(RHI_VertexElementFormat fmt)
{
    switch (fmt) {
    case RHI_VERTEX_FORMAT_FLOAT:       return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    case RHI_VERTEX_FORMAT_FLOAT2:      return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    case RHI_VERTEX_FORMAT_FLOAT3:      return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    case RHI_VERTEX_FORMAT_FLOAT4:      return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    case RHI_VERTEX_FORMAT_UBYTE4_NORM: return SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    default:                            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    }
}

static SDL_GPUIndexElementSize rhiToSDLIndexSize(RHI_IndexFormat fmt)
{
    return (fmt == RHI_INDEX_UINT16) ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT;
}

static SDL_GPUCompareOp rhiToSDLCompareOp(RHI_CompareOp op)
{
    switch (op) {
    case RHI_COMPARE_NEVER:            return SDL_GPU_COMPAREOP_NEVER;
    case RHI_COMPARE_LESS:             return SDL_GPU_COMPAREOP_LESS;
    case RHI_COMPARE_EQUAL:            return SDL_GPU_COMPAREOP_EQUAL;
    case RHI_COMPARE_LESS_EQUAL:       return SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    case RHI_COMPARE_GREATER:          return SDL_GPU_COMPAREOP_GREATER;
    case RHI_COMPARE_NOT_EQUAL:        return SDL_GPU_COMPAREOP_NOT_EQUAL;
    case RHI_COMPARE_GREATER_EQUAL:    return SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
    case RHI_COMPARE_ALWAYS:           return SDL_GPU_COMPAREOP_ALWAYS;
    default:                           return SDL_GPU_COMPAREOP_NEVER;
    }
}

static SDL_GPUCullMode rhiToSDLCullMode(RHI_CullMode m)
{
    switch (m) {
    case RHI_CULL_NONE:  return SDL_GPU_CULLMODE_NONE;
    case RHI_CULL_FRONT: return SDL_GPU_CULLMODE_FRONT;
    case RHI_CULL_BACK:  return SDL_GPU_CULLMODE_BACK;
    default:             return SDL_GPU_CULLMODE_NONE;
    }
}

static SDL_GPUFrontFace rhiToSDLFrontFace(RHI_FrontFace f)
{
    return (f == RHI_FRONT_CCW) ? SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE : SDL_GPU_FRONTFACE_CLOCKWISE;
}

static SDL_GPUStencilOp rhiToSDLStencilOp(int op)
{
    return SDL_GPU_STENCILOP_KEEP;
}

static SDL_GPUBlendFactor rhiToSDLBlendFactor(RHI_BlendFactor f)
{
    switch (f) {
    case RHI_BLEND_ZERO:                return SDL_GPU_BLENDFACTOR_ZERO;
    case RHI_BLEND_ONE:                 return SDL_GPU_BLENDFACTOR_ONE;
    case RHI_BLEND_SRC_COLOR:           return SDL_GPU_BLENDFACTOR_SRC_COLOR;
    case RHI_BLEND_ONE_MINUS_SRC_COLOR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
    case RHI_BLEND_SRC_ALPHA:           return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    case RHI_BLEND_ONE_MINUS_SRC_ALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    case RHI_BLEND_DST_ALPHA:           return SDL_GPU_BLENDFACTOR_DST_ALPHA;
    case RHI_BLEND_ONE_MINUS_DST_ALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
    default:                            return SDL_GPU_BLENDFACTOR_ZERO;
    }
}

static SDL_GPUBlendOp rhiToSDLBlendOp(RHI_BlendOp op)
{
    switch (op) {
    case RHI_BLEND_OP_ADD:              return SDL_GPU_BLENDOP_ADD;
    case RHI_BLEND_OP_SUBTRACT:         return SDL_GPU_BLENDOP_SUBTRACT;
    case RHI_BLEND_OP_REVERSE_SUBTRACT: return SDL_GPU_BLENDOP_REVERSE_SUBTRACT;
    case RHI_BLEND_OP_MIN:              return SDL_GPU_BLENDOP_MIN;
    case RHI_BLEND_OP_MAX:              return SDL_GPU_BLENDOP_MAX;
    default:                            return SDL_GPU_BLENDOP_ADD;
    }
}

static SDL_GPUFilter rhiToSDLFilter(RHI_Filter f)
{
    return (f == RHI_FILTER_NEAREST) ? SDL_GPU_FILTER_NEAREST : SDL_GPU_FILTER_LINEAR;
}

static SDL_GPUSamplerAddressMode rhiToSDLAddressMode(RHI_AddressMode m)
{
    switch (m) {
    case RHI_ADDRESS_REPEAT:        return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    case RHI_ADDRESS_CLAMP_TO_EDGE: return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    case RHI_ADDRESS_CLAMP_TO_BORDER: return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
    default:                        return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    }
}

static SDL_GPUBufferUsageFlags rhiToSDLBufferUsage(RHI_BufferUsage u)
{
    SDL_GPUBufferUsageFlags f = 0;
    if (u & RHI_BUFFER_USAGE_VERTEX)       f |= SDL_GPU_BUFFERUSAGE_VERTEX;
    if (u & RHI_BUFFER_USAGE_INDEX)        f |= SDL_GPU_BUFFERUSAGE_INDEX;
    if (u & RHI_BUFFER_USAGE_UNIFORM)      f |= SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    if (u & RHI_BUFFER_USAGE_STORAGE)      f |= SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
    if (u & RHI_BUFFER_USAGE_INDIRECT)     f |= SDL_GPU_BUFFERUSAGE_INDIRECT;
    return f;
}

static SDL_GPUTextureUsageFlags rhiToSDLTextureUsage(uint32_t u)
{
    SDL_GPUTextureUsageFlags f = 0;
    if (u & RHI_TEXTURE_USAGE_SAMPLED)          f |= SDL_GPU_TEXTUREUSAGE_SAMPLER;
    if (u & RHI_TEXTURE_USAGE_STORAGE)          f |= SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE;
    if (u & RHI_TEXTURE_USAGE_COLOR_ATTACHMENT) f |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    if (u & RHI_TEXTURE_USAGE_DEPTH_ATTACHMENT) f |= SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    return f;
}

static SDL_GPUShaderStage rhiToSDLShaderStage(RHI_ShaderStage s)
{
    switch (s) {
    case RHI_SHADER_STAGE_VERTEX:   return SDL_GPU_SHADERSTAGE_VERTEX;
    case RHI_SHADER_STAGE_FRAGMENT: return SDL_GPU_SHADERSTAGE_FRAGMENT;
    case RHI_SHADER_STAGE_COMPUTE:  return SDL_GPU_SHADERSTAGE_VERTEX;
    default:                        return SDL_GPU_SHADERSTAGE_VERTEX;
    }
}

/* ------ Device ------ */

RHI_Device *RHI_CreateDevice(const RHI_DeviceCreateInfo *info)
{
    if (!info) return NULL;

    RHI_Device *dev = (RHI_Device *)calloc(1, sizeof(RHI_Device));
    if (!dev) return NULL;

    dev->backend = info->backend;
    dev->validationEnabled = info->enableValidation;

    const char *driverName = NULL;
    SDL_GPUShaderFormat shaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;
    switch (info->backend) {
    case RHI_BACKEND_VULKAN:
        driverName = "vulkan";
        shaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;
        break;
    case RHI_BACKEND_D3D12:
        driverName = "direct3d12";
        shaderFormat = SDL_GPU_SHADERFORMAT_DXIL;
        break;
    case RHI_BACKEND_METAL:
        driverName = "metal";
        shaderFormat = SDL_GPU_SHADERFORMAT_MSL;
        break;
    default:
        break;
    }

    dev->shaderFormat = shaderFormat;

    dev->sdlDevice = SDL_CreateGPUDevice(shaderFormat, info->enableValidation, driverName);
    if (!dev->sdlDevice) {
        char msg[256];
        snprintf(msg, sizeof(msg), "SDL_CreateGPUDevice failed: %s", SDL_GetError());
        rhiReportError(dev, RHI_ERROR_VALIDATION, msg);
        free(dev);
        return NULL;
    }

    return dev;
}

void RHI_DestroyDevice(RHI_Device *device)
{
    if (!device) return;
    if (device->sdlDevice) {
        SDL_DestroyGPUDevice(device->sdlDevice);
    }
    free(device);
}

void RHI_DeviceWaitIdle(RHI_Device *device)
{
    if (!device || !device->sdlDevice) return;
    SDL_WaitForGPUIdle(device->sdlDevice);
}

RHI_Queue *RHI_GetQueue(RHI_Device *device, RHI_QueueType type, uint32_t index)
{
    if (!device || !device->sdlDevice) return NULL;
    RHI_Queue *q = (RHI_Queue *)calloc(1, sizeof(RHI_Queue));
    if (!q) return NULL;
    q->sdlDevice = device->sdlDevice;
    q->type = type;
    return q;
}

void RHI_QueueSubmit(RHI_Queue *queue, const RHI_SubmitInfo *info, RHI_Fence *fence)
{
    if (!queue || !info) return;

    for (uint32_t i = 0; i < info->commandBufferCount; i++) {
        if (info->commandBuffers[i] && info->commandBuffers[i]->sdlCB) {
            if (fence) {
                fence->sdlFence = SDL_SubmitGPUCommandBufferAndAcquireFence(
                    info->commandBuffers[i]->sdlCB);
            } else {
                SDL_SubmitGPUCommandBuffer(info->commandBuffers[i]->sdlCB);
            }
        }
    }
}

void RHI_QueueWaitIdle(RHI_Queue *queue)
{
    if (!queue || !queue->sdlDevice) return;
    SDL_WaitForGPUIdle(queue->sdlDevice);
}

void RHI_SetErrorCallback(RHI_Device *device, RHI_ErrorCallback callback, void *userData)
{
    if (!device) return;
    device->errorCallback = callback;
    device->errorCallbackUserData = userData;
}

/* ------ Swapchain ------ */

RHI_Swapchain *RHI_CreateSwapchain(RHI_Device *device, const RHI_SwapchainCreateInfo *info)
{
    if (!device || !info) return NULL;

    RHI_Swapchain *sc = (RHI_Swapchain *)calloc(1, sizeof(RHI_Swapchain));
    if (!sc) return NULL;

    sc->device = device;
    sc->sdlWindow = (SDL_Window *)info->windowHandle;
    sc->format = info->format;
    sc->width = info->width;
    sc->height = info->height;

    SDL_ClaimWindowForGPUDevice(device->sdlDevice, sc->sdlWindow);
    SDL_SetGPUSwapchainParameters(device->sdlDevice, sc->sdlWindow,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        info->vsync ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE);

    return sc;
}

void RHI_DestroySwapchain(RHI_Device *device, RHI_Swapchain *swapchain)
{
    if (!device || !swapchain) return;
    if (swapchain->sdlWindow) {
        SDL_ReleaseWindowFromGPUDevice(device->sdlDevice, swapchain->sdlWindow);
    }
    free(swapchain);
}

RHI_Texture *RHI_GetSwapchainTexture(RHI_Swapchain *swapchain)
{
    if (!swapchain || !swapchain->device) return NULL;

    RHI_CommandBuffer *cb = (RHI_CommandBuffer *)swapchain->pendingCB;
    if (!cb || !cb->sdlCB) return NULL;

    SDL_GPUTexture *swapTex = NULL;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cb->sdlCB, swapchain->sdlWindow, &swapTex, NULL, NULL)) {
        return NULL;
    }
    if (!swapTex) return NULL;

    RHI_Texture *tex = (RHI_Texture *)calloc(1, sizeof(RHI_Texture));
    if (!tex) return NULL;
    tex->sdlTexture = swapTex;
    tex->fmt = swapchain->format;
    tex->width = swapchain->width;
    tex->height = swapchain->height;
    return tex;
}

void RHI_Present(RHI_Queue *queue, RHI_Swapchain *swapchain)
{
    /* SDL3 handles present via the command buffer submit flow */
}

RHI_Format RHI_GetSwapchainFormat(RHI_Swapchain *swapchain)
{
    if (!swapchain) return RHI_FORMAT_UNDEFINED;
    return swapchain->format;
}

void RHI_SetSwapchainCommandBuffer(RHI_Swapchain *swapchain, RHI_CommandBuffer *cmd)
{
    if (swapchain) swapchain->pendingCB = cmd;
}

/* ------ Buffer ------ */

RHI_Buffer *RHI_CreateBuffer(RHI_Device *device, const RHI_BufferCreateInfo *info)
{
    if (!device || !info) return NULL;

    RHI_Buffer *buf = (RHI_Buffer *)calloc(1, sizeof(RHI_Buffer));
    if (!buf) return NULL;

    buf->size = info->size;
    buf->memoryType = info->memoryType;
    buf->device = device;

    if (info->memoryType == RHI_MEMORY_HOST_VISIBLE ||
        info->memoryType == RHI_MEMORY_HOST_COHERENT) {
        SDL_GPUTransferBufferCreateInfo tci = {0};
        tci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tci.size = (uint32_t)info->size;
        buf->sdlTransferBuffer = SDL_CreateGPUTransferBuffer(device->sdlDevice, &tci);
        if (!buf->sdlTransferBuffer) {
            free(buf);
            return NULL;
        }
    } else {
        SDL_GPUBufferCreateInfo bci = {0};
        bci.usage = rhiToSDLBufferUsage(info->usage);
        bci.size = (uint32_t)info->size;
        buf->sdlBuffer = SDL_CreateGPUBuffer(device->sdlDevice, &bci);
        if (!buf->sdlBuffer) {
            free(buf);
            return NULL;
        }
    }

    return buf;
}

void RHI_DestroyBuffer(RHI_Device *device, RHI_Buffer *buffer)
{
    if (!device || !buffer) return;
    if (buffer->sdlTransferBuffer) {
        SDL_ReleaseGPUTransferBuffer(device->sdlDevice, buffer->sdlTransferBuffer);
    }
    if (buffer->sdlBuffer) {
        SDL_ReleaseGPUBuffer(device->sdlDevice, buffer->sdlBuffer);
    }
    free(buffer);
}

void *RHI_MapBuffer(RHI_Buffer *buffer, uint64_t offset, uint64_t size)
{
    if (!buffer || !buffer->sdlTransferBuffer || !buffer->device) return NULL;
    return SDL_MapGPUTransferBuffer(buffer->device->sdlDevice, buffer->sdlTransferBuffer, false);
}

void RHI_UnmapBuffer(RHI_Buffer *buffer)
{
    if (!buffer || !buffer->sdlTransferBuffer || !buffer->device) return;
    SDL_UnmapGPUTransferBuffer(buffer->device->sdlDevice, buffer->sdlTransferBuffer);
}

/* ------ Texture ------ */

RHI_Texture *RHI_CreateTexture(RHI_Device *device, const RHI_TextureCreateInfo *info)
{
    if (!device || !info) return NULL;

    RHI_Texture *tex = (RHI_Texture *)calloc(1, sizeof(RHI_Texture));
    if (!tex) return NULL;

    SDL_GPUTextureCreateInfo tci = {0};
    tci.type = SDL_GPU_TEXTURETYPE_2D;
    tci.format = rhiToSDLTextureFormat(info->format);
    tci.usage = rhiToSDLTextureUsage(info->usage);
    tci.width = info->width;
    tci.height = info->height;
    tci.layer_count_or_depth = info->arrayLayers > 0 ? info->arrayLayers : info->depth;
    tci.num_levels = info->mipLevels > 0 ? info->mipLevels : 1;

    tex->sdlTexture = SDL_CreateGPUTexture(device->sdlDevice, &tci);
    if (!tex->sdlTexture) {
        free(tex);
        return NULL;
    }

    tex->type = info->type;
    tex->fmt = info->format;
    tex->width = info->width;
    tex->height = info->height;
    tex->mipLevels = info->mipLevels;

    return tex;
}

void RHI_DestroyTexture(RHI_Device *device, RHI_Texture *texture)
{
    if (!device || !texture) return;
    if (texture->sdlTexture) {
        SDL_ReleaseGPUTexture(device->sdlDevice, texture->sdlTexture);
    }
    free(texture);
}

/* ------ Sampler ------ */

RHI_Sampler *RHI_CreateSampler(RHI_Device *device, const RHI_SamplerCreateInfo *info)
{
    if (!device || !info) return NULL;

    RHI_Sampler *s = (RHI_Sampler *)calloc(1, sizeof(RHI_Sampler));
    if (!s) return NULL;

    SDL_GPUSamplerCreateInfo sci = {0};
    sci.min_filter = rhiToSDLFilter(info->minFilter);
    sci.mag_filter = rhiToSDLFilter(info->magFilter);
    sci.address_mode_u = rhiToSDLAddressMode(info->addressU);
    sci.address_mode_v = rhiToSDLAddressMode(info->addressV);
    sci.address_mode_w = rhiToSDLAddressMode(info->addressW);
    sci.enable_anisotropy = info->maxAnisotropy > 1.0f;
    sci.max_anisotropy = (uint8_t)info->maxAnisotropy;
    sci.mip_lod_bias = info->mipLodBias;
    sci.min_lod = info->minLod;
    sci.max_lod = info->maxLod;

    s->sdlSampler = SDL_CreateGPUSampler(device->sdlDevice, &sci);
    if (!s->sdlSampler) {
        free(s);
        return NULL;
    }

    return s;
}

void RHI_DestroySampler(RHI_Device *device, RHI_Sampler *sampler)
{
    if (!device || !sampler) return;
    if (sampler->sdlSampler) {
        SDL_ReleaseGPUSampler(device->sdlDevice, sampler->sdlSampler);
    }
    free(sampler);
}

/* ------ Shader Module ------ */

RHI_ShaderModule *RHI_CreateShaderModule(RHI_Device *device, const RHI_ShaderModuleCreateInfo *info)
{
    if (!device || !info) return NULL;

    RHI_ShaderModule *mod = (RHI_ShaderModule *)calloc(1, sizeof(RHI_ShaderModule));
    if (!mod) return NULL;

    SDL_GPUShaderCreateInfo sci = {0};
    sci.code = (const Uint8 *)info->code;
    sci.code_size = info->codeSize;
    sci.stage = rhiToSDLShaderStage(info->stage);
    sci.entrypoint = info->entryPoint;
    sci.format = device->shaderFormat;

    sci.num_samplers = 0;
    sci.num_storage_textures = 0;
    sci.num_storage_buffers = 0;
    sci.num_uniform_buffers = 0;

    mod->sdlShader = SDL_CreateGPUShader(device->sdlDevice, &sci);
    if (!mod->sdlShader) {
        char msg[256];
        snprintf(msg, sizeof(msg), "SDL_CreateGPUShader failed: %s", SDL_GetError());
        rhiReportError(device, RHI_ERROR_VALIDATION, msg);
        free(mod);
        return NULL;
    }
    mod->stage = info->stage;

    return mod;
}

void RHI_DestroyShaderModule(RHI_Device *device, RHI_ShaderModule *module)
{
    if (!device || !module) return;
    if (module->sdlShader) {
        SDL_ReleaseGPUShader(device->sdlDevice, module->sdlShader);
    }
    free(module);
}

/* ------ Graphics Pipeline ------ */

RHI_GraphicsPipeline *RHI_CreateGraphicsPipeline(RHI_Device *device, const RHI_GraphicsPipelineCreateInfo *info)
{
    if (!device || !info) return NULL;

    RHI_GraphicsPipeline *pipe = (RHI_GraphicsPipeline *)calloc(1, sizeof(RHI_GraphicsPipeline));
    if (!pipe) return NULL;

    SDL_GPUGraphicsPipelineCreateInfo sdlInfo = {0};

    if (info->vertexShader) {
        sdlInfo.vertex_shader = info->vertexShader->sdlShader;
    }
    if (info->fragmentShader) {
        sdlInfo.fragment_shader = info->fragmentShader->sdlShader;
    }

    sdlInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    sdlInfo.rasterizer_state.cull_mode = rhiToSDLCullMode(info->cullMode);
    sdlInfo.rasterizer_state.front_face = rhiToSDLFrontFace(info->frontFace);
    sdlInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;

    sdlInfo.depth_stencil_state.enable_depth_test = info->depthTestEnable;
    sdlInfo.depth_stencil_state.enable_depth_write = info->depthWriteEnable;
    sdlInfo.depth_stencil_state.compare_op = rhiToSDLCompareOp(info->depthCompareOp);

    SDL_GPUColorTargetDescription sdlColorTargets[8] = {0};
    sdlInfo.target_info.num_color_targets = info->renderTargetCount;
    for (uint32_t i = 0; i < info->renderTargetCount && i < 8; i++) {
        sdlColorTargets[i].format = rhiToSDLTextureFormat(info->renderTargetFormats[i]);
        sdlColorTargets[i].blend_state.enable_blend = info->blendState.blendEnable;
        sdlColorTargets[i].blend_state.src_color_blendfactor = rhiToSDLBlendFactor(info->blendState.srcColorBlendFactor);
        sdlColorTargets[i].blend_state.dst_color_blendfactor = rhiToSDLBlendFactor(info->blendState.dstColorBlendFactor);
        sdlColorTargets[i].blend_state.color_blend_op = rhiToSDLBlendOp(info->blendState.colorBlendOp);
        sdlColorTargets[i].blend_state.src_alpha_blendfactor = rhiToSDLBlendFactor(info->blendState.srcAlphaBlendFactor);
        sdlColorTargets[i].blend_state.dst_alpha_blendfactor = rhiToSDLBlendFactor(info->blendState.dstAlphaBlendFactor);
        sdlColorTargets[i].blend_state.alpha_blend_op = rhiToSDLBlendOp(info->blendState.alphaBlendOp);
        sdlColorTargets[i].blend_state.color_write_mask = info->blendState.colorWriteMask;
    }
    sdlInfo.target_info.color_target_descriptions = sdlColorTargets;

    if (info->depthStencilFormat != RHI_FORMAT_UNDEFINED) {
        sdlInfo.target_info.depth_stencil_format = rhiToSDLTextureFormat(info->depthStencilFormat);
        sdlInfo.target_info.has_depth_stencil_target = true;
    }

    SDL_GPUVertexBufferDescription sdlVBBufs[16] = {0};
    if (info->vertexBindingCount > 0) {
        sdlInfo.vertex_input_state.num_vertex_buffers = info->vertexBindingCount;
        for (uint32_t i = 0; i < info->vertexBindingCount && i < 16; i++) {
            sdlVBBufs[i].slot = info->vertexBindings[i].binding;
            sdlVBBufs[i].pitch = info->vertexBindings[i].stride;
            sdlVBBufs[i].input_rate =
                info->vertexBindings[i].perInstance ? SDL_GPU_VERTEXINPUTRATE_INSTANCE : SDL_GPU_VERTEXINPUTRATE_VERTEX;
        }
        sdlInfo.vertex_input_state.vertex_buffer_descriptions = sdlVBBufs;
    }

    SDL_GPUVertexAttribute sdlAttrs[16] = {0};
    if (info->vertexAttributeCount > 0) {
        sdlInfo.vertex_input_state.num_vertex_attributes = info->vertexAttributeCount;
        for (uint32_t i = 0; i < info->vertexAttributeCount && i < 16; i++) {
            sdlAttrs[i].location = info->vertexAttributes[i].location;
            sdlAttrs[i].buffer_slot = info->vertexAttributes[i].binding;
            sdlAttrs[i].format = rhiToSDLVertexFmt(info->vertexAttributes[i].format);
            sdlAttrs[i].offset = info->vertexAttributes[i].offset;
        }
        sdlInfo.vertex_input_state.vertex_attributes = sdlAttrs;
    }

    pipe->sdlPipeline = SDL_CreateGPUGraphicsPipeline(device->sdlDevice, &sdlInfo);
    if (!pipe->sdlPipeline) {
        char msg[256];
        snprintf(msg, sizeof(msg), "SDL_CreateGPUGraphicsPipeline failed: %s", SDL_GetError());
        rhiReportError(device, RHI_ERROR_VALIDATION, msg);
        free(pipe);
        return NULL;
    }

    return pipe;
}

void RHI_DestroyGraphicsPipeline(RHI_Device *device, RHI_GraphicsPipeline *pipeline)
{
    if (!device || !pipeline) return;
    if (pipeline->sdlPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device->sdlDevice, pipeline->sdlPipeline);
    }
    free(pipeline);
}

/* ------ Command Pool & Buffer ------ */

RHI_CommandPool *RHI_CreateCommandPool(RHI_Device *device, const RHI_CommandPoolCreateInfo *info)
{
    if (!device) return NULL;
    RHI_CommandPool *pool = (RHI_CommandPool *)calloc(1, sizeof(RHI_CommandPool));
    if (!pool) return NULL;
    pool->device = device;
    pool->queueType = info ? info->queueType : RHI_QUEUE_GRAPHICS;
    return pool;
}

void RHI_DestroyCommandPool(RHI_Device *device, RHI_CommandPool *pool)
{
    if (pool) free(pool);
}

void RHI_ResetCommandPool(RHI_CommandPool *pool)
{
    /* SDL3 recycles command buffers automatically per frame */
}

RHI_CommandBuffer *RHI_AllocateCommandBuffer(RHI_CommandPool *pool)
{
    if (!pool || !pool->device) return NULL;

    RHI_CommandBuffer *cb = (RHI_CommandBuffer *)calloc(1, sizeof(RHI_CommandBuffer));
    if (!cb) return NULL;

    cb->device = pool->device;
    cb->sdlCB = SDL_AcquireGPUCommandBuffer(pool->device->sdlDevice);
    if (!cb->sdlCB) {
        free(cb);
        return NULL;
    }

    return cb;
}

RHI_CommandBuffer *RHI_AcquireCommandBuffer(RHI_Device *device, RHI_QueueType queue)
{
    if (!device) return NULL;

    RHI_CommandBuffer *cb = (RHI_CommandBuffer *)calloc(1, sizeof(RHI_CommandBuffer));
    if (!cb) return NULL;

    cb->device = device;
    cb->sdlCB = SDL_AcquireGPUCommandBuffer(device->sdlDevice);
    if (!cb->sdlCB) {
        free(cb);
        return NULL;
    }

    return cb;
}

void RHI_BeginCommandBuffer(RHI_CommandBuffer *cmd)
{
    if (cmd) cmd->recording = true;
}

void RHI_EndCommandBuffer(RHI_CommandBuffer *cmd)
{
    if (!cmd) return;
    if (cmd->inCopyPass) {
        SDL_EndGPUCopyPass(cmd->sdlCopyPass);
        cmd->sdlCopyPass = NULL;
        cmd->inCopyPass = false;
    }
    cmd->recording = false;
}

/* ------ Command recording ------ */

void RHI_CmdBeginRenderPass(RHI_CommandBuffer *cmd,
                              const RHI_ColorTargetInfo *colorTargets,
                              uint32_t colorTargetCount,
                              const RHI_DepthStencilTargetInfo *depthStencil)
{
    if (!cmd || !cmd->sdlCB) return;

    if (cmd->inCopyPass) {
        SDL_EndGPUCopyPass(cmd->sdlCopyPass);
        cmd->sdlCopyPass = NULL;
        cmd->inCopyPass = false;
    }

    SDL_GPUColorTargetInfo sdlColors[8] = {0};
    for (uint32_t i = 0; i < colorTargetCount && i < 8; i++) {
        sdlColors[i].texture = colorTargets[i].texture ? colorTargets[i].texture->sdlTexture : NULL;
        sdlColors[i].load_op = (colorTargets[i].loadOp == RHI_LOAD_OP_CLEAR) ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
        sdlColors[i].store_op = (colorTargets[i].storeOp == RHI_STORE_OP_DONTCARE) ? SDL_GPU_STOREOP_DONT_CARE : SDL_GPU_STOREOP_STORE;
        sdlColors[i].clear_color.r = colorTargets[i].clearR;
        sdlColors[i].clear_color.g = colorTargets[i].clearG;
        sdlColors[i].clear_color.b = colorTargets[i].clearB;
        sdlColors[i].clear_color.a = colorTargets[i].clearA;
    }

    SDL_GPUDepthStencilTargetInfo sdlDepth = {0};
    if (depthStencil) {
        sdlDepth.texture = depthStencil->texture ? depthStencil->texture->sdlTexture : NULL;
        sdlDepth.load_op = (depthStencil->loadOp == RHI_LOAD_OP_CLEAR) ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
        sdlDepth.store_op = (depthStencil->storeOp == RHI_STORE_OP_DONTCARE) ? SDL_GPU_STOREOP_DONT_CARE : SDL_GPU_STOREOP_STORE;
        sdlDepth.clear_depth = depthStencil->clearDepth;
        sdlDepth.clear_stencil = depthStencil->clearStencil;
        sdlDepth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        sdlDepth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    }

    cmd->sdlRenderPass = SDL_BeginGPURenderPass(cmd->sdlCB,
                           sdlColors, colorTargetCount,
                           depthStencil ? &sdlDepth : NULL);
    cmd->inRenderPass = (cmd->sdlRenderPass != NULL);
}

void RHI_CmdEndRenderPass(RHI_CommandBuffer *cmd)
{
    if (!cmd || !cmd->sdlRenderPass) return;
    SDL_EndGPURenderPass(cmd->sdlRenderPass);
    cmd->sdlRenderPass = NULL;
    cmd->inRenderPass = false;
}

void RHI_CmdBindGraphicsPipeline(RHI_CommandBuffer *cmd, RHI_GraphicsPipeline *pipeline)
{
    if (!cmd || !cmd->sdlRenderPass || !pipeline) return;
    SDL_BindGPUGraphicsPipeline(cmd->sdlRenderPass, pipeline->sdlPipeline);
}

void RHI_CmdBindVertexBuffer(RHI_CommandBuffer *cmd, uint32_t slot, RHI_Buffer *buffer, uint64_t offset)
{
    if (!cmd || !cmd->sdlRenderPass || !buffer) return;
    SDL_GPUBufferBinding binding = {0};
    binding.buffer = buffer->sdlBuffer;
    binding.offset = (uint32_t)offset;
    SDL_BindGPUVertexBuffers(cmd->sdlRenderPass, slot, &binding, 1);
}

void RHI_CmdBindIndexBuffer(RHI_CommandBuffer *cmd, RHI_Buffer *buffer, uint64_t offset, RHI_IndexFormat format)
{
    if (!cmd || !cmd->sdlRenderPass || !buffer) return;
    SDL_GPUBufferBinding binding = {0};
    binding.buffer = buffer->sdlBuffer;
    binding.offset = (uint32_t)offset;
    SDL_BindGPUIndexBuffer(cmd->sdlRenderPass, &binding, rhiToSDLIndexSize(format));
}

void RHI_CmdDraw(RHI_CommandBuffer *cmd,
                  uint32_t vertexCount, uint32_t instanceCount,
                  uint32_t firstVertex, uint32_t firstInstance)
{
    if (!cmd || !cmd->sdlRenderPass) return;
    SDL_DrawGPUPrimitives(cmd->sdlRenderPass, vertexCount, instanceCount, firstVertex, firstInstance);
}

void RHI_CmdDrawIndexed(RHI_CommandBuffer *cmd,
                         uint32_t indexCount, uint32_t instanceCount,
                         uint32_t firstIndex, int32_t vertexOffset,
                         uint32_t firstInstance)
{
    if (!cmd || !cmd->sdlRenderPass) return;
    SDL_DrawGPUIndexedPrimitives(cmd->sdlRenderPass,
                                  indexCount, instanceCount,
                                  firstIndex, vertexOffset,
                                  firstInstance);
}

void RHI_CmdPushConstants(RHI_CommandBuffer *cmd, uint32_t offset, uint32_t size, const void *data)
{
    if (!cmd || !cmd->sdlRenderPass || !data) return;
    SDL_PushGPUVertexUniformData(cmd->sdlCB, 0, data, size);
    SDL_PushGPUFragmentUniformData(cmd->sdlCB, 0, data, size);
}

/* ------ Copy / Upload ------ */

void RHI_CmdUploadBuffer(RHI_CommandBuffer *cmd,
                          const RHI_TransferBufferLocation *src,
                          const RHI_BufferRegion *dst)
{
    if (!cmd || !cmd->sdlCB || !src || !dst) return;
    if (!src->transferBuffer || !src->transferBuffer->sdlTransferBuffer) return;
    if (!dst->buffer || !dst->buffer->sdlBuffer) return;

    if (!cmd->inCopyPass) {
        cmd->sdlCopyPass = SDL_BeginGPUCopyPass(cmd->sdlCB);
        cmd->inCopyPass = (cmd->sdlCopyPass != NULL);
    }

    if (!cmd->sdlCopyPass) return;

    SDL_GPUTransferBufferLocation sdlSrc = {0};
    sdlSrc.transfer_buffer = src->transferBuffer->sdlTransferBuffer;
    sdlSrc.offset = src->offset;

    SDL_GPUBufferRegion sdlDst = {0};
    sdlDst.buffer = dst->buffer->sdlBuffer;
    sdlDst.offset = dst->offset;
    sdlDst.size = dst->size;

    SDL_UploadToGPUBuffer(cmd->sdlCopyPass, &sdlSrc, &sdlDst, true);
}

/* ------ Fence ------ */

RHI_Fence *RHI_CreateFence(RHI_Device *device, bool signaled)
{
    if (!device) return NULL;
    RHI_Fence *f = (RHI_Fence *)calloc(1, sizeof(RHI_Fence));
    return f;
}

void RHI_WaitForFence(RHI_Device *device, RHI_Fence *fence, uint64_t timeoutNs)
{
    if (!device || !fence || !fence->sdlFence) return;
    SDL_WaitForGPUFences(device->sdlDevice, true, &fence->sdlFence, 1);
}

void RHI_ResetFence(RHI_Device *device, RHI_Fence *fence)
{
    if (!device || !fence) return;
    if (fence->sdlFence) {
        SDL_ReleaseGPUFence(device->sdlDevice, fence->sdlFence);
        fence->sdlFence = NULL;
    }
}

void RHI_DestroyFence(RHI_Device *device, RHI_Fence *fence)
{
    if (!device || !fence) return;
    if (fence->sdlFence) {
        SDL_ReleaseGPUFence(device->sdlDevice, fence->sdlFence);
    }
    free(fence);
}

/* ------ Semaphore (stub — SDL3 has no timeline semaphore API) ------ */

RHI_Semaphore *RHI_CreateTimelineSemaphore(RHI_Device *device, uint64_t initialValue)
{
    return NULL;
}

uint64_t RHI_GetSemaphoreValue(RHI_Semaphore *semaphore)
{
    return 0;
}

void RHI_WaitSemaphore(RHI_Device *device, RHI_Semaphore *semaphore, uint64_t value, uint64_t timeoutNs)
{
}

void RHI_SignalSemaphore(RHI_Device *device, RHI_Semaphore *semaphore, uint64_t value)
{
}

void RHI_DestroySemaphore(RHI_Device *device, RHI_Semaphore *semaphore)
{
}
