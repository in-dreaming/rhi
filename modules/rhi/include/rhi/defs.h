#ifndef RHI_DEFS_H
#define RHI_DEFS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
    #ifdef RHI_BUILD_DLL
        #define RHI_API __declspec(dllexport)
    #else
        #define RHI_API __declspec(dllimport)
    #endif
#else
    #define RHI_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Handles                                                             */
/* ------------------------------------------------------------------ */

typedef struct RHI_Handle {
    uint32_t index      : 24;
    uint32_t generation : 8;
} RHI_Handle;

typedef RHI_Handle RHI_BufferHandle;
typedef RHI_Handle RHI_TextureHandle;
typedef RHI_Handle RHI_SamplerHandle;

static inline bool RHI_HandleIsValid(RHI_Handle h)
{
    return h.generation != 0;
}

static inline RHI_Handle RHI_HandleMake(uint32_t index, uint32_t gen)
{
    RHI_Handle h;
    h.index = index;
    h.generation = gen;
    return h;
}

/* ------------------------------------------------------------------ */
/* Opaque object types                                                 */
/* ------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------ */
/* Enums                                                               */
/* ------------------------------------------------------------------ */

typedef enum RHI_Backend {
    RHI_BACKEND_VULKAN = 0,
    RHI_BACKEND_D3D12,
    RHI_BACKEND_METAL,
    RHI_BACKEND_WEBGPU,
} RHI_Backend;

typedef enum RHI_QueueType {
    RHI_QUEUE_GRAPHICS = 0,
    RHI_QUEUE_COMPUTE,
    RHI_QUEUE_TRANSFER,
} RHI_QueueType;

typedef enum RHI_Format {
    RHI_FORMAT_UNDEFINED = 0,
    RHI_FORMAT_R8_UNORM,
    RHI_FORMAT_RG8_UNORM,
    RHI_FORMAT_RGBA8_UNORM,
    RHI_FORMAT_RGBA8_SRGB,
    RHI_FORMAT_BGRA8_UNORM,
    RHI_FORMAT_BGRA8_SRGB,
    RHI_FORMAT_RGBA16_FLOAT,
    RHI_FORMAT_RGBA32_FLOAT,
    RHI_FORMAT_R32_FLOAT,
    RHI_FORMAT_RG32_FLOAT,
    RHI_FORMAT_D16_UNORM,
    RHI_FORMAT_D24_UNORM_S8_UINT,
    RHI_FORMAT_D32_FLOAT,
    RHI_FORMAT_BC7_UNORM,
    RHI_FORMAT_ASTC_4x4_UNORM,
    RHI_FORMAT_COUNT
} RHI_Format;

typedef enum RHI_TextureType {
    RHI_TEXTURE_2D = 0,
    RHI_TEXTURE_3D,
    RHI_TEXTURE_CUBE,
    RHI_TEXTURE_2D_ARRAY,
} RHI_TextureType;

typedef enum RHI_TextureUsage {
    RHI_TEXTURE_USAGE_SAMPLED          = 1 << 0,
    RHI_TEXTURE_USAGE_STORAGE          = 1 << 1,
    RHI_TEXTURE_USAGE_COLOR_ATTACHMENT = 1 << 2,
    RHI_TEXTURE_USAGE_DEPTH_ATTACHMENT = 1 << 3,
    RHI_TEXTURE_USAGE_TRANSFER_SRC     = 1 << 4,
    RHI_TEXTURE_USAGE_TRANSFER_DST     = 1 << 5,
} RHI_TextureUsage;

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
    RHI_MEMORY_DEVICE_LOCAL = 0,
    RHI_MEMORY_HOST_VISIBLE,
    RHI_MEMORY_HOST_COHERENT,
} RHI_MemoryType;

typedef enum RHI_Filter {
    RHI_FILTER_NEAREST = 0,
    RHI_FILTER_LINEAR,
} RHI_Filter;

typedef enum RHI_AddressMode {
    RHI_ADDRESS_REPEAT = 0,
    RHI_ADDRESS_CLAMP_TO_EDGE,
    RHI_ADDRESS_CLAMP_TO_BORDER,
} RHI_AddressMode;

typedef enum RHI_ShaderStage {
    RHI_SHADER_STAGE_VERTEX = 0,
    RHI_SHADER_STAGE_FRAGMENT,
    RHI_SHADER_STAGE_COMPUTE,
    RHI_SHADER_STAGE_TASK,
    RHI_SHADER_STAGE_MESH,
} RHI_ShaderStage;

