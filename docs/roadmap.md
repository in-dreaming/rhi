# RHI Roadmap V3

## Milestone Overview

```
 V0 ───── V1 ──────────────────────── V1.5 ──────────── V2 ────────────── V3
 │         │                           │                 │                 │
 │         │                           │                 │                 │
 ▼         ▼                           ▼                 ▼                 ▼
Boot    Core RHI                   Mobile/Web       Advanced GPU      Neural &
        + Bindless                 Platform         Features          RT Era
        + MTR
        + Render Graph
```

---

## V0 — Bootstrap (Week 1–4)

**Goal**: Project skeleton compiles, SDL3 + Slang submodules wired, hello-triangle renders.

### Tasks

| # | Task | Detail |
|---|------|--------|
| 0.1 | CMake build system | Multi-module CMake: `rhi`, `shader`, `render`, `plugins` |
| 0.2 | SDL3 submodule init | `modules/3rd/sdl` on `sdl_enjin_rhi`, CMake integrated |
| 0.3 | Slang submodule init | `modules/3rd/slang` on `slang_enjin`, CMake integrated |
| 0.4 | `RHI_Device` bootstrap | Wrap `SDL_GPUDevice` creation, adapter enumeration |
| 0.5 | Hello triangle | `RHI_Buffer` (vertex/index), `RHI_GraphicsPipeline`, `RHI_CommandBuffer`, `RHI_Present` — colored triangle on Windows (Vulkan + D3D12) |
| 0.6 | `RHI_RenderPass` | Begin/End render pass, clear, attachment binding |
| 0.7 | `RHI_Swapchain` | Wrap swapchain creation, acquire, present, resize |
| 0.8 | CI | Windows (Vulkan + D3D12) builds on push |

### Exit Criteria

- [ ] Colored triangle on Vulkan and D3D12 via identical RHI calls
- [ ] Zero SDL types in `modules/rhi/include/`
- [ ] CMake builds cleanly with no warnings on MSVC and Clang

---

## V1 — Core RHI + Bindless + MTR + Render Graph (Month 2–7)

**Goal**: Full RHI abstraction, bindless resource table, multi-threaded command recording, Render Graph system, indirect draw, multi-queue, Slang pipeline. GPU-driven rendering pipeline operational.

### V1.0 — RHI Foundation (Month 2)

| # | Task | Detail |
|---|------|--------|
| 1.01 | `RHI_Buffer` full impl | All usage flags, host-visible vs device-local, `RHI_MapBuffer` / `RHI_UnmapBuffer` |
| 1.02 | `RHI_Texture` full impl | 2D, 3D, Cube, Array; all common formats; mip generation |
| 1.03 | `RHI_Sampler` | All filter / address / LOD modes |
| 1.04 | `RHI_GraphicsPipeline` | Full pipeline state: vertex layout, blend, depth-stencil, rasterizer, render pass compatibility |
| 1.05 | `RHI_ComputePipeline` | Compute-only pipeline creation |
| 1.06 | `RHI_BindGroup` | Descriptor set abstraction (legacy path) |
| 1.07 | `RHI_CommandBuffer` | Full draw, dispatch, copy, barrier commands |
| 1.08 | `RHI_Fence` | Timeline semaphore on Vulkan/D3D12; emulated on Metal |
| 1.09 | Resource barriers | Layout transitions, UAV barriers, aliasing barriers |
| 1.10 | macOS + Metal | Full Metal backend via SDL GPU, all above RHI objects working |
| 1.11 | Deferred destruction | Frame-counter GC queue; `RHI_Destroy*`立即invalidate slot + 推入延迟队列 |
| 1.12 | Error callback | `RHI_SetErrorCallback` 集成到device; 验证层错误→回调 |

### V1.1 — Bindless (Month 3)

