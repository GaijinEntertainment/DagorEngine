options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/flatten
require daslib/ast
require daslib/ast_boost
require daslib/rtti
require strings
require daslib/fio

//! Fold-completeness verification over the ACTUAL compiled output. It compiles REAL flatten
//! units and asserts no const-foldable residual (`x*1`/`x+0`/`x*-1`, an all-const call, a
//! const-condition `?:`, a const `!`) survives in the final post-flatten AST — using
//! `flatten_fold_residuals` (daslib/flatten) — the only fold-completeness check (there is no
//! compile-time flag). Running over compiled output, it covers the `[pixel_shader]` backend
//! path uniformly with `[flatten]` (the single-fold + re-infer loop that left the center-tap
//! `uv + float2(0,0)` residual). Three unit kinds:
//!   • every example shader (project-resolved) — the `@pixel_shader` entry function;
//!   • _fold_fixtures.das (this dir) — the `[flatten]` reveal-cascade corpus (`*_flat`);
//!   • test_flatten_const.das — the const-identity corpus. This is the structural FIRED
//!     half of that file's verification; the file itself carries only differential VALUE
//!     checks (a missed fold computes the same value, so it needs this residual proof).
//!
//! Lives in tests/flatten/no_aot/ (excluded from test_aot via tests/.das_test): it requires
//! daslib/flatten at runtime, pulling macro_boost generic instantiations that don't AOT-link
//! in the full-tree batch. Runs in interp + JIT; the residual check is tier-independent.

// Count non-overlapping occurrences of `sub` in `s` — used to tally ` ? ` select nodes in a
// twin's describe() for the select-fusion FIRED proof.
def private count_sub(s, sub : string) : int {
    var n = 0
    var p = find(s, sub, 0)
    while (p >= 0) {
        n ++
        p = find(s, sub, p + length(sub))
    }
    return n
}

// A flatten OUTPUT to validate: a `[flatten]` twin (`*_flat`) or a `[pixel_shader]` entry.
def private is_flatten_output(func : FunctionPtr) : bool {
    if (func.name |> ends_with("_flat")) {
        return true
    }
    for (a in func.annotations) {
        if (a.annotation.name == "pixel_shader") {
            return true
        }
    }
    return false
}

// Compile `path` (project-aware when `proj` is non-empty) and assert every flatten output's
// body is residual-free. Returns the number of flatten outputs checked.
def private check_unit(t : T?; proj, path : string) : int {
    var checked = 0
    var inscope access <- make_file_access(proj)
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.version_2_syntax = true
            compile_file(path, access, unsafe(addr(mg)), cop) $(ok; program; iss) {
                if (!ok) {
                    t |> failure("compile failed: {path}\n{iss}")
                    return
                }
                // honor the unit's own `options _flatten_no_fast_math` — gated rewrites are
                // skipped there by design, and the oracle must mirror that (no false flags)
                let noFastMath = program._options |> find_arg("_flatten_no_fast_math") ?as tBool ?? false
                program_for_each_module(program) $(mod) {
                    for_each_function(mod, "") $(func) {
                        if (func.body == null || func.flags.builtIn || !is_flatten_output(func)) {
                            return
                        }
                        checked ++
                        let r <- flatten_fold_residuals(func, noFastMath)
                        t |> equal(length(r), 0, "{base_name(path)}::{func.name}: {r}")
                        let optr <- flatten_opt_residuals(func, noFastMath)
                        t |> equal(length(optr), 0, "{base_name(path)}::{func.name} (opt): {optr}")
                    }
                }
            }
        }
    }
    return checked
}

// Compile `path` and return describe() of each `*_flat` twin body, keyed by twin name — the
// structural skip-proof half of the `_flatten_no_fast_math` tests (a skipped fold computes the
// same VALUE, so only the surviving shape proves the gate fired).
def private twin_bodies(t : T?; path : string) : table<string; string> {
    var bodies : table<string; string>
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.version_2_syntax = true
            compile_file(path, access, unsafe(addr(mg)), cop) $(ok; program; iss) {
                if (!ok) {
                    t |> failure("compile failed: {path}\n{iss}")
                    return
                }
                program_for_each_module(program) $(mod) {
                    for_each_function(mod, "") $(func) {
                        if (func.body != null && !func.flags.builtIn && func.name |> ends_with("_flat")) {
                            bodies["{func.name}"] = "{describe(func.body)}"
                        }
                    }
                }
            }
        }
    }
    return <- bodies
}

