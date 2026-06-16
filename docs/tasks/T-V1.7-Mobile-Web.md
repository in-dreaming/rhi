# T-V1.7: Mobile + Web

**Milestone**: V1.7 — Mobile + Web
**Duration**: Month 8–10
**Goal**: Vulkan on Android, Metal on iOS, WebGPU on Web. Feature parity where possible, graceful degradation otherwise.

**Demo**: No new demo; existing examples run on target platforms.

---

## T-V1.7.1 — Android Vulkan

### Steps
1. Add NDK CMake toolchain to `CMakePresets.json`.
2. SDL3 Android backend: `SDL_CreateWindow` with `SDL_WINDOW_VULKAN`.
3. Cross-compile `rhi` for `aarch64-linux-android`.
4. Android manifest + `MainActivity` via SDL's Android entry point.
5. Test: push `HelloWorld` to Android device, verify triangle renders.
6. Test: `bindless` demo on Android.
7. Handle Android lifecycle: `SDL_APP_WILLENTERBACKGROUND` → device wait idle.

### Deliverables
- [ ] HelloWorld renders on Android (Vulkan)
- [ ] Bindless demo functional on Android

---

## T-V1.7.2 — iOS Metal

### Steps
1. Add Xcode CMake generator preset.
2. SDL3 iOS backend: `SDL_CreateWindow` with Metal layer.
3. Cross-compile for `arm64-apple-ios`.
4. Test: HelloWorld on iOS simulator + device.
5. Handle iOS lifecycle: `SDL_APP_DIDENTERBACKGROUND` → pause rendering.

### Deliverables
- [ ] HelloWorld renders on iOS (Metal)

---

## T-V1.7.3 — Web/WebGPU

### Steps
1. Add Emscripten CMake toolchain preset.
2. Build with `emcmake cmake`. Link `libSDL3.so` (Emscripten SDL3 port).
3. WebGPU backend: SDL3 WebGPU support.
4. Constraints: no multi-threading (single CB), smaller bindless limits.
5. Test: HelloWorld in Chrome via `emrun`.
6. Test: bindless with reduced texture count (e.g., 16 instead of 128).

### Deliverables
- [ ] HelloWorld runs in Chrome (WebGPU)
- [ ] Bindless demo runs with reduced limits

---

## T-V1.7.4 — Feature Degradation

### Steps
1. Implement `RHI_QueryFeature(device, feature)` API:
   ```c
   typedef enum RHI_Feature {
       RHI_FEATURE_BINDLESS,
       RHI_FEATURE_MESH_SHADER,
       RHI_FEATURE_RAY_TRACING,
       RHI_FEATURE_INDIRECT_COUNT,
       RHI_FEATURE_TIMELINE_SEMAPHORE,
       // ...
   } RHI_Feature;

   bool RHI_QueryFeature(RHI_Device* device, RHI_Feature feature);
   ```
2. Per-platform feature table:
   | Feature | Vulkan Desktop | D3D12 | Metal macOS | Vulkan Android | Metal iOS | WebGPU |
   |---------|---------------|-------|-------------|---------------|-----------|--------|
   | Bindless | Yes | Yes | Yes (limit 4096) | Yes | Yes (limit 4096) | Partial (limit 256) |
   | IndirectCount | Yes (`VK_KHR`) | Yes | Yes | Varies | Yes | No |
   | MeshShader | Varies | Varies | No | No | No | No |
3. Fallback paths:
   - No IndirectCount → CPU reads count buffer, issues `DrawIndirect` N times
   - Small Bindless limit → multi-set binding with `BindGroup`
   - No MeshShader → compute meshlet culling + indirect draw fallback (V2.4)

### Deliverables
- [ ] Feature query API functional
- [ ] Documented degradation table per platform

---

## T-V1.7.5 — Mobile GPU Profiling Integration

### Steps
1. Android: RenderDoc remote capture integration.
2. iOS: Xcode GPU frame capture.
3. Document profiling workflow.

### Deliverables
- [ ] GPU frame capture works on Android + iOS

---

## T-V1.7.6 — Memory Budget & Streaming Resolution

### Steps
1. `RHI_GetDeviceMemoryBudget(device)` — query VRAM budget.
2. Streaming system: adjust texture max resolution based on budget.
3. Default texture budget tiers: 8GB+ → full res, 4-8GB → 75% res, <4GB → 50% res.

### Deliverables
- [ ] Texture resolution scales with VRAM budget
