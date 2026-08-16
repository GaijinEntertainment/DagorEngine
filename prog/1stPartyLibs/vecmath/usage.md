# Using vecmath

Platform-abstracted SIMD vector math. Wraps SSE2/SSSE3/SSE4.1 (x86) and NEON (ARM) behind a
unified C API. Used pervasively throughout the Dagor Engine for all performance-critical math:
transforms, physics, BVH traversal, culling, animation.

`vecmath/dag_vecMath.h` is the API reference: every `v_`-prefixed function is declared there with
a comment. Grep it by prefix before writing anything by hand -- what you need probably exists
(v_mat44_*, v_mat33_*, v_mat43_*, v_quat_*, v_bbox3_*, v_bsph*, v_plane*, v_frustum*, v_ray*,
v_triangle*).

## Key types
- `vec4f` / `vec3f` -- 128-bit float vector (__m128 on SSE, float32x4_t on NEON)
- `vec4i` -- 128-bit integer vector (__m128i / int32x4_t)
- `mat33f` -- 3x3 column-major matrix (3 x vec3f)
- `mat44f` -- 4x4 column-major matrix (4 x vec4f)
- `mat43f` -- 4x3 row-major matrix (3 x vec4f, each row is xyzw where w = translation component)
- `bbox3f` -- AABB {bmin, bmax} as two vec4f
- `bsph3f` -- bounding sphere {center.xyz, radius.w}
- `quat4f` -- quaternion as vec4f
- `plane3f` -- plane {normal.xyz, D.w}

Double precision (`vec4d`, `vd_`-prefixed) is a separate rarely-used corner: see `vec4d.md`.

## API conventions
- Suffix scheme:
  - `_x` -- result in .x only, .yzw are allowed to contain anything (even NaN); cheaper than the
    broadcast form
  - `_est` -- hardware estimate refined by Newton-Raphson steps; sequence and step count vary
    per op and platform to land at similar useful precision (not an automatic win, see
    `microarch.md`)
  - `_unprecise` -- raw hardware estimate only (fastest, least precise)
  - `_safe` -- tolerates edge inputs (zero divisors, page-boundary loads); the divisor check
    (v_is_unsafe_divisor: |x| < V_C_VERY_SMALL_VAL, 4e-19 ~ sqrt(FLT_MIN)) is tuned to stay safe
    under squaring: if x passes, x*x does not underflow and 1/(x*x) does not overflow
- Precision/safety policy: the default (unsuffixed) function is precise and identical on every
  platform; speed-for-precision variants (_est/_unprecise) are opt-in by name, never the default,
  since estimates are not bit-reproducible across ISAs. Softer for inputs: defaults assume
  well-formed data, _safe is only for where edge cases (zero divisor, page-edge load) are real.
- Basic rcp/sqrt/rsqrt use real division/square-root instructions, never hardware estimate
  instructions; estimates enter only through the _est/_unprecise variants
- 3-component ops ignore .w of inputs: it can be anything, even NaN. Most gameplay code safely
  carries 3d vectors in vec3f with garbage in .w, but some render code requires .w = 0. Same for
  mat44f holding a 4x3 transform: row3 (the .w lane of the columns) can be anything for gameplay,
  but the GPU requires 0,0,0,1 there
- Loads/stores: v_ld/v_st aligned, v_ldu/v_stu unaligned, *i for integer. On modern cores
  unaligned is as fast as aligned within a cache line, so aligned forms act mostly as a hardware
  alignment assert. Encodings differ: legacy (non-VEX) SSE load+op memory operands (addps xmm,
  [mem]) require alignment, so on the SSE4.1 PC build only v_ld folds into the consuming
  instruction (v_ldu stays a separate movups); VEX operands don't require alignment, both forms
  fold, and a folded v_ld loses the movaps fault; NEON never checks alignment
