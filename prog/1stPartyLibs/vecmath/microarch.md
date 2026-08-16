# vecmath: per-instruction cost and target hardware

What the machine costs. The API-level rules these costs justify - which vecmath call to write -
live in `usage.md`, each with its reason, and are actionable without this file. Come here when
adding or porting a vecmath function, or when squeezing a SIMD hot loop (culling, physics, BVH,
animation) raises a question no rule answers.

Intrinsics are not exact assembly coding: the compiler treats them as semantics, not fixed
encodings, and may use equivalent instructions when they are faster; our compilation options
(fast-math, contraction) deliberately give it that freedom. Everything below is about what is
actually emitted - when a cost matters, check the disassembly of the hot loop.

## Lane utilization: SoA vs per-vector
The largest factor here, and the only one with a 4x ceiling rather than a single-digit percentage.

Per-vector 3d math wastes lane width by construction: a vec3f carrying a Point3 does useful work
in 3 lanes of 4, and an `_x` form does useful work in 1 of 4. Both are the right per-vector choice
- skipping .w buys nothing, and `_x` is genuinely cheaper on the narrow dividers below - but they
cap a loop at 75% or 25% of the machine's width however well each individual call is written.

SoA lifts the cap: hold N objects as parallel arrays, one vec4f of x, one of y, one of z, so every
operation computes 4 objects at once with all four lanes carrying real work. Cross-lane traffic
disappears with it. A dot product becomes 3 mul + 2 add - no shuffles, no horizontal reduction, no
.w discipline, and no v_ldu_p3 over-read question, since you load a full vec4f of x rather than a
3-float point.

Worth doing for algorithms that sweep many elements: culling, particle updates, batch transforms,
broadphase, ray packets, skinning. Not worth doing for one-off per-object math, or for branchy
per-body work where each element takes a different path (`vec4d.md` reaches the same conclusion
from the double-precision side).

SoA does not have to be the storage format, and in practice it usually is not: engine data stays
in the classic AoS types and the SoA form is built when it is loaded. That is the expected
pattern, and the deinterleaving loads exist for it - v_ld_soa2/3/4 and v_ldu_soa2/3/4 turn packed
float2/float3/float4 arrays into SoA lanes as part of the load (one ld2/ld3/ld4 on NEON, a short
shuffle sequence on SSE), and v_st_soa*/v_stu_soa* interleave back on the way out. vecmath itself
computes in SoA in many places for the same reason.

The repack is therefore cheap, but it is not free on SSE, so work too short to amortize a
deinterleave over 4 elements still loses. The other constraint is real: SoA needs branchless
code, because the 4 elements in flight cannot branch separately - masks and v_sel/v_btsel, which
is how vecmath is written anyway.

A loop full of v_dot3, horizontal reductions and `_x` forms is usually paying the per-vector tax
rather than asking to be micro-optimized; the fix is the layout.

## Our target hardware
- Main build is x86-64 Windows PC client compiled for 64-bit target with SSE 4.1
- Typical PC is Skylake/Zen or newer with AVX2+FMA; SSE 4.1 is the compatibility floor, not the
  typical machine, and Skylake (2015) is the old tail - weight optimization toward recent big
  cores. On desktop hybrids SIMD-heavy threadpool jobs mostly run on P-cores, but laptop and
  budget chips have few or no P-cores, so the small-core guidance below applies to cheap PCs too
- Calibrate PC capability against the Steam hardware survey (store.steampowered.com/hwsurvey; June
  2026: SSE4.1 98%, AVX 97%, AVX2 95%, AVX-512 23%; 6 or 8 physical cores are the most common),
  keeping in mind our games also run well on machines older than the Steam average
- Current generation consoles (Xbox Series X/S, PS5) are AMD Zen2: Scarlett builds with
  -march=znver2 (PS5 Sony clang targets the same CPU), so AVX2 + FMA + BMI2 with real 256-bit
  datapaths
- Previous generation consoles (Xbox One, PS4) are AMD Jaguar: Xbox One builds with -march=btver2
  -mno-bmi2 (PS4 Sony clang targets the same CPU), so AVX but NO FMA and no BMI2, and 256-bit
  operations are slow (emulated by 2x128)
- Console performance matters on both generations; both are active platforms
- Dedicated servers compiled for Linux 64-bit target with -march=haswell (AVX2 + FMA + BMI2; see
  skyquake/prog/jamfile)
- Mobile phones (ARMv8, mostly Android) are a first-class production platform - NEON code quality
  matters as much as SSE
