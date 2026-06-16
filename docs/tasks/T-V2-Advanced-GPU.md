# T-V2: Advanced GPU Features

**Milestone**: V2 — Advanced GPU Features
**Duration**: Month 11–16
**Goal**: Mesh shader pipeline, sparse resources / virtual texture, multi-draw, work graph stub, shader object stub.

**Demo**: `examples/vt` (virtual texture)

---

## T-V2.0 — Mesh Shader (Month 11–12)

### T-V2.0.1 — SDL_GPUMeshPipeline Extension
1. Define `SDL_GPUMeshPipelineCreateInfo` with optional task + required mesh + fragment shaders.
2. Implement `SDL_CreateGPUMeshPipeline` per `docs/arch.md` §5.5.
3. Implement `SDL_DrawGPUMeshTasks(cb, x, y, z)`.
4. Backend implementations:
   - **Vulkan**: `vkCmdDrawMeshTasksEXT` / `vkCmdDrawMeshTasksNV`
   - **D3D12**: `DispatchMesh`
   - **Metal**: `drawMeshThreadgroups`
5. Feature gate: `RHI_QueryFeature(device, RHI_FEATURE_MESH_SHADER)`.

### T-V2.0.2 — RHI_MeshPipeline
1. Wrap SDL extension in C API.
2. `RHI_CreateMeshPipeline`, `RHI_CmdDrawMeshTasks`.

### T-V2.0.3 — Cluster Rendering Demo
1. Build meshlet pipeline using `examples/gpu_driven` as base.
2. Add GPU cluster culling compute pass (meshlet bounding sphere vs HiZ).
3. Render 100k+ meshlets via mesh shader.
4. HUD: visible cluster count, culling ratio.

### Deliverables
- [ ] Mesh shader renders 100k+ meshlets
- [ ] GPU cluster culling working

---

## T-V2.1 — Sparse Resources / Virtual Texture (Month 13–14)

### T-V2.1.1 — Sparse Texture Backend
1. Implement sparse texture creation:
   - **Vulkan**: `VK_KHR_sparse_binding` + `VkSparseImageMemoryBind`
   - **D3D12**: `D3D12_TILED_RESOURCE` + `UpdateTileMappings`
   - **Metal**: `MTLHeapTypeAutomatic` + `makeAliasable`
2. Implement page residency management: bind/unbind pages based on feedback.

### T-V2.1.2 — Virtual Texture System
1. Feedback pass: render screen-space UVs to feedback buffer.
2. Read back feedback on CPU (async), determine needed mip pages.
3. Page table: GPU-side texture array or buffer, indexed by page coordinates.
4. Stream missing pages via `RHI_TransferRing`.
5. Integration: virtual texture handle in bindless table → page table lookup in shader.

### T-V2.1.3 — Virtual Texture Demo
1. `examples/vt/main.c`:
   - Load a 16k×16k texture (or procedurally generate tiles).
   - Only stream visible mip pages.
   - Display: resident page count, total VRAM used, feedback latency.
2. Verify < 4 MB resident for full 16k texture at standard FOV.

### Acceptance Criteria
- [ ] 16k×16k texture rendered with < 4 MB resident
- [ ] Page-in/pop-in imperceptible at normal speed
- [ ] No invalid page access in shader (black tiles etc.)

### README Entry
```
| vt | Virtual Texture streaming: 16k×16k texture with < 4 MB resident. Feedback-driven page residency via bindless table. |
```

---

## T-V2.2 — Multi Draw + Stubs (Month 15–16)

### T-V2.2.1 — Multi Draw Indirect in Render Graph
1. Integrate `DrawIndirectCount` as a standard Render Graph pass type.
2. Test: complex scene reduced to < 10 draw calls.

### T-V2.2.2 — Work Graph Stub
1. Define `SDL_GPUWorkGraph` type (no implementation).
2. Reserve `RHI_WorkGraph` API slot.
3. Document expected usage pattern.

### T-V2.2.3 — Shader Object Stub
1. Detect `VK_EXT_shader_object` availability at runtime.
2. Define `RHI_ShaderObject` type (no implementation).
3. If backend supports, log; otherwise, skip silently.

### Deliverables
- [ ] DrawIndirectCount in Render Graph passes
- [ ] Work Graph and Shader Object stubs compile
