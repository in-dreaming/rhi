
# 3. C API层接口文档

## 3.1 设计原则

### 3.1.1 世代索引句柄系统（Generational Index）

**目标**：解决ABA问题，提供内存安全的句柄访问。

**句柄结构**：
c
typedef struct RHI_Handle {
    uint32_t index;      // 槽位索引（低24位）
    uint32_t generation; // 世代计数（高8位）
} RHI_Handle;

// 内部槽位结构
typedef struct RHI_Slot {
    void* resource;      // 实际资源指针
    uint32_t generation; // 当前世代
    bool isAlive;        // 是否有效
} RHI_Slot;

// 验证句柄
bool RHI_IsHandleValid(RHI_Handle handle) {
    RHI_Slot* slot = &g_slots[handle.index];
    return slot->isAlive && slot->generation == handle.generation;
}


**优势**：
- **O(1)访问速度**：直接通过索引访问槽位
- **内存安全**：旧句柄因世代不匹配而失效
- **无需引用计数**：简化生命周期管理

### 3.1.2 PIMPL模式与ABI稳定性

**原理**：通过不透明指针隐藏结构体布局，保证二进制兼容性。

**示例**：
c
// 公开头文件（rhi.h）
typedef struct RHI_Device RHI_Device; // 不透明类型

RHI_Device* RHI_CreateDevice(const RHI_DeviceCreateInfo* info);
void RHI_DestroyDevice(RHI_Device* device);

// 内部实现（rhi_internal.h）
struct RHI_Device {
    void* nativeDevice;        // Vulkan VkDevice / D3D12 ID3D12Device
    RHI_Queue* graphicsQueue;
    RHI_Queue* computeQueue;
    RHI_Queue* transferQueue;
    // ... 内部字段可自由修改
};


### 3.1.3 错误处理机制

**回调模式**：
c
typedef enum RHI_ErrorType {
    RHI_ERROR_VALIDATION,      // 验证层错误
    RHI_ERROR_OUT_OF_MEMORY,   // 显存不足
    RHI_ERROR_DEVICE_LOST,     // 设备丢失
} RHI_ErrorType;

typedef void (*RHI_ErrorCallback)(RHI_ErrorType type, const char* message, void* userData);

// 设置错误回调
void RHI_SetErrorCallback(RHI_Device* device, RHI_ErrorCallback callback, void* userData);


## 3.2 核心对象句柄定义

c
// 设备与队列
typedef struct RHI_Device RHI_Device;
typedef struct RHI_Queue RHI_Queue;

// 资源对象
typedef struct RHI_Buffer RHI_Buffer;
typedef struct RHI_Texture RHI_Texture;
typedef struct RHI_Sampler RHI_Sampler;

// 管线对象
typedef struct RHI_ShaderModule RHI_ShaderModule;
typedef struct RHI_PipelineLayout RHI_PipelineLayout;
typedef struct RHI_GraphicsPipeline RHI_GraphicsPipeline;
typedef struct RHI_ComputePipeline RHI_ComputePipeline;

// 命令对象
typedef struct RHI_CommandPool RHI_CommandPool;
typedef struct RHI_CommandBuffer RHI_CommandBuffer;

// 同步对象
typedef struct RHI_Fence RHI_Fence;
typedef struct RHI_Semaphore RHI_Semaphore;

// Bindless扩展
typedef struct RHI_BindlessTable RHI_BindlessTable;


## 3.3 设备与队列管理API

### 3.3.1 设备创建

c
typedef enum RHI_Backend {
    RHI_BACKEND_VULKAN,
    RHI_BACKEND_D3D12,
    RHI_BACKEND_METAL,
    RHI_BACKEND_WEBGPU,
} RHI_Backend;

typedef struct RHI_DeviceCreateInfo {
    RHI_Backend backend;
    bool enableValidation;       // 启用验证层
    bool enableBindless;         // 启用Bindless支持
    bool enableMeshShader;       // 启用Mesh Shader
    bool enableRayTracing;       // 启用光线追踪
    const char* applicationName;
    uint32_t applicationVersion;
} RHI_DeviceCreateInfo;