[test]
def test_flatten_fold(t : T?) {  // nolint:STYLE038 — flat list of independent `t |> run` corpora, one per flatten pass
    let root = get_das_root()
    t |> run("[flatten] reveal-cascade corpus folds clean") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/no_aot/_fold_fixtures.das")
        t |> success(n >= 3, "expected >=3 twins, checked {n}")
        // mask const-prop FIRED proof: a break loop's first unrolled iteration is unconditional
        // (the loop mask is statically `true` there), so the acc zero-init is consumed — no
        // `(mask ? new : float3(0,0,0))` first-iter select survives. Without the fold the
        // predicated first iteration keeps the `float3(0f,0f,0f)` init as its false arm.
        var bodies <- twin_bodies(t, "{root}/tests/flatten/no_aot/_fold_fixtures.das")
        let lb = bodies?["fx_loop_break_flat"] ?? ""
        // existence guard: a missing twin leaves lb == "" and the negative find below would pass
        // vacuously (false positive). Assert the twin was produced before checking its shape.
        t |> success(!empty(lb), "fx_loop_break_flat twin must be produced")
        t |> success(lb |> find("float3(0f,0f,0f)") < 0,
            "loop-break first iter must fold unconditional (mask const-prop): {lb}")
        delete bodies
    }
    t |> run("const-identity corpus folds clean (test_flatten_const.das twins)") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/test_flatten_const.das")
        t |> success(n >= 70, "expected >=70 twins, checked {n}")
    }
    t |> run("select-fusion corpus folds clean (test_flatten_select_fusion.das twins)") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/test_flatten_select_fusion.das")
        t |> success(n >= 7, "expected >=7 twins, checked {n}")
        // FIRED proof: an invariant-guarded accumulation chain collapses to exactly ONE select,
        // whatever the op (+, *, min, clamp, vector). The per-iteration-guard control keeps all its
        // distinct-condition selects (4) — the same-condition boundary. (Symmetric-oracle guard:
        // the residual above shares build_select_chain with the transform, so a matcher break would
        // blind both; this positive count is the independent check.)
        var bodies <- twin_bodies(t, "{root}/tests/flatten/test_flatten_select_fusion.das")
        for (nm in ["sf_add_flat", "sf_add_base_flat", "sf_mul_flat", "sf_min_flat", "sf_clamp_flat", "sf_vec_flat"]) {
            let b = bodies?[nm] ?? ""
            t |> success(!empty(b), "{nm} twin must be produced")
            t |> equal(count_sub(b, " ? "), 1, "{nm} must fuse to one select: {b}")
        }
        let pv = bodies?["sf_pervar_flat"] ?? ""
        t |> success(count_sub(pv, " ? ") >= 2, "per-iteration guard must NOT fuse (distinct masks): {pv}")
        // eval-safety gate: a fault-capable true arm (integer division) must NOT be stripped to run
        // unconditionally, so its selects survive (the chain can't grow past the unsafe link).
        let idv = bodies?["sf_intdiv_flat"] ?? ""
        t |> success(!empty(idv), "sf_intdiv_flat twin must be produced")
        t |> success(count_sub(idv, " ? ") >= 2, "integer-division true arm must NOT fuse (eval-safety gate): {idv}")
        delete bodies
    }
    t |> run("optimize corpus folds clean (test_flatten_opt.das twins)") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/test_flatten_opt.das")
        t |> success(n >= 14, "expected >=14 twins, checked {n}")
        // regroup-to-share FIRED proof: reassoc canonicalizes alpha to `(e - b) + 1f`, so a
        // shared `_cse_` let only appears when the regroup re-paired `1f - b` for the CSE
        var bodies <- twin_bodies(t, "{root}/tests/flatten/test_flatten_opt.das")
        let rg = bodies?["r_oneminus_flat"] ?? ""
        t |> success(rg |> find("_cse_") >= 0, "regrouped '1 - b' must CSE-share with the albedo use: {rg}")
        let ra = bodies?["r_anyconst_flat"] ?? ""
        t |> success(ra |> find("_cse_") >= 0, "regrouped '0.5 - b' must CSE-share: {ra}")
        delete bodies
    }
    t |> run("dot corpus fuses clean (test_flatten_dot.das twins)") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/test_flatten_dot.das")
        t |> success(n >= 12, "expected >=12 twins, checked {n}")
        // weighted + negated lanes in one chain land in the mask: dot(a, (2, 3, -4))
        // the hadd must come back as a dot call (symmetric-oracle guard: fuser and residual
        // share dot_chain_groups, so only a POSITIVE assert catches a matcher break blinding both)
        var bodies <- twin_bodies(t, "{root}/tests/flatten/test_flatten_dot.das")
        let h4 = bodies?["d_hadd4_flat"] ?? ""
        t |> success(h4 |> find("dot(") >= 0, "v.x+v.y+v.z+v.w must fuse to dot: {h4}")
        let lu = bodies?["d_luma_flat"] ?? ""
        t |> success(lu |> find("dot(") >= 0, "weighted lanes must fuse to dot: {lu}")
        let ne = bodies?["d_nested_flat"] ?? ""
        t |> success(ne |> find("dot(") >= 0, "lane sum inside a call arg must fuse: {ne}")
        let wn = bodies?["d_wneg_flat"] ?? ""
        t |> success(wn |> find("float3(2f,3f,-4f)") >= 0, "weighted/negated lanes must land in the mask: {wn}")
        delete bodies
    }
    t |> run("hadd corpus fuses clean — DEFAULT width>=3 (test_flatten_hadd.das twins)") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/test_flatten_hadd.das")
        t |> success(n >= 16, "expected >=16 twins, checked {n}")
        var bodies <- twin_bodies(t, "{root}/tests/flatten/test_flatten_hadd.das")
        // FIRED proof: width-3/4 sums pack (each width emits its own hadd ctor); a bare pair stays scalar
        let h3 = bodies?["h_three_flat"] ?? ""
        t |> success(!empty(h3), "h_three_flat twin must be produced")
        t |> success(h3 |> find("hadd(float3(") >= 0, "a+b+c must fuse to hadd3: {h3}")
        let h4 = bodies?["h_four_flat"] ?? ""
        t |> success(!empty(h4), "h_four_flat twin must be produced")
        t |> success(h4 |> find("hadd(float4(") >= 0, "a+b+c+d must fuse to hadd4: {h4}")
        // 2-way is below the default width-3 floor: stays a scalar add, no combine overhead
        let h2 = bodies?["h_two_flat"] ?? ""
        t |> success(!empty(h2), "h_two_flat twin must be produced")
        t |> success(h2 |> find("hadd(") < 0, "a+b must NOT fuse by default (width-2 below the floor): {h2}")
        // 5-way -> plan_groups(5,3) = [4] + 1 scalar tail: one hadd4, the 5th lane stays a scalar add
        let h5 = bodies?["h_five_flat"] ?? ""
        t |> success(!empty(h5), "h_five_flat twin must be produced")
        t |> success(h5 |> find("hadd(float4(") >= 0, "5-way must pack a hadd4: {h5}")
        t |> equal(count_sub(h5, "hadd("), 1, "5-way is hadd4 + a scalar tail, not two hadds: {h5}")
        // 7-way -> [4,3]: hadd4 + hadd3 (the width-3 remainder packs under the default)
        let h7 = bodies?["h_seven_flat"] ?? ""
        t |> success(!empty(h7), "h_seven_flat twin must be produced")
        t |> success(h7 |> find("hadd(float4(") >= 0 && h7 |> find("hadd(float3(") >= 0,
                     "7-way must split into hadd4 + hadd3: {h7}")
        // 8-way packs into TWO PARALLEL hadds, not a nested one — an existing hadd is never a lane,
        // so `hadd(float4(...` never contains an inner `hadd(` (guards the bottom-up nesting trap)
        let h8 = bodies?["h_eight_flat"] ?? ""
        t |> success(!empty(h8), "h_eight_flat twin must be produced")
        t |> equal(count_sub(h8, "hadd("), 2, "8-way must pack into two hadds: {h8}")
        t |> success(h8 |> find("float4(hadd(") < 0, "the two hadds must be parallel, not nested: {h8}")
        // a single reduce lane (`a`; `b*c` is a runtime product -> mad) never reaches a group — no reduce
        let h1 = bodies?["h_one_lane_flat"] ?? ""
        t |> success(!empty(h1), "h_one_lane_flat twin must be produced")
        t |> success(h1 |> find("hadd(") < 0 && h1 |> find("dot(") < 0, "a single lane must NOT fuse: {h1}")
        // const-weighted lanes pack into a DOT (combine + dot beats the mad-chain at width>=3 — the fBm)
        let hw = bodies?["h_weighted_flat"] ?? ""
        t |> success(!empty(hw), "h_weighted_flat twin must be produced")
        t |> success(hw |> find("dot(float4(") >= 0, "a*2+b*3+c*4+d*5 must fuse to a weighted dot4: {hw}")
        // mixed plain + weighted lanes share ONE dot3 (a plain hadd3 would be unweighted), the plain
        // lane carried as a unit weight in the const mask (reassoc may permute the lane/weight order)
        let hwm = bodies?["h_wmixed_flat"] ?? ""
        t |> success(!empty(hwm), "h_wmixed_flat twin must be produced")
        t |> success(hwm |> find("dot(float3(") >= 0 && count_sub(hwm, "dot(") == 1 && hwm |> find("1f") >= 0,
                     "a + b*2 + c*3 must fuse to one weighted dot3 with a unit weight for the plain lane: {hwm}")
        // producer-vectorization: shared elementwise-unary lanes collapse the producer into one call
        let ht = bodies?["h_trig_flat"] ?? ""
        t |> success(!empty(ht), "h_trig_flat twin must be produced")
        t |> success(ht |> find("hadd(sin(float3(") >= 0, "sin(a)+sin(b)+sin(c) must collapse to hadd(sin(float3(...))): {ht}")
        let ht4 = bodies?["h_trig4_flat"] ?? ""
        t |> success(!empty(ht4), "h_trig4_flat twin must be produced")
        t |> success(ht4 |> find("hadd(cos(float4(") >= 0, "four cos lanes must collapse to hadd(cos(float4(...))): {ht4}")
        // mixed producers (sin/cos/sin) do NOT vectorize — leaf-pack keeps the per-lane calls
        let hmx = bodies?["h_mixed_flat"] ?? ""
        t |> success(!empty(hmx), "h_mixed_flat twin must be produced")
        t |> success(hmx |> find("hadd(float3(") >= 0 && hmx |> find("hadd(sin(") < 0,
                     "mixed sin/cos/sin must leaf-pack, not vectorize the producer: {hmx}")
        delete bodies
    }
    t |> run("hadd width-2 path — OPTION _flatten_hadd_pack_pairs (test_flatten_hadd_pairs.das twins)") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/no_aot/test_flatten_hadd_pairs.das")
        t |> success(n >= 2, "expected >=2 twins, checked {n}")
        var bodies <- twin_bodies(t, "{root}/tests/flatten/no_aot/test_flatten_hadd_pairs.das")
        // with the floor lowered to 2, a bare pair packs into hadd2 ...
        let p2 = bodies?["hp_two_flat"] ?? ""
        t |> success(!empty(p2), "hp_two_flat twin must be produced")
        t |> success(p2 |> find("hadd(float2(") >= 0, "a+b must fuse to hadd2 under the option: {p2}")
        // ... and plan_groups(5,2) = [3,2] emits the width-2 remainder alongside the hadd3
        let p5 = bodies?["hp_five_flat"] ?? ""
        t |> success(!empty(p5), "hp_five_flat twin must be produced")
        t |> success(p5 |> find("hadd(float3(") >= 0 && p5 |> find("hadd(float2(") >= 0,
                     "5-way must split into hadd3 + hadd2 under the option: {p5}")
        delete bodies
    }
    t |> run("swizzle corpus folds clean (test_flatten_swizzle.das twins)") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/test_flatten_swizzle.das")
        t |> success(n >= 14, "expected >=14 twins, checked {n}")
        // the FIRED proof (symmetric-oracle guard: fold arm and residual share
        // swizzle_fold_classify, so only a POSITIVE assert catches a matcher break blinding both)
        var bodies <- twin_bodies(t, "{root}/tests/flatten/test_flatten_swizzle.das")
        let c3 = bodies?["s_ctor3_flat"] ?? ""
        t |> success(c3 |> find("float4") < 0, "float4(v,1f).xyz must collapse to v: {c3}")
        let nx = bodies?["s_nestedxz_flat"] ?? ""
        t |> success(nx |> find("v.zx") >= 0, "float4(v.zyx,1f).xz must compose to v.zx: {nx}")
        let dd = bodies?["s_dropdead_flat"] ?? ""
        t |> success(dd |> find("sin") < 0 && dd |> find("float2(x,3f)") >= 0,
            "dropped lanes' compute must die with the re-pack: {dd}")
        // the cost-gate control: a partial base run must survive UN-folded (and un-flagged)
        let pt = bodies?["s_partial_flat"] ?? ""
        t |> success(pt |> find(".yz") >= 0, "a partial base run must stay put: {pt}")
        delete bodies
    }
    t |> run("mad/lerp corpus fuses clean (test_flatten_mad.das twins)") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/test_flatten_mad.das")
        t |> success(n >= 20, "expected >=20 twins, checked {n}")
        // default mode: a plain lerp survives as the call (no expansion), and organic
        // (b-a)*t + a arithmetic still canonicalizes INTO the lerp node via the fuser
        var bodies <- twin_bodies(t, "{root}/tests/flatten/test_flatten_mad.das")
        let lr = bodies?["l_round_flat"] ?? ""
        t |> success(lr |> find("lerp") >= 0, "plain lerp must survive whole by default: {lr}")
        let lm = bodies?["l_manual_flat"] ?? ""
        t |> success(lm |> find("lerp") >= 0, "organic (b-a)*t+a must still re-fuse to lerp: {lm}")
        delete bodies
    }
    t |> run("expand-lerp corpus folds clean (test_flatten_lerp_expand.das twins)") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/test_flatten_lerp_expand.das")
        t |> success(n >= 6, "expected >=6 twins, checked {n}")
        var bodies <- twin_bodies(t, "{root}/tests/flatten/test_flatten_lerp_expand.das")
        // the expand -> re-fuse round trip must come back (the symmetric-oracle guard:
        // fuser and residual oracle share match_lerp, so only a POSITIVE assert catches
        // a matcher break blinding both)
        let rt = bodies?["le_round_flat"] ?? ""
        t |> success(rt |> find("lerp") >= 0, "lerp must round-trip under _flatten_expand_lerp: {rt}")
        // uniform bounds preshade ONLY when expansion exposes G_B - G_A — the FIRED proof
        // (kept whole, the bounds ride as plain operands and nothing extracts)
        let pre = bodies?["le_preshader_flat"] ?? ""
        t |> success(pre |> find("_preshader_") >= 0, "expanded uniform bounds must preshade: {pre}")
        delete bodies
    }
    t |> run("rcp corpus rewrites clean (test_flatten_rcp.das twins)") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/test_flatten_rcp.das")
        t |> success(n >= 8, "expected >=8 twins, checked {n}")
    }
    t |> run("ctor lane algebra corpus rewrites clean (test_flatten_ctor.das twins)") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/test_flatten_ctor.das")
        t |> success(n >= 15, "expected >=15 twins, checked {n}")
        // structural half for the uint kill — the oracle shares const_vec_zero_mask with the
        // transform, so only the surviving shape proves the uint branches exist
        var bodies <- twin_bodies(t, "{root}/tests/flatten/test_flatten_ctor.das")
        let zu = bodies?["z_uint_flat"] ?? ""
        t |> success(zu |> find(" * ") < 0, "uint zero-lane kill must fold the whole product: {zu}")
        delete bodies
    }
    t |> run("no-fast-math corpus: option-aware oracle clean") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/test_flatten_nofastmath.das")
        t |> success(n >= 13, "expected >=13 twins, checked {n}")
    }
    t |> run("no-fast-math corpus: gated shapes survive (skip-proof)") @(t : T?) {
        var bodies <- twin_bodies(t, "{root}/tests/flatten/test_flatten_nofastmath.das")
        let mul0 = bodies?["nf_mul0_flat"] ?? ""
        t |> success(mul0 |> find("* 0f") >= 0, "x*0 must survive: {mul0}")
        let vmul0 = bodies?["nf_vmul0_flat"] ?? ""
        t |> success(vmul0 |> find("float3(0") >= 0, "v*vec0 must survive: {vmul0}")
        let subself = bodies?["nf_subself_flat"] ?? ""
        t |> success(subself |> find(" - ") >= 0, "x-x must survive: {subself}")
        let reas = bodies?["nf_reassoc_flat"] ?? ""
        t |> success(reas |> find("0.75") < 0 && reas |> find("0.25") >= 0, "float consts must stay scattered: {reas}")
        let lrp = bodies?["nf_lerp0_flat"] ?? ""
        t |> success(lrp |> find("lerp") >= 0, "lerp(a,b,0) must survive as lerp (selector is fast-math gated): {lrp}")
        let imul0 = bodies?["nf_imul0_flat"] ?? ""
        t |> success(imul0 |> find("* 0") < 0, "int n*0 must STILL fold: {imul0}")
        let ire = bodies?["nf_ireassoc_flat"] ?? ""
        t |> success(ire |> find("2 +") < 0, "int consts must gather+fold: {ire}")
        let pre = bodies?["nf_preshader_flat"] ?? ""
        t |> success(pre |> find("_preshader_0") >= 0, "preshader extraction must still run: {pre}")
        let dv = bodies?["nf_div_flat"] ?? ""
        t |> success(dv |> find("/ 3f") >= 0, "x / C must survive: {dv}")
        let udv = bodies?["nf_unidiv_flat"] ?? ""
        t |> success(udv |> find("/ G_NF") >= 0, "x / uniform must survive (no rcp): {udv}")
        let zk = bodies?["nf_zkill_flat"] ?? ""
        t |> success(zk |> find("sin") >= 0, "the zero-lane kill must be skipped (sin survives): {zk}")
        let mj = bodies?["nf_mj_flat"] ?? ""
        t |> success(mj |> find("* 12f") >= 0, "majority compensation must be skipped: {mj}")
        let oinv = bodies?["nf_ordered_inv_flat"] ?? ""
        t |> success(oinv |> find("!(") >= 0, "ordered float `!(s>0.9)` flip is NaN-unsafe — must survive no-fast-math: {oinv}")
        let einv = bodies?["nf_eq_inv_flat"] ?? ""
        t |> success(einv |> find("!(") < 0, "equality `!(s==0.9)` flip is NaN-exact — must still fold (to `!=`): {einv}")
        delete bodies
    }
    t |> run("every example shader folds clean (pixel_shader path)") @(t : T?) {
        let proj = "{root}/examples/flatten/flatten_shaders.das_project"
        let base = "{root}/examples/flatten/shaders"
        // an embedder may vendor the sources without examples/ (dagor does) - there is no
        // corpus to fold there, and asserting a shader count would just report the absence
        if (!stat(base).is_dir) {
            t |> success(true, "examples/flatten corpus is not vendored here - nothing to fold")
            return
        }
        var total = 0
        fio::dir(base) $(dirname) {
            let sub = "{base}/{dirname}"
            if (dirname |> starts_with(".") || !stat(sub).is_dir) {
                return
            }
            fio::dir(sub) $(fn) {
                if (fn |> ends_with(".shader")) {
                    total += check_unit(t, proj, "{sub}/{fn}")
                }
            }
        }
        t |> success(total >= 60, "expected >=60 shaders, checked {total}")
    }
}
