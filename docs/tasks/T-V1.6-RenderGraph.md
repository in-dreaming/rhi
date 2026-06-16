# T-V1.6: Render Graph

**Milestone**: V1.6 — Render Graph
**Duration**: Month 6–7
**Goal**: Declarative Render Graph with automatic barrier insertion, dead pass elimination, and memory aliasing. Multi-threaded pass recording integrated with V1.4 CommandPools.

**Demo**: `examples/render_graph`

---

## T-V1.6.1 — Setup Phase

### Context
Per `docs/arch.md` §14.1.

### Steps
1. Define types:
   ```c
   typedef uint32_t RHI_PassHandle;
   typedef uint32_t RHI_ResourceHandle;
   ```
2. Implement `RHI_CreateRenderGraph(device)`.
3. Implement `RHI_AddPass(graph, name, type)` — returns `RHI_PassHandle`.
   - Pass types: `RHI_PASS_GRAPHICS`, `RHI_PASS_COMPUTE`, `RHI_PASS_TRANSFER`.
4. Implement `RHI_CreateTransientTexture(graph, name, width, height, format)` — returns `RHI_ResourceHandle`.
5. Implement `RHI_CreateTransientBuffer(graph, name, size, usage)`.
6. Implement `RHI_PassRead(pass, resource)` / `RHI_PassWrite(pass, resource)`.
7. Track DAG edges internally (pass → resource → pass).

---

## T-V1.6.2 — Compile Phase

### Context
Per `docs/arch.md` §14.1 Phase 2.

### Steps
1. `RHI_CompileRenderGraph(graph)`:
   a. **Topological sort**: Kahn's algorithm on DAG. Result = ordered pass list.
   b. **Dead code elimination**: Walk backward from declared outputs. Remove passes/resources not contributing.
   c. **Lifetime analysis**: For each transient resource, compute `firstUse` and `lastUse` pass indices.
   d. **Barrier schedule**: Between consecutive passes sharing a resource, insert appropriate barrier (layout transition for textures, UAV for buffers).
   e. **Alias assignment**: Greedy interval-packing algorithm:
      - Sort resources by `lastUse - firstUse` (short-lived first).
      - Assign each resource to the first memory block that is free during `[firstUse, lastUse]`.
      - Track alias groups for create/destroy barriers.
2. Output: compiled graph with pass order, barrier list, alias map.

### Deliverables
- [ ] Topological sort produces valid ordering
- [ ] Dead passes eliminated (test: add unused pass, verify it doesn't execute)
- [ ] Alias map non-overlapping for same-block resources

---

## T-V1.6.3 — Resource Aliasing

### Steps
1. Implement `RHI_CreateAliasedTexture(graph, name, width, height, format)`.
2. During compile, aliasing assigns physical memory offsets.
3. At execute, for each alias group:
   a. Create physical `RHI_Texture` at first use.
   b. Insert aliasing barrier between consecutive aliased resources.
   c. Destroy at last use (deferred).
4. Memory saving = sum of unique physical blocks / sum of all transient sizes.

### Deliverables
- [ ] Alias saving ≥ 30% for a deferred renderer's transient resources
- [ ] No visual corruption (validation mode: unique memory per resource → identical output)

---

## T-V1.6.4 — Execute Phase

### Context
Per `docs/arch.md` §14.1 Phase 3. Integrates with V1.4 multi-threaded recording.

### Steps
1. `RHI_ExecuteRenderGraph(graph, device)`:
   a. Batch passes into parallel groups (passes within same batch have no dependencies).
   b. For each batch:
      - Submit each pass to a worker thread.
      - Worker: acquire CB from thread-local pool, begin render pass or dispatch, record commands, end.
      - After all workers complete, collect CBs.
      - Submit CBs to appropriate queue (graphics or compute).
      - Signal fence for batch completion.
   c. Between batches, wait for previous batch fence.
2. Persistent passes: allow registering a callback per pass for custom recording.

### Deliverables
- [ ] Multi-threaded recording verified with TSan
- [ ] Correct rendering matches single-threaded execution

---

## T-V1.6.5 — Render Graph Demo

### Steps
1. `examples/render_graph/main.c`:
   - Build a full deferred renderer as a Render Graph:
     - Passes: ShadowMap, GBuffer, SSAO, Lighting, Tonemap
     - Transient resources: ShadowMap (D32), GBuffer0 (RGBA8), GBuffer1 (RGBA8), Depth (D32), SSAO (R8), HDR (RGBA16F)
   - Compile graph, print alias map and memory savings
   - Execute with multi-threaded recording (V1.4)
   - Display: total transient VRAM, alias ratio, per-pass GPU time
2. Camera controls for orbit.
3. Toggle aliasing on/off to compare VRAM usage.

### Acceptance Criteria
- [ ] Full deferred renderer runs via Render Graph
- [ ] Memory aliasing saves ≥ 30% transient VRAM
- [ ] Multi-threaded recording produces identical output
- [ ] Shadow map alias reused by Depth buffer (or similar)

### README Entry
```
| render_graph | Full deferred renderer built as a declarative Render Graph with automatic barriers, dead pass elimination, and memory aliasing. |
```

### References
- `docs/arch.md` §14
