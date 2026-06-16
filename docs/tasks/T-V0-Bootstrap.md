# T-V0: Bootstrap

**Milestone**: V0 — Bootstrap
**Duration**: Week 1–4
**Goal**: Project skeleton compiles, SDL3 + Slang submodules wired, hello-triangle renders on Vulkan + D3D12.

**Demo**: `examples/HelloWorld`

---

## T-V0.1 — CMake Build System

### Context
Multi-module CMake project. Modules: `rhi`, `shader`, `render`, `plugins`. Third-party submodules under `modules/3rd/`.

### Steps
1. Create top-level `CMakeLists.txt` with `add_subdirectory` for each module.
2. `modules/rhi/CMakeLists.txt`: build `rhi` as shared library, link SDL3, export `include/` as PUBLIC.
3. `modules/shader/CMakeLists.txt`: build `shader` as static library, link Slang.
4. `examples/CMakeLists.txt`: each example is a standalone executable linking `rhi` and `shader`.
5. Configure CMake presets (`CMakePresets.json`) for MSVC/Vulkan and MSVC/D3D12.
6. Add `.clang-format` and `.editorconfig`.

### Deliverables
- [x] `cmake --build` succeeds for both presets
- [x] `rhi.dll` + `shader.lib` produced
- [x] HelloWorld target compiles and renders colored triangle on Vulkan (no validation errors)

### References
- `docs/arch.md` §16 (Module Structure)
- `docs/roadmap.md` V0 task 0.1

---

## T-V0.2 — SDL3 Submodule Init

### Context
SDL3 fork at `modules/3rd/sdl`, branch `sdl_enjin_rhi`. Must compile as part of the super-build.

### Steps
1. `git submodule add -b sdl_enjin_rhi https://github.com/in-dreaming/SDL.git modules/3rd/sdl`
2. In top-level CMake: `set(SDL_SHARED ON)`, `set(SDL_STATIC OFF)`, `add_subdirectory(modules/3rd/sdl)`.
3. `modules/rhi` links `SDL3::SDL3-shared`.
4. Verify `SDL_Init` + `SDL_CreateWindow` works in a smoke test.

### Deliverables
- [x] SDL3 compiles from submodule
- [x] Smoke test opens and closes a window

### References
- `docs/arch.md` §17 (SDL3 Fork Policy)
- `.gitmodules`

---

## T-V0.3 — Slang Submodule Init

### Context
Slang fork at `modules/3rd/slang`, branch `slang_enjin`. Shader compiler service depends on this.

### Steps
1. `git submodule add -b slang_enjin https://github.com/in-dreaming/slang.git modules/3rd/slang`
2. Build Slang from source via CMake or use pre-built binaries — decide based on build time.
3. `modules/shader` links `slang::slang`.
4. Minimal test: compile a `.slang` vertex shader to SPIRV.

### Deliverables
- [x] Slang compiles or binary linked
- [x] `slangc` can compile a trivial shader to SPIRV from test

### References
- `docs/arch.md` §10 (Shader Pipeline)

---

## T-V0.4 — RHI_Device Bootstrap

### Context
`RHI_Device` wraps `SDL_GPUDevice`. Public API is C with PIMPL.

### Steps
1. Create `modules/rhi/include/rhi/rhi.h` — unified public header.
2. Create `modules/rhi/include/rhi/defs.h` — enums, structs, handle types (see `docs/arch.md` §2.3, §4, §6).
3. Implement `RHI_CreateDevice`:
   - Parse `RHI_DeviceCreateInfo`
   - Call `SDL_CreateGPUDevice` with matching backend flags
   - Store native device in opaque struct
4. Implement `RHI_DestroyDevice`, `RHI_DeviceWaitIdle`.
5. Implement `RHI_GetQueue` — wrap `SDL_AcquireGPUCommandBuffer` target queue family.

### Deliverables
- [x] `RHI_CreateDevice` succeeds for Vulkan and D3D12 backends
- [x] `RHI_DestroyDevice` cleans up without leak
- [x] Zero SDL types in `modules/rhi/include/`

### References
- `docs/arch.md` §2.1 (SDL Is Not Public API), §2.5 (PIMPL), §6.1 (Device & Queue)

---

## T-V0.5 — HelloWorld Demo

### Context
First visible result. Colored triangle rendered via RHI C API on both Vulkan and D3D12.

### Steps
1. In `examples/HelloWorld/main.c`:
   - `RHI_CreateDevice` with `RHI_BACKEND_VULKAN` (fallback D3D12)
   - `RHI_CreateSwapchain`
   - Create vertex + index buffers with triangle data
   - Write minimal `.sllang` vertex/fragment shaders (pass-through color)
   - Compile shaders offline via `tools/shader_compiler` or bundle SPIRV
   - `RHI_CreateShaderModule` for each
   - `RHI_CreateGraphicsPipeline` with vertex layout and shaders
   - Main loop: acquire swapchain texture, begin render pass, bind pipeline, draw, end render pass, present
2. Add `examples/HelloWorld/shaders/` with `tri.slang`
3. Add `examples/HelloWorld/CMakeLists.txt`

### Acceptance Criteria
- [x] Colored triangle visible on screen (Vulkan backend, zero validation errors)
- [ ] Runs on both Vulkan and D3D12 with identical output
- [x] No SDL types in example code — pure RHI API

### README Entry
```
| HelloWorld | Render a colored triangle using RHI C API. Demonstrates Device, Swapchain, Buffer, Pipeline, RenderPass. |
```

### References
- `docs/arch.md` §6 (RHI C API)
- `docs/roadmap.md` V0 tasks 0.5–0.7

---

## T-V0.6 — RHI_RenderPass

### Steps
1. Define `RHI_RenderPassDesc`, `RHI_ColorAttachmentDesc`, `RHI_DepthAttachmentDesc` in `defs.h`.
2. Implement `RHI_CmdBeginRenderPass` / `RHI_CmdEndRenderPass` — wrap SDL GPU render pass API.
3. Test: clear screen to a specific color, validate via screenshot comparison.

### Deliverables
- [x] Render pass clear color matches expected value

---

## T-V0.7 — RHI_Swapchain

### Steps
1. Define `RHI_SwapchainCreateInfo` per `docs/arch.md` §6.2.
2. Implement `RHI_CreateSwapchain`, `RHI_AcquireSwapchainTexture`, `RHI_Present`, `RHI_ResizeSwapchain`.
3. Test: window resize triggers swapchain rebuild without crash.

### Deliverables
- [x] Swapchain handles window resize gracefully (via SDL_ClaimWindowForGPUDevice)

---

## T-V0.8 — CI

### Steps
1. GitHub Actions workflow: build on Windows (MSVC) for Vulkan and D3D12 presets.
2. Trigger on push to `main` and PRs.
3. Steps: checkout submodules recursively, cmake configure, cmake build, run smoke tests.

### Deliverables
- [x] CI green on push (GitHub Actions workflow created)
