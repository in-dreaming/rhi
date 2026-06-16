# RHI Architecture V3

## 1. Overview

```
┌─────────────────────────────────────────────────────────┐
│              Application / Editor / Agent                 │
├─────────────────────────────────────────────────────────┤
│                Engine Framework Layer                     │
│  Scene │ Material │ RenderGraph │ GPU Driven │ Streaming │
│        │ Visibility│ AI Bridge  │            │           │
├─────────────────────────────────────────────────────────┤
│                     RHI Layer                            │
│  Device │ Buffer │ Texture │ Sampler │ Pipeline          │
│  CommandPool │ CommandBuffer │ BindGroup │ Fence         │
│  BindlessTable │ ShaderReflection │ Swapchain            │
├─────────────────────────────────────────────────────────┤
│               SDL GPU Extended Layer                     │
│  SDL_GPUBindlessTable  │ SDL_GPUReflection              │
│  SDL_GPUPipelineCache  │ SDL_GPUIndirect                │
│  SDL_GPUCommandPool    │ SDL_GPUMeshPipeline            │
│  SDL_GPURayTracing (stub)                               │
├────────┬────────┬────────┬──────────────────────────────┤
│ Vulkan │ Metal  │ D3D12  │ WebGPU                       │
└────────┴────────┴────────┴──────────────────────────────┘
```

---

## 2. Design Principles

### 2.1 SDL Is Not Public API

SDL types never appear in RHI or above. All SDL handles are wrapped.

```cpp
// Forbidden
SDL_GPUTexture* tex = ...;

// Required
RHITexture* tex = RHI_CreateTexture(device, &desc);
```

### 2.2 RHI Is Not Feature Layer

RHI provides **mechanism**, not **policy**.

| Belongs in RHI              | Does NOT belong in RHI     |
|-----------------------------|----------------------------|
| Bindless resource table     | FSR / DLSS / XeSS          |
| Indirect draw commands      | Frame generation           |
| Multi-queue submit          | Tone mapping               |
| Shader reflection query     | Specific post-process      |

Vendor SDKs and post-process algorithms live in plugins or the Render Framework.

### 2.3 Handle-First Resource Access with Generational Index

Runtime never passes raw GPU pointers. All resources are accessed via generational index handles — single-word values that solve the ABA problem without reference counting.

```cpp
typedef struct RHI_Handle {
    uint32_t index      : 24;   // slot index
    uint32_t generation : 8;    // generation counter
} RHI_Handle;

typedef RHI_Handle RHI_TextureHandle;
typedef RHI_Handle RHI_BufferHandle;
typedef RHI_Handle RHI_SamplerHandle;
```

Slot layout:

```cpp
typedef struct RHI_Slot {
    void*    resource;      // actual resource pointer
    uint32_t generation;    // current generation (0 = dead)
} RHI_Slot;
```

Validation — O(1), no lock:

```cpp
bool RHI_IsHandleValid(RHI_Handle handle) {
    RHI_Slot* slot = &g_slots[handle.index];
    return slot->generation == handle.generation && slot->generation != 0;
}
```

When a resource is destroyed, its slot generation increments and `resource` is nulled. Any stale handle still carrying the old generation will fail validation — no ABA bug, no use-after-free.

### 2.4 Data-Driven Everything

Shader, Material, Pass, RenderGraph, Pipeline — all serializable as assets. No hardcoded render paths.

### 2.5 PIMPL for ABI Stability

All public types are opaque. Internal layout is free to change without rebuilding downstream modules.

```cpp
// rhi.h (public)
typedef struct RHI_Device RHI_Device;

RHI_Device* RHI_CreateDevice(const RHI_DeviceCreateInfo* info);
void        RHI_DestroyDevice(RHI_Device* device);

// rhi_internal.h (private)
struct RHI_Device {
    void*       nativeDevice;        // VkDevice / ID3D12Device / MTLDevice
    RHI_Queue*  graphicsQueue;
    RHI_Queue*  computeQueue;
    RHI_Queue*  transferQueue;
    RHI_Slot*   resourceSlots;
    uint32_t    slotCapacity;
    // ... freely modifiable
};
```

### 2.6 Error Handling via Callback

```cpp
typedef enum RHI_ErrorType {
    RHI_ERROR_VALIDATION,
    RHI_ERROR_OUT_OF_MEMORY,
    RHI_ERROR_DEVICE_LOST,
} RHI_ErrorType;

typedef void (*RHI_ErrorCallback)(RHI_ErrorType type, const char* message, void* userData);

void RHI_SetErrorCallback(RHI_Device* device, RHI_ErrorCallback callback, void* userData);
```

---

## 3. Platform Matrix

| Platform | Graphics API   | Status |
|----------|---------------|--------|
| Windows  | Vulkan, D3D12 | V1     |
| Linux    | Vulkan        | V1     |
| macOS    | Metal         | V1     |
| Android  | Vulkan        | V1.5   |
| iOS      | Metal         | V1.5   |
| Web      | WebGPU        | V2     |

Legacy APIs (OpenGL, GLES, D3D11) are NOT supported.

---

## 4. Core Object Handles (C API)

```c
typedef struct RHI_Device        RHI_Device;
typedef struct RHI_Queue         RHI_Queue;

typedef struct RHI_Buffer        RHI_Buffer;
typedef struct RHI_Texture       RHI_Texture;
typedef struct RHI_Sampler       RHI_Sampler;

typedef struct RHI_ShaderModule  RHI_ShaderModule;
typedef struct RHI_PipelineLayout RHI_PipelineLayout;
typedef struct RHI_GraphicsPipeline RHI_GraphicsPipeline;
typedef struct RHI_ComputePipeline  RHI_ComputePipeline;
typedef struct RHI_MeshPipeline     RHI_MeshPipeline;

typedef struct RHI_BindGroup     RHI_BindGroup;
typedef struct RHI_BindlessTable RHI_BindlessTable;

typedef struct RHI_CommandPool   RHI_CommandPool;
typedef struct RHI_CommandBuffer RHI_CommandBuffer;

typedef struct RHI_Fence         RHI_Fence;
typedef struct RHI_Semaphore     RHI_Semaphore;

typedef struct RHI_Swapchain     RHI_Swapchain;
```

---

## 5. SDL GPU Extended Layer

Fork of SDL3 (`sdl_enjin_rhi` branch). Invasive modifications allowed. Long-term maintained as submodule.

### 5.1 SDL_GPUBindlessTable