typedef enum RHI_CompareOp {
    RHI_COMPARE_NEVER = 0,
    RHI_COMPARE_LESS,
    RHI_COMPARE_EQUAL,
    RHI_COMPARE_LESS_EQUAL,
    RHI_COMPARE_GREATER,
    RHI_COMPARE_NOT_EQUAL,
    RHI_COMPARE_GREATER_EQUAL,
    RHI_COMPARE_ALWAYS,
} RHI_CompareOp;

typedef enum RHI_CullMode {
    RHI_CULL_NONE = 0,
    RHI_CULL_FRONT,
    RHI_CULL_BACK,
} RHI_CullMode;

typedef enum RHI_FrontFace {
    RHI_FRONT_CCW = 0,
    RHI_FRONT_CW,
} RHI_FrontFace;

typedef enum RHI_PresentMode {
    RHI_PRESENT_VSYNC = 0,
    RHI_PRESENT_IMMEDIATE,
    RHI_PRESENT_MAILBOX,
} RHI_PresentMode;

typedef enum RHI_SwapchainComposition {
    RHI_SWAPCHAIN_SDR = 0,
    RHI_SWAPCHAIN_SDR_LINEAR,
    RHI_SWAPCHAIN_HDR_EXTENDED,
    RHI_SWAPCHAIN_HDR10_ST2048,
} RHI_SwapchainComposition;

typedef enum RHI_StoreOp {
    RHI_STORE_OP_STORE = 0,
    RHI_STORE_OP_DONTCARE,
} RHI_StoreOp;

typedef enum RHI_LoadOp {
    RHI_LOAD_OP_LOAD = 0,
    RHI_LOAD_OP_CLEAR,
    RHI_LOAD_OP_DONTCARE,
} RHI_LoadOp;

typedef enum RHI_IndexFormat {
    RHI_INDEX_UINT16 = 0,
    RHI_INDEX_UINT32,
} RHI_IndexFormat;

typedef enum RHI_VertexElementFormat {
    RHI_VERTEX_FORMAT_FLOAT = 0,
    RHI_VERTEX_FORMAT_FLOAT2,
    RHI_VERTEX_FORMAT_FLOAT3,
    RHI_VERTEX_FORMAT_FLOAT4,
    RHI_VERTEX_FORMAT_BYTE4,
    RHI_VERTEX_FORMAT_UBYTE4,
    RHI_VERTEX_FORMAT_UBYTE4_NORM,
} RHI_VertexElementFormat;

typedef enum RHI_BlendFactor {
    RHI_BLEND_ZERO = 0,
    RHI_BLEND_ONE,
    RHI_BLEND_SRC_COLOR,
    RHI_BLEND_ONE_MINUS_SRC_COLOR,
    RHI_BLEND_DST_COLOR,
    RHI_BLEND_ONE_MINUS_DST_COLOR,
    RHI_BLEND_SRC_ALPHA,
    RHI_BLEND_ONE_MINUS_SRC_ALPHA,
    RHI_BLEND_DST_ALPHA,
    RHI_BLEND_ONE_MINUS_DST_ALPHA,
} RHI_BlendFactor;

typedef enum RHI_BlendOp {
    RHI_BLEND_OP_ADD = 0,
    RHI_BLEND_OP_SUBTRACT,
    RHI_BLEND_OP_REVERSE_SUBTRACT,
    RHI_BLEND_OP_MIN,
    RHI_BLEND_OP_MAX,
} RHI_BlendOp;

typedef enum RHI_ErrorType {
    RHI_ERROR_VALIDATION = 0,
    RHI_ERROR_OUT_OF_MEMORY,
    RHI_ERROR_DEVICE_LOST,
} RHI_ErrorType;

/* ------------------------------------------------------------------ */
/* Structs                                                             */
/* ------------------------------------------------------------------ */

typedef struct RHI_DeviceCreateInfo {
    RHI_Backend backend;
    bool        enableValidation;
    bool        enableBindless;
    bool        enableMeshShader;
    bool        enableRayTracing;
    const char *applicationName;
    uint32_t    applicationVersion;
} RHI_DeviceCreateInfo;

typedef struct RHI_BufferCreateInfo {
    uint64_t       size;
    RHI_BufferUsage usage;
    RHI_MemoryType  memoryType;
    const char     *name;
} RHI_BufferCreateInfo;

