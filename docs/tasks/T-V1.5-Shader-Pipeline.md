# T-V1.5: Shader Pipeline

**Milestone**: V1.5 — Shader Pipeline
**Duration**: Month 5–6
**Goal**: Slang-based offline + runtime shader compilation, reflection, material schema generation, pipeline caching, hot-reload.

**Demo**: No standalone demo; integrated into existing examples.

---

## T-V1.5.1 — Shader Compiler Service (Offline)

### Steps
1. Create `tools/shader_compiler/` — CLI tool wrapping `slangc`.
2. Usage: `shader_compiler compile --input foo.slang --output-dir shaders/ --targets spirv,dxil,msl,wgsl`
3. Produces: `foo.spv`, `foo.dxil`, `foo.msl`, `foo.wgsl`, `foo.reflection.json`
4. Add `--emit-reflection-json` flag — queries Slang reflection API, serializes to JSON per `docs/arch.md` §10.5.
5. CMake custom command: auto-compile `.slang` files as build step.

### Deliverables
- [ ] `shader_compiler` CLI compiles a `.slang` file to all 4 targets + reflection JSON
- [ ] Build system auto-compiles shaders on change

---

## T-V1.5.2 — SDL_GPUShaderReflection Extension

### Steps
1. Define `SDL_GPUShaderParam`, `SDL_GPUShaderReflection` structs per `docs/arch.md` §5.2.
2. Implement `SDL_ReflectGPUShader(slang_source, entry_point)`:
   - Call Slang compile + reflection API
   - Fill parameter arrays (name, binding, set, type, size, offset)
   - Generate `json` string pre-serialized
3. Implement `SDL_FreeGPUShaderReflection`.
4. Test: reflect `pbr.slang`, compare JSON output with offline tool.

---

## T-V1.5.3 — RHI_Reflection API

### Steps
1. Define `RHI_ReflectionData` opaque type, `RHI_ReflectionParam` struct.
2. `RHI_LoadReflection(path)` — load `.reflection.json` from disk.
3. `RHI_Reflection_GetParameter(data, name)` — query by name.
4. `RHI_Reflection_GetParameterCount(data)` — total count.
5. `RHI_Reflection_GetParameterByIndex(data, index)` — query by index.
6. `RHI_DestroyReflection(data)`.

---

## T-V1.5.4 — Material Schema Generation

### Steps
1. Add `shader_compiler schema --input foo.slang --output foo.material_schema.json` command.
2. Algorithm:
   a. Load reflection data for `foo.slang`.
   b. Extract all uniform buffer fields (recursively flatten nested structs).
   c. Extract all texture/sampler parameters.
   d. Generate JSON schema:
   ```json
   {
     "shader": "foo",
     "uniforms": [
       {"name": "baseColor",  "type": "float4", "offset": 0},
       {"name": "roughness",  "type": "float",  "offset": 16}
     ],
     "textures": [
       {"name": "albedoMap", "binding": 0, "set": 1},
       {"name": "normalMap", "binding": 1, "set": 1}
     ],
     "samplers": [
       {"name": "linearClamp", "binding": 0, "set": 2}
     ]
   }
   ```
3. Runtime: `RHI_CreateMaterialLayoutFromReflection(reflection)` — builds bind group layout + uniform buffer layout.

### Deliverables
- [ ] Material schema JSON generated from any `.slang` file
- [ ] Material layout auto-constructed at runtime from reflection

---

## T-V1.5.5 — Pipeline Cache

### Steps
1. Extend `SDL_GPUPipelineCache` per `docs/arch.md` §5.3.
2. On `RHI_CreateGraphicsPipeline`:
   a. Compute PSO hash from all state + shader hashes.
   b. Check cache file. If hit — load binary, skip `vkCreateGraphicsPipelines` / `CreateGraphicsPipelineState`.
   c. If miss — compile normally, store binary via `SDL_SaveGPUPipelineCache`.
3. Version the cache with a schema ID — invalidate on engine upgrade.

### Deliverables
- [ ] Pipeline cache hit reduces startup time by > 50% in benchmark
- [ ] Cache invalidation works on schema change

---

## T-V1.5.6 — Shader Hot-Reload

### Steps
1. `RHI_WatchShaderDirectory(path)` — platform file watcher (`ReadDirectoryChangesW` on Windows, `kqueue` on macOS, `inotify` on Linux).
2. On `.slang` file change:
   a. Recompile to all targets.
   b. Create new `RHI_ShaderModule` instances.
   c. Recreate affected `RHI_GraphicsPipeline` / `RHI_ComputePipeline` instances.
   d. Swap pipeline pointers atomically.
3. Rate-limit: debounce changes within 100ms window.

### Deliverables
- [ ] Edit `.slang` → visual change appears in <1 second
- [ ] No crash on compilation error (log error, keep old pipeline)