```c
typedef struct SDL_GPUBindlessTable SDL_GPUBindlessTable;

SDL_GPUBindlessTable* SDL_CreateGPUBindlessTable(
    SDL_GPUDevice* device,
    uint32_t maxTextures,
    uint32_t maxBuffers,
    uint32_t maxSamplers
);

uint32_t SDL_BindlessTableAddTexture(SDL_GPUBindlessTable* table, SDL_GPUTexture* texture);
uint32_t SDL_BindlessTableAddBuffer(SDL_GPUBindlessTable* table, SDL_GPUBuffer* buffer);
uint32_t SDL_BindlessTableAddSampler(SDL_GPUBindlessTable* table, SDL_GPUSampler* sampler);

void SDL_BindlessTableRemoveTexture(SDL_GPUBindlessTable* table, uint32_t handle);
void SDL_BindlessTableRemoveBuffer(SDL_GPUBindlessTable* table, uint32_t handle);
void SDL_BindlessTableRemoveSampler(SDL_GPUBindlessTable* table, uint32_t handle);

void SDL_BindGPUBindlessTable(SDL_GPUCommandBuffer* cb, SDL_GPUBindlessTable* table, uint32_t slot);
void SDL_FreeGPUBindlessTable(SDL_GPUBindlessTable* table);
```

Backend mapping:

| Vulkan                                                  | D3D12                                    | Metal                   |
|---------------------------------------------------------|------------------------------------------|-------------------------|
| `VK_EXT_descriptor_indexing` + unbounded descriptor arrays | Shader Visible CBV_SRV_UAV Descriptor Heap | `MTLArgumentBuffer`     |

Shader access:

```hlsl
// Vulkan / D3D12
Texture2D gTextures[] : register(t0, space1);
Buffer<>   gBuffers[] : register(t0, space2);
SamplerState gSamplers[] : register(s0, space3);

// Metal
array<texture2d<float>, 4096> gTextures [[id(0)]];
```

### 5.2 SDL_GPUShaderReflection

```c
typedef struct SDL_GPUShaderParam {
    const char* name;
    uint32_t    binding;
    uint32_t    set;
    const char* type_name;    // e.g. "ConstantBuffer<TransformData>"
    uint32_t    size;
    uint32_t    offset;
} SDL_GPUShaderParam;

typedef struct SDL_GPUShaderReflection {
    uint32_t num_inputs;
    uint32_t num_outputs;
    uint32_t num_uniform_buffers;
    uint32_t num_storage_buffers;
    uint32_t num_samplers;
    uint32_t num_textures;

    SDL_GPUShaderParam* inputs;
    SDL_GPUShaderParam* outputs;
    SDL_GPUShaderParam* uniform_buffers;
    SDL_GPUShaderParam* storage_buffers;
    SDL_GPUShaderParam* samplers;
    SDL_GPUShaderParam* textures;

    char*    json;             // Pre-serialized JSON for agent consumption
    uint32_t json_size;
} SDL_GPUShaderReflection;

SDL_GPUShaderReflection* SDL_ReflectGPUShader(const char* slang_source, const char* entry_point);
void SDL_FreeGPUShaderReflection(SDL_GPUShaderReflection* reflection);
```

Source of truth: **Slang reflection API**. Not SPIRV-Cross. Not DXIL reflection.

### 5.3 SDL_GPUPipelineCache

```c
typedef struct SDL_GPUPipelineCache SDL_GPUPipelineCache;

SDL_GPUPipelineCache* SDL_CreateGPUPipelineCache(SDL_GPUDevice* device, const char* path);
size_t                SDL_GetGPUPipelineCacheData(SDL_GPUPipelineCache* cache, void* data, size_t capacity);
void                  SDL_SaveGPUPipelineCache(SDL_GPUPipelineCache* cache);
void                  SDL_FreeGPUPipelineCache(SDL_GPUPipelineCache* cache);
```

### 5.4 SDL_GPUIndirect

```c
typedef struct SDL_GPUDrawIndirectCommand {
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
} SDL_GPUDrawIndirectCommand;

typedef struct SDL_GPUDrawIndexedIndirectCommand {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t  vertexOffset;
    uint32_t firstInstance;
} SDL_GPUDrawIndexedIndirectCommand;

void SDL_DrawGPUIndirect(SDL_GPUCommandBuffer* cb, SDL_GPUBuffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride);
void SDL_DrawGPUIndexedIndirect(SDL_GPUCommandBuffer* cb, SDL_GPUBuffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride);
void SDL_DrawGPUIndirectCount(SDL_GPUCommandBuffer* cb, SDL_GPUBuffer* indirectBuffer, uint64_t indirectOffset, SDL_GPUBuffer* countBuffer, uint64_t countOffset, uint32_t maxDrawCount, uint32_t stride);
void SDL_DispatchGPUIndirect(SDL_GPUCommandBuffer* cb, SDL_GPUBuffer* buffer, uint64_t offset);
```

### 5.5 SDL_GPUMeshPipeline

```c
typedef struct SDL_GPUMeshPipelineCreateInfo {
    SDL_GPUShader* taskShader;      // optional
    SDL_GPUShader* meshShader;      // required
    SDL_GPUShader* fragmentShader;
    SDL_GPURasterizerState rasterizerState;
    SDL_GPUDepthStencilState depthStencilState;
    uint32_t colorBlendAttachmentCount;
    SDL_GPUColorBlendAttachmentState* colorBlendAttachments;
} SDL_GPUMeshPipelineCreateInfo;

typedef struct SDL_GPUMeshPipeline SDL_GPUMeshPipeline;

SDL_GPUMeshPipeline* SDL_CreateGPUMeshPipeline(SDL_GPUDevice* device, SDL_GPUMeshPipelineCreateInfo* info);
void SDL_DrawGPUMeshTasks(SDL_GPUCommandBuffer* cb, uint32_t x, uint32_t y, uint32_t z);
void SDL_ReleaseGPUMeshPipeline(SDL_GPUMeshPipeline* pipeline);
```

Platform mapping:

| Vulkan (`NV_mesh_shader`, `EXT_mesh_shader`) | D3D12 (`D3D12_MESH_SHADER`) | Metal (`MTLMeshRenderPipelineState`) |

### 5.6 SDL_GPUCommandPool

```c
typedef struct SDL_GPUCommandPool SDL_GPUCommandPool;

SDL_GPUCommandPool*     SDL_CreateGPUCommandPool(SDL_GPUDevice* device, SDL_GPUQueueType queue_type);
void                    SDL_FreeGPUCommandPool(SDL_GPUCommandPool* pool);
SDL_GPUCommandBuffer*   SDL_AcquireGPUCommandBufferFromPool(SDL_GPUCommandPool* pool);
void                    SDL_ResetGPUCommandPool(SDL_GPUCommandPool* pool);
```

Each thread owns its own `SDL_GPUCommandPool`. Command buffers from the same pool MUST NOT be recorded concurrently. Command buffers from different pools can be recorded in parallel and submitted together.

Backend mapping:

| Vulkan (`VkCommandPool` per thread) | D3D12 (`ID3D12CommandAllocator` per thread) | Metal (`MTLCommandBuffer` from shared `MTLCommandQueue`, already thread-safe) |

### 5.7 SDL_GPURayTracing (Stub)

```c
typedef struct SDL_GPUAccelerationStructure SDL_GPUAccelerationStructure;
typedef struct SDL_GPURTPipeline SDL_GPURTPipeline;
// V1: type only, no implementation
```

---

## 6. RHI C API

### 6.1 Device & Queue