- Deinterleaving loads/stores: v_ld_soa2/3/4 (16-byte aligned) and v_ldu_soa2/3/4 read an array of
  packed float2/float3/float4 straight into SoA x/y/z/w lanes - 4 elements per call, one ld2/ld3/
  ld4 on NEON - and v_st_soa*/v_stu_soa* write SoA lanes back interleaved. This is how a classic
  Point3 array is fed to a batched algorithm without changing its storage
- Point3/DPoint3 layout has its own load/store ops: v_ldu_p3, v_ldu_p3_safe, v_ldui_p3, v_stu_p3.
  v_ldu_p3 reads 4 floats for a 3-component vector (fast): harmless on the stack, but a heap
  array of packed 3-float positions whose last element ends exactly at a page end crashes on the
  final load - use v_ldu_p3_safe (reads exactly 3, slower) there. ASAN/TSAN builds route v_ldu_p3
  to the safe path automatically, so sanitizers neither catch nor reproduce the over-read
- Store ports are scarcer than load ports (big cores sustain 2-3 loads but only 1-2 stores per
  cycle), and v_stu_p3 issues two store uops (8-byte + 4-byte); when the destination really has
  writable padding, prefer one full v_st/v_stu. Do not manufacture that padding, see Antipatterns
- Lane ops: v_splats(float) / v_splatsi(int) broadcast a scalar, v_splat_x/y/z/w broadcast a lane,
  v_extract_x/y/z/w read a lane out
- Compares (v_cmp_gt/ge/lt/le/eq) return per-lane masks (all-ones or 0); consume masks with bitwise
  ops (v_and/v_or/v_xor/v_andnot), branchless selects (v_sel/v_sel_b/v_btsel/v_bbox3_sel), the
  v_check_*/v_test_* branching helpers, or v_signmask/v_truemask (lane bits as an int)
- v_cast_* reinterprets bits (free), v_cvt_* converts values (int<->float)
- FP16: v_half_to_float / v_float_to_half_rtne; _up/_down variants round directionally for
  conservative bounds

## Prefer the provided forms over hand-rolled sequences
Each of these families is hand-mapped to the cheapest form on both ISAs. A hand-written
equivalent is tuned for whichever ISA its author had in mind - almost always SSE - and usually
loses on the other one. Per-core cost detail behind these rules is in `microarch.md`; the rules
themselves do not need it.

- Swizzles: v_perm_<lanes> with xyzw lane names; two-vector perms name the second operand's lanes
  abcd (v_perm_xzac); v_rot_1/2/3 rotate lanes. Prefer them over raw intrinsics or exotic custom
  shuffles: shufps encodes any lane pattern in one op so SSE code shuffles freely, but NEON has no
  generic immediate shuffle - clang maps common patterns to ext/zip/uzp/trn/rev and lowers exotic
  ones to tbl with a loaded index constant. The NEON-native two-vector set is single-instruction
  there and 1-2 ops on SSE - prefer these patterns when any layout works: zip (v_merge_hw/lw),
  uzp (v_perm_xzac/ywbd), trn (v_perm_xazc/ybwd), ext (v_perm_yzwa/zwab/wabc). Where the lane
  pattern allows either, a blend-based perm (v_perm_xbzw etc.) beats a shuffle-based one on
  Intel big cores, which run all shuffles on a single port
- Horizontal (cross-lane) reductions have named helpers - v_hmin/v_hmax (+ v_hmini/v_hmaxi and
  the 3-component forms), v_hadd4_x/v_hadd3_x, v_hand/v_hor (+3), v_hmul/v_hmul3, v_dot3/v_dot4,
  v_length3/v_length4. Prefer them over a shuffle/rotate + op chain. aarch64 has fminv/fmaxv/
  sminv/smaxv and integer addv as single instructions with no portable equivalent (float faddv is
  SVE-only, so vaddvq_f32 compiles to an faddp pair), which is why several of these have
  NEON-specific implementations; on x86 the obvious hand-written form reaches for haddps, which
  is on the avoid list and loses to the explicit shuffle + add these helpers use. Note the _x
  forms (v_hadd4_x, v_dot4_x, v_length4_x) define .x only and are cheaper when you do not need
  the broadcast
