# vecmath: per-instruction cost and target hardware

`usage.md` holds the call-level rules with their reasons; they work without this file. Read this
file while you write or port a function, or when a SIMD hot loop raises a question that no rule
answers. Intrinsics express semantics, not fixed assembly: our options (fast-math, contraction)
let the compiler pick equivalent, faster instructions, so when a cost matters, inspect the
emitted hot loop.

## Lane utilization: SoA vs per-vector

Layout is the only factor here with a 4x ceiling instead of a single-digit percentage. Per-vector
Point3 work uses 3 of 4 lanes, and `_x` work uses 1 of 4. Both can be right locally (skipping `.w`
buys nothing, and a narrow divider makes `_x` cheaper), but they cap a loop at 75% or 25% of the
width. SoA holds x, y and z of four objects in separate `vec4f`s: every lane works, cross-lane
traffic disappears, and a dot product is 3 mul + 2 add (no shuffles, no horizontal reduction, no
`.w` discipline, no `v_ldu_p3` over-read).

Use SoA for long sweeps (culling, particles, batch transforms, broadphase, ray packets, skinning),
not for one-off per-object math or branchy per-body work. Storage usually stays AoS, and that is
the expected pattern: `v_ld_soa2/3/4`/`v_ldu_soa2/3/4` repack packed float2/3/4 arrays on load
(one `ld2/ld3/ld4` on NEON, a short shuffle sequence on SSE), and `v_st_soa*`/`v_stu_soa*` write
them back. The SSE repack is cheap but not free, so work too short to amortize it over 4
elements still loses. SoA code must be branchless (masks, `v_sel`, `v_btsel`), because the 4
elements in flight cannot branch separately. A loop full of `v_dot3`, horizontal reductions and
`_x` calls has a layout problem, not a micro-op problem.

## Supported targets

| Target | Facts that affect code |
|---|---|
| PC client | Main build: Windows x86-64, SSE4.1. SSE4.1 is the compatibility floor; a typical PC is Skylake/Zen or newer with AVX2+FMA. Favor recent big cores, but laptop and budget chips have few or no P-cores, so the small-core guidance applies to them too. |
| Current consoles | Xbox Series X/S and PS5, AMD Zen2: Scarlett builds with `-march=znver2` (PS5 Sony clang targets the same CPU). AVX2+FMA+BMI2 with real 256-bit datapaths. |
| Previous consoles | Xbox One and PS4, AMD Jaguar: Xbox One builds with `-march=btver2 -mno-bmi2` (PS4 Sony clang targets the same CPU). AVX but no FMA and no BMI2; 256-bit ops are slow (2x128). |
| Servers | Linux x64, `-march=haswell`: AVX2+FMA+BMI2 (`skyquake/prog/jamfile`). |
| Mobile | ARMv8, mostly Android, first-class: NEON code quality matters as much as SSE. |
| Switch | AArch64 Cortex-A57: 128-bit ASIMD without the A53/A55 64-bit split, but asymmetric pipes. Integer SIMD dual-issues more than FP; 128-bit FP FMA is ~1/cycle (half of A76+), 128-bit integer multiply ~0.5/cycle; FP divide blocks for 7-32 cycles. |

Both console generations are active platforms. A rarely used SSE2 min-cpu build exists; ARMv7 is
not supported. Steam survey, June 2026: SSE4.1 98%, AVX 97%, AVX2 95%, AVX-512 23%, and 6 or 8
physical cores are most common; our games also run well on machines older than the Steam
average. The FP environment (fast-math, FMA contraction, `VECMATH_NO_FMA`, flushed denormals,
relaxed NaN compares) is a caller-visible contract in `usage.md`.

## Div/sqrt width

Big cores give scalar and packed full-precision div/sqrt similar throughput. Small cores serve
lanes from a narrow unit, so a packed op costs up to lanes-times more: Jaguar `sqrtss` is ~16c
against ~27c for `sqrtps` (div is similar, neither is fully pipelined); Intel E-cores (Gracemont)
run packed div/sqrt ~2x slower than scalar; A53/A55 split each 128-bit NEON op into two uops, so
the two-lane forms cost half. This is why the broadcast `v_length*`/`v_norm*`/
`v_mat44_max_scale43` use `v_sqrt_x` internally. Cheap ops (add, mul, min, max, shuffles) have no
such penalty on x86: do not scalarize them. When all lanes carry real work, batch them into one
packed op (one rsqrt for the three `v_mat44_remove_scale33` lengths).

## Avoid

| Instruction or form | Why, and what to use instead |
|---|---|
| `dpps`/`dppd` | multi-uop and high latency on every x86 core (worst on Jaguar); explicit mul + shuffle + add schedules better and maps to NEON |
| `haddps`/`hsubps` | decode to two shuffles + add and load the shuffle port; the explicit sequence is never worse. NEON pairwise `vpadd`/`faddp` is one cheap instruction. |
| `pmulld` (`v_muli` on SSE4) | expensive on several Intel big-core generations (cheap on Zen); keep it off critical chains unless measured on the target |
| MXCSR rounding-mode change | serializes the FP pipeline; use `v_floor`/`v_ceil`/`v_trunc`/`v_round_ieee` (one `roundps` each on SSE4.1), or `v_round` (ties away from zero: a trunc plus a tie fix on SSE, one `vrndaq` on NEON) |
| NEON `tbl2/3/4` | 2-4x cost on little cores; single-register `vqtbl1q` is fine |

## Cost model and crossings