// 创建设备
RHI_Device* RHI_CreateDevice(const RHI_DeviceCreateInfo* info);

// 销毁设备（等待所有GPU任务完成）
void RHI_DestroyDevice(RHI_Device* device);

// 等待设备空闲
void RHI_DeviceWaitIdle(RHI_Device* device);


### 3.3.2 队列获取

c
typedef enum RHI_QueueType {
    RHI_QUEUE_GRAPHICS,
    RHI_QUEUE_COMPUTE,
    RHI_QUEUE_TRANSFER,
} RHI_QueueType;

// 获取队列
RHI_Queue* RHI_GetQueue(RHI_Device* device, RHI_QueueType type, uint32_t index);

// 队列提交
typedef struct RHI_SubmitInfo {
    RHI_CommandBuffer** commandBuffers;
    uint32_t commandBufferCount;
    
    RHI_Semaphore** waitSemaphores;
    uint64_t* waitValues;           // Timeline Semaphore等待值
    uint32_t waitSemaphoreCount;
    
    RHI_Semaphore** signalSemaphores;
    uint64_t* signalValues;         // Timeline Semaphore信号值
    uint32_t signalSemaphoreCount;
} RHI_SubmitInfo;

void RHI_QueueSubmit(RHI_Queue* queue, const RHI_SubmitInfo* info, RHI_Fence* fence);

// 队列等待
void RHI_QueueWaitIdle(RHI_Queue* queue);


## 3.4 资源创建与管理API

### 3.4.1 Buffer创建

c
typedef enum RHI_BufferUsage {
    RHI_BUFFER_USAGE_VERTEX         = 1 << 0,
    RHI_BUFFER_USAGE_INDEX          = 1 << 1,
    RHI_BUFFER_USAGE_UNIFORM        = 1 << 2,
    RHI_BUFFER_USAGE_STORAGE        = 1 << 3,
    RHI_BUFFER_USAGE_INDIRECT       = 1 << 4,
    RHI_BUFFER_USAGE_TRANSFER_SRC   = 1 << 5,
    RHI_BUFFER_USAGE_TRANSFER_DST   = 1 << 6,
} RHI_BufferUsage;

typedef enum RHI_MemoryType {
    RHI_MEMORY_DEVICE_LOCAL,        // GPU显存
    RHI_MEMORY_HOST_VISIBLE,        // CPU可见内存
    RHI_MEMORY_HOST_COHERENT,       // CPU-GPU一致性内存
} RHI_MemoryType;

typedef struct RHI_BufferCreateInfo {
    uint64_t size;
    RHI_BufferUsage usage;
    RHI_MemoryType memoryType;
} RHI_BufferCreateInfo;

// 创建Buffer
RHI_Buffer* RHI_CreateBuffer(RHI_Device* device, const RHI_BufferCreateInfo* info);

// 销毁Buffer（延迟销毁，等待GPU使用完毕）
void RHI_DestroyBuffer(RHI_Device* device, RHI_Buffer* buffer);

// 映射内存
void* RHI_MapBuffer(RHI_Buffer* buffer, uint64_t offset, uint64_t size);
void RHI_UnmapBuffer(RHI_Buffer* buffer);


### 3.4.2 Texture创建

c
typedef enum RHI_Format {
    RHI_FORMAT_UNDEFINED,
    RHI_FORMAT_R8_UNORM,
    RHI_FORMAT_RGBA8_UNORM,
    RHI_FORMAT_RGBA8_SRGB,
    RHI_FORMAT_RGBA16_FLOAT,
    RHI_FORMAT_RGBA32_FLOAT,
    RHI_FORMAT_D32_FLOAT,
    RHI_FORMAT_D24_UNORM_S8_UINT,
    // ... 更多格式
} RHI_Format;