```c
typedef enum RHI_Backend {
    RHI_BACKEND_VULKAN,
    RHI_BACKEND_D3D12,
    RHI_BACKEND_METAL,
    RHI_BACKEND_WEBGPU,
} RHI_Backend;

typedef struct RHI_DeviceCreateInfo {
    RHI_Backend backend;
    bool        enableValidation;
    bool        enableBindless;
    bool        enableMeshShader;
    bool        enableRayTracing;
    const char* applicationName;
    uint32_t    applicationVersion;
} RHI_DeviceCreateInfo;

RHI_Device* RHI_CreateDevice(const RHI_DeviceCreateInfo* info);
void        RHI_DestroyDevice(RHI_Device* device);
void        RHI_DeviceWaitIdle(RHI_Device* device);

typedef enum RHI_QueueType {
    RHI_QUEUE_GRAPHICS,
    RHI_QUEUE_COMPUTE,
    RHI_QUEUE_TRANSFER,
} RHI_QueueType;

RHI_Queue* RHI_GetQueue(RHI_Device* device, RHI_QueueType type, uint32_t index);
void       RHI_QueueWaitIdle(RHI_Queue* queue);

typedef struct RHI_SubmitInfo {
    RHI_CommandBuffer** commandBuffers;
    uint32_t            commandBufferCount;

    RHI_Semaphore** waitSemaphores;
    uint64_t*        waitValues;
    uint32_t         waitSemaphoreCount;

    RHI_Semaphore** signalSemaphores;
    uint64_t*        signalValues;
    uint32_t         signalSemaphoreCount;
} RHI_SubmitInfo;

void RHI_QueueSubmit(RHI_Queue* queue, const RHI_SubmitInfo* info, RHI_Fence* fence);
```

### 6.2 Swapchain

```c
typedef struct RHI_SwapchainCreateInfo {
    void*     windowHandle;    // SDL_Window* (cast internally)
    uint32_t  width;
    uint32_t  height;
    bool      vsync;
    RHI_Format format;         // preferred, may be adjusted
} RHI_SwapchainCreateInfo;

RHI_Swapchain*    RHI_CreateSwapchain(RHI_Device* device, const RHI_SwapchainCreateInfo* info);
void              RHI_DestroySwapchain(RHI_Device* device, RHI_Swapchain* swapchain);
RHI_TextureHandle RHI_AcquireSwapchainTexture(RHI_Swapchain* swapchain);
void              RHI_Present(RHI_Queue* queue, RHI_Swapchain* swapchain);
void              RHI_ResizeSwapchain(RHI_Swapchain* swapchain, uint32_t width, uint32_t height);
```

### 6.3 Buffer

```c
typedef enum RHI_BufferUsage {
    RHI_BUFFER_USAGE_VERTEX       = 1 << 0,
    RHI_BUFFER_USAGE_INDEX        = 1 << 1,
    RHI_BUFFER_USAGE_UNIFORM      = 1 << 2,
    RHI_BUFFER_USAGE_STORAGE      = 1 << 3,
    RHI_BUFFER_USAGE_INDIRECT     = 1 << 4,
    RHI_BUFFER_USAGE_TRANSFER_SRC = 1 << 5,
    RHI_BUFFER_USAGE_TRANSFER_DST = 1 << 6,
} RHI_BufferUsage;

typedef enum RHI_MemoryType {
    RHI_MEMORY_DEVICE_LOCAL,
    RHI_MEMORY_HOST_VISIBLE,
    RHI_MEMORY_HOST_COHERENT,
} RHI_MemoryType;

typedef struct RHI_BufferCreateInfo {
    uint64_t       size;
    RHI_BufferUsage usage;
    RHI_MemoryType  memoryType;
    const char*    name;
} RHI_BufferCreateInfo;

RHI_Buffer*       RHI_CreateBuffer(RHI_Device* device, const RHI_BufferCreateInfo* info);
RHI_BufferHandle  RHI_RegisterBuffer(RHI_BindlessTable* table, RHI_Buffer* buffer);
void              RHI_DestroyBuffer(RHI_Device* device, RHI_Buffer* buffer);
void*             RHI_MapBuffer(RHI_Buffer* buffer, uint64_t offset, uint64_t size);
void              RHI_UnmapBuffer(RHI_Buffer* buffer);
```

### 6.4 Texture

```c
typedef enum RHI_TextureType { RHI_TEXTURE_2D, RHI_TEXTURE_3D, RHI_TEXTURE_CUBE, RHI_TEXTURE_2D_ARRAY };
typedef enum RHI_Format {
    RHI_FORMAT_UNDEFINED,
    RHI_FORMAT_R8_UNORM, RHI_FORMAT_RG8_UNORM, RHI_FORMAT_RGBA8_UNORM, RHI_FORMAT_RGBA8_SRGB,
    RHI_FORMAT_RGBA16_FLOAT, RHI_FORMAT_RGBA32_FLOAT,
    RHI_FORMAT_R32_FLOAT, RHI_FORMAT_RG32_FLOAT,
    RHI_FORMAT_D32_FLOAT, RHI_FORMAT_D24_UNORM_S8_UINT,
    RHI_FORMAT_BC7_UNORM, RHI_FORMAT_ASTC_4x4_UNORM,
} RHI_Format;

typedef enum RHI_TextureUsage {
    RHI_TEXTURE_USAGE_SAMPLED          = 1 << 0,
    RHI_TEXTURE_USAGE_STORAGE          = 1 << 1,
    RHI_TEXTURE_USAGE_COLOR_ATTACHMENT = 1 << 2,
    RHI_TEXTURE_USAGE_DEPTH_ATTACHMENT = 1 << 3,
    RHI_TEXTURE_USAGE_TRANSFER_SRC     = 1 << 4,
    RHI_TEXTURE_USAGE_TRANSFER_DST     = 1 << 5,
} RHI_TextureUsage;

typedef struct RHI_TextureCreateInfo {
    RHI_TextureType   type;
    RHI_Format        format;
    uint32_t          width;
    uint32_t          height;
    uint32_t          depth;
    uint32_t          mipLevels;
    uint32_t          arrayLayers;
    RHI_TextureUsage  usage;
    const char*       name;
} RHI_TextureCreateInfo;

RHI_Texture*       RHI_CreateTexture(RHI_Device* device, const RHI_TextureCreateInfo* info);
RHI_TextureHandle  RHI_RegisterTexture(RHI_BindlessTable* table, RHI_Texture* texture);
void               RHI_DestroyTexture(RHI_Device* device, RHI_Texture* texture);
```

### 6.5 Sampler