- **Intel shuffle port:** Skylake-family cores run all vector shuffles on port 5, but blends and
  logic on 3 ports. `movss`/`movsd` merges count as shuffles, and so do `insertps`/`unpck` that
  zero or merge a lane; use an and-mask or immediate blend where it is equivalent. Ice Lake adds
  a second in-lane shuffle port (cross-lane stays on port 5), and Zen always has two, so this is
  mainly a Skylake-era concern, but the tuning never hurts later cores.
- **Dependencies:** big x86 cores have two FP mul/add/FMA ports, so dependency chains, not ports,
  are usually the limit: use independent accumulators joined at the end (`v_mat44_mul_vec4`).
  Straight-line code called per object is bounded by the reorder buffer, so critical-path depth
  and uop count both pay: three serial shuffles can lose to six independent ones. Measure latency
  and throughput (`compilers.md`).
- **Converts, permutes, constants, moves:** float<->double converts (packed, and the register forms
  of `cvtss2sd`/`cvtsd2ss`) are 2 uops on Intel big cores, one on the shuffle port; a packed
  convert does two lanes for that cost. AVX permutes across the 128-bit halves (`vpermpd`,
  `vperm2f128`) take 3 cycles against 1 in-lane, and `vec4d` rotations and cross products pay
  them. A compile-time `v_splats(1.f)` is usually a full-width constant load (load port); a splat
  of a runtime scalar in a register is a shuffle, and `vbroadcastss` helps only from memory.
  `movaps` is usually eliminated at rename on big x86 cores, but it costs front-end bandwidth,
  elimination has limits (errata disabled it on some generations), and Jaguar executes it: do not
  fear temporaries, but inspect move-heavy hot loops.
- **ARM pipes:** A76+ and Apple M have two symmetric 128-bit ASIMD pipes; A57/A72 put FP on one
  pipe. Intel shuffle-port pressure does not transfer: the scarce NEON resources are the divider
  and the little-core width.
- **Divider:** div/sqrt capacity is scarce on all targets. Big x86 cores pipeline it partially
  (Skylake `divps`: ~11c latency, one result per ~3c), ARM cores up to A72 block for 7-32
  cycles, and consecutive independent div/sqrt serialize either way (packed vs `_x`: see above).
- **Scalar/vector boundary:** `movd`/`pinsr`/`pextr` and ARM `umov`/`fmov` cost several cycles
  each way (especially on Jaguar): keep the math in vector registers and extract once.
  `movmskps`/`ptest` results live on the GPR side: branch on them, do not feed them back into
  vector code. A wide load that overlaps several narrower earlier stores cannot use store
  forwarding and waits until they reach L1 (the `Point3_vec4` antipattern in `usage.md`).
  Integer shuffles on float data (or the reverse) add 1-cycle bypass delays on Jaguar and older
  Intel cores; use same-domain shuffles when possible.

## AMD and drift

Most published optimization data is Intel-focused, but Zen matters: current consoles are Zen2,
and AMD is ~45% of Steam PCs. Zen has two shuffle ports, a cheap `pmulld` (~3-4c on Zen), and a
1-uop `blendvps`/`v_sel` (2+ uops on most Intel cores). Its separate add and mul/FMA pipes give
mixed streams twice the Skylake-era throughput (Golden Cove later added dedicated adders).
`dpps` is even worse on AMD. FMA latency is 5c on Zen1/2 against 4c on Zen3+ and Intel, so
independent accumulators matter even more on consoles. Zen2 AVX2 gathers are many-uop and no
faster than scalar loads: do not port Intel gather tricks. The `rcpps`/`rsqrtps` estimate bits
differ between AMD, Intel and NEON, so keep `_est`/`_unprecise` out of code that must be
bit-identical across clients (replays, lockstep). When two encodings are otherwise equal, use
the AMD-friendly one; it rarely costs Intel anything.

Costs drift: calibrate on recent Intel (Golden/Raptor Cove 2021-2023, Arrow Lake) and Zen3/4/5,
not on Skylake (2015). FP add latency went from 4c (Skylake) to 3c (Golden Cove) and ~2c (Zen5);
FMA stays at 4c. div/sqrt got faster in every generation, so on Zen3+/Alder Lake+ an occasional
div or sqrt rarely merits a restructure, and estimates pay off less. Skymont E-cores (Arrow Lake,
2024) roughly double Gracemont SIMD throughput, so the E-core penalty shrinks. For exact costs,
use uops.info and the AMD per-family optimization guides (family 17h covers the console Zen2).

ARM drifts too. A76 (2018) doubled big-core NEON to two symmetric pipes, and Cortex-X and Apple
M run four. ARMv9 little cores (A510/A520) move past the A53/A55 model, but their width and
sharing depend on the configuration: do not extrapolate old little-core costs. FP divide is
faster and no longer fully blocking from A76 on. Android fleets span all of these, so check the
actual core, not "ARM". ARMv8/AArch64 is our only ARM target, and ARMv7-era ARM optimization
articles are often not valid for it: it has 32 x 128-bit registers (ARMv7: 16), IEEE NEON FP
(ARMv7: flush-to-zero only), FMA, vector div/sqrt (ARMv7: estimates only) and horizontal
reductions, and no VFP/NEON mode-switch or NEON-to-GPR pipeline stalls (crossings still cost a
few cycles).

Full-precision div/sqrt is fast on big x86 cores, so `_est` mostly pays off on small cores, on
ARM (fdiv/fsqrt stay long-latency even on big cores), or with several in flight: it is not an
automatic win. Exception: precise `v_rsqrt` is a sqrt and a div that serialize on the one
divider, so `v_rsqrt_est` keeps a ~2x throughput edge even on big cores until the divider is
nearly free (Zen3+/Ice Lake+).
