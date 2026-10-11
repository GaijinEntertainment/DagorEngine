# Using vecmath

SIMD C API over SSE2/SSSE3/SSE4.1 and NEON, with a scalar per-lane backend (`_TARGET_SIMD_SCALAR`,
auto-selected without a SIMD ISA, e.g. Cortex-M or RISC-V without V). `vecMathUnitTest`
validates it on x64 via `jam -sVecMathScalar=yes`; the rest of the engine still assumes SSE or
NEON. Vecmath serves transforms, physics, BVH, culling and animation.

`vecmath/dag_vecMath.h` is the complete API reference, with a comment on every `v_` function.
Grep its families before you hand-roll anything; what you need probably exists: `v_mat44_*`,
`v_mat33_*`, `v_mat43_*`, `v_quat_*`, `v_bbox3_*`, `v_bsph*`, `v_plane*`, `v_frustum*`, `v_ray*`,
`v_triangle*`.

## Types

| Type | Meaning |
|---|---|
| `vec4f`/`vec3f` | 128-bit float (`__m128` / `float32x4_t` / 16-byte aligned `float f[4]` struct) |
| `vec4i` | 128-bit int (`__m128i` / `int32x4_t` / `int32_t i[4]` struct) |
| `mat33f` / `mat44f` | 3x3 (3 `vec3f`) / 4x4 (4 `vec4f`), column-major |
| `mat43f` | 4x3 row-major: 3 `vec4f`, each row `xyzw` with the translation in `w` |
| `bbox3f` / `bsph3f` | AABB `{bmin, bmax}` / `{center.xyz, radius.w}` |
| `quat4f` / `plane3f` | quaternion / `{normal.xyz, D.w}` |

Double precision (`vec4d`, `vd_`) is a separate, rarely used corner; see `vec4d.md`.

## API contracts

| Form | Contract |
|---|---|
| `_x` | only `.x` is defined; `.yzw` can hold anything, even NaN; cheaper than broadcast |
| `_est` | estimate refined by Newton-Raphson steps; the sequence and count vary by op and ISA, for a similar useful precision. Not an automatic win (`microarch.md`) |
| `_unprecise` | raw hardware estimate: fastest, least precise |
| `_safe` | tolerates zero divisors or page-boundary loads |

- Unsuffixed means precise and identical across platforms. Estimates are opt-in by name, never
  the default, because they are not bit-reproducible across ISAs. Defaults assume well-formed
  inputs; use `_safe` only where edge data is real. Basic rcp/sqrt/rsqrt use real div/sqrt
  instructions, never estimates.
- `v_is_unsafe_divisor` checks `|x| < V_C_VERY_SMALL_VAL` (4e-19, about `sqrt(FLT_MIN)`): if `x`
  passes, `x*x` does not underflow and `1/(x*x)` does not overflow.
- 3-component ops ignore the input `.w`, even NaN. Gameplay code can carry garbage in `.w`, but
  some render code requires zero. A `mat44f` used as 4x3 can likewise have anything in row3 for
  gameplay, but the GPU requires `0,0,0,1`.
- Load/store: `v_ld`/`v_st` aligned, `v_ldu`/`v_stu` unaligned; `*i` is integer. Within a cache
  line, unaligned access is as fast as aligned on modern cores, so the aligned forms act mostly
  as a hardware alignment assert. Legacy (non-VEX) SSE memory operands require alignment, so on
  the SSE4.1 PC build only `v_ld` folds into the consuming op (`v_ldu` stays a separate
  `movups`). VEX folds both, and a folded `v_ld` loses the `movaps` fault. NEON never checks
  alignment.
- `v_ld_soa2/3/4`, `v_ldu_soa2/3/4` and `v_st_soa*`/`v_stu_soa*` de/interleave 4 packed
  float2/3/4 elements to SoA lanes (one `ld2/ld3/ld4` on NEON): use them to batch a classic
  Point3 array without a storage change.
