# T-V1.0: RHI Foundation

**Milestone**: V1.0 — RHI Foundation
**Duration**: Month 2
**Goal**: All core RHI objects implemented on Vulkan + D3D12 + Metal. Deferred destruction and error callback operational.

**Demo**: No new demo (HelloWorld extended internally)

---

## T-V1.0.1 — RHI_Buffer Full Implementation

### Context
Buffer supports ALL usage flags. Host-visible vs device-local. Map/Unmap for upload.

### Steps
1. Define `RHI_BufferUsage` flags, `RHI_MemoryType`, `RHI_BufferCreateInfo` in `defs.h` per `docs/arch.md` §6.3.
2. Implement `RHI_CreateBuffer`:
   - Translate `RHI_BufferUsage` to SDL GPU buffer usage flags
   - Choose memory type (device-local for GPU-only, host-visible for upload)
3. Implement `RHI_MapBuffer` / `RHI_UnmapBuffer` — wrap SDL GPU map API.
4. Implement `RHI_DestroyBuffer` — invalidate generational slot, push to deferred queue.
5. Test: upload vertex data via map, render with it.

### Deliverables
- [ ] Buffer creation with all usage flags
- [ ] Map/Unmap round-trip produces correct GPU data
- [ ] Destroy invalidates handle (generational check fails)

### References
- `docs/arch.md` §6.3, §7

---

## T-V1.0.2 — RHI_Texture Full Implementation

### Steps
1. Define `RHI_TextureType`, `RHI_Format`, `RHI_TextureUsage`, `RHI_TextureCreateInfo` per `docs/arch.md` §6.4.
2. Implement `RHI_CreateTexture` / `RHI_DestroyTexture`.
3. Implement `RHI_RegisterTexture` (bindless, see V1.1 prep).
4. Implement `RHI_CmdGenerateMipmaps`.
5. Support all common formats; validate format-support query per device.

### Deliverables
- [ ] Texture 2D, Cube, 2DArray creation
- [ ] Mipmap generation produces correct levels

---

## T-V1.0.3 — RHI_Sampler

### Steps
1. Define `RHI_Filter`, `RHI_AddressMode`, `RHI_SamplerCreateInfo` per `docs/arch.md` §6.5.
2. Implement `RHI_CreateSampler` / `RHI_DestroySampler`.

### Deliverables
- [ ] Sampler creation with all filter/address modes

---

## T-V1.0.4 — RHI_GraphicsPipeline

### Steps
1. Define full pipeline state structs: `RHI_VertexBinding`, `RHI_VertexAttribute`, `RHI_RasterizerState`, `RHI_DepthStencilState`, `RHI_BlendState`, `RHI_GraphicsPipelineCreateInfo`.
2. Implement `RHI_CreateGraphicsPipeline` / `RHI_DestroyGraphicsPipeline`.
3. Implement `RHI_CmdBindGraphicsPipeline`.

### Deliverables
- [ ] Pipeline with vertex layout, blend, depth state renders correctly

---

## T-V1.0.5 — RHI_ComputePipeline

### Steps
1. Define `RHI_ComputePipelineCreateInfo`.
2. Implement `RHI_CreateComputePipeline` / `RHI_DestroyComputePipeline`.
3. Implement `RHI_CmdBindComputePipeline`.

---

## T-V1.0.6 — RHI_BindGroup (Legacy)

### Steps
1. Define `RHI_BindGroupDesc`, `RHI_BindGroup`.
2. Implement `RHI_CreateBindGroup`, `RHI_BindGroup_SetBuffer/SetTexture/SetSampler`, `RHI_CmdBindBindGroup`.
3. Note: will be deprecated after V1.1 Bindless is default.

---

## T-V1.0.7 — RHI_CommandBuffer Full

### Steps
1. Implement all draw commands: `RHI_CmdDraw`, `RHI_CmdDrawIndexed`.
2. Implement dispatch: `RHI_CmdDispatch`.
3. Implement copy: `RHI_CmdCopyBuffer`, `RHI_CmdCopyBufferToTexture`.
4. Implement barriers: `RHI_CmdBarrier`.
5. Implement push constants: `RHI_CmdPushConstants`.

### Deliverables
- [ ] All command types functional on Vulkan + D3D12

---

## T-V1.0.8 — RHI_Fence (Timeline Semaphore)

### Steps
1. Define `RHI_Fence`, `RHI_CreateFence`, `RHI_WaitForFence`, `RHI_ResetFence`.
2. Backend: Vulkan uses `VkSemaphoreType::TIMELINE`, D3D12 uses `ID3D12Fence`, Metal emulates with `MTLEvent` + spin.
3. Implement `RHI_GetFenceValue` for CPU-side polling.

### Deliverables
- [ ] Timeline semaphore works on all 3 backends

---

## T-V1.0.9 — Resource Barriers

### Steps
1. Define `RHI_BarrierType` (layout transition, UAV, aliasing), `RHI_BarrierDesc`.
2. Implement `RHI_CmdBarrier` mapping to:
   - Vulkan: `vkCmdPipelineBarrier`
   - D3D12: `ResourceBarrier`
   - Metal: `MTLBlitCommandEncoder` or implicit

---

## T-V1.0.10 — macOS + Metal Backend

### Steps
1. CI: add macOS runner (GitHub Actions macos-latest).
2. Verify all V1.0 objects work on Metal via SDL GPU.
3. Fix any Metal-specific issues (argument buffer layout, format differences).

### Deliverables
- [ ] HelloWorld runs on macOS Metal

---

## T-V1.0.11 — Deferred Destruction

### Context
Per `docs/arch.md` §7. All `RHI_Destroy*` immediately invalidate the generational slot and push the native handle to a deferred queue tagged with `frameCounter + MAX_FRAMES_IN_FLIGHT`.

### Steps
1. Add `RHI_DeferredFree` struct and ring buffer to `RHI_Device` internal.
2. At the start of each frame, call `RHI_ProcessDeferredQueue`:
   - Query completed fence value.
   - Free all entries where `frameNumber <= completedFrame`.
3. Every `RHI_Destroy*` function:
   - Increment slot generation.
   - Null out slot resource pointer.
   - Push native handle to deferred queue.
4. Add `RHI_MAX_FRAMES_IN_FLIGHT` define (default 2).

### Deliverables
- [ ] 10k frame stress test: create/destroy buffers every frame, no validation errors
- [ ] Stale handle correctly rejected by `RHI_IsHandleValid`

### References
- `docs/arch.md` §7

---

## T-V1.0.12 — Error Callback

### Steps
1. Define `RHI_ErrorType`, `RHI_ErrorCallback` in `defs.h` per `docs/arch.md` §2.6.
2. Implement `RHI_SetErrorCallback`.
3. Hook SDL GPU validation output → callback.
4. Add internal validation checks on creation params (invalid usage flags, etc.) → callback.
5. Test: intentionally pass invalid params, verify callback fires.

### Deliverables
- [ ] Error callback fires on invalid API usage
