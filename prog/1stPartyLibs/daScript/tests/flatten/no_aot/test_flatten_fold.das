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
def test_flatten_fold(t : T?) {
    let root = get_das_root()
    t |> run("[flatten] reveal-cascade corpus folds clean") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/no_aot/_fold_fixtures.das")
        t |> success(n >= 3, "expected >=3 twins, checked {n}")
    }
    t |> run("const-identity corpus folds clean (test_flatten_const.das twins)") @(t : T?) {
        let n = check_unit(t, "", "{root}/tests/flatten/test_flatten_const.das")
        t |> success(n >= 70, "expected >=70 twins, checked {n}")
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
        delete bodies
    }
}