- Nintendo Switch is aarch64 Cortex-A57 (-mcpu=cortex-a57+fp+simd+crypto+crc): 128-bit ASIMD
  execution without the A53/A55-style 64-bit split, but the pipes are asymmetric - integer SIMD
  dual-issues more than FP, 128-bit FP FMA is about one per cycle (half of A76+), 128-bit integer
  multiply ~0.5/cycle, FP divide blocks for 7-32 cycles
- We support an SSE2 min-cpu build (very rarely used), but ARMv7 is not supported

The floating-point environment we build with (fast-math, FMA contraction, VECMATH_NO_FMA,
flushed denormals, relaxed NaN compares) is a caller-visible contract and lives in `usage.md`.

## Scalar vs packed div/sqrt
On the big cores we target, scalar and packed full-precision div/sqrt have similar throughput,
but small cores serve lanes from a narrow unit, so the packed form costs up to lanes-times more:
- AMD Jaguar (XB1/PS4): sqrtss ~16c vs sqrtps ~27c, div similar, neither fully pipelined
- Intel E-cores (Gracemont in hybrid PCs): narrow divider, packed div/sqrt ~2x worse than scalar
- ARM little cores (Cortex-A53/A55): 64-bit NEON datapath splits every 128-bit op into 2 uops, so
  the 2-lane forms sqrt/div use are half cost

This is why broadcast v_length*/v_norm*/v_mat44_max_scale43 route through v_sqrt_x internally.
Cheap ops (add/mul/min/max/shuffles) have no such penalty on x86; do not contort code to
scalarize them.

## Instructions we avoid (microcoded or high latency)
- dpps/dppd: multi-uop and high-latency on every x86 core we care about, worst on Jaguar; explicit
  mul + shuffle + add sequences schedule better and transfer naturally to NEON.
- haddps/hsubps: decode to 2 shuffles + add on all x86 cores we target - never beat explicit
  shuffle + add and they load the shuffle port; NEON pairwise vpadd/faddp IS single and cheap.
- pmulld (v_muli on SSE4): historically expensive on several Intel big-core generations (cheap
  on Zen); keep it off critical dependency chains unless measured on the actual target.
- MXCSR changes (rounding mode etc.): serialize the FP pipeline; use roundps based
  v_floor/v_ceil/v_round instead of touching control registers.
- NEON multi-register table lookups (tbl2/tbl3/tbl4): 2-4x cost on little cores; single-register
  vqtbl1q is fine.

## Port-unique units and domain crossings
- Skylake-family Intel big cores execute ALL vector shuffles on one port (port 5), while blends
  and bitwise logic run on 3 ports; movss/movsd register merges count as shuffles, and so does
  insertps/unpck used to zero or merge a lane, where an and-mask or immediate blend would not.
  Ice Lake adds a second in-lane shuffle port (cross-lane stays on port 5), Zen always has two,
  so this is mainly a Skylake-era concern - tuning for one shuffle port never hurts newer cores.
- FP mul/add/FMA have two ports on big x86 cores, so dependency chains, not ports, are usually the
  limit: structure reductions as two independent accumulator chains joined at the end (the
  v_mat44_mul_vec4 pattern).
- A compile-time vector constant (v_splats(1.f)) is typically a full-width constant-pool load -
  load-port work; splatting a runtime scalar already in a register costs a shuffle instead.
  vbroadcastss helps only with a memory source; from a register it is another port 5 shuffle.
- Register-register moves (movaps) are usually eliminated at rename on big x86 cores, but they
  cost front-end bandwidth, elimination has limits (errata disabled it on some generations), and
  Jaguar executes them: do not fear compiler temporaries, but eyeball move-heavy hot loops.
- Modern ARM big cores (Cortex-A76+, Apple M) have two symmetric 128-bit ASIMD pipes that accept
  nearly everything; older A57/A72 concentrate FP on one pipe. Shuffle-port pressure is an
  x86/Intel concern; the scarce NEON resources are the divider and little-core width (see above).
- Divider/sqrt capacity is scarce on all targets: big x86 cores pipeline FP div/sqrt partially
  (Skylake divps: ~11c latency, a result per ~3c), ARM cores through A72 block outright (7-32
  cycles, no overlap), and consecutive independent div/sqrt serialize either way. When all lanes
  carry real work, batch them into one packed op (the v_mat44_remove_scale33 pattern: one rsqrt
  for three lengths); when only one lane does, use _x forms (see above).