- `v_ldu_p3` reads four floats for a packed Point3. It is fast and safe on the stack, but on the
  last element of a heap array that ends at a page end it faults: use `v_ldu_p3_safe` there
  (reads exactly three). ASAN/TSAN route `v_ldu_p3` to the safe path, so sanitizers neither
  catch nor reproduce the over-read. `v_ldui_p3`/`v_stu_p3` complete the family.
- Store ports are scarcer than load ports (2-3 loads but only 1-2 stores per cycle on big cores),
  and `v_stu_p3` is two store uops (8 + 4 bytes). When the destination has intentional writable
  padding, one full `v_st`/`v_stu` is better; do not add padding only for this (see the
  antipattern below).
- Lane ops: `v_splats`/`v_splatsi`, `v_splat_x/y/z/w`, `v_extract_x/y/z/w`. Compares
  (`v_cmp_gt/ge/lt/le/eq`) return canonical all-ones/zero masks; consume them with bitwise ops,
  `v_sel`/`v_sel_b`/`v_btsel`/`v_bbox3_sel`, `v_check_*`/`v_test_*`, or `v_signmask`/`v_truemask`.
  `v_cast_*` reinterprets bits (free); `v_cvt_*` converts values (int<->float). FP16 conversions
  are `v_half_to_float`/`v_float_to_half_rtne`; `_up`/`_down` round directionally for
  conservative bounds.

## Prefer the named operation

These families are hand-mapped to the cheapest form on each ISA; a hand-written equivalent
usually favors SSE and loses on NEON.

- **Swizzle:** use `v_perm_<xyzw>`; a two-vector perm names the second source's lanes `abcd`
  (`v_perm_xzac`), and `v_rot_1/2/3` rotate lanes. SSE `shufps` does any pattern in one op, but
  NEON has no generic immediate shuffle: clang maps common layouts to `ext/zip/uzp/trn/rev` and
  exotic ones to `tbl` with a loaded index. When the layout permits, choose NEON-native zip
  (`v_merge_hw/lw`), uzp (`v_perm_xzac/ywbd`), trn (`v_perm_xazc/ybwd`) or ext
  (`v_perm_yzwa/zwab/wabc`): 1 op on NEON, 1-2 on SSE. When either works, a blend perm such as
  `v_perm_xbzw` beats a shuffle perm on Intel big cores, which have fewer shuffle ports than
  blend ports.
- **Reduction:** use `v_hmin`/`v_hmax` (+ `v_hmini`/`v_hmaxi` and 3-component forms),
  `v_hadd4_x`/`v_hadd3_x`, `v_hand`/`v_hor` (+3), `v_hmul`/`v_hmul3`, `v_dot3`/`v_dot4`,
  `v_length3`/`v_length4`; the `_x` forms skip the broadcast. AArch64 has single-instruction
  `fminv/fmaxv/sminv/smaxv` and integer `addv` (float `faddv` is SVE-only, so `vaddvq_f32` is an
  `faddp` pair), which several helpers use; the obvious x86 form uses `haddps`, which loses to
  the helpers' explicit shuffle + add. Do not require the same bits across backends: `v_hadd4_x`
  is `(x+z)+(y+w)` on SSE2 and scalar, `(x+y)+(z+w)` on SSE4.1 and NEON; `v_dot3_x` is `(x+y)+z`
  on SSE2, NEON and scalar, `(x+z)+y` on SSE4.1. A lane-wise rewrite matches a backend's bits only
  with that association and unfused mul + add.
- **Compare/branch:** prefer `v_check_*`/`v_test_*` to `v_signmask` plus an integer compare. NEON
  has no movemask and emulates it: with current clang, `v_signmask(a)==0b1111` is 10 NEON
  instructions against 5 for `v_check_xyzw_all_true`, and on SSE both are 4. For the bits of a
  compare result, `v_truemask` gives the same value as `v_signmask` with one NEON op less. Better
  still, stay branchless.
