# NextGen Engine RHI 设计文档 V1

## 1. 设计目标

### 目标平台

* Windows
* MacOS
* Linux
* Android
* iOS
* Web

### 图形API

* Vulkan
* Metal
* D3D12
* WebGPU

### 不支持

* OpenGL
* GLES
* D3D11

仅保留兼容层。

---

# 2. 分层架构

```text
Editor
    ↓

Engine Framework
────────────────────────

Scene
Material
RenderGraph
Visibility
GPU Driven
Streaming

────────────────────────

RHI

Buffer
Texture
Sampler
Pipeline
Descriptor
Command

────────────────────────

SDL GPU Extended

────────────────────────

Vulkan
Metal
D3D12
WebGPU
```

原则：

SDL_gpu 仅承担 Device Layer。

---

# 3. SDL GPU 客制化策略

允许侵入式修改 SDL。

Fork SDL3。

建立：

```text
ThirdParty/
    SDL3/
```

长期维护。

原则：

* 不向上暴露 SDL API
* SDL 类型不得出现在 Engine API
* SDL 仅 Backend 使用

例如：

禁止：

```cpp
SDL_GPUTexture*
```

出现在引擎代码。

统一包装：

```cpp
class RHITexture
{
};
```

---

# 4. SDL GPU 扩展规划

## SDL_GPUBindlessTable

新增：

```cpp
SDL_GPUBindlessTable
```

支持：

```cpp
uint32 RegisterTexture(...)
uint32 RegisterBuffer(...)
```

Shader：

```glsl
Textures[index]
Buffers[index]
```

后端：

Vulkan

```cpp
VK_EXT_descriptor_indexing
```

D3D12

```cpp
Descriptor Heap
```

Metal

```cpp
Argument Buffer
```

---

## SDL_GPUReflection

新增：

```cpp
SDL_GPUShaderReflection
```

来源：

Slang Reflection

支持：

```cpp
Texture
Sampler
Buffer
PushConstant
Struct
```

自动生成：

```cpp
Material Layout
```

---

## SDL_GPUPipelineCache

新增：

```cpp
SDL_GPUPipelineCache
```

支持：

```cpp
PSO Binary Cache
```

持久化。

---

## SDL_GPUIndirect

新增：

```cpp
SDL_GPUIndirectBuffer
```

支持：

```cpp
DrawIndirect
DrawIndexedIndirect
DispatchIndirect
```

---

## SDL_GPUMeshPipeline

新增：

```cpp
SDL_GPUMeshPipeline
```

支持：

```cpp
Task Shader
Mesh Shader
```

平台：

* Vulkan Mesh Shader
* D3D12 Mesh Shader
* Metal Mesh Shader

---

## SDL_GPURayTracing

预留接口。

```cpp
SDL_GPURTAccelerationStructure
SDL_GPURTPipeline
```

V1不实现。

---

# 5. RHI设计

RHI只包含最小对象。

---

## Device

```cpp
class RHIDevice
```

职责：

* Resource Creation
* Queue Submit
* Present

---

## Buffer

```cpp
class RHIBuffer
```

类型：

```cpp
Vertex
Index
Uniform
Storage
Indirect
```

---

## Texture

```cpp
class RHITexture
```

支持：

```cpp
2D
3D
Cube
Array
```

---

## Sampler

```cpp
class RHISampler
```

---

## Pipeline

```cpp
class RHIGraphicsPipeline
class RHIComputePipeline
class RHIMeshPipeline
```

---

## Descriptor

仅保留抽象。

```cpp
class RHIBindGroup
```

未来统一为：

```cpp
Bindless
```

模型。

---

## CommandBuffer

```cpp
class RHICommandBuffer
```

支持：

```cpp
Render
Compute
Transfer
```

---

# 6. Render Framework

Render Framework 不属于 RHI。

---

## RenderGraph

```cpp
graph.AddPass(...)
```

编译：

```cpp
Barrier
Aliasing
Lifetime
```

自动推导。

---

## Resource Graph

统一管理：

```cpp
Texture
Buffer
```

生命周期。

---

## Material System

Material 为纯数据。

```json
{
    "shader":"pbr",
    "textures":
    {
        "baseColor":"hero_d"
    }
}
```

---

## Shader System

统一采用：

```text
Slang
```

编译：

```text
SPIRV
MSL
DXIL
WGSL
```

---

## Reflection

通过 Slang 自动生成：

```cpp
Material Layout
Inspector UI
Serialization
```

---

# 7. Bindless架构

核心原则：

资源永远以 Handle 访问。

禁止：

```cpp
Texture*
```

在 Runtime 中传递。

统一：

```cpp
TextureHandle
```

定义：

```cpp
struct TextureHandle
{
    uint32 index;
};
```

Shader：

```glsl
gTextures[handle]
```

访问。

---

# 8. GPU Driven

V1目标：

```cpp
GPU Culling
LOD Selection
Instance Generation
Indirect Draw
```

流程：

```text
Scene
 ↓

Compute

 ↓

DrawIndirect

 ↓

Render
```

CPU不参与Draw列表生成。

---

# 9. 数据驱动

所有渲染配置可序列化。

```json
Shader
Material
Pass
RenderGraph
Pipeline
```

全部Asset化。

Agent可直接生成。

---

# 10. AI Native支持

Reflection系统直接暴露：

```cpp
Texture
Material
Scene
Graph
```

元数据。

允许：

```text
LLM
 ↓

RenderGraph生成

 ↓

Material生成

 ↓

Pipeline生成
```

无需硬编码。