| # | Task | Detail |
|---|------|--------|
| 1.13 | `SDL_GPUBindlessTable` | Vulkan: `VK_EXT_descriptor_indexing` + unbounded arrays; D3D12: descriptor heap; Metal: Argument Buffer |
| 1.14 | `RHI_BindlessTable` | Wraps SDL extension with generational-index handles |
| 1.15 | Handle recycling | Free-list index reuse; generation counter increment on deregister |
| 1.16 | Slang bindless shader | `ParameterBlock<Texture2D>` arrays, `StructuredBuffer` arrays in `.slang` |
| 1.17 | End-to-end bindless test | Render 100+ textures from bindless table in a single draw |

### V1.2 — Indirect Draw + GPU Driven (Month 3–4)

| # | Task | Detail |
|---|------|--------|
| 1.18 | `SDL_GPUIndirect` | `DrawIndirect`, `DrawIndexedIndirect`, `DispatchIndirect`, `DrawIndirectCount` |
| 1.19 | GPU culling shader | Frustum + occlusion culling compute shader |
| 1.20 | GPU LOD selection | Compute shader selects LOD, writes indirect draw args |
| 1.21 | GPU driven demo | 10k+ instances culled + drawn on GPU, CPU issues single `DrawIndirectCount` |

### V1.3 — Multi-Queue + Async Compute (Month 4–5)

| # | Task | Detail |
|---|------|--------|
| 1.22 | Queue enumeration | Query available Graphics/Compute/Transfer queues |
| 1.23 | Cross-queue sync | Timeline semaphore wait between queues |
| 1.24 | Async compute demo | Overlap compute culling with graphics rendering; profiling data |
| 1.25 | Async transfer ring | `RHI_TransferRing` implementation; background texture upload demo |

### V1.4 — Multi-Threaded Rendering (Month 5)

| # | Task | Detail |
|---|------|--------|
| 1.26 | `SDL_GPUCommandPool` | Per-thread command pool; `SDL_CreateGPUCommandPool`, `SDL_AcquireGPUCommandBufferFromPool`, `SDL_ResetGPUCommandPool` |
| 1.27 | `RHI_CommandPool` | Public C API wrapping SDL extension; `RHI_CreateCommandPool`, `RHI_AllocateCommandBuffer`, `RHI_ResetCommandPool` |
| 1.28 | Per-thread pool management | Thread-local `RHI_ThreadContext`; auto-creation on first use |
| 1.29 | Parallel command recording | Record shadow, gbuffer, compute passes on separate threads; collect + submit on main thread |
| 1.30 | Multi-threaded demo | 3+ threads, measurable frame time reduction vs single-threaded baseline |

### V1.5 — Shader Pipeline (Month 5–6)

| # | Task | Detail |
|---|------|--------|
| 1.31 | Shader compiler service | Offline: `.slang` → `.spv` / `.dxil` / `.msl` / `.wgsl`; runtime: on-demand + cache |
| 1.32 | `SDL_GPUShaderReflection` | Slang reflection → `SDL_GPUShaderReflection` + JSON export |
| 1.33 | `RHI_Reflection` API | Query parameters, types, bindings from reflection data |
| 1.34 | Material schema generation | Auto-generate material property layout from shader reflection |
| 1.35 | Pipeline cache | `SDL_GPUPipelineCache` — serialize/deserialize PSO cache |
| 1.36 | Shader hot-reload | `.slang` change → recompile → recreate pipelines (<1s) |

### V1.6 — Render Graph (Month 6–7)

| # | Task | Detail |
|---|------|--------|
| 1.37 | Render Graph setup phase | `RHI_CreateRenderGraph`, `RHI_AddPass`, `RHI_CreateTransientTexture`, dependency declaration |
| 1.38 | Render Graph compile phase | Topological sort, dead code elimination, lifetime analysis |
| 1.39 | Resource aliasing | Non-overlapping transient resources share physical memory |
| 1.40 | Render Graph execute phase | Multi-threaded pass recording, batched submit integrated with CommandPool |
| 1.41 | Render Graph demo | Full deferred renderer expressed as Render Graph; measure VRAM saving from aliasing |