- GPR<->vector crossings (movd/pinsr/pextr, umov/fmov on ARM) cost several cycles each way and are
  especially slow on Jaguar. Keep math in vector registers; extract once at the end. movmskps/ptest
  results live on the GPR side: fine for branching, avoid feeding them back into vector code.
- Store-to-load forwarding does not cover a load that overlaps several narrower prior stores, so
  the load waits for them to reach L1 (see the Point3_vec4 antipattern in `usage.md`).
- Integer shuffles on float data (and vice versa) can add 1-cycle bypass delays on Jaguar and older
  Intel cores; prefer same-domain shuffles when an equivalent exists.

## AMD vs Intel
Most published optimization data is Intel-focused, yet Zen-aware tuning is especially relevant for
us - current-generation consoles are Zen 2, and AMD is ~45% of CPUs in the Steam Hardware Survey,
a large and growing share of the gaming-PC audience:
- several Intel costs simply do not exist on Zen: the single shuffle port (Zen always has two),
  expensive pmulld (~3-4c on Zen), and variable blends (blendvps, i.e. v_sel) are 1 uop on Zen vs
  2+ on most Intel cores
- Zen has separate FP add and FP mul/FMA pipes, so mixed add/mul streams sustain twice the
  throughput of Skylake-era Intel (which ran both on the same two FMA ports until Golden Cove
  added dedicated adders)
- dpps is even worse on AMD than on Intel (already avoided, see above)
- FMA latency is 5 cycles on Zen1/Zen2 (4 on Zen3+ and Intel big cores): madd dependency chains
  run a cycle per link slower on current consoles than Intel numbers suggest, so independent
  accumulators matter even more there
- AVX2 gathers are many-uop and slow on Zen2: on consoles vgatherdps is no faster than the
  equivalent scalar loads, so do not port Intel-tuned gather tricks there
- rcpps/rsqrtps estimate bits differ between AMD and Intel (and NEON differs from both), so
  _est/_unprecise results are not bit-reproducible across machines; keep estimates out of code
  that must be deterministic across clients (replays, lockstep)
- ties between otherwise equal encodings go to the AMD-friendly one; Intel big cores are rarely
  hurt by it

## Generational drift
- Per-instruction costs drift between generations; calibrate on the last 2-3 (Intel Golden
  Cove/Raptor Cove P-cores 2021-2023, Arrow Lake; AMD Zen3/Zen4/Zen5), not on Skylake (2015):
  - FP add latency dropped from 4c (Skylake ran adds on the FMA units) to 3c on Golden Cove and
    ~2c on Zen5; FMA stays 4c
  - div/sqrt got faster every generation; on Zen3+/Alder Lake+ an occasional div or sqrt is rarely
    worth restructuring around, and _est pays off even less than on Skylake
  - Intel E-cores: Skymont (Arrow Lake, 2024) roughly doubles Gracemont's SIMD throughput, so the
    E-core penalty shrinks on the newest parts
  When a specific cost matters, check uops.info and AMD's per-family Software Optimization Guides
  (family 17h covers the console Zen2) instead of assuming 'modern x86'.
- ARM cores drift the same way: Cortex-A76 (2018) doubled big-core NEON throughput (two symmetric
  128-bit pipes), Cortex-X and Apple M run four; ARMv9 little cores (A510/A520) move past the
  A53/A55 64-bit-SIMD model, but SIMD width and sharing are configuration dependent - do not
  extrapolate A53/A55 costs to them; FP divide is much faster and no longer fully blocking from
  A76 on. Android fleets span all of these, so check the actual core, not 'ARM'.
- We target ARMv8/AArch64 only - a significant break from ARMv7, so ARM optimization articles from
  that era are often no longer valid: AArch64 has 32 x 128-bit registers (ARMv7 had 16),
  IEEE-compliant NEON FP (ARMv7 was flush-to-zero only), FMA, vector div/sqrt (ARMv7 NEON had only
  estimates) and horizontal reductions always available, and no VFP/NEON mode-switch or NEON-to-GPR
  pipeline stalls (crossings still cost a few cycles, see above)
- Full-precision div/sqrt is fast on big x86 cores, so _est variants mostly pay off on small
  cores, on ARM (where fdiv/fsqrt stay long-latency even on big cores), or when several are in
  flight; they are not an automatic win. Exception: precise v_rsqrt is sqrt plus div - two ops
  serializing on the single divider - so v_rsqrt_est keeps a ~2x throughput edge even on big
  cores until the divider is nearly free (Zen3+/Ice Lake+).
