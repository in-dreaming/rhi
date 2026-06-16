# SDL3 GPU API 陷阱与差异备忘

V0 Bootstrap 实践中踩过的坑。后续开发必读。

---

## 1. Device 创建

```c
// ❌ 错误：旧假设，传入驱动索引
SDL_CreateGPUDevice(driverIndex, debug, NULL);

// ✅ 正确：传入着色器格式标志位 + 驱动名
SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;  // Vulkan
SDL_GPUDevice *dev = SDL_CreateGPUDevice(format, debug_mode, "vulkan");
```

**后端映射**：

| RHI_Backend | 驱动名 | ShaderFormat |
|---|---|---|
| VULKAN | `"vulkan"` | `SDL_GPU_SHADERFORMAT_SPIRV` |
| D3D12 | `"direct3d12"` | `SDL_GPU_SHADERFORMAT_DXIL` |
| METAL | `"metal"` | `SDL_GPU_SHADERFORMAT_MSL` |

Device 内部结构新增 `shaderFormat` 字段，供 `SDL_CreateGPUShader` 使用。

---

## 2. 纹理格式枚举名

SDL3 不遵循 Vulkan 的 `_SFLOAT` 后缀，使用 `_FLOAT`。

| RHI_Format | SDL3 枚举 |
|---|---|
| RGBA16_FLOAT | `SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT` ✅ ~~_SFLOAT~~ ❌ |
| RGBA32_FLOAT | `SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT` ✅ |
| R32_FLOAT | `SDL_GPU_TEXTUREFORMAT_R32_FLOAT` ✅ |
| D32_FLOAT | `SDL_GPU_TEXTUREFORMAT_D32_FLOAT` ✅ ~~_SFLOAT~~ ❌ |
| BC7_UNORM | `SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM` ✅ ~~_UNORM~~ ❌ |
| BGRA8_UNORM | `SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM` ✅ |

---

## 3. Swapchain 纹理获取

```c
// ❌ 错误：没有随时可用的 swapchain 纹理
SDL_GPUTexture *tex = SDL_AcquireGPUSwapchainTexture(device, window);

// ✅ 正确：必须通过 CommandBuffer 获取
SDL_GPUTexture *swapTex = NULL;
SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuffer, window, &swapTex, &w, &h);
```

在 RHI 层的实现：调用 `RHI_SetSwapchainCommandBuffer` 关联当前命令缓冲区，再调用 `RHI_GetSwapchainTexture` 获取纹理。返回的 `RHI_Texture*` 是临时分配的，用完需 `free()`。

---

## 4. Present 流程

SDL3 中没有显式 Present API。提交与 swapchain 关联的 CommandBuffer 即自动呈现：

```c
SDL_SubmitGPUCommandBuffer(cmdBuffer);  // 自动 present
```

`RHI_Present()` 因此是空操作。

---

## 5. Buffer 数据上传

SDL3 没有 "HOST_VISIBLE + VERTEX" 的组合。必须走 Transfer Buffer 流程：

```text
1. 创建 SDL_GPUTransferBuffer (UPLOAD) + 映射写入数据
2. 创建 SDL_GPUBuffer (VERTEX/INDEX/etc.)
3. BeginCopyPass → UploadToGPUBuffer → EndCopyPass
4. SubmitCommandBuffer
5. WaitForGPUIdle (确保上传完成后再渲染)
```

RHI 封装为 `RHI_CmdUploadBuffer`：

```c
RHI_TransferBufferLocation src = { .transferBuffer = uploadBuf, .offset = 0 };
RHI_BufferRegion dst = { .buffer = gpuBuf, .offset = 0, .size = dataSize };
RHI_CmdUploadBuffer(cmd, &src, &dst);
```

会在首次调用时自动开启 CopyPass，在 `RHI_CmdBeginRenderPass` 时自动关闭。

---

## 6. Shader 绑定计数

`SDL_GPUShaderCreateInfo` 的 `num_samplers`/`num_uniform_buffers` 必须与着色器实际使用严格一致，否则触发断言：

```
Assertion failure: '!"Missing vertex sampler binding!"'
```

目前简单着色器（三角形）设为 `0`。后续 Pipeline Layout 系统会自动从 Slang Reflection 中提取。

---

## 7. CompareOp 枚举差异

```c
// Vulkan: VK_COMPARE_OP_LESS_OR_EQUAL
// SDL3:
SDL_GPU_COMPAREOP_LESS_OR_EQUAL   // ✅ 不是 _LESS_EQUAL
```

---

## 8. DepthStencilState 字段名

```c
// ❌ 错误
ds.depth_test_enable = true;

// ✅ 正确
ds.enable_depth_test = true;
ds.enable_depth_write = true;
```

---

## 9. Shader Stage 枚举

SDL3 只有 `VERTEX` 和 `FRAGMENT`，没有 `COMPUTE`：

```c
// Compute 着色器通过 SDL_CreateGPUComputePipeline 创建，不经过 ShaderStage 枚举
```

临时方案：`RHI_SHADER_STAGE_COMPUTE` 映射到 `VERTEX` 作为占位。

---

## 10. BufferUsage 标志

SDL3 没有 `TRANSFER_SRC`/`TRANSFER_DST`：

```c
SDL_GPU_BUFFERUSAGE_VERTEX
SDL_GPU_BUFFERUSAGE_INDEX
SDL_GPU_BUFFERUSAGE_INDIRECT
SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ
SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ
SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE
```

传输通过 `SDL_GPUTransferBuffer` 独立处理，不走 BufferUsage 路径。

---

## 11. Slang 编译 SPIR-V 版本

Vulkan 1.1 驱动要求 SPIR-V 1.3，slangc 默认输出 1.5：

```bash
# ❌ 默认 spirv_1_5 → Vulkan 验证层报错
slangc -target spirv -entry main tri.slang

# ✅ 指定 spirv_1_3
slangc -target spirv -profile spirv_1_3 -entry main tri.slang
```

---

## 12. Slang SPIR-V 入口点命名

Slang 编译到 SPIR-V 时，无论源码中 entry point 叫什么，输出一律为 `"main"`：

```c
// .slang 中: [shader("vertex")] VSOutput vertMain(VSInput input)
// SPIR-V 中: OpEntryPoint Vertex %main "main"
// 因此 SDL_CreateGPUShader 的 entrypoint 必须是 "main"
```

---

## 13. Slang C++ API 差异（vs. 预期）

| 预期 API | 实际状态 |
|---|---|
| `ISession::getDiagnostics()` | ❌ 不存在 — 通过 `loadModuleFromSourceString` 的 `outDiagnostics` 参数获取 |
| `IEntryPoint::getName()` | ❌ 不存在 — entry point 名称无需运行时获取 |
| `IComponentType::getReflectionJSON()` | ❌ 不存在 — 使用 `-emit-reflection-json` 命令行参数或 `spReflection_GetUnitTestResults` |
| `ComPtr<T>` | 需要 `#include <slang-com-ptr.h>` + `using Slang::ComPtr` |
| `getTargetCode(idx, ...)` | 返回 `SlangResult`，需检查 `SLANG_SUCCEEDED` |
| `createCompositeComponentType` | 需要 `outDiagnostics` 参数 |
