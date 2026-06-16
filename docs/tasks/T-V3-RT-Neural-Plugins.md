# T-V3: Ray Tracing + Neural + Plugins

**Milestone**: V3 — RT + Neural + Plugins
**Duration**: Month 17–22
**Goal**: RT acceleration structures, cooperative matrix for neural rendering, plugin system with FSR/DLSS/XeSS.

**Demo**: No standalone demo; features integrated into `examples/render_graph` and examples as plugins.

---

## T-V3.0 — Ray Tracing (Month 17–19)

### T-V3.0.1 — SDL_GPUAccelerationStructure
1. Define `SDL_GPUAccelerationStructure` opaque type.
2. Implement BLAS creation:
   - **Vulkan**: `vkCreateAccelerationStructureKHR` + `vkCmdBuildAccelerationStructuresKHR`
   - **D3D12**: `CreateRaytracingAccelerationStructure`
3. Implement TLAS creation (per-frame or on-demand).
4. Implement `SDL_FreeGPUAccelerationStructure`.

### T-V3.0.2 — SDL_GPURTPipeline
1. Define `SDL_GPURTPipelineCreateInfo` with ray-gen / closest-hit / miss / intersection shader modules.
2. Implement `SDL_CreateGPURTPipeline`:
   - **Vulkan**: `vkCreateRayTracingPipelinesKHR`
   - **D3D12**: `CreateStateObject` with `D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE`
3. Implement `SDL_TraceGPURays(cb, width, height)`.

### T-V3.0.3 — RHI Wrappers
1. `RHI_CreateAccelerationStructure`, `RHI_DestroyAccelerationStructure`.
2. `RHI_CreateRTPipeline`, `RHI_DestroyRTPipeline`.
3. `RHI_CmdDispatchRays`.

### T-V3.0.4 — Shader Binding Table
1. SBT layout utility: compute offsets for ray-gen / hit / miss groups.
2. `RHI_CreateSBTable(pipeline, entries)` — allocate + fill GPU buffer.
3. Bind SBT before `RHI_CmdDispatchRays`.

### T-V3.0.5 — Hybrid Rendering Demo
1. Extend `examples/render_graph` with RT pass:
   - Rasterize G-buffer in existing passes.
   - Add RT reflections pass (trace rays against TLAS using G-buffer normals + world pos).
   - Add RT shadows pass (trace rays from world pos to light).
2. Toggle RT on/off to compare.

### T-V3.0.6 — Metal RT (Deferred)
Metal ray tracing API differs significantly (intersection queries vs pipeline). Defer to V3.1 unless early bandwidth available.

### Deliverables
- [ ] Hybrid raster + RT rendering on Vulkan and D3D12
- [ ] RT reflections and shadows visible and correct

---

## T-V3.1 — Neural / AI Features (Month 20–21)

### T-V3.1.1 — Cooperative Matrix
1. Query `VK_KHR_cooperative_matrix` support.
2. Map to:
   - **D3D12**: Wave MMA intrinsics
   - **Metal**: SIMD matrix multiply (`simd_matrix_multiply`)
3. Wrap in `RHI_CooperativeMatrix` utility (thread-group tile helper).

### T-V3.1.2 — Neural Texture Evaluation
1. Write Slang inference shader using cooperative matrix for a small NTC-style decoder.
2. Encode weight tensors into bindless storage buffers.
3. Test: decode a neural-compressed texture, display side-by-side with BC7 original.

### T-V3.1.3 — Tensor Memory Layout (Stub)
1. Reserve `RHI_TensorDescriptor` type with layout enum (row-major, NHWC, etc.).
2. No allocation — placeholder for `VK_KHR_tensor_memory`.

### Deliverables
- [ ] Cooperative matrix shader compiles and runs on Vulkan (NVIDIA + AMD)
- [ ] Neural texture decode produces visibly correct output

---

## T-V3.2 — Plugin System (Month 22)

### T-V3.2.1 — Plugin Interface
1. Define `RHI_Plugin` interface:
   ```c
   typedef struct RHI_Plugin {
       const char* name;
       const char* version;
       bool (*init)(RHI_Device* device);
       void (*shutdown)(void);
       void (*on_render_pass)(RHI_CommandBuffer* cmd, const char* pass_name);
       void (*on_post_process)(RHI_CommandBuffer* cmd);
       void (*on_frame_end)(void);
   } RHI_Plugin;
   ```
2. `RHI_LoadPlugin(device, path)` — dynamic library load (`LoadLibrary` / `dlopen`).
3. `RHI_UnloadPlugin(device, plugin)`.
4. Hook calls at appropriate Render Graph execution points.

### T-V3.2.2 — FSR Plugin
1. Wrap FSR3/4 SDK as `fsr_plugin.dll` / `fsr_plugin.so`.
2. Implement `on_post_process`: run FSR upscaling as Render Graph pass.
3. Expose quality level via data-driven config.

### T-V3.2.3 — DLSS Plugin
1. Wrap DLSS SDK as `dlss_plugin.dll`.
2. Implement `on_post_process`: run DLSS upscaling.
3. Feature gate: disable if NVIDIA GPU not detected.

### T-V3.2.4 — XeSS Plugin
1. Wrap XeSS SDK as `xess_plugin.dll`.
2. Implement `on_post_process`: run XeSS upscaling.

### T-V3.2.5 — Frame Generation Hook
1. Add `on_before_present` hook to plugin interface.
2. DLSS-FG / FSR-FG: insert frame generation in `on_before_present`.
3. AFMF: AMD frame generation via `on_before_present`.

### Deliverables
- [ ] FSR and DLSS plugins compile and load at runtime
- [ ] Toggling between plugins via config works
- [ ] Zero plugin types in `modules/rhi/include/`
- [ ] Frame generation produces smooth doubled frame rate

### References
- `docs/arch.md` — plugin system is post-RHI, lives in `modules/plugins/`