### V1 Exit Criteria

- [ ] Bindless rendering of 10k+ instances on Vulkan + D3D12 + Metal
- [ ] GPU-driven culling + indirect draw with zero CPU-side draw list
- [ ] Async compute overlap demonstrated with profiling data
- [ ] Async texture upload via Ring Buffer demonstrated
- [ ] Multi-threaded command recording: 3+ threads, measurable frame time reduction
- [ ] Slang compile + reflection + JSON export pipeline fully functional
- [ ] PSO cache reduces startup time by > 50% on replay
- [ ] Shader hot-reload works in < 1 second
- [ ] Render Graph drives a deferred renderer; memory aliasing saves ≥ 30% transient VRAM
- [ ] Deferred destruction — no GPU use-after-free under validation layers over 10k frames
- [ ] Error callback fires on intentional invalid API usage

---

## V1.7 — Mobile + Web (Month 8–10)

**Goal**: Vulkan on Android, Metal on iOS, WebGPU on Web. Feature parity where possible, graceful degradation otherwise.

### Tasks

| # | Task | Detail |
|---|------|--------|
| 1.51 | Android Vulkan | NDK build, Android window + swapchain via SDL |
| 1.52 | iOS Metal | Xcode project, Metal validation, UIApplication lifecycle |
| 1.53 | Web/WebGPU | Emscripten build, WebGPU via SDL, browser compat |
| 1.54 | Feature degradation | Metal argument buffer size limits → multi-tier table; WebGPU indirect support matrix → feature query fallback |
| 1.55 | Mobile GPU profiling | RenderDoc / Xcode GPU capture |
| 1.56 | Memory budget | Device-dependent limits; streaming texture resolution scaling |

### V1.7 Exit Criteria

- [ ] GPU-driven bindless demo runs on Android (Vulkan), iOS (Metal), Chrome (WebGPU)
- [ ] Feature degradation documented per platform
- [ ] GPU frame capture works on all desktop platforms

---

## V2 — Advanced GPU Features (Month 11–16)

**Goal**: Mesh shader pipeline, sparse resources/validation, multi-draw, work graph stub, shader object exploration.

### V2.0 — Mesh Shader (Month 11–12)

| # | Task | Detail |
|---|------|--------|
| 2.01 | `SDL_GPUMeshPipeline` | Task + Mesh stages; Vulkan `EXT_mesh_shader`, D3D12 mesh shader, Metal mesh pipeline |
| 2.02 | `RHI_MeshPipeline` | Wraps SDL extension; `RHI_CmdDrawMeshTasks` command |
| 2.03 | Cluster rendering demo | Meshlet-based rendering with GPU cluster culling (100k+ meshlets) |
| 2.04 | Fallback | No mesh shader → compute-based meshlet culling + indirect draw |

### V2.1 — Sparse Resources (Month 13–14)

| # | Task | Detail |
|---|------|--------|
| 2.05 | Sparse texture | `VK_KHR_sparse_binding` + `D3D12_TILED_RESOURCE` + `MTLHeapTypeAutomatic` |
| 2.06 | Virtual texture system | Feedback-driven mip residency; page table on GPU; integrate with bindless table |
| 2.07 | Sparse buffer | Streaming vertex/index data with sparse binding |
| 2.08 | Integration | Virtual texture handles in material system (bindless handle → sparse texture) |

### V2.2 — Multi Draw + Stubs (Month 15–16)

| # | Task | Detail |
|---|------|--------|
| 2.09 | Multi-draw indirect | `DrawIndirectCount` integrated into Render Graph passes |
| 2.10 | Work Graph stub | Type definitions for `SDL_GPUWorkGraph`; reserved API slot; no implementation |
| 2.11 | Shader Object stub | Evaluate `VK_EXT_shader_object`; conditional inclusion backend query |

### V2 Exit Criteria