- **Divider width:** when one lane holds real work, use an `_x` form (then `v_splat_x` if you need
  a broadcast) instead of a packed div/sqrt: narrow dividers on small cores charge packed work up
  to lanes-times more. When all lanes carry real work, use one packed op. A loop built from
  one-lane work and reductions needs SoA, not call-level tuning (`microarch.md`).
- **Matrix-wide:** prefer `v_mat33_remove_scale`, `v_mat44_remove_scale33`, `v_mat44_apply_scale33`,
  `v_mat44_max_scale43(_sq/_x)` and `v_mat44_scale43_sq` (column lengths),
  `v_mat44_spectral_norm43_x`/`_bound_x` (the real stretch of a sheared basis),
  `v_mat33_orthonormalize`, `v_mat44_orthonormalize33`, `v_mat33_decompose`, `v_mat4_decompose`,
  and `_det`/`_inverse`/`_orthonormal_inverse` to per-column code. They batch what per-column code
  serializes (three column lengths in one packed rsqrt, not three trips through the divider),
  and the decompose and quat-from-matrix functions apply the mirror convention below.
- **3x3 inverse:** its columns are cross products of the rows (and its rows, of the columns):
  start from the side the data arrives on (`v_ldu_soa3` reads the rows of a packed 4x3), and no
  transpose is needed.
- **Lane order:** a helper that returns related quantities across lanes documents their order.
  Reductions and per-lane thresholds do not depend on it; pairing lane k of two results, or
  subtracting a lane from a total, does. Keep the order when you rewrite such a helper.

## Correctness traps

- Mirrored (det < 0) matrices have one engine-wide convention: `v_mat33_decompose`,
  `v_mat4_decompose` and `v_quat_from_mat*` test the determinant, flip col2 and fold the sign into
  `scale.z`. Hand-written per-column normalization silently disagrees on mirrored input, which is
  the main correctness reason to use these functions.
- `v_mat33_remove_scale`/`v_mat44_remove_scale33` only divide each column by its length, so a
  mirrored basis stays mirrored. They are the primitive under the functions above, not a
  replacement for them: `v_quat_from_mat33` removes the scale, then flips col2 itself.
- Matrix destinations can alias inputs (`v_mat44_mul(m, m, rel)`).
- `v_sel`/`v_seli` read only the sign bit of the selector on every backend (SSE4.1 `blendvps`, a
  sign-fill shift on SSE2 and NEON, a sign test on scalar), so a sign-only selector works.
  `v_btsel`/`v_btseli` select per bit: they need canonical all-ones/zero masks (as `v_cmp_*`
  produce), or the result mixes the bits of both inputs.
- `v_norm*` of a zero or near-zero vector gives inf/NaN; `v_norm*_safe(a, def)` returns `def`
  when length^2 fails the unsafe-divisor test.
- `v_quat_from_orthonormal_mat*` (fast tier) requires orthonormal input and is wrong for scaled
  input; its `0.5*rsqrt` case scaling gives unit output to 4e-7 (rotation 1.6e-6).
  `v_quat_from_mat*` (robust tier, for untrusted matrices) removes per-column scale with one
  rsqrt, applies the mirror convention, then `v_norm4`: scale removal cannot repair sheared
  columns, and the conversion is unit only for a truly orthonormal basis.
- Results usually define all four lanes; `_x` forms define only `.x`. `v_cross3`, `v_norm3`,
  `v_mat44_scale43_sq` and vec3 transforms leave `.w` unspecified unless the header says
  otherwise: never let it leak into 4-component math.

## Floating-point environment

- Builds use fast math: clang `-ffast-math -ffinite-math-only -mrecip=none`, Windows and Xbox
  `/fp:fast`. The compiler can reassociate and contract, but must not substitute rcpps/rsqrtps
  estimates for div/sqrt: estimates enter only through `_est`/`_unprecise`. Finite-math lets the
  optimizer delete NaN/Inf tests, which is why `v_is_nan` and `v_is_not_finite` use `volatile`.
