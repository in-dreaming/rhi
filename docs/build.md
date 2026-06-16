# 构建与工具链

---

## CMake 预设

```bash
cmake --preset msvc-vulkan    # VS2022 + Vulkan (SPIR-V)
cmake --preset msvc-d3d12     # VS2022 + D3D12 (DXIL)
cmake --build --preset msvc-vulkan --config RelWithDebInfo
```

## 产物

| 目标 | 类型 | 路径 |
|---|---|---|
| `rhi` | DLL | `build/msvc-vulkan/modules/rhi/RelWithDebInfo/rhi.dll` |
| `shader` | Static LIB | `build/msvc-vulkan/modules/shader/RelWithDebInfo/shader.lib` |
| `slang-compiler` | DLL | `build/msvc-vulkan/RelWithDebInfo/bin/slang-compiler.dll` |
| `HelloWorld` | EXE | `build/msvc-vulkan/examples/HelloWorld/RelWithDebInfo/HelloWorld.exe` |

## DLL 部署

运行时 PATH 需包含：

- `slang-compiler.dll` 所在目录（含 slang 标准模块子目录）
- `SDL3.dll` 所在目录
- `rhi.dll` 所在目录

PowerShell 一键设置：

```powershell
$b = "G:\work\tech\infra\rhi\build\msvc-vulkan"
$env:PATH = "$b\RelWithDebInfo\bin;$b\modules\rhi\RelWithDebInfo;$b\modules\3rd\sdl\RelWithDebInfo;" + $env:PATH
```

从任意路径运行：

```powershell
# 着色器已随构建自动复制到 exe 同级 shaders/ 目录
& "$b\examples\HelloWorld\RelWithDebInfo\HelloWorld.exe"
```

---

## 着色器编译

### 自动编译（推荐）

CMake 自定义命令在构建 HelloWorld 时自动触发，调用 `slangc` 编译 `.slang` → `.spv`/`.dxil`。

输出目录：`build/msvc-vulkan/examples/HelloWorld/shaders/`

### 手动编译（Python 工具）

```bash
python tools/shader_compiler/compile.py examples/HelloWorld/shaders/tri.slang \
  -o build/msvc-vulkan/examples/HelloWorld/shaders \
  -e vertMain -t spirv dxil
```

支持 target：`spirv`、`dxil`、`msl`、`wgsl`

Profile 默认值：
- SPIR-V: `spirv_1_3`（Vulkan 1.1 要求）
- DXIL: `sm_6_0`
- Metal: `metallib_3_0`

### 直接调用 slangc

```bash
# 顶点着色器 → SPIR-V
slangc -target spirv -profile spirv_1.3 -entry vertMain -o tri-vert.spv tri.slang

# 片段着色器 → SPIR-V
slangc -target spirv -profile spirv_1.3 -entry fragMain -o tri-frag.spv tri.slang

# 顶点着色器 → DXIL
slangc -target dxil -profile sm_6_0 -entry vertMain -o tri-vert.dxil tri.slang
```

### 着色器 Binary 布局约定

```
examples/HelloWorld/shaders/
  tri.slang            # 源码
  tri-vert.spv         # SPIR-V 顶点
  tri-frag.spv         # SPIR-V 片段
  tri-vert.dxil        # DXIL 顶点
  tri-frag.dxil        # DXIL 片段
```

命名规则：`{stem}-{entry}.spv` / `{stem}-{entry}.dxil`

---

## Shader Compiler Service (C API)

`modules/shader/include/shader/compiler.h`

```c
ShaderCompiler *ShaderCompiler_Create(void);
void            ShaderCompiler_Destroy(ShaderCompiler *compiler);

ShaderCompileResult ShaderCompiler_CompileFile(
    ShaderCompiler *compiler,
    const char *slangPath,
    const char *entryPoint);

ShaderCompileResult ShaderCompiler_CompileSource(
    ShaderCompiler *compiler,
    const char *source, size_t sourceSize,
    const char *pathHint,
    const char *entryPoint);

void ShaderCompileResult_Free(ShaderCompileResult *result);
```

4-target 输出：SPIR-V, DXIL, MSL metallib, WGSL。

**限制**：
- `getReflectionJSON()` 在当前 Slang C++ API 中不可用，需通过 `-emit-reflection-json` 命令行参数获取
- `IEntryPoint::getName()` 不存在，entry point 名称由调用方传入
- `ISession::getDiagnostics()` 不存在，通过 `loadModuleFromSourceString` 的 outDiagnostics 获取

---

## 模块依赖图

```text
HelloWorld ──→ rhi ──→ SDL3::SDL3-shared
           ──→ SDL3::SDL3-shared

shader ──→ slang (slang-compiler.dll)

注：HelloWorld 当前不链接 shader，仅运行时加载预编译的 .spv/.dxil 文件。
shader 模块主要用于 Asset Pipeline 或 Editor 集成。
```