```c
typedef enum RHI_Filter { RHI_FILTER_NEAREST, RHI_FILTER_LINEAR };
typedef enum RHI_AddressMode { RHI_ADDRESS_REPEAT, RHI_ADDRESS_CLAMP_TO_EDGE, RHI_ADDRESS_CLAMP_TO_BORDER };

typedef struct RHI_SamplerCreateInfo {
    RHI_Filter    minFilter;
    RHI_Filter    magFilter;
    RHI_AddressMode addressU;
    RHI_AddressMode addressV;
    RHI_AddressMode addressW;
    float         maxAnisotropy;
    float         mipLodBias;
    float         minLod;
    float         maxLod;
} RHI_SamplerCreateInfo;

RHI_Sampler*       RHI_CreateSampler(RHI_Device* device, const RHI_SamplerCreateInfo* info);
RHI_SamplerHandle  RHI_RegisterSampler(RHI_BindlessTable* table, RHI_Sampler* sampler);
void               RHI_DestroySampler(RHI_Device* device, RHI_Sampler* sampler);
```

### 6.6 Shader Module

```c
typedef enum RHI_ShaderStage {
    RHI_SHADER_STAGE_VERTEX,
    RHI_SHADER_STAGE_FRAGMENT,
    RHI_SHADER_STAGE_COMPUTE,
    RHI_SHADER_STAGE_TASK,
    RHI_SHADER_STAGE_MESH,
} RHI_ShaderStage;

typedef struct RHI_ShaderModuleCreateInfo {
    const void*    code;           // SPIRV / DXIL / MSL / WGSL binary
    size_t         codeSize;
    RHI_ShaderStage stage;
    const char*    entryPoint;
} RHI_ShaderModuleCreateInfo;

RHI_ShaderModule* RHI_CreateShaderModule(RHI_Device* device, const RHI_ShaderModuleCreateInfo* info);
void              RHI_DestroyShaderModule(RHI_Device* device, RHI_ShaderModule* module);
```

### 6.7 Pipeline

```c
typedef struct RHI_GraphicsPipelineCreateInfo {
    RHI_ShaderModule* vertexShader;
    RHI_ShaderModule* fragmentShader;
    RHI_PipelineLayout* layout;

    uint32_t vertexBindingCount;
    RHI_VertexBinding* vertexBindings;
    uint32_t vertexAttributeCount;
    RHI_VertexAttribute* vertexAttributes;

    RHI_RasterizerState  rasterizerState;
    RHI_DepthStencilState depthStencilState;
    RHI_BlendState       blendState;

    uint32_t	renderTargetCount;
    RHI_Format* renderTargetFormats;
    RHI_Format  depthStencilFormat;
} RHI_GraphicsPipelineCreateInfo;

RHI_GraphicsPipeline* RHI_CreateGraphicsPipeline(RHI_Device* device, const RHI_GraphicsPipelineCreateInfo* info);
void                  RHI_DestroyGraphicsPipeline(RHI_Device* device, RHI_GraphicsPipeline* pipeline);

typedef struct RHI_ComputePipelineCreateInfo {
    RHI_ShaderModule* computeShader;
    RHI_PipelineLayout* layout;
} RHI_ComputePipelineCreateInfo;

RHI_ComputePipeline* RHI_CreateComputePipeline(RHI_Device* device, const RHI_ComputePipelineCreateInfo* info);
void                 RHI_DestroyComputePipeline(RHI_Device* device, RHI_ComputePipeline* pipeline);

typedef struct RHI_MeshPipelineCreateInfo {
    RHI_ShaderModule* taskShader;      // optional
    RHI_ShaderModule* meshShader;      // required
    RHI_ShaderModule* fragmentShader;
    RHI_PipelineLayout* layout;
    RHI_RasterizerState  rasterizerState;
    RHI_DepthStencilState depthStencilState;
    RHI_BlendState       blendState;
    uint32_t	renderTargetCount;
    RHI_Format* renderTargetFormats;
    RHI_Format  depthStencilFormat;
} RHI_MeshPipelineCreateInfo;

RHI_MeshPipeline* RHI_CreateMeshPipeline(RHI_Device* device, const RHI_MeshPipelineCreateInfo* info);
void              RHI_DestroyMeshPipeline(RHI_Device* device, RHI_MeshPipeline* pipeline);
```

### 6.8 Bind Group (Legacy → Deprecated)

```c
RHI_BindGroup* RHI_CreateBindGroup(RHI_Device* device, const RHI_BindGroupDesc* desc);
void RHI_BindGroup_SetBuffer(RHI_BindGroup* group, uint32_t binding, RHI_Buffer* buffer, uint64_t offset, uint64_t range);
void RHI_BindGroup_SetTexture(RHI_BindGroup* group, uint32_t binding, RHI_Texture* texture);
void RHI_BindGroup_SetSampler(RHI_BindGroup* group, uint32_t binding, RHI_Sampler* sampler);
void RHI_DestroyBindGroup(RHI_Device* device, RHI_BindGroup* group);
```

Will be deprecated once Bindless is the default path.

### 6.9 Bindless Table

```c
typedef struct RHI_BindlessTableCreateInfo {
    uint32_t maxTextures;
    uint32_t maxBuffers;
    uint32_t maxSamplers;
} RHI_BindlessTableCreateInfo;

RHI_BindlessTable* RHI_CreateBindlessTable(RHI_Device* device, const RHI_BindlessTableCreateInfo* info);
void               RHI_DestroyBindlessTable(RHI_Device* device, RHI_BindlessTable* table);

RHI_TextureHandle RHI_BindlessTableAddTexture(RHI_BindlessTable* table, RHI_Texture* texture);
RHI_BufferHandle  RHI_BindlessTableAddBuffer(RHI_BindlessTable* table, RHI_Buffer* buffer);
RHI_SamplerHandle RHI_BindlessTableAddSampler(RHI_BindlessTable* table, RHI_Sampler* sampler);

void RHI_BindlessTableRemoveTexture(RHI_BindlessTable* table, RHI_TextureHandle handle);
void RHI_BindlessTableRemoveBuffer(RHI_BindlessTable* table, RHI_BufferHandle handle);
void RHI_BindlessTableRemoveSampler(RHI_BindlessTable* table, RHI_SamplerHandle handle);

void RHI_CmdBindBindlessTable(RHI_CommandBuffer* cmd, RHI_BindlessTable* table);
```

Rules:

1. **All** textures, buffers, samplers go through the bindless table.
2. `RHI_TextureHandle` / `RHI_BufferHandle` / `RHI_SamplerHandle` are the only ways to reference resources at runtime.
3. Materials store handles, not GPU objects.
4. Push constants pass handles (encoded generational indices), not bindings.
5. `RHI_BindGroup` exists only for legacy compatibility and will be removed.
6. Handle recycling uses free-list with generation counters — stale handles fail `RHI_IsHandleValid()`.

### 6.10 Command Pool & Command Buffer

```c
typedef struct RHI_CommandPoolCreateInfo {
    RHI_QueueType queueType;
    bool          transient;
    bool          resetCommandBuffer;
} RHI_CommandPoolCreateInfo;

RHI_CommandPool*   RHI_CreateCommandPool(RHI_Device* device, const RHI_CommandPoolCreateInfo* info);
void               RHI_DestroyCommandPool(RHI_Device* device, RHI_CommandPool* pool);
void               RHI_ResetCommandPool(RHI_CommandPool* pool);

RHI_CommandBuffer* RHI_AllocateCommandBuffer(RHI_CommandPool* pool);
void               RHI_BeginCommandBuffer(RHI_CommandBuffer* cmd);
void               RHI_EndCommandBuffer(RHI_CommandBuffer* cmd);

// Convenience: single-thread shortcut (internally uses a default pool)
RHI_CommandBuffer* RHI_AcquireCommandBuffer(RHI_Device* device, RHI_QueueType queue);
```