- clang fast-math (`-ffp-contract=fast`) and VS2022+ `/fp:fast` contract intrinsic mul + add into
  FMA when the compile target has FMA; neither contracts under default FP settings. `v_madd`,
  `v_msub` and `v_nmsub` fuse explicitly where the backend can: on NEON `v_madd`/`v_nmsub` are
  `vfmaq_f32`/`vfmsq_f32` (`v_msub` is a mul and a sub), and on x86 they use FMA only when the
  target has it (`__FMA__`, or AVX2 on MSVC). So the SSE4.1 PC build gets mul + add on every CPU.
  FMA rounds once, so a test like `a*b + c*d != 0` can differ between fused and unfused builds,
  and so between platforms.
- Libs that need cross-ISA consistency (physJolt, gamePhys/collision, landMesh, rendInst) define
  `VECMATH_NO_FMA` and build with x86 `-mno-fma`, `-fno-unsafe-math-optimizations` and
  `-ffp-contract=off`. The define unfuses the explicit FMA of `v_madd` and its siblings on NEON
  and x86; the flags stop compiler contraction (`-ffp-contract=off` alone is not enough under
  fast math).
- A difference of two products is where contraction bites: fuse one product, and `a*b - b*a`
  returns the rounding error of the other instead of 0. `v_cross3` prevents contraction on every
  build with multiplies that no compiler can fuse (`fmulx` on NEON; on FMA x86, muls that feed
  `hsubps`, because FMA has no horizontal form): `a x a` is exactly zero, antisymmetry is bitwise,
  and results match across platforms. Do not rebuild it with `v_nmsub`. Shuffling the products
  before the sub does not help: the optimizer moves shuffles across the math and restores the
  fusable form. For `b = s*a` the result is still nonzero, because the products meet
  already-rounded `b` lanes.
- NaN compare behavior is not stable: fast-math can mirror or invert a predicate (`a < b` into
  `!(a >= b)`), and IEEE compares with a NaN operand are false, so the binary can treat NaN
  differently from the source, and differently between builds and platforms. Do not use compare
  direction to filter NaNs.
- Denormals flush to zero (x86 FTZ/DAZ, ARM FPCR.FZ): there is no gradual underflow, and the
  smallest surviving magnitude is `FLT_MIN` (~1.18e-38).
- 32-bit MSVC DEV builds run scalar code on x87 with NaN exceptions enabled; SSE intrinsics keep
  exceptions disabled.
- Scalar references move with the compiler (SLP, contraction, 32-bit MSVC reassociation;
  `compilers.md`): bit-compare them only within one build. The exception is the `VECMATH_NO_FMA`
  libs above, whose results carry across ISAs.

## Antipattern: transient `Point3_vec4`

`Point3_vec4` is `Point3` plus a pad float, aligned to 16 so that `v_ld`/`v_st` are legal on it.
Use it as intentionally aligned field or array storage. A temporary of it, made only to load or
store a plain `Point3`, costs far more than the unaligned access it avoids.

```cpp
// WRONG: the temp burns a stack slot, forces v out of registers, and the
// read-back copies the 3 floats a second time.
void store_p3(Point3 &p3, vec3f v)
{
  Point3_vec4 tempP4;
  v_st(&tempP4.x, v);
  p3 = tempP4;
}

// RIGHT
void store_p3(Point3 &p3, vec3f v) { v_stu_p3(&p3.x, v); }
```

For loads, use `v_ldu_p3(&p3.x)`. The load proxy is worse still: three narrow stores that feed
one overlapping 16-byte load defeat x86 store-to-load forwarding, and a copy through GPRs pays
the GPR<->vector crossing instead. The pattern hides in profiles when the compiler removes the
copy of a visible local, but it hurts array elements, struct fields, by-reference parameters and
anything across a function boundary.

## Includes

```cpp
#include <vecmath/dag_vecMath.h>      // full API
#include <vecmath/dag_vecMathDecl.h>  // types only (forward declarations)
```