- [ ] Meshlet cluster rendering demo with GPU culling (100k+ meshlets)
- [ ] Virtual texture streaming with < 4 MB resident for a 16k×16k texture
- [ ] Multi-draw indirect reducing draw calls to < 10 for a complex scene
- [ ] Work Graph and Shader Object stubs compile and are documented

---

## V3 — Ray Tracing + Neural + Plugins (Month 17–22)

**Goal**: RT acceleration structure, cooperative matrix/tensor for neural rendering, plugin system for FSR/DLSS/XeSS.

### V3.0 — Ray Tracing (Month 17–19)

| # | Task | Detail |
|---|------|--------|
| 3.01 | `SDL_GPUAccelerationStructure` | BLAS/TLAS on Vulkan (`VK_KHR_ray_tracing_pipeline`) and D3D12 (`D3D12_RAYTRACING`) |
| 3.02 | `SDL_GPURTPipeline` | Ray-gen / closest-hit / miss / intersection stages |
| 3.03 | `RHI_AccelerationStructure` + `RHI_RTPipeline` | RHI wrappers; `RHI_CmdDispatchRays` |
| 3.04 | Shader binding table | SBT layout + management utilities |
| 3.05 | Hybrid rendering demo | Rasterize G-buffer + RT reflections/shadows |
| 3.06 | Metal RT | Metal ray tracing via intersection queries (deferred) |

### V3.1 — Neural / AI Features (Month 20–21)

| # | Task | Detail |
|---|------|--------|
| 3.07 | Cooperative Matrix | `VK_KHR_cooperative_matrix` → D3D12 Wave MMA, Metal SIMD matrix |
| 3.08 | Cooperative Vector | Evaluate `VK_KHR_cooperative_vector` — skip if unavailable |
| 3.09 | Neural texture eval | Slang inference shader using cooperative matrix for NTC / neural material |
| 3.10 | Tensor memory layout | Reserved descriptors for future `VK_KHR_tensor_memory` |

### V3.2 — Plugin System (Month 22)

| # | Task | Detail |
|---|------|--------|
| 3.11 | Plugin interface | `RHI_Plugin` base; `OnRenderPass`, `OnPostProcess`, `OnFrameEnd` hooks |
| 3.12 | FSR plugin | FSR3/4 as Render Graph pass plugin |
| 3.13 | DLSS plugin | DLSS SDK as Render Graph pass plugin |
| 3.14 | XeSS plugin | XeSS as Render Graph pass plugin |
| 3.15 | Frame Generation | `RHI_Plugin` hook for DLSS-FG / FSR-FG / AFMF |

### V3 Exit Criteria

- [ ] Hybrid raster + RT demo on Vulkan and D3D12
- [ ] Cooperative matrix inference shader on Vulkan (NVIDIA + AMD)
- [ ] FSR and DLSS plugins toggleable via data-driven config
- [ ] Zero plugin types in core RHI headers

---

## Feature Priority Reference

```
P0 ─── Must Have (V1)
  ★★★★★ Bindless + Descriptor Indexing
  ★★★★★ Indirect Draw + Multi Draw
  ★★★★★ Async Compute (Multi Queue)
  ★★★★★ Timeline Semaphore
  ★★★★★ Slang Pipeline (Compile + Reflection)
  ★★★★★ Multi-Threaded Command Recording
  ★★★★★ Render Graph (Setup / Compile / Execute + Aliasing)
  ★★★★★ Generational Index Handles
  ★★★★★ Deferred Destruction
  ★★★★★ Error Callback

P1 ─── One Year (V2)
  ★★★★☆ Mesh Shader
  ★★★★☆ Sparse Resource / Virtual Texture
  ★★★★☆ Multi Draw Indirect Count

P2 ─── Reserved (V2–V3)
  ★★★☆☆ Work Graph
  ★★★☆☆ Shader Object
  ★★★☆☆ Ray Tracing

P3 ─── Plugin (V3)
  ★★☆☆☆ FSR
  ★★☆☆☆ DLSS
  ★★☆☆☆ XeSS
  ★★☆☆☆ Frame Generation

P4 ─── AI Era (V3+)
  ★☆☆☆☆ Cooperative Matrix
  ★☆☆☆☆ Tensor Core API
  ★☆☆☆☆ Neural Compression
  ★☆☆☆☆ Neural Texture
```

