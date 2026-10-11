# vecmath: codegen and measurement

The same source compiles differently under MSVC (PC client) and clang (consoles, mobile,
servers). `microarch.md` gives the hardware cost; this file gives what is emitted and how to
measure it.

## Codegen facts

- **SLP:** clang pairs adjacent scalar float/double math, loads and converts included, into packed
  SSE even outside loops; MSVC leaves scalar double math scalar. So a vec rewrite gains several
  times more on MSVC than a clang measurement shows, and per-component source can already be
  vector code under clang: judge by the assembly of the build that ships.
- **Legacy SSE copies:** two-operand SSE needs a register copy for each live source. MSVC emits
  2-3x more copies than clang for two-register structs (the non-AVX `vec4d`), about the same for
  `vec4f`; recent Intel cores eliminate only some moves, so copy-heavy MSVC code lags clang.
- **Float->double:** neither compiler folds a float load into a scalar float->double convert: it
  is `movss` plus a register convert (2 uops, one on the shuffle port). Only a vector source or
  an SLP pair gets the packed convert, which does two lanes for that cost.
- **Fixed loops:** MSVC keeps short fixed-count loops that clang unrolls and folds, so a value
  that is constant for a literal index (a `v_make_vec4f_mask` lane mask, a `v_bbox3_point`
  corner) is rebuilt on every iteration: compute it outside the loop with literal indices.
- **FP agreement:** x64 `/fp:fast` does not reassociate intrinsic chains, so MSVC and clang agree
  for one ISA; 32-bit `/fp:fast` does (~2 ulp). Scalar references move with SLP and contraction:
  expect bit equality within one build, and a 1-ulp band across builds.

## Measurement protocol

1. Keep every output live (store all lanes to memory the compiler cannot prove dead): a harness
   that reads one lane measures a pruned function.
2. Measure **latency** (each input derives from the previous result) and **throughput**
   (independent inputs). Straight-line code in a loop is bounded by how many calls fit the
   reorder buffer, so variants can rank differently under the two (`microarch.md`).
3. On hybrid CPUs, pin to a P-core (`SetThreadAffinityMask`; Windows enumerates them first), run
   the variants interleaved over several passes, and report the minimum and median: noise falls
   from ~7% to ~2%.
4. Use the shipping compiler and flags (jam's `cl.exe` with `/fp:fast`, clang with the console or
   server `-march`). Put noinline variants (`__declspec` on MSVC) in one TU and read `-FA`/`-S`
   for the per-function instruction mix.
5. Use llvm-mca for structure and port pressure, not for a ranking: its recent-core models have
   inverted measured orders.

`windows.h` defines `IN`, `OUT`, `min` and `max` as macros (`NOMINMAX` removes the last two): a
template parameter named `OUT` does not compile.