typedef enum RHI_TextureUsage {
    RHI_TEXTURE_USAGE_SAMPLED           = 1 << 0,
    RHI_TEXTURE_USAGE_STORAGE           = 1 << 1,
    RHI_TEXTURE_USAGE_COLOR_ATTACHMENT  = 1 << 2,
    RHI_TEXTURE_USAGE_DEPTH_ATTACHMENT  = 1 << 3,
    RHI_TEXTURE_USAGE_TRANSFER_SRC      = 1 << 4,
    RHI_TEXTURE_USAGE_TRANSFER_DST      = 1 << 5,
} RHI_TextureUsage;

typedef struct RHI_TextureCreateInfo {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mipLevels;
    uint32_t arrayLayers;
    RHI_Format format;
    RHI_TextureUsage usage;
} RHI_TextureCreateInfo;

// 创建Texture
RHI_Texture* RHI_CreateTexture(RHI_Device* device, const RHI_TextureCreateInfo* info);

// 销毁Texture
void RHI_DestroyTexture(RHI_Device* device, RHI_Texture* texture);


### 3.4.3 Sampler创建

c
typedef enum RHI_Filter {
    RHI_FILTER_NEAREST,
    RHI_FILTER_LINEAR,
} RHI_Filter;

typedef enum RHI_AddressMode {
    RHI_ADDRESS_MODE_REPEAT,
    RHI_ADDRESS_MODE_CLAMP_TO_EDGE,
    RHI_ADDRESS_MODE_CLAMP_TO_BORDER,
} RHI_AddressMode;

typedef struct RHI_SamplerCreateInfo {
    RHI_Filter minFilter;
    RHI_Filter magFilter;
    RHI_AddressMode addressModeU;
    RHI_AddressMode addressModeV;
    RHI_AddressMode addressModeW;
    float maxAnisotropy;
} RHI_SamplerCreateInfo;

// 创建Sampler
RHI_Sampler* RHI_CreateSampler(RHI_Device* device, const RHI_SamplerCreateInfo* info);

// 销毁Sampler
void RHI_DestroySampler(RHI_Device* device, RHI_Sampler* sampler);


## 3.5 命令录制与提交API

### 3.5.1 Command Pool管理

c
typedef struct RHI_CommandPoolCreateInfo {
    RHI_QueueType queueType;
    bool transient;              // 短寿命缓冲区优化
    bool resetCommandBuffer;     // 允许单独重置CommandBuffer
} RHI_CommandPoolCreateInfo;

// 创建Command Pool（每个线程一个）
RHI_CommandPool* RHI_CreateCommandPool(RHI_Device* device, const RHI_CommandPoolCreateInfo* info);

// 重置Command Pool（O(1)复杂度）
void RHI_ResetCommandPool(RHI_CommandPool* pool);

// 销毁Command Pool
void RHI_DestroyCommandPool(RHI_Device* device, RHI_CommandPool* pool);


### 3.5.2 Command Buffer录制

c
// 分配Command Buffer
RHI_CommandBuffer* RHI_AllocateCommandBuffer(RHI_CommandPool* pool);

// 开始录制
void RHI_BeginCommandBuffer(RHI_CommandBuffer* cmd);

// 结束录制
void RHI_EndCommandBuffer(RHI_CommandBuffer* cmd);

// 绑定管线
void RHI_CmdBindGraphicsPipeline(RHI_CommandBuffer* cmd, RHI_GraphicsPipeline* pipeline);
void RHI_CmdBindComputePipeline(RHI_CommandBuffer* cmd, RHI_ComputePipeline* pipeline);

// 绑定资源
void RHI_CmdBindVertexBuffer(RHI_CommandBuffer* cmd, RHI_Buffer* buffer, uint64_t offset);
void RHI_CmdBindIndexBuffer(RHI_CommandBuffer* cmd, RHI_Buffer* buffer, uint64_t offset);