---

## Release Cadence

| Milestone | Target Date | Key Deliverable |
|-----------|------------|-----------------|
| V0 | Month 1 | Hello triangle, build system, CI |
| V1.0 | Month 2 | Full RHI objects, 3 desktop backends |
| V1.1 | Month 3 | Bindless table functional |
| V1.2 | Month 4 | GPU driven demo |
| V1.3 | Month 5 | Async compute + transfer ring |
| V1.4 | Month 5 | Multi-threaded rendering |
| V1.5 | Month 6 | Slang pipeline + reflection + cache |
| V1.6 | Month 7 | Render Graph + aliasing |
| **V1** | **Month 7** | **Core RHI complete** |
| V1.7 | Month 10 | Mobile + Web platforms |
| V2.0 | Month 12 | Mesh shader |
| V2.1 | Month 14 | Sparse resources |
| V2.2 | Month 16 | Multi draw + stubs |
| **V2** | **Month 16** | **Advanced GPU features** |
| V3.0 | Month 19 | Ray tracing |
| V3.1 | Month 21 | Neural / AI features |
| V3.2 | Month 22 | Plugin system |
| **V3** | **Month 22** | **Full feature stack** |

---

## Dependency Graph

```
V0 (Bootstrap)
  │
  ├──► V1.0 (RHI Foundation)
  │       │
  │       ├──► V1.1 (Bindless)
  │       │       │
  │       │       └──► V1.2 (GPU Driven) ──► V1.3 (Async Compute + Transfer)
  │       │                                         │
  │       ├──► V1.4 (Multi-Threaded) ──────────────┤
  │       │                                         │
  │       └──► V1.5 (Slang Pipeline) ──────────────┤
  │                                                 │
  │       ┌────────────────────────────────────────┘
  │       ▼
  │     V1.6 (Render Graph)
  │       │
  │       ▼
  │    V1.7 (Mobile + Web)
  │       │
  │       ▼
  │    V2.0 (Mesh Shader)
  │       │
  │       ├──► V2.1 (Sparse Resources)
  │       │
  │       └──► V2.2 (Multi Draw + Stubs)
  │               │
  │               ▼
  │            V3.0 (Ray Tracing)
  │               │
  │               ├──► V3.1 (Neural / AI)
  │               │
  │               └──► V3.2 (Plugin System)
```

---

## Risk Register

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| SDL3 upstream breaks fork | Build failure | Medium | Pin submodule commit; quarterly merge; regression tests |
| SDL3 command pool extension conflicts with upstream | Fork divergence | Medium | Isolate patch behind `SDL_GPUCommandPool` namespace; upstream proposal |
| Slang backend bugs (MSL/WGSL) | Shader compilation failure | Medium | Report upstream; carry patches in `slang_enjin`; SPIRV-Cross fallback for MSL only |
| Metal argument buffer size limits | Bindless table overflow | High | Multi-tier table; feature degradation per platform |
| WebGPU feature gaps | Features unavailable | High | Feature query API; runtime fallback path with warning |
| Render Graph compile latency | Frame spike | Low | Cache compiled graphs; incremental recompile on graph change |
| Resource aliasing bugs | Visual corruption | Medium | Validation mode: aliasing disabled, unique memory per resource |
| Cooperative matrix availability | Neural features limited | Low | Fallback to compute; skip P4 on unsupported hardware |
| RT API complexity blows schedule | V3 delays | Medium | RT is P2; can slip to V3.1+ without blocking other work |
| Deferred destruction frame-latency tuning | GPU memory spike | Low | Configurable `RHI_MAX_FRAMES_IN_FLIGHT` (default 2); profiling mode for tuning |