- Branching on a compare: use the v_check_*/v_test_* helpers rather than v_signmask + integer
  compare. x86 has movmskps so the signmask idiom is cheap there, but NEON has no movemask
  instruction and emulates it - with current clang codegen v_signmask(a) == 0b1111 is 10
  instructions against 5 for v_check_xyzw_all_true(a), where on SSE both are 4. When you do want
  the bits, prefer v_truemask over v_signmask on compare results: same value, one op cheaper on
  NEON. Better still, stay branchless
- When only one lane holds real work, use an _x form (plus v_splat_x if you need it broadcast)
  instead of a packed div/sqrt: big cores are indifferent, but the small cores we ship on serve
  lanes from a narrow divider and charge up to lanes-times more. When every lane does carry real
  work the advice inverts - batch them into a single packed op rather than repeating a scalar one.
  If a whole loop is built out of one-lane work, the data layout is the ceiling and not the call:
  see the SoA section of `microarch.md`
- Matrix-wide operations, not per-column work: v_mat33_remove_scale, v_mat44_remove_scale33,
  v_mat44_apply_scale33, v_mat44_max_scale43(_sq/_x), v_mat44_scale43_sq, v_mat33_orthonormalize,
  v_mat44_orthonormalize33, v_mat33_decompose, v_mat4_decompose, the _det/_inverse/
  _orthonormal_inverse sets. Two reasons. Speed: the matrix-wide form batches what per-column code
  serializes - three column lengths become one packed rsqrt instead of three trips through the
  scarce divider. Correctness: the decompose and quat-from-matrix functions apply one shared
  convention for mirrored input, see below

## Correctness traps
- Mirrored (det < 0) input has one engine-wide convention: flip col2 and fold the negative into
  scale.z. It is applied by v_mat33_decompose, v_mat4_decompose and v_quat_from_mat*, each of
  which tests the determinant itself. Per-column normalization written by hand silently disagrees
  on mirrored matrices, which is the main correctness reason to use those functions
- Scale removal is sign-neutral and does NOT canonicalize handedness: v_mat33_remove_scale and
  v_mat44_remove_scale33 only divide each column by its length, so a mirrored basis comes back
  mirrored. They are the primitive the functions above build on, not a replacement for them -
  v_quat_from_mat33 calls v_mat33_remove_scale and then applies the col2 flip itself
- Matrix functions tolerate aliasing: dest may be an input, so callers can write
  v_mat44_mul(m, m, rel)
- v_sel selectors must be canonical per-lane masks (all-ones/zero, as v_cmp_* produce): SSE4.1
  blendvps reads only the sign bit, but the SSE2 path and NEON vbsl select per bit - a sign-only
  selector works on the PC build and silently breaks on other targets
- v_norm* of a zero or near-zero vector produces inf/NaN lanes; v_norm*_safe(a, def) returns def
  when length^2 fails the unsafe-divisor check
- quat-from-matrix has two tiers. v_quat_from_orthonormal_mat* are the fast tier: they REQUIRE
  orthonormal input (wrong for scaled matrices), relying on the 0.5*rsqrt case scaling for unit
  output (4e-7, rotation 1.6e-6). v_quat_from_mat* are the robust tier for untrusted matrices:
  per-column scale removal (one packed rsqrt via v_mat33_remove_scale), a col2 flip on mirrored
  input per the convention above, and a trailing v_norm4, because scale removal makes columns unit
  but cannot fix sheared (non-orthogonal) columns and the conversion is unit-by-construction only
  for a truly orthonormal basis
- Function results are usually fully defined: _x forms define .x only (see suffix scheme) and
  broadcast forms fill all 4 lanes. The few ops that return a vec3f (v_cross3, v_norm3,
  v_mat44_scale43_sq, vec3 transforms) leave .w unspecified unless the header comment says
  otherwise - do not let it leak into 4-component math

