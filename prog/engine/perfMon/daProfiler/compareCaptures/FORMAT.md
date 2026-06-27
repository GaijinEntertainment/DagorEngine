# `daprofiler-capture` JSON format

Compact, LLM- and tool-friendly intermediate produced by `dap_to_json.py` from a
binary daProfiler `.dap`, and consumed by `compare_captures.py`. The design goal is
to be small enough to keep around (a few hundred KB) yet carry everything needed
for statistical comparison: a per-frame time series for the overall frame, the GPU,
and every meaningful CPU/GPU scope.

All durations are **integer microseconds** (`timeUnit: "us"`). Scopes are keyed by
**name** (plus owning thread), not by the binary's per-board description index, so
they merge across appended capture segments and match cleanly between two captures.

## Top level

```jsonc
{
  "format": "daprofiler-capture",
  "formatVersion": 1,
  "timeUnit": "us",
  "meta": { ... },
  "threads": [ ... ],
  "frames": { ... },
  "gpu": { ... },          // present only if the capture has a GPU thread
  "scopes": [ ... ],       // per-frame detail
  "scopesTail": [ ... ],   // summary-only (scopes below the detail threshold)
  "samples": { ... }       // present only if the capture has stack sampling
}
```

### `meta`

| field | meaning |
|---|---|
| `source` | basename of the `.dap` |
| `platform`, `executable`, `cpu` | from the SummaryPack |
| `dumpTime`, `timeAfterLaunch` | capture timestamps |
| `uniqueName` | profile run name, if the dump carried one |
| `cpuFreqHz` | CPU timer frequency (ticks/second) used for the us conversion |
| `frameCount` | number of CPU frames (length of every per-frame array) |
| `mainThread` | name of the frame thread |
| `hasGpu`, `hasSamples` | whether `gpu` / `samples` sections exist |
| `summaryMeanMs` | mean frame ms from the SummaryPack (cross-check) |

### `threads`

`[{ "name", "tid", "mask" }]`. `mask` bit 1 = Main, bit 2 = GPU (see `ThreadMask`).
The GPU thread has `tid: -1`.

### `frames`

Per-frame arrays, each of length `frameCount`:

| field | meaning |
|---|---|
| `cpuUs` | total CPU frame time (from the FramesPack CPU boundaries) |
| `gpuUs` | GPU busy time per frame (duration of the depth-0 GPU event), if `hasGpu` |
| `frameNo` | engine global frame number per frame, if present in the dump |

### `gpu`

Per-frame GPU draw-stat totals (length `frameCount`), if `hasGpu` and stats exist:
`drawPrims`, `tris`, `instructions`, `renderTargets`, `programs`, `renderPasses`.

Each GPU event records a per-event drawstat **delta**, and nested scopes are already
contained in their ancestors, so the per-frame total is taken from the depth-0
(root) GPU event only (summing all depths would multiply-count). See the
`gpu_root_starts` logic in `dap_to_json.py`.

### `scopes` and `scopesTail`

A scope is one named instrumentation point on one thread. `scopes` carries per-frame
detail for the costliest scopes (controlled by `--top-scopes` / `--min-incl-ms`);
the rest land in `scopesTail` with summary fields only.

```jsonc
{
  "name": "RenderShadows",
  "src":  "shadows.cpp:123",       // file:line, or "" if unknown
  "thread": "Main",
  "type": "cpu",                    // "cpu" | "gpu"
  "callsTotal": 312,
  "inclTotalUs": 1234567,           // inclusive (self + children) over the capture
  "selfTotalUs": 234567,            // exclusive (children subtracted)
  "isWait": true,                   // present only when the IsWait desc flag is set
  "idle": true,                     // present only for an idle-thread placeholder span
  "perFrame": {                     // omitted for scopesTail entries
    "calls":  [ ... ],              // calls of this scope in each frame
    "inclUs": [ ... ],              // inclusive us in each frame (0 where idle)
    "selfUs": [ ... ]               // exclusive us in each frame
  }
}
```

- **Inclusive** time = sum of the scope's event durations in a frame.
- **Self** time = inclusive minus the time spent in direct children, reconstructed
  per thread with an interval stack.
- Events are bucketed into the frame containing the event start. GPU events are
  stored in CPU ticks, so the whole capture uses one CPU-frame axis.
- The profiler emits a synthetic placeholder span (the sole event of an eventless
  thread's per-segment frame). It is detected per segment and marked `"idle": true`
  (summary-only; `compare_captures.py` skips it), routed to a scope record kept separate
  from real work, so a thread idle in one appended segment but busy in another never
  mixes the fake span into real scope time. The dump does not preserve the empty-thread
  flag, so a real single self-named scope is marked the same way; marking rather than
  dropping keeps it visible in the output.
- Per-thread timelines are keyed by board thread id, not display name, so same-named
  threads (worker pools) keep separate self-time instead of nesting into each other.
- `isWait` carries the profiler's explicit `IsWait` description flag, so a lock wait
  is classified as a wait even when its name (e.g. `critsec`, `mutex`) does not match
  the wait-name heuristics in `compare_captures.py`.

### `samples` (only if the capture has stack sampling)

```jsonc
{
  "byScope": {
    "RenderShadows": {
      "totalSamples": 845,
      "topFunctions": [ { "func", "src", "samples", "pct" }, ... ]
    }
  }
}
```

Each stack sample is attributed to the innermost scope active on its thread at the
sample tick; the leaf frame is resolved against the dump's symbol table. This is the
"drill into a changed timeline" view. Best-effort (the sample path is skipped if the
dump has no callstacks; the two reference testGI captures have none).

## Source of truth

The binary layout is whatever `da_profiler::save_dump`
(`prog/engine/perfMon/daProfiler/daProfilerSaveDump.cpp`) writes; `dap_to_json.py`
mirrors it response-by-response. If the dump format version changes, update the
parser there.