Drawing commands:

```c
// Render pass
void RHI_CmdBeginRenderPass(RHI_CommandBuffer* cmd, const RHI_RenderPassDesc* desc);
void RHI_CmdEndRenderPass(RHI_CommandBuffer* cmd);

// Bind
void RHI_CmdBindGraphicsPipeline(RHI_CommandBuffer* cmd, RHI_GraphicsPipeline* pipeline);
void RHI_CmdBindComputePipeline(RHI_CommandBuffer* cmd, RHI_ComputePipeline* pipeline);
void RHI_CmdBindMeshPipeline(RHI_CommandBuffer* cmd, RHI_MeshPipeline* pipeline);
void RHI_CmdBindBindGroup(RHI_CommandBuffer* cmd, uint32_t set, RHI_BindGroup* group);
void RHI_CmdBindBindlessTable(RHI_CommandBuffer* cmd, RHI_BindlessTable* table);

// Vertex / Index
void RHI_CmdBindVertexBuffer(RHI_CommandBuffer* cmd, uint32_t slot, RHI_Buffer* buffer, uint64_t offset);
void RHI_CmdBindIndexBuffer(RHI_CommandBuffer* cmd, RHI_Buffer* buffer, uint64_t offset, RHI_IndexFormat format);

// Push constants
void RHI_CmdPushConstants(RHI_CommandBuffer* cmd, uint32_t offset, uint32_t size, const void* data);

// Draw
void RHI_CmdDraw(RHI_CommandBuffer* cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
void RHI_CmdDrawIndexed(RHI_CommandBuffer* cmd, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

// Indirect
void RHI_CmdDrawIndirect(RHI_CommandBuffer* cmd, RHI_Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride);
void RHI_CmdDrawIndexedIndirect(RHI_CommandBuffer* cmd, RHI_Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride);
void RHI_CmdDrawIndirectCount(RHI_CommandBuffer* cmd, RHI_Buffer* indirectBuffer, uint64_t indirectOffset, RHI_Buffer* countBuffer, uint64_t countOffset, uint32_t maxDrawCount, uint32_t stride);
void RHI_CmdDispatchIndirect(RHI_CommandBuffer* cmd, RHI_Buffer* buffer, uint64_t offset);

// Compute
void RHI_CmdDispatch(RHI_CommandBuffer* cmd, uint32_t x, uint32_t y, uint32_t z);

// Mesh
void RHI_CmdDrawMeshTasks(RHI_CommandBuffer* cmd, uint32_t x, uint32_t y, uint32_t z);

// Barriers
void RHI_CmdBarrier(RHI_CommandBuffer* cmd, const RHI_BarrierDesc* barriers, uint32_t count);

// Copy
void RHI_CmdCopyBuffer(RHI_CommandBuffer* cmd, RHI_Buffer* src, uint64_t srcOffset, RHI_Buffer* dst, uint64_t dstOffset, uint64_t size);
void RHI_CmdCopyBufferToTexture(RHI_CommandBuffer* cmd, RHI_Buffer* src, RHI_Texture* dst, const RHI_BufferTextureCopyRegion* regions, uint32_t count);
void RHI_CmdGenerateMipmaps(RHI_CommandBuffer* cmd, RHI_Texture* texture);
```

### 6.11 Synchronization

```c
// Fence (legacy binary)
RHI_Fence* RHI_CreateFence(RHI_Device* device, bool signaled);
void       RHI_WaitForFence(RHI_Device* device, RHI_Fence* fence, uint64_t timeoutNs);
void       RHI_ResetFence(RHI_Device* device, RHI_Fence* fence);
void       RHI_DestroyFence(RHI_Device* device, RHI_Fence* fence);

// Timeline Semaphore (preferred)
RHI_Semaphore* RHI_CreateTimelineSemaphore(RHI_Device* device, uint64_t initialValue);
uint64_t       RHI_GetSemaphoreValue(RHI_Semaphore* semaphore);
void           RHI_WaitSemaphore(RHI_Device* device, RHI_Semaphore* semaphore, uint64_t value, uint64_t timeoutNs);
void           RHI_SignalSemaphore(RHI_Device* device, RHI_Semaphore* semaphore, uint64_t value);
void           RHI_DestroySemaphore(RHI_Device* device, RHI_Semaphore* semaphore);
```

Backend mapping:

| Vulkan (`VkSemaphoreType::TIMELINE`) | D3D12 (`ID3D12Fence`) | Metal (`MTLEvent` + spin) |

---

## 7. Deferred Destruction

All resource destruction is deferred until the GPU has finished using the resource.

```c
typedef struct RHI_DeferredFree {
    void*    resource;       // opaque native handle
    uint64_t frameNumber;    // safe to free after this frame completes
    uint32_t type;           // RHIDeferredType enum
} RHI_DeferredFree;
```

```c
void RHI_DestroyBuffer(RHI_Device* device, RHI_Buffer* buffer) {
    // 1. Remove from bindless table (if registered)
    // 2. Invalidation — slot generation increment in the global slot array
    // 3. Push to deferred queue with current frame + N (typically N = 2 or 3)
    RHI_DeferFree(device, buffer->native, device->frameCounter + RHI_MAX_FRAMES_IN_FLIGHT);
}
```

The deferred queue is drained at the start of each frame:

```c
void RHI_ProcessDeferredQueue(RHI_Device* device) {
    uint64_t completedFrame = RHI_GetCompletedFrame(device);
    while (queue.len > 0 && queue.front().frameNumber <= completedFrame) {
        RHI_NativeFree(queue.front().resource, queue.front().type);
        queue.pop();
    }
}
```

---

## 8. Synchronization Model

### 8.1 Three-Queue Architecture

| Queue Type       | Purpose                    | Typical Workload                       |
|-----------------|----------------------------|----------------------------------------|
| Graphics Queue   | Main rendering pipeline    | Draw calls, rasterization, ROP         |
| Compute Queue    | Async compute              | Culling, particles, post-process, AI   |
| Transfer Queue   | Background data upload     | Texture streaming, buffer copies       |

```
 ┌─────────────┐     ┌─────────────┐     ┌──────────────┐
 │   Graphics   │────▶│   Compute    │────▶│   Transfer   │
 │    Queue      │     │    Queue     │     │    Queue      │
 └─────────────┘     └─────────────┘     └──────────────┘
   Semaphore            Semaphore            Semaphore
```

### 8.2 Hardware Differences

**AMD (ACE — Asynchronous Compute Engine):**
- Independent hardware compute engines, fully parallel with graphics
- Up to 8 async compute queues
- Best for long-running compute (e.g., GI, streaming)

