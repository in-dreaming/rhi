# T-V1.1: Bindless

**Milestone**: V1.1 — Bindless
**Duration**: Month 3
**Goal**: Global bindless resource table operational with generational index handles. Shader-side array access working.

**Demo**: `examples/bindless`

---

## T-V1.1.1 — SDL_GPUBindlessTable Extension

### Context
New SDL GPU extension. Must be implemented in the SDL3 fork at `modules/3rd/sdl`.

### Steps
1. Define `SDL_GPUBindlessTable` opaque type in SDL GPU headers.
2. Implement `SDL_CreateGPUBindlessTable(device, maxTextures, maxBuffers, maxSamplers)`.
3. Implement `SDL_BindlessTableAddTexture/Buffer/Sampler` — returns `uint32_t` index.
4. Implement `SDL_BindlessTableRemoveTexture/Buffer/Sampler` — marks slot free.
5. Implement `SDL_BindGPUBindlessTable(cb, table, slot)`.
6. Implement `SDL_FreeGPUBindlessTable`.
7. Backend implementations:
   - **Vulkan**: Create `VkDescriptorPool` with `VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT`. Allocate one `VkDescriptorSet` from unbounded array layout. `vkUpdateDescriptorSets` on add/remove. Bind at `slot`.
   - **D3D12**: Create shader-visible `ID3D12DescriptorHeap` (CBV_SRV_UAV + Sampler). `コピーDescriptor` on add/remove. Bind heap via `SetDescriptorHeaps` + root table.
   - **Metal**: Create `MTLArgumentBuffer`. `setTexture/buffer` at index offset. Bind via `setArgumentBuffer`.
8. Add `VK_EXT_descriptor_indexing` device feature query — fail gracefully if unavailable.

### Deliverables
- [ ] SDL extension compiles on all 3 backends
- [ ] Add/remove texture cycles without crash

### References
- `docs/arch.md` §5.1

---

## T-V1.1.2 — RHI_BindlessTable

### Steps
1. Define `RHI_BindlessTableCreateInfo`, `RHI_BindlessTable` in `defs.h`.
2. Wraps `SDL_GPUBindlessTable`.
3. `RHI_BindlessTableAddTexture` returns `RHI_TextureHandle` (generational index: low 24 bits = slot, high 8 bits = generation).
4. `RHI_BindlessTableRemoveTexture` increments slot generation and pushes slot to free-list.
5. `RHI_CmdBindBindlessTable` wraps `SDL_BindGPUBindlessTable`.
6. `RHI_BindlessTableAddBuffer/Sampler` analogous.

### Deliverables
- [ ] Generational handles validated — stale handle rejected
- [ ] Free-list recycling: add 100, remove 50, add 50 — indices reused, generations incremented

---

## T-V1.1.3 — Handle Recycling

### Steps
1. Maintain per-table free-list (stack of freed indices).
2. On `Add*`: pop from free-list if available, else allocate next sequential index.
3. On `Remove*`: push index to free-list, increment slot generation in global slot array.
4. Generation wraps at 255 → slot permanently dead (should never happen in practice).
5. Add `RHI_IsHandleValid` — checks `g_slots[handle.index].generation == handle.generation`.

---

## T-V1.1.4 — Slang Bindless Shader

### Steps
1. Write `bindless_demo.slang`:
   ```slang
   ParameterBlock<Texture2D> gTextures[];
   SamplerState gSamplers[];

   float4 main(...) : SV_Target {
       return gTextures[material.textureHandle].Sample(gSamplers[material.samplerHandle], uv);
   }
   ```
2. Compile to SPIRV + DXIL + MSL.
3. Verify Slang emits correct descriptor bindings (`register(t0, space1)`, etc.).

---

## T-V1.1.5 — Bindless Demo

### Context
Standalone demo proving bindless works end-to-end.

### Steps
1. `examples/bindless/main.c`:
   - Create bindless table with 1024 texture slots
   - Load 128 different textures (procedural checkerboards with unique colors)
   - Register all textures + one sampler
   - For each draw: push constant with `textureHandle`
   - Render a grid of 128 quads, each sampling different texture
2. Verify all 128 textures appear distinct on screen.
3. Hot-reload test: replace a texture mid-run, verify it updates.

### Acceptance Criteria
- [ ] 128 unique textures rendered from bindless table
- [ ] No bind groups used
- [ ] Handles validate correctly

### README Entry
```
| bindless | Render 128+ textures from a global BindlessTable using generational index handles. No BindGroup usage. |
```

### References
- `docs/arch.md` §6.9, §11
