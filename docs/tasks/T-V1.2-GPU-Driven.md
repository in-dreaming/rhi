# T-V1.2: Indirect Draw + GPU Driven

**Milestone**: V1.2 — Indirect Draw + GPU Driven
**Duration**: Month 3–4
**Goal**: GPU performs culling + LOD selection, writes indirect draw args, CPU issues single DrawIndirectCount.

**Demo**: `examples/gpu_driven`

---

## T-V1.2.1 — SDL_GPUIndirect Extension

### Steps
1. Define `SDL_GPUDrawIndirectCommand`, `SDL_GPUDrawIndexedIndirectCommand` structs in SDL headers.
2. Implement `SDL_DrawGPUIndirect(cb, buffer, offset, drawCount, stride)`.
3. Implement `SDL_DrawGPUIndexedIndirect(cb, buffer, offset, drawCount, stride)`.
4. Implement `SDL_DrawGPUIndirectCount(cb, indirectBuffer, indirectOffset, countBuffer, countOffset, maxDrawCount, stride)`.
5. Implement `SDL_DispatchGPUIndirect(cb, buffer, offset)`.
6. Backend implementations:
   - **Vulkan**: `vkCmdDrawIndirect`, `vkCmdDrawIndexedIndirect`, `vkCmdDrawIndirectCount` (requires `VK_KHR_draw_indirect_count`), `vkCmdDispatchIndirect`.
   - **D3D12**: `ExecuteIndirect` with command signature.
   - **Metal**: `drawIndirect`, `drawIndexedIndirect`, `dispatchThreadgroupsWithIndirectBuffer`.

### Deliverables
- [ ] All indirect commands work on Vulkan + D3D12 + Metal
- [ ] `DrawIndirectCount` functions on Vulkan (check extension availability)

---

## T-V1.2.2 — GPU Culling Shader

### Steps
1. Write `frustum_cull.slang`:
   - Input: instance buffer (world matrix + bounding sphere per instance)
   - Compute shader: frustum test each instance
   - Output: visible instance buffer (append via atomic counter)
2. Write `occlusion_cull.slang` (optional for V1.2, can be depth buffer-based HiZ):
   - Input: depth pyramid + instance bounds
   - Conservative depth test per instance
3. Both shaders use bindless buffers for input/output.

---

## T-V1.2.3 — GPU LOD Selection

### Steps
1. Write `lod_select.slang`:
   - Input: visible instance buffer + distance thresholds
   - Compute shader: calculate instance-to-camera distance, select LOD index
   - Output: indirect draw args buffer (one `DrawIndexedIndirectCommand` per LOD group)
   - Also write count buffer for `DrawIndirectCount`
2. Indirect args buffer uses `RHI_Buffer` with `RHI_BUFFER_USAGE_INDIRECT`.

---

## T-V1.2.4 — GPU Driven Demo

### Steps
1. `examples/gpu_driven/main.c`:
   - Create scene with 10k+ cube instances (random positions, varying scales)
   - Instance data uploaded to bindless buffer
   - Frame loop:
     a. Dispatch frustum cull CS
     b. Dispatch LOD select CS
     c. Single `RHI_CmdDrawIndirectCount` to render all visible instances
     d. No CPU-side loop over instances — draw list is purely GPU-generated
   - Display HUD: visible count, LOD distribution
2. Camera orbit to trigger culling — verify objects disappear when off-screen.

### Acceptance Criteria
- [ ] 10k+ instances rendered
- [ ] Frustum culling visible (objects pop in/out at screen edges)
- [ ] CPU draw call count = 1 (or small fixed number per LODGroup)
- [ ] No validation errors

### README Entry
```
| gpu_driven | 10k+ instances culled and drawn entirely on GPU. CPU issues a single DrawIndirectCount. |
```

### References
- `docs/arch.md` §12
