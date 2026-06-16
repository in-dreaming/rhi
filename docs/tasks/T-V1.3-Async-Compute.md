# T-V1.3: Multi-Queue + Async Compute + Transfer Ring

**Milestone**: V1.3 — Multi-Queue + Async Compute + Transfer Ring
**Duration**: Month 4–5
**Goal**: Overlap compute culling with graphics rendering. Background texture upload via DMA ring buffer.

**Demo**: `examples/async_compute`

---

## T-V1.3.1 — Queue Enumeration

### Steps
1. Implement `RHI_GetQueue(device, queueType, index)`:
   - Query SDL GPU for available queue families
   - Vulkan: query `queueFamilyProperties`, separate Graphics/Compute/Transfer
   - D3D12: single queue typically; Compute queue if `D3D12_COMMAND_LIST_TYPE_COMPUTE` available
   - Metal: single `MTLCommandQueue`, but multiple `MTLCommandBuffer` concurrent
2. Store reference to each acquired queue in device internals.

---

## T-V1.3.2 — Cross-Queue Sync via Timeline Semaphore

### Steps
1. Extend `RHI_SubmitInfo` to include `waitSemaphores/waitValues` and `signalSemaphores/signalValues` per `docs/arch.md` §6.1.
2. Implement `RHI_QueueSubmit` with semaphore arrays:
   - Vulkan: `VkSubmitInfo::pWaitSemaphores` / `pSignalSemaphores`
   - D3D12: `ID3D12CommandQueue::Wait` / `Signal` on fence
   - Metal: encode `MTLEvent.wait` / `signalEvent` in command buffer
3. Test: compute queue signals semaphore=100, graphics queue waits semaphore=100.

---

## T-V1.3.3 — Async Compute Demo

### Steps
1. `examples/async_compute/main.c`:
   - Frame structure:
     a. Submit compute cull dispatch (compute queue, signals `comp_sem=frame*2`)
     b. Submit opaque render (graphics queue, waits `comp_sem=frame*2`, signals `gfx_sem=frame*2`)
     c. Submit transparent render (graphics queue, waits `gfx_sem=frame*2`)
   - Use GPU timestamp queries to measure overlap
   - Display: compute duration, graphics duration, overlap percentage
2. Compare frame time vs single-queue baseline.

### Acceptance Criteria
- [ ] Measurable overlap between compute and graphics
- [ ] Correct rendering (no sync artifacts)
- [ ] Timestamp query data displayed in HUD

### README Entry
```
| async_compute | Overlap GPU culling (compute queue) with rendering (graphics queue) using Timeline Semaphores. GPU timestamps show overlap. |
```

---

## T-V1.3.4 — Async Transfer Ring Buffer

### Context
Per `docs/arch.md` §13. DMA-based background texture upload without stalling the render queue.

### Steps
1. Implement `RHI_TransferRing`:
   - `RHI_CreateTransferRing(device, capacity)` — creates host-visible staging buffer + timeline semaphore
   - `RHI_TransferUpload(ring, dstTexture, mipLevel, data, size)` — map, memcpy, submit copy on transfer queue
   - `RHI_TransferWait(ring, frameNumber)` — wait until uploads from frame N are complete
2. Ring buffer wrap-around: track head/tail offsets, fail gracefully if ring is full (log warning, stall).
3. Integrate with device frame loop: `RHI_TransferWait` at frame start, reclaim ring space.

### Deliverables
- [ ] 16k×16k texture uploaded in background over multiple frames
- [ ] No graphics queue stall during upload
- [ ] Ring wrap-around handled without corruption

### References
- `docs/arch.md` §13