**NVIDIA (Dynamic SM Allocation):**
- Graphics and compute share the same SM pool
- Driver dynamically partitions SM count
- Best for short compute tasks (e.g., post-process, culling)

**Implementation implication:** The RHI does not expose hardware details directly. The Render Framework's scheduler tunes workload placement based on a detected GPU profile.

### 8.3 Submit & Sync Example

```c
RHI_Semaphore* comp_sem = RHI_CreateTimelineSemaphore(device, 0);

// Record compute
RHI_CommandBuffer* comp_cb = RHI_AcquireCommandBuffer(device, RHI_QUEUE_COMPUTE);
RHI_BeginCommandBuffer(comp_cb);
RHI_CmdBindComputePipeline(comp_cb, cull_pipeline);
RHI_CmdDispatch(comp_cb, ...);
RHI_CmdBarrier(comp_cb, &uav_barrier, 1);
RHI_EndCommandBuffer(comp_cb);

RHI_SubmitInfo comp_submit = {
    .commandBuffers = &comp_cb,
    .commandBufferCount = 1,
    .signalSemaphores = &comp_sem,
    .signalValues = (uint64_t[]){100},
    .signalSemaphoreCount = 1,
};
RHI_QueueSubmit(compute_queue, &comp_submit, NULL);

// Record graphics (wait compute)
RHI_CommandBuffer* gfx_cb = RHI_AcquireCommandBuffer(device, RHI_QUEUE_GRAPHICS);
RHI_BeginCommandBuffer(gfx_cb);
RHI_CmdBindGraphicsPipeline(gfx_cb, render_pipeline);
RHI_CmdDraw(gfx_cb, ...);
RHI_EndCommandBuffer(gfx_cb);

RHI_SubmitInfo gfx_submit = {
    .commandBuffers = &gfx_cb,
    .commandBufferCount = 1,
    .waitSemaphores = &comp_sem,
    .waitValues = (uint64_t[]){100},
    .waitSemaphoreCount = 1,
};
RHI_QueueSubmit(graphics_queue, &gfx_submit, NULL);
```

---

## 9. Multi-Threaded Command Recording

### 9.1 Architecture

```
 Thread 0 (Main)          Thread 1               Thread 2
 ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
 │  CommandPool0 │     │  CommandPool1 │     │  CommandPool2 │
 │  ┌──────────┐│     │  ┌──────────┐│     │  ┌──────────┐│
 │  │  CB 0    ││     │  │  CB 1    ││     │  │  CB 2    ││
 │  │ shadow   ││     │  │ gbuffer  ││     │  │ compute  ││
 │  └──────────┘│     │  └──────────┘│     │  └──────────┘│
 └──────┬───────┘     └──────┬───────┘     └──────┬───────┘
        │                     │                     │
        ▼                     ▼                     ▼
        └─────────────┬───────┴─────────────────────┘
                      │
                      ▼
              Submit([CB0, CB1, CB2])
              on main thread
```

### 9.2 Rules

1. Each rendering thread holds its own `RHI_CommandPool` (wrapping `SDL_GPUCommandPool`).
2. `RHI_CommandBuffer` acquired from a pool is **not** thread-safe — only the owning thread records.
3. Pools are **not** thread-safe — one pool per thread.
4. After all threads finish recording, command buffers are collected and submitted on the main thread via `RHI_QueueSubmit()`.
5. Submit order determines GPU execution order within the same queue.
6. Cross-queue submissions follow the semaphore-based sync model.
7. Per-frame `RHI_ResetCommandPool()` is O(1) on Vulkan/D3D12 — always prefer pool reset over individual buffer reset.

### 9.3 Per-Thread Pool Management

```c
typedef struct RHI_ThreadContext {
    RHI_CommandPool*   commandPool;
    RHI_CommandBuffer* commandBuffers[RHI_MAX_CBS_PER_THREAD];
    uint32_t           currentBufferIndex;
} RHI_ThreadContext;

// Thread-local storage
static RHI_THREAD_LOCAL RHI_ThreadContext* tl_context = NULL;

RHI_ThreadContext* RHI_GetThreadContext(RHI_Device* device) {
    if (!tl_context) {
        tl_context = RHI_Alloc(sizeof(RHI_ThreadContext));
        tl_context->commandPool = RHI_CreateCommandPool(device, &(RHI_CommandPoolCreateInfo){
            .queueType = RHI_QUEUE_GRAPHICS,
            .transient = true,
            .resetCommandBuffer = false,
        });
    }
    return tl_context;
}
```

### 9.4 Multi-Threaded Usage Example

```c
// Main thread
RHI_CommandPool* worker_pools[4];
for (int i = 0; i < 4; i++) {
    worker_pools[i] = RHI_CreateCommandPool(device, &pool_info);
}

RHI_CommandBuffer* cmds[4];
for (int i = 0; i < 4; i++) {
    ThreadPool_Submit(RecordPass, worker_pools[i], &cmds[i]);
}
ThreadPool_WaitAll();

RHI_SubmitInfo submit = {
    .commandBuffers = cmds,
    .commandBufferCount = 4,
};
RHI_QueueSubmit(graphics_queue, &submit, fence);

RHI_WaitForFence(device, fence, UINT64_MAX);
for (int i = 0; i < 4; i++) {
    RHI_ResetCommandPool(worker_pools[i]);
}

// Worker thread
void RecordPass(RHI_CommandPool* pool, RHI_CommandBuffer** out_cmd) {
    RHI_CommandBuffer* cmd = RHI_AllocateCommandBuffer(pool);
    RHI_BeginCommandBuffer(cmd);
    RHI_CmdBindGraphicsPipeline(cmd, pipeline);
    RHI_CmdBindVertexBuffer(cmd, vb, 0);
    RHI_CmdDraw(cmd, ...);
    RHI_EndCommandBuffer(cmd);
    *out_cmd = cmd;
}
```

---

## 10. Shader Pipeline

### 10.1 Compilation Flow

```
     .slang source
          │
          ▼
   ┌──────────────┐
   │  Slang Compiler │
   └──────┬───────┘
          │
    ┌─────┼─────┬─────┬─────┐
    ▼     ▼     ▼     ▼     ▼
  SPIRV  DXIL  MSL  WGSL  Reflection
    │     │     │     │     │
    ▼     ▼     ▼     ▼     ▼
  Vulkan D3D12 Metal Web  .reflection.json
```

**SDL_shadercross is NOT used.** Slang directly emits all target formats。

### 10.2 Reflection Flow

```
     .slang source
          │
          ▼
   Slang Reflection API
          │
    ┌─────┼─────┐
    ▼     ▼     ▼
  Material   Inspector  Agent
  Schema     UI Auto    Generation
             Generate
```

Key invariant: **Slang is the single source of truth** for both compilation and reflection. No SPIRV-Cross, no DXIL reflection, no intermediate lossy transformation.

### 10.3 Asset Schema

```json
{
    "source": "pbr.slang",
    "entry_points": ["vertMain", "fragMain"],
    "defines": {},
    "variants": []
}
```

### 10.4 Compiled Output on Disk