// 绘制命令
void RHI_CmdDraw(RHI_CommandBuffer* cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
void RHI_CmdDrawIndexed(RHI_CommandBuffer* cmd, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

// 间接绘制
void RHI_CmdDrawIndirect(RHI_CommandBuffer* cmd, RHI_Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride);
void RHI_CmdDrawIndirectCount(RHI_CommandBuffer* cmd, RHI_Buffer* buffer, uint64_t offset, RHI_Buffer* countBuffer, uint64_t countOffset, uint32_t maxDrawCount, uint32_t stride);

// 计算调度
void RHI_CmdDispatch(RHI_CommandBuffer* cmd, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
void RHI_CmdDispatchIndirect(RHI_CommandBuffer* cmd, RHI_Buffer* buffer, uint64_t offset);

// 资源拷贝
void RHI_CmdCopyBuffer(RHI_CommandBuffer* cmd, RHI_Buffer* src, RHI_Buffer* dst, uint64_t srcOffset, uint64_t dstOffset, uint64_t size);
void RHI_CmdCopyBufferToTexture(RHI_CommandBuffer* cmd, RHI_Buffer* src, RHI_Texture* dst, uint32_t mipLevel);


## 3.6 同步原语API

### 3.6.1 Fence

c
// 创建Fence
RHI_Fence* RHI_CreateFence(RHI_Device* device, bool signaled);

// 等待Fence
void RHI_WaitForFence(RHI_Device* device, RHI_Fence* fence, uint64_t timeout);

// 重置Fence
void RHI_ResetFence(RHI_Device* device, RHI_Fence* fence);

// 销毁Fence
void RHI_DestroyFence(RHI_Device* device, RHI_Fence* fence);


### 3.6.2 Timeline Semaphore

c
// 创建Timeline Semaphore
RHI_Semaphore* RHI_CreateTimelineSemaphore(RHI_Device* device, uint64_t initialValue);

// 获取当前值
uint64_t RHI_GetSemaphoreValue(RHI_Semaphore* semaphore);

// CPU端等待
void RHI_WaitSemaphore(RHI_Device* device, RHI_Semaphore* semaphore, uint64_t value, uint64_t timeout);

// CPU端信号
void RHI_SignalSemaphore(RHI_Device* device, RHI_Semaphore* semaphore, uint64_t value);

// 销毁Semaphore
void RHI_DestroySemaphore(RHI_Device* device, RHI_Semaphore* semaphore);


## 3.7 Bindless资源绑定API

c
// 创建Bindless表
typedef struct RHI_BindlessTableCreateInfo {
    uint32_t maxTextures;
    uint32_t maxBuffers;
    uint32_t maxSamplers;
} RHI_BindlessTableCreateInfo;

RHI_BindlessTable* RHI_CreateBindlessTable(RHI_Device* device, const RHI_BindlessTableCreateInfo* info);

// 注册资源并返回句柄
uint32_t RHI_BindlessTableAddTexture(RHI_BindlessTable* table, RHI_Texture* texture);
uint32_t RHI_BindlessTableAddBuffer(RHI_BindlessTable* table, RHI_Buffer* buffer);
uint32_t RHI_BindlessTableAddSampler(RHI_BindlessTable* table, RHI_Sampler* sampler);

// 移除资源（延迟释放）
void RHI_BindlessTableRemoveTexture(RHI_BindlessTable* table, uint32_t handle);
void RHI_BindlessTableRemoveBuffer(RHI_BindlessTable* table, uint32_t handle);
void RHI_BindlessTableRemoveSampler(RHI_BindlessTable* table, uint32_t handle);

// 绑定到命令缓冲区
void RHI_CmdBindBindlessTable(RHI_CommandBuffer* cmd, RHI_BindlessTable* table);

// 销毁Bindless表
void RHI_DestroyBindlessTable(RHI_Device* device, RHI_BindlessTable* table);


## 3.8 管线创建API

### 3.8.1 Shader Module

c
typedef enum RHI_ShaderStage {
    RHI_SHADER_STAGE_VERTEX,
    RHI_SHADER_STAGE_FRAGMENT,
    RHI_SHADER_STAGE_COMPUTE,
    RHI_SHADER_STAGE_TASK,
    RHI_SHADER_STAGE_MESH,
} RHI_ShaderStage;

typedef struct RHI_ShaderModuleCreateInfo {
    const void* code;            // SPIRV/DXIL/MSL二进制
    size_t codeSize;
    RHI_ShaderStage stage;
    const char* entryPoint;
} RHI_ShaderModuleCreateInfo;

// 创建Shader Module
RHI_ShaderModule* RHI_CreateShaderModule(RHI_Device* device, const RHI_ShaderModuleCreateInfo* info);

// 销毁Shader Module
void RHI_DestroyShaderModule(RHI_Device* device, RHI_ShaderModule* module);


### 3.8.2 Graphics Pipeline

c
typedef struct RHI_GraphicsPipelineCreateInfo {
    RHI_ShaderModule* vertexShader;
    RHI_ShaderModule* fragmentShader;
    RHI_PipelineLayout* layout;
    
    // 顶点输入
    uint32_t vertexBindingCount;
    RHI_VertexBinding* vertexBindings;
    
    // 光栅化状态
    bool depthTestEnable;
    bool depthWriteEnable;
    RHI_CompareOp depthCompareOp;
    
    // 混合状态
    bool blendEnable;
    RHI_BlendFactor srcColorBlendFactor;
    RHI_BlendFactor dstColorBlendFactor;
    
    // ... 其他状态
} RHI_GraphicsPipelineCreateInfo;

// 创建Graphics Pipeline
RHI_GraphicsPipeline* RHI_CreateGraphicsPipeline(RHI_Device* device, const RHI_GraphicsPipelineCreateInfo* info);

// 销毁Graphics Pipeline
void RHI_DestroyGraphicsPipeline(RHI_Device* device, RHI_GraphicsPipeline* pipeline);


### 3.8.3 Compute Pipeline

c
typedef struct RHI_ComputePipelineCreateInfo {
    RHI_ShaderModule* computeShader;
    RHI_PipelineLayout* layout;
} RHI_ComputePipelineCreateInfo;

// 创建Compute Pipeline
RHI_ComputePipeline* RHI_CreateComputePipeline(RHI_Device* device, const RHI_ComputePipelineCreateInfo* info);

// 销毁Compute Pipeline
void RHI_DestroyComputePipeline(RHI_Device* device, RHI_ComputePipeline* pipeline);


## 3.9 多线程使用示例

c
// 主线程
void MainThread() {
    RHI_Device* device = RHI_CreateDevice(&deviceInfo);
    RHI_Queue* graphicsQueue = RHI_GetQueue(device, RHI_QUEUE_GRAPHICS, 0);
    
    // 为每个工作线程创建Command Pool
    RHI_CommandPool* workerPools[4];
    for (int i = 0; i < 4; i++) {
        workerPools[i] = RHI_CreateCommandPool(device, &poolInfo);
    }
    
    // 分发任务到工作线程
    RHI_CommandBuffer* cmds[4];
    for (int i = 0; i < 4; i++) {
        ThreadPool_Submit(RecordCommands, workerPools[i], &cmds[i]);
    }
    ThreadPool_WaitAll();
    
    // 提交所有命令缓冲区
    RHI_SubmitInfo submitInfo = {
        .commandBuffers = cmds,
        .commandBufferCount = 4,
    };
    RHI_QueueSubmit(graphicsQueue, &submitInfo, fence);
    
    // 下一帧重置所有Command Pool
    RHI_WaitForFence(device, fence, UINT64_MAX);
    for (int i = 0; i < 4; i++) {
        RHI_ResetCommandPool(workerPools[i]);
    }
}

// 工作线程
void RecordCommands(RHI_CommandPool* pool, RHI_CommandBuffer** outCmd) {
    RHI_CommandBuffer* cmd = RHI_AllocateCommandBuffer(pool);
    RHI_BeginCommandBuffer(cmd);
    
    // 录制绘制命令
    RHI_CmdBindGraphicsPipeline(cmd, pipeline);
    RHI_CmdBindVertexBuffer(cmd, vertexBuffer, 0);
    RHI_CmdDraw(cmd, vertexCount, 1, 0, 0);
    
    RHI_EndCommandBuffer(cmd);
    *outCmd = cmd;
}
