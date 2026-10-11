# vec4d - double-precision vector math

Rarely used: most engine math is float. `vec4d` keeps `DPoint3` precision for large-world
coordinates and some physics while vectorizing it. The implementation is `dag_vecMath_double.h`
(SSE/AVX, NEON and scalar), included through `dag_vecMath.h`; public functions use `vd_`.

## Representation and boundary

| Build | `vec4d` layout |
|---|---|
| AVX | one `__m256d` |
| non-AVX | two 128-bit halves (`.xy`, `.zw`) |
| scalar | `double d[4]` |

The layout changes between AVX and non-AVX TUs, so use `vec4d` only as local compute state:
never serialize it or embed it in a struct that crosses a TU boundary.

## API and precision contracts

- No lane-wise 3-component forms: add/sub/mul/neg get `.w` free from the packed op, and skipping
  `.w` in a div/sqrt pays off nowhere we ship (every narrow-divider target has AVX, where `vec4d`
  is one register; NEON re-widens a scalar-lane op and adds moves). 3-component forms exist only
  where the math differs: `vd_hadd3`, `vd_dot3`, `vd_cross3`, `vd_length3`.
- `vd_ldu_p3`, `vd_ldu_p3_safe` and `vd_stu_p3` mirror the float `v_*_p3`, including the over-read
  and sanitizer rules in `usage.md`.
- From floats in memory, use `vd_make_vec4d` plus casts: each float is a load plus a scalar
  convert, and clang SLP-pairs adjacent ones into packed converts. `v_ldu_soa3` then
  `vd_cvt_from_vec4f` puts shuffles ahead of the converts: measured no faster on SSE4.1, slower
  on AVX.
- Reductions (`vd_hadd*`, `vd_dot*`) add lanes left to right like scalar `DPoint3`, so with the
  physics libs' `-ffp-contract=off` results are bit-identical. Keep that association when you
  edit them: it makes a `DPoint3` migration a speed-only change that review can trust.

## When it helps

Speed depends mostly on whether `vec4d` is one register (AVX) or two.

| Workload | Result, and why |
|---|---|
| Branchy per-body physics, not auto-vectorizable | against scalar `DPoint3`: ~1.35x on SSE2/SSE4.1, ~1.5x on AVX, ~1.7x on AVX2+FMA. The PC client is SSE4.1 (smallest gain); current consoles and servers are AVX2, previous consoles AVX. |
| Tight array loop | ~2x **slower**: the compiler already auto-vectorizes the scalar code four elements at a time. Use SoA for bulk work. |

The compiler effect is large (`compilers.md`): clang already packs scalar double math and MSVC does
not, so MSVC gains ~3x more, but its extra copies for the two-register layout slow `vd_` code
itself. On AVX every 4x64 lane rotation is a 3-cycle cross-lane permute (`microarch.md`), so
rotation-heavy code such as cross products can be slower in one register than in two halves.
