# T-V1.4: Multi-Threaded Rendering

**Milestone**: V1.4 — Multi-Threaded Rendering
**Duration**: Month 5
**Goal**: Parallel command buffer recording on multiple threads with per-thread command pools.

**Demo**: `examples/multithread`

---

## T-V1.4.1 — SDL_GPUCommandPool Extension

### Steps
1. Define `SDL_GPUCommandPool` opaque type in SDL headers.
2. Implement `SDL_CreateGPUCommandPool(device, queueType)`:
   - Vulkan: `vkCreateCommandPool` with `queueFamilyIndex`
   - D3D12: `CreateCommandAllocator` with matching `D3D12_COMMAND_LIST_TYPE`
   - Metal: no-op (Metal command buffers are already thread-safe from shared queue)
3. Implement `SDL_AcquireGPUCommandBufferFromPool(pool)`:
   - Vulkan: `vkAllocateCommandBuffers` from pool, `vkBeginCommandBuffer`
   - D3D12: `CreateCommandList` from allocator, close-reopen pattern
   - Metal: `MTLCommandBuffer` from device queue
4. Implement `SDL_ResetGPUCommandPool(pool)`:
   - Vulkan: `vkResetCommandPool` (O(1), resets all buffers at once)
   - D3D12: `ID3D12CommandAllocator::Reset`
   - Metal: no-op (auto-recycled)
5. Implement `SDL_FreeGPUCommandPool(pool)`.

### Deliverables
- [ ] Command pool works on all 3 backends
- [ ] Pool reset is O(1) on Vulkan/D3D12

---

## T-V1.4.2 — RHI_CommandPool API

### Steps
1. Define `RHI_CommandPoolCreateInfo { queueType, transient, resetCommandBuffer }`, `RHI_CommandPool` in `defs.h` per `docs/arch.md` §6.10.
2. Implement `RHI_CreateCommandPool`, `RHI_DestroyCommandPool`, `RHI_ResetCommandPool`.
3. Implement `RHI_AllocateCommandBuffer(pool)` — wraps `SDL_AcquireGPUCommandBufferFromPool`.
4. Implement `RHI_BeginCommandBuffer`, `RHI_EndCommandBuffer`.
5. Keep `RHI_AcquireCommandBuffer(device, queueType)` as convenience for single-threaded use.

---

## T-V1.4.3 — Per-Thread Pool Management

### Context
Per `docs/arch.md` §9.3.

### Steps
1. Define `RHI_ThreadContext { commandPool, commandBuffers[], currentIndex }`.
2. Use platform thread-local storage for `RHI_ThreadContext*`.
3. `RHI_GetThreadContext(device)`:
   - If no context exists for calling thread, create one with `RHI_CreateCommandPool`.
   - Return existing context.
4. At frame end, each thread calls `RHI_ResetCommandPool` (fast O(1)).
5. Add `RHI_DestroyThreadContext` for thread shutdown.

### Deliverables
- [ ] Thread contexts auto-created per thread
- [ ] No race conditions under TSan

---

## T-V1.4.4 — Parallel Command Recording Integration

### Steps
1. Modify Render Graph executor (or manual submission loop) to:
   a. Identify independent passes (no read-after-write dependency within same frame batch).
   b. Assign each pass to a worker thread.
   c. Each worker acquires CB from its thread-local pool.
   d. After all workers finish, collect CBs and submit on main thread.
2. Ensure submit order matches desired GPU order (CB array order = execution order).

---

## T-V1.4.5 — Multi-Threaded Demo

### Steps
1. `examples/multithread/main.c`:
   - Create 4 worker threads, each with its own `RHI_CommandPool`
   - Split deferred renderer into 4 passes recorded in parallel:
     - Thread 0: shadow map
     - Thread 1: G-buffer
     - Thread 2: SSAO (compute)
     - Thread 3: lighting (compute)
   - Collect all CBs, submit on main thread
   - Display: per-thread recording time, total frame time
2. Compare with single-threaded recording of same passes.
3. Run under ThreadSanitizer (TSan) — zero data races.

### Acceptance Criteria
- [ ] 3+ threads recording in parallel
- [ ] Measurable frame time reduction vs single-threaded baseline
- [ ] Zero TSan errors
- [ ] Identical visual output to single-threaded version

### README Entry
```
| multithread | Split a deferred renderer across 4 threads using per-thread CommandPools. TSan-clean. |
```

### References
- `docs/arch.md` §9
