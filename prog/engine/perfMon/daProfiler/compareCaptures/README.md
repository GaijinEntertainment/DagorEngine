# compareCaptures

Two pure-Python (stdlib-only) tools to compare daProfiler `.dap` captures:

- `dap_to_json.py` -- convert a binary `.dap` to the compact `daprofiler-capture`
  JSON ([FORMAT.md](FORMAT.md)).
- `compare_captures.py` -- statistically compare two captures and report which
  timelines are significantly slower/faster.

No build step and no third-party packages (Python 3.6+). For the underlying binary
format and aggregation rules see [FORMAT.md](FORMAT.md).

## Quick start

```sh
# Compare two captures directly (.dap inputs are converted in-memory):
python compare_captures.py OLD.dap NEW.dap --out report.md

# Or convert first (keeps the reusable JSON), then compare:
python dap_to_json.py OLD.dap old.json
python dap_to_json.py NEW.dap new.json
python compare_captures.py old.json new.json --out report.md --json-out report.json
```

Convention: the **first** argument is the baseline (OLD), the **second** is the
change under test (NEW). A positive delta means NEW is slower / does more.

## `dap_to_json.py`

```
python dap_to_json.py <in.dap> [out.json]
    --top-scopes N      per-frame detail for the N costliest scopes (default 1024)
    --min-incl-ms X     per-frame detail only for scopes with total inclusive >= X ms
    --no-perframe       summary only (smallest output)
    --scope SUBSTR      keep only scopes whose name contains SUBSTR (drill-down)
    --compact           no pretty indentation (roughly 3x smaller)
```

## `compare_captures.py`

```
python compare_captures.py <OLD.json|OLD.dap> <NEW.json|NEW.dap>
    --skip-warmup N     drop the first N frames of each capture (default 0)
    --alpha A           q-value gate for significance (default 0.05)
    --min-change-pct P  minimum mean per-frame change in percent to flag (default 2)
    --min-delta-ms M    minimum mean per-frame change in ms to flag (default 0.05)
    --format md|json|both
    --out FILE          write the markdown report (default: stdout)
    --json-out FILE     also write a machine-readable JSON of the deltas
```

### What it reports

1. **Overall frame time** -- OLD vs NEW mean/median/p95, delta, Mann-Whitney p,
   Welch p, Cliff's delta, and a plain verdict.
2. **CPU vs GPU attribution** (the headline "which side regressed") -- the frame is
   split into the main thread blocked on the GPU and the CPU residual. "CPU blocked
   on GPU" is the inclusive time of the two backend-independent work-cycle brackets
   (`low_latency` = `d3d::begin_frame` GPU-latency wait, `blockedPresent` = present);
   a rise there means the GPU got slower, not the CPU. This works even for captures
   with **no detailed GPU timeline**. When a detailed GPU timeline is present its
   frame time is shown alongside.
3. **GPU draw stats** (draw calls, triangles, instructions, render targets, programs,
   render passes) with significance. Draw stats are rasterization counters only --
   compute / ray tracing is invisible to them, so equal draw stats are NOT equal GPU
   workload.
4. **Regressions / improvements** (the primary answer), reported in **separate GPU
   and CPU sections** since the two need opposite fixes. Each scope is ranked by ms
   per frame with self-time and call-count deltas, q-value, effect size, and
   `file:line`. GPU rows are all GPU-timeline events (no `kind` column). CPU rows
   carry a `kind`: **CPU** (actual CPU work), **GPU-wait** (the main thread blocked on
   the GPU: present / swapchain / latency / fence / readback), or **Wait** (blocked on
   other work -- job pool, another thread: `tpool_wait`, `ecs_pfor_wait`, ...).
5. **Moved more than the frame as a whole** -- a *secondary* lens: scopes whose
   change exceeds the overall frame drift. Useful only if you independently suspect a
   uniform run-to-run drift (GPU clock/thermal). A scope that *caused* the frame
   change moves in step with it, so in-step regressions in (4) are NOT noise.
6. **New / removed scopes** -- present in only one capture.

### Method

Each capture's per-frame values are independent samples of a distribution (two
separate runs -- frame i in OLD is unrelated to frame i in NEW), so distributions
are compared, not frames pairwise:

- **Mann-Whitney U** (two-sided, tie-corrected normal approximation) is the
  significance test -- robust to the right-skewed, spike-prone shape of frame times.
- **Welch's t-test** (exact p via the regularized incomplete beta) is reported as a
  secondary signal.
- **Cliff's delta** is the nonparametric effect size (|d|>0.33 medium, >0.47 large).
- **Benjamini-Hochberg** controls the false-discovery rate across the ~hundreds of
  scope tests, so breadth of testing does not manufacture significance.

A scope is flagged only when `q < alpha` AND the mean per-frame change clears both a
relative (`--min-change-pct`) and an absolute (`--min-delta-ms`) floor.

### Tips

- For firmer conclusions, capture several runs per side; single-run variance is real
  (comparing two runs of the same build typically shows a small uniform drift plus a
  few genuinely out-of-step scopes).
- Use `--skip-warmup` to drop load-spike frames at the start.
- To investigate one regressed scope's internals, re-convert with
  `--scope <name>` (and, if the capture has sampling, inspect its `samples.byScope`).