typedef struct RHI_TextureCreateInfo {
    RHI_TextureType   type;
    RHI_Format        format;
    uint32_t          width;
    uint32_t          height;
    uint32_t          depth;
    uint32_t          mipLevels;
    uint32_t          arrayLayers;
    uint32_t          usage;
    const char       *name;
} RHI_TextureCreateInfo;

typedef struct RHI_SamplerCreateInfo {
    RHI_Filter     minFilter;
    RHI_Filter     magFilter;
    RHI_AddressMode addressU;
    RHI_AddressMode addressV;
    RHI_AddressMode addressW;
    float          maxAnisotropy;
    float          mipLodBias;
    float          minLod;
    float          maxLod;
} RHI_SamplerCreateInfo;

typedef struct RHI_SwapchainCreateInfo {
    void     *windowHandle;
    uint32_t  width;
    uint32_t  height;
    bool      vsync;
    RHI_Format format;
} RHI_SwapchainCreateInfo;

typedef struct RHI_ColorTargetInfo {
    RHI_Texture *texture;
    RHI_LoadOp   loadOp;
    RHI_StoreOp  storeOp;
    float        clearR, clearG, clearB, clearA;
} RHI_ColorTargetInfo;

typedef struct RHI_DepthStencilTargetInfo {
    RHI_Texture *texture;
    RHI_LoadOp   loadOp;
    RHI_StoreOp  storeOp;
    float        clearDepth;
    uint8_t      clearStencil;
    bool         stencilLoadOp;
    bool         stencilStoreOp;
} RHI_DepthStencilTargetInfo;

typedef struct RHI_VertexAttribute {
    uint32_t location;
    uint32_t binding;
    RHI_VertexElementFormat format;
    uint32_t offset;
} RHI_VertexAttribute;

typedef struct RHI_VertexBinding {
    uint32_t binding;
    uint32_t stride;
    bool     perInstance;
} RHI_VertexBinding;

typedef struct RHI_BlendState {
    bool            blendEnable;
    RHI_BlendFactor srcColorBlendFactor;
    RHI_BlendFactor dstColorBlendFactor;
    RHI_BlendOp     colorBlendOp;
    RHI_BlendFactor srcAlphaBlendFactor;
    RHI_BlendFactor dstAlphaBlendFactor;
    RHI_BlendOp     alphaBlendOp;
    uint8_t         colorWriteMask;
} RHI_BlendState;

typedef struct RHI_ShaderModuleCreateInfo {
    const void *code;
    size_t      codeSize;
    RHI_ShaderStage stage;
    const char *entryPoint;
} RHI_ShaderModuleCreateInfo;

typedef struct RHI_GraphicsPipelineCreateInfo {
    RHI_ShaderModule *vertexShader;
    RHI_ShaderModule *fragmentShader;
    RHI_PipelineLayout *layout;

    uint32_t vertexBindingCount;
    RHI_VertexBinding *vertexBindings;
    uint32_t vertexAttributeCount;
    RHI_VertexAttribute *vertexAttributes;

    RHI_CullMode      cullMode;
    RHI_FrontFace     frontFace;
    bool              depthTestEnable;
    bool              depthWriteEnable;
    RHI_CompareOp     depthCompareOp;

    RHI_BlendState    blendState;

    uint32_t     renderTargetCount;
    RHI_Format  *renderTargetFormats;
    RHI_Format   depthStencilFormat;
} RHI_GraphicsPipelineCreateInfo;

typedef struct RHI_SubmitInfo {
    RHI_CommandBuffer **commandBuffers;
    uint32_t            commandBufferCount;
    RHI_Semaphore     **waitSemaphores;
    uint64_t           *waitValues;
    uint32_t            waitSemaphoreCount;
    RHI_Semaphore     **signalSemaphores;
    uint64_t           *signalValues;
    uint32_t            signalSemaphoreCount;
} RHI_SubmitInfo;

typedef struct RHI_CommandPoolCreateInfo {
    RHI_QueueType queueType;
    bool          transient;
    bool          resetCommandBuffer;
} RHI_CommandPoolCreateInfo;

typedef void (*RHI_ErrorCallback)(RHI_ErrorType type, const char *message, void *userData);

#define RHI_MAX_FRAMES_IN_FLIGHT 2

#ifdef __cplusplus
}
#endif

#endif
