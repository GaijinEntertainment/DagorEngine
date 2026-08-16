# vec4d - double-precision vector math

Rarely used: almost all engine math is single precision. This exists for the places that must
keep `DPoint3` precision (large world coordinates, some physics) and want them vectorized.
Implementation is `dag_vecMath_double.h` (SSE/AVX and NEON in one file), included via
`dag_vecMath.h`. The API is `vd_`-prefixed.

## Layout
One `__m256d` on AVX, else two 128-bit halves (.xy, .zw). The layout therefore differs between
AVX and non-AVX translation units, so keep `vec4d` a local compute type - do not put it in a
struct that crosses TU boundaries or gets serialized.

## API shape
No lane-wise 3-component forms: add/sub/mul/neg get `.w` for free from the packed op, and
skipping `.w` in a div/sqrt pays off nowhere we ship (every narrow-divider target has AVX and so
is single-register; NEON re-widens a scalar lane op and adds moves). 3-component forms exist only
where the math differs: `vd_hadd3`, `vd_dot3`, `vd_cross3`, `vd_length3`.

`vd_ldu_p3`/`vd_ldu_p3_safe`/`vd_stu_p3` mirror the float `v_*_p3` DPoint3-layout ops, including
the over-read trade-off and sanitizer routing described in `usage.md`.

## Precision
Unchanged from scalar `DPoint3`: reductions (`vd_hadd*`, `vd_dot*`) add lanes left to right, the
same association scalar code has, so results are bit-identical (given `-ffp-contract=off`, as the
physics libs build). Migrating double math to `vec4d` is a speed change, not a numerical one.
Keep that lane order when editing these functions - it is the property that makes the migration
reviewable.

## Speed
Depends mostly on whether the build has AVX, i.e. whether `vec4d` is one register or two. Against
scalar `DPoint3` in branchy per-body physics code (walker-style, nothing auto-vectorizable):
~1.35x on SSE2/SSE4.1, ~1.5x on AVX, ~1.7x on AVX2+FMA. The default PC client is SSE4.1 and so
gets the smallest win; consoles and dedicated servers are AVX2.

In tight loops over arrays `vec4d` is instead ~2x SLOWER, because the compiler already
auto-vectorizes the scalar version 4 elements at a time - bulk work wants SoA, not per-vector
`vec4d`.