```
pbr.spv             // Vulkan
pbr.dxil            // D3D12
pbr.msl             // Metal
pbr.wgsl            // WebGPU
pbr.reflection.json // Material schema + Agent metadata
```

### 10.5 Reflection JSON Example

```json
{
  "entryPoints": [
    {
      "name": "VSMain",
      "stage": "vertex",
      "parameters": [
        {"name": "gTransform", "type": "ConstantBuffer<TransformData>", "binding": 0, "set": 0}
      ]
    }
  ],
  "types": [
    {
      "name": "TransformData",
      "fields": [
        {"name": "modelMatrix",   "type": "float4x4", "offset": 0},
        {"name": "viewProjMatrix", "type": "float4x4", "offset": 64}
      ]
    }
  ]
}
```

---

## 11. Bindless Architecture

```
┌──────────────────────────────────────────┐
│               Bindless Table              │
│                                           │
│  [0] Texture ─── SDL_GPUTexture*          │
│  [1] Texture ─── SDL_GPUTexture*          │
│  [2] Buffer  ─── SDL_GPUBuffer*           │
│  ...                                      │
│  [N] Sampler ─── SDL_GPUSampler*          │
│                                           │
│  Dispatched as single descriptor set /    │
│  argument buffer / descriptor heap        │
└──────────────────────────────────────────┘
```

Rules (same as §6.9, restated for clarity):

1. **All** textures, buffers, samplers go through the bindless table.
2. `TextureHandle` / `BufferHandle` / `SamplerHandle` are generational indices.
3. Materials store handles, not GPU objects.
4. Push constants pass the handle's `.index` field (low 24 bits); the generation is validated on the CPU side before dispatch.
5. `RHI_BindGroup` exists only for legacy compatibility and will be removed.
6. Deregistered slots have generation incremented — stale indices fail lookup.

---

## 12. GPU Driven Pipeline

```
 CPU Thread                   GPU Compute Queue              GPU Graphics Queue
     │                              │                              │
     ├─ Upload Scene Data ──────────┤                              │
     │  (Meshlets, Bounds, etc.)    │                              │
     │                              ├─ Frustum Culling             │
     │                              ├─ Occlusion Culling           │
     │                              ├─ LOD Selection               │
     │                              ├─ Generate DrawIndirect       │
     │                              │   Commands                   │
     │                              └─ Signal Semaphore ───────────┤
     │                                                             ├─ ExecuteIndirect
     │                                                             ├─ Render Meshlets
     └─ Present ◄──────────────────────────────────────────────────┘
```

Key techniques:
- **Indirect Draw:** `SDL_DrawGPUIndirectCount` / `ExecuteIndirect`
- **Compute Shader Culling:** Frustum + occlusion on GPU
- **Meshlet Rendering:** 64–128 triangle clusters, fine-grained GPU culling

---

## 13. Async Transfer Ring Buffer

### 13.1 Architecture

```
 CPU Thread                Transfer Queue           Graphics Queue
     │                           │                        │
     ├─ Map Ring Buffer          │                        │
     ├─ Memcpy Texture Data      │                        │
     ├─ Unmap                    │                        │
     ├─ Submit Transfer Cmd ─────┤                        │
     │                           ├─ DMA Copy              │
     │                           └─ Signal Semaphore ─────┤
     │                                                    ├─ Wait Semaphore
     │                                                    ├─ Use Texture
     └─ Continue Rendering ◄─────────────────────────────┘
```

### 13.2 Implementation

```c
#define RHI_RING_BUFFER_SIZE (256 * 1024 * 1024)

typedef struct RHI_TransferRing {
    RHI_Buffer*  staging;
    uint64_t     head;
    uint64_t     tail;
    uint64_t     capacity;
    RHI_Semaphore* semaphore;
    uint64_t     counter;
} RHI_TransferRing;

RHI_TransferRing* RHI_CreateTransferRing(RHI_Device* device) {
    RHI_TransferRing* ring = RHI_Alloc(sizeof(RHI_TransferRing));
    ring->staging = RHI_CreateBuffer(device, &(RHI_BufferCreateInfo){
        .size = RHI_RING_BUFFER_SIZE,
        .usage = RHI_BUFFER_USAGE_TRANSFER_SRC,
        .memoryType = RHI_MEMORY_HOST_VISIBLE,
        .name = "TransferRing"
    });
    ring->capacity = RHI_RING_BUFFER_SIZE;
    ring->head = 0;
    ring->tail = 0;
    ring->semaphore = RHI_CreateTimelineSemaphore(device, 0);
    ring->counter = 0;
    return ring;
}

void RHI_UploadTexture(RHI_TransferRing* ring, RHI_Texture* dst,
                        uint32_t mipLevel, const void* data, size_t size) {
    uint64_t offset = ring->head % ring->capacity;
    void* mapped = RHI_MapBuffer(ring->staging, offset, size);
    memcpy(mapped, data, size);
    RHI_UnmapBuffer(ring->staging);

    RHI_CommandBuffer* cmd = RHI_AcquireCommandBuffer(ring->device, RHI_QUEUE_TRANSFER);
    RHI_CmdCopyBufferToTexture(cmd, ring->staging, dst,
        &(RHI_BufferTextureCopyRegion){ .bufferOffset = offset, .mipLevel = mipLevel }, 1);

    RHI_SubmitInfo submit = {
        .commandBuffers = &cmd,
        .commandBufferCount = 1,
        .signalSemaphores = &ring->semaphore,
        .signalValues = &(uint64_t){ ++ring->counter },
        .signalSemaphoreCount = 1,
    };
    RHI_QueueSubmit(ring->transferQueue, &submit, NULL);

    ring->head += size;
}
```

---

## 14. Render Graph System

### 14.1 Three-Phase Execution Model

**Phase 1: Setup (single-threaded)**

```c
RHI_RenderGraph* graph = RHI_CreateRenderGraph(device);

RHI_PassHandle shadow  = RHI_AddPass(graph, "ShadowMap",  RHI_PASS_GRAPHICS);
RHI_PassHandle gbuffer = RHI_AddPass(graph, "GBuffer",    RHI_PASS_GRAPHICS);
RHI_PassHandle ssao    = RHI_AddPass(graph, "SSAO",       RHI_PASS_COMPUTE);
RHI_PassHandle lighting = RHI_AddPass(graph, "Lighting",  RHI_PASS_COMPUTE);

RHI_ResourceHandle shadowMap  = RHI_CreateTransientTexture(graph, "ShadowMap",  2048, 2048, RHI_FORMAT_D32_FLOAT);
RHI_ResourceHandle gbufAlbedo = RHI_CreateTransientTexture(graph, "GBufferAlbedo", 1920, 1080, RHI_FORMAT_RGBA8_UNORM);
RHI_ResourceHandle depthBuf   = RHI_CreateTransientTexture(graph, "DepthBuf", 1920, 1080, RHI_FORMAT_D32_FLOAT);

RHI_PassWrite(shadow, shadowMap);
RHI_PassRead(gbuffer, shadowMap);
RHI_PassWrite(gbuffer, gbufAlbedo);
RHI_PassWrite(gbuffer, depthBuf);
RHI_PassRead(ssao, depthBuf);
RHI_PassRead(lighting, gbufAlbedo);
RHI_PassRead(lighting, shadowMap);
```