## Floating-point environment
- All platforms build with fast-math semantics: clang platforms pass -ffast-math
  -ffinite-math-only plus explicit -mrecip=none, Windows and Xbox build with /fp:fast. Either way
  the compiler may reassociate and contract FP math but must NOT auto-substitute div/sqrt with
  rcpps/rsqrtps estimates - estimates enter only through the explicit _est/_unprecise API.
  Finite-math-only also means NaN/Inf tests must defeat the optimizer, which is why v_is_nan and
  v_is_not_finite use volatile
- Both clang (fast-math / -ffp-contract=fast) and MSVC since VS2022 (/fp:fast) contract intrinsic
  mul+add into FMA when the target has FMA; neither contracts under default FP settings.
  v_madd/v_msub/v_nmsub are the flag-independent spelling: on NEON v_madd is explicit vfmaq_f32,
  on x86 it uses FMA when the target has it. FMA also rounds differently than mul+add: the product
  is not rounded before the add - more precise, but a trap in code like a*b + c*d != 0.0, where
  the result can differ between contracted and non-contracted builds (and so between platforms).
  Libs that need cross-ISA consistency (physJolt, gamePhys/collision, rendInst) define
  VECMATH_NO_FMA and build with -mno-fma (x86), -fno-unsafe-math-optimizations and
  -ffp-contract=off: the define unfuses the explicit vfmaq in v_madd, the flags stop compiler
  contraction (contract=off alone is not enough under fast-math)
- A difference of two products is where that bites: fuse one of them and a*b - b*a returns the
  rounding error of the other instead of 0. v_cross3 defeats contraction on every build with
  multiplies no compiler can fuse (fmulx on NEON, muls feeding hsubps on FMA-enabled x86 - FMA
  has no horizontal form), so a x a is exactly 0, antisymmetry is bitwise, and results match
  across platforms - do not hand-roll a cross product out of v_nmsub.
  Shuffling the products before the sub is not a substitute: the optimizer moves shuffles across
  the math and restores the fusable form. Note that merely parallel input is still nonzero: for
  b = s*a the products meet already-rounded b lanes, which no rounding discipline can cancel
- NaN semantics are relaxed by fast-math: the compiler assumes ordered inputs and freely mirrors
  or inverts comparisons (a < b into !(a >= b), reversed or negated cmp predicates) when that
  encodes better. IEEE comparisons with a NaN operand are all false, so a mirrored form returns
  the opposite - NaN behavior in the binary can differ from what the code says, and between
  builds and platforms. Do not rely on compare direction to filter NaNs
- Denormals are flushed to zero (FTZ/DAZ on x86, FPCR.FZ on ARM), so subnormals never appear in
  our math: no gradual underflow, the smallest magnitude surviving is FLT_MIN (~1.18e-38)
- DEV builds compiled by MSVC for 32-bit target with x87 scalar operations and enabled NaN
  exceptions, but for SSE intrinsics any exceptions are disabled

## Antipatterns

### Point3_vec4 as a transient proxy for aligned load/store
`Point3_vec4` is `Point3` plus a pad float, aligned to 16 so that v_ld/v_st are legal on it.
Reaching for it to move a value in or out of a plain `Point3` costs far more than the unaligned
access it avoids, and the aligned load or store is the cheapest part of the sequence.

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

The same in reverse for loads: `v_ldu_p3(&p3.x)`, never a copy into a `Point3_vec4` followed by
`v_ld`. There the proxy is worse still - three narrow scalar stores feeding one 16-byte load is
exactly the overlap x86 store-to-load forwarding cannot handle, so the load stalls until they
retire; a copy routed through GPRs pays the GPR<->vector crossing instead.

This survives review because when the `Point3` is a visible local the compiler often sees through
the copy and the profile shows nothing. It bites where the hot loops are: array elements, struct
fields, by-reference parameters, anything across a function boundary.

`Point3_vec4` remains correct as *storage* - a struct field or array element that is genuinely
aligned, where the padding is intentional and v_ld/v_st on it are free. The antipattern is the
throwaway temp, not the type.

## Include path
```cpp
#include <vecmath/dag_vecMath.h>        // full API
#include <vecmath/dag_vecMathDecl.h>    // types only (forward declarations)
```
