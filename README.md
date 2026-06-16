# rhi

A next-gen RHI (Render Hardware Interface) based on SDL3 GPU + Slang.

**Target APIs**: Vulkan 1.3 / D3D12 / Metal 3 / WebGPU
**Target Platforms**: Windows / Linux / macOS / Android / iOS / Web

## Documentation

| Document | Description |
|-|-|
| [docs/arch.md](docs/arch.md) | Architecture design (C API, generational handles, PIMPL, bindless, Render Graph, ...) |
| [docs/roadmap.md](docs/roadmap.md) | Milestone roadmap (V0 → V3, 22 months) |
| [docs/pitfalls.md](docs/pitfalls.md) | SDL3 GPU & Slang API 陷阱备忘（V0 实践总结） |
| [docs/build.md](docs/build.md) | 构建系统、工具链、着色器编译指南 |
| [docs/tasks/](docs/tasks/) | Detailed task breakdown per milestone |
| [docs/research/](docs/research/) | Feature research notes |

## Examples

Located under `examples/`. Build with `cmake --build --target <name>`.

| Name | Milestone | Description |
|-|-|-|
| HelloWorld | V0 | Render a colored triangle. Demonstrates Device, Swapchain, Buffer, Pipeline, RenderPass, CommandBuffer. |
| bindless | V1.1 | Render 128+ textures from a global BindlessTable using generational index handles. No BindGroup. |
| gpu_driven | V1.2 | 10k+ instances culled and drawn on GPU. CPU issues a single DrawIndirectCount. |
| async_compute | V1.3 | Overlap GPU culling (compute queue) with rendering (graphics queue) via Timeline Semaphores. |
| multithread | V1.4 | Split a deferred renderer across 4 threads using per-thread CommandPools. TSan-clean. |
| render_graph | V1.6 | Full deferred renderer as a declarative Render Graph with automatic barriers, dead pass elimination, and memory aliasing. |

| vt | V2.1 | Virtual Texture streaming: 16k×16k texture with < 4 MB resident. Feedback-driven page residency. |

## Build

```bash
cmake --preset msvc-vulkan
cmake --build --preset msvc-vulkan
```

Presets: `msvc-vulkan`, `msvc-d3d12`, `clang-macos-metal`.

## Task Index

| Task File | Milestone | Duration |
|-|-|-|
| [T-V0-Bootstrap](docs/tasks/T-V0-Bootstrap.md) | V0 | Week 1–4 |
| [T-V1.0-RHI-Foundation](docs/tasks/T-V1.0-RHI-Foundation.md) | V1.0 | Month 2 |
| [T-V1.1-Bindless](docs/tasks/T-V1.1-Bindless.md) | V1.1 | Month 3 |
| [T-V1.2-GPU-Driven](docs/tasks/T-V1.2-GPU-Driven.md) | V1.2 | Month 3–4 |
| [T-V1.3-Async-Compute](docs/tasks/T-V1.3-Async-Compute.md) | V1.3 | Month 4–5 |
| [T-V1.4-MultiThread](docs/tasks/T-V1.4-MultiThread.md) | V1.4 | Month 5 |
| [T-V1.5-Shader-Pipeline](docs/tasks/T-V1.5-Shader-Pipeline.md) | V1.5 | Month 5–6 |
| [T-V1.6-RenderGraph](docs/tasks/T-V1.6-RenderGraph.md) | V1.6 | Month 6–7 |
| [T-V1.7-Mobile-Web](docs/tasks/T-V1.7-Mobile-Web.md) | V1.7 | Month 8–10 |
| [T-V2-Advanced-GPU](docs/tasks/T-V2-Advanced-GPU.md) | V2 | Month 11–16 |
| [T-V3-RT-Neural-Plugins](docs/tasks/T-V3-RT-Neural-Plugins.md) | V3 | Month 17–22 |