**Phase 2: Compile (single-threaded)**

- **Topological sort** — DAG-based pass ordering
- **Dead code elimination** — remove passes not contributing to output
- **Lifetime analysis** — first/last use frame for each transient resource
- **Memory aliasing** — non-overlapping resources share physical memory

**Phase 3: Execute (multi-threaded)**

```c
void RHI_ExecuteRenderGraph(RHI_RenderGraph* graph, RHI_Device* device) {
    RHI_PassBatch* batches = RHI_GetParallelBatches(graph);

    for (uint32_t i = 0; i < batches->count; i++) {
        RHI_PassBatch* batch = &batches->batches[i];

        for (uint32_t j = 0; j < batch->passCount; j++) {
            ThreadPool_Submit(RecordPass, batch->passes[j]);
        }

        ThreadPool_WaitAll();

        RHI_QueueSubmit(device->graphicsQueue, &batch->submitInfo, NULL);
    }
}
```

### 14.2 Resource Aliasing

Non-overlapping transient resources share physical GPU memory.

```
 Timeline:
   ShadowPass ───────┐
   GBufferPass  ─────┼──────┐
   SSAOPass           │      ├──────┐
   LightingPass       │      │      ├──────
                       │      │      │
 Resource Lifetime:    │      │      │
   ShadowMap (D32)    ─┴──────┘      │
   DepthBuf  (D32)          ─────────┴──────
                                       │
 Aliased Memory:                        │
   Block A (16MB):                     │
     [0-8MB] ← ShadowMap               │
     [0-8MB] ← DepthBuf (reuse)       ─┘
```

```c
RHI_ResourceHandle res1 = RHI_CreateAliasedTexture(graph, "Temp1", 1024, 1024, RHI_FORMAT_RGBA16_FLOAT);
RHI_ResourceHandle res2 = RHI_CreateAliasedTexture(graph, "Temp2", 1024, 1024, RHI_FORMAT_RGBA16_FLOAT);
RHI_CompileGraph(graph); // detects non-overlapping lifetimes → same physical memory
```

Performance reference (Frostbite data): transient VRAM reduced from 3.2 GB to 1.6 GB (50% saving), reduced fragmentation.

---

## 15. Data-Driven Model

### 15.1 Asset Schema

Everything is an asset. Everything is serializable. Everything is editable by agents.

```json
// shader.json
{
    "type": "shader",
    "source": "pbr.slang",
    "entry_points": ["vertMain", "fragMain"]
}

// material.json
{
    "type": "material",
    "shader": "pbr",
    "properties": {
        "baseColor":  { "type": "texture", "value": "hero_d"   },
        "normalMap":  { "type": "texture", "value": "hero_n"   },
        "roughness":  { "type": "float",   "value": 0.5        },
        "metallic":   { "type": "float",   "value": 0.0        }
    }
}

// render_pass.json
{
    "type": "render_pass",
    "name": "gbuffer",
    "attachments": ["gbuffer0", "gbuffer1", "depth"],
    "shader": "gbuffer"
}

// render_graph.json
{
    "type": "render_graph",
    "passes": ["depth_prepass", "gbuffer", "shading", "postprocess"],
    "resources": {
        "gbuffer0":  { "format": "RGBA8",  "usage": "color"   },
        "gbuffer1":  { "format": "RGBA8",  "usage": "color"   },
        "depth":     { "format": "D32F",   "usage": "depth"   },
        "hdr_color": { "format": "RGBA16F", "usage": "color"  }
    }
}
```

### 15.2 Agent Bridge

```
  LLM / Agent
      │
      ▼
  Reflection Metadata
  (shader params, material schema, graph topology)
      │
      ├──▶ Generate render_graph.json
      ├──▶ Generate material.json
      ├──▶ Generate shader variants
      └──▶ Generate pipeline configs
```

No hardcoded render paths. Agents can compose, modify, and iterate on the full rendering pipeline at the asset level.

---

## 16. Module Structure

```
rhi/
├── docs/
│   ├── arch.md
│   ├── roadmap.md
│   └── research/
├── modules/
│   ├── 3rd/
│   │   ├── sdl/              # SDL3 fork (sdl_enjin_rhi)
│   │   └── slang/            # Slang fork (slang_enjin)
│   ├── rhi/                  # RHI C API + implementation
│   │   ├── include/
│   │   │   └── rhi/
│   │   │       ├── rhi.h         # unified public header
│   │   │       ├── defs.h        # enums, structs, handles
│   │   │       ├── device.h
│   │   │       ├── buffer.h
│   │   │       ├── texture.h
│   │   │       ├── sampler.h
│   │   │       ├── pipeline.h
│   │   │       ├── bind_group.h
│   │   │       ├── bindless.h
│   │   │       ├── command_buffer.h
│   │   │       ├── command_pool.h
│   │   │       ├── fence.h
│   │   │       ├── semaphore.h
│   │   │       ├── swapchain.h
│   │   │       └── reflection.h
│   │   └── src/
│   │       ├── device.c
│   │       ├── buffer.c
│   │       ├── texture.c
│   │       ├── ...
│   │       └── deferred.c       # deferred destruction queue
│   ├── shader/               # Shader compiler service + reflection
│   │   ├── include/
│   │   │   └── shader/
│   │   │       ├── compiler.h
│   │   │       └── reflection.h
│   │   └── src/
│   ├── render/               # Render framework (separate from RHI)
│   │   ├── render_graph/
│   │   ├── material/
│   │   └── gpu_driven/
│   └── plugins/              # FSR, DLSS, XeSS, etc.
└── tools/
    └── shader_compiler/      # Offline Slang → SPIRV/DXIL/MSL/WGSL
```

---

## 17. SDL3 Fork Policy

- Fork repo: `in-dreaming/SDL` branch `sdl_enjin_rhi`
- Invasive modifications are allowed
- Modifications MUST stay behind SDL GPU internal boundaries
- SDL types MUST NOT leak into `modules/rhi/include/`
- Upstream merges are periodic (quarterly), conflict resolution is expected
- All extensions follow `SDL_GPU*` naming conventions
- Extensions that prove stable should be proposed upstream

---

## 18. API Style Summary

| Aspect          | Decision                                            |
|-----------------|-----------------------------------------------------|
| Language        | C99 for public API; C11/C++17 for internal impl     |
| Handle style    | Generational index (24+8 bit), single `uint32_t`    |
| Type opacity    | All types are PIMPL opaque pointers                 |
| ABI stability   | Yes — struct layouts are internal, public API is C   |
| Error handling  | Callback — `RHI_ErrorCallback`                      |
| Naming          | `RHI_PascalCase` for all public symbols             |
| Thread safety   | Per-thread CommandPool; Device submit is main-only   |
| Lifetime        | Deferred destruction with frame-counter GC          |
