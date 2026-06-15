如果从 **2026 年新引擎** 的视角来看，Bindless、FSR、DLSS 其实只是冰山一角。

更准确地说，你应该先思考：

```text
哪些能力必须进入 RHI
哪些能力进入 Render Framework
哪些能力属于可插拔扩展
```

很多引擎失败的原因是把一堆厂商特性塞进 RHI。

实际上：

```text
RHI ≠ Feature Layer
```

---

# 第一层：必须进入 RHI 的现代特性

这些是未来一定会影响 API 设计的。

## 1. Bindless

这个已经不是高级特性了。

实际上：

```text
Bindless = 新时代基础设施
```

涉及：

```cpp
TextureHandle
BufferHandle
SamplerHandle
```

Shader：

```cpp
gTextures[index]
```

而不是：

```cpp
set=0 binding=1
```

---

建议：

```text
V1直接做
```

---

## 2. Descriptor Indexing

Bindless 底层依赖。

Vulkan：

```cpp
VK_EXT_descriptor_indexing
```

D3D12：

```cpp
Descriptor Heap
```

Metal：

```cpp
Argument Buffer
```

---

## 3. Indirect Draw

现代 GPU Driven 基础。

必须支持：

```cpp
DrawIndirect
DrawIndexedIndirect
DispatchIndirect
```

---

## 4. Multi Draw

例如：

```cpp
vkCmdDrawIndirectCount
```

对应：

```cpp
ExecuteIndirect
```

---

## 5. Timeline Semaphore

建议暴露。

否则未来：

```text
Render
Compute
Streaming
```

同步会痛苦。

---

## 6. Async Compute

未来 AI、压缩、Streaming 都会用。

RHI 要有：

```cpp
Graphics Queue
Compute Queue
Transfer Queue
```

抽象。

---

# 第二层：强烈建议扩展

这些不是基础设施。

但未来大概率会用。

---

## 7. Mesh Shader

对应：

```cpp
DispatchMesh()
```

---

适用：

```text
Nanite
Cluster Rendering
GPU Driven Scene
```

---

我认为：

```text
2026 新引擎必须预留
```

---

## 8. Work Graph

微软近两年重点。

对应：

```cpp
D3D12 Work Graph
```

---

未来：

```text
GPU任务调度
GPU Agent
```

可能很重要。

---

目前：

```text
预留即可
```

---

## 9. Sparse Resource

也叫：

```text
Virtual Texture
Tiled Resource
Sparse Image
```

---

用于：

```text
Mega Texture
Volume Data
Neural Texture
```

---

建议：

```text
V2加入
```

---

## 10. Shader Object

Vulkan：

```cpp
VK_EXT_shader_object
```

---

未来可能替代部分 Pipeline。

---

先观察。

---

# 第三层：不要放进RHI

很多人喜欢往 RHI 塞这些。

其实不应该。

---

## 11. FSR

AMD 的：

```text
FSR2
FSR3
FSR4
```

---

本质：

```text
Render Pass
```

---

属于：

```text
PostProcess
```

层。

---

不属于：

```text
RHI
```

---

## 12. DLSS

NVIDIA：

```text
DLSS
DLAA
DLSS Ray Reconstruction
```

---

本质：

```text
SDK
```

---

应该：

```text
Plugin
```

形式。

---

## 13. XeSS

同理。

---

## 14. Frame Generation

包括：

```text
DLSS FG
FSR FG
AFMF
```

---

属于：

```text
Display Pipeline
```

层。

---

# 第四层：未来可能爆发

你做 AI Native Engine，我认为这批更值得关注。

---

## 15. Cooperative Matrix

Vulkan：

```cpp
VK_KHR_cooperative_matrix
```

---

对应：

```text
Tensor Core
Matrix Core
```

---

未来：

```text
神经压缩
神经材质
推理
```

都会用。

---

我认为：

```text
比RT重要
```

---

## 16. Cooperative Vector

新提案。

---

用于：

```text
Transformer
ML Inference
```

GPU执行。

---

## 17. Tensor Memory

未来神经渲染关键。

---

## 18. Neural Texture Compression

趋势越来越明显。

例如：

```text
NTC
RTXNTC
Neural Material
```

---

未来可能替代：

```text
BC7
ASTC
```

部分场景。

---

# 第五层：光追

很多人第一反应：

```text
RT
```

---

但我反而建议：

```text
最后做
```

---

因为：

```text
RT API 非常复杂
```

涉及：

```cpp
BLAS
TLAS
SBT
```

---

而你目前目标：

```text
工具链
AI编辑器
Data Driven
跨平台
```

RT收益很低。

---

# 如果是我给你的优先级

```text
P0（必须）

★★★★★ Bindless
★★★★★ Descriptor Indexing
★★★★★ Indirect Draw
★★★★★ Async Compute
★★★★★ Timeline Semaphore

----------------

P1（一年内）

★★★★ Mesh Shader
★★★★ Sparse Resource
★★★★ Multi Draw

----------------

P2（预留）

★★★ Work Graph
★★★ Shader Object
★★★ Ray Tracing

----------------

P3（插件）

FSR
DLSS
XeSS
Frame Generation

----------------

P4（AI时代）

Cooperative Matrix
Tensor Core API
Neural Compression
Neural Texture
```

对于你这个引擎，我其实会把：

```text
Bindless
+
GPU Driven
+
Cooperative Matrix
```

看得比：

```text
DLSS
RT
```

重要得多。

因为前者会影响整个架构设计，而后者基本都可以在 Render Framework 甚至 Plugin 层后加进去。
