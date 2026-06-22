// flatten-fuzz — metamorphic differential fuzzer for daslib/flatten.
//
// Generates random trap-free, pure-integer daslang functions, compiles them
// in-process, and checks the flattened twin against the pristine original over
// many integer inputs: `f(x) == f_flat(x)` must hold for all x. The un-flattened
// original is a free, independent ground truth that never trusts flatten.
//
// Mask-biased grammar (if/early-return/break/continue/nested loops/inlining) —
// that combinatorial surface is where flatten bugs live. Integer domain only, so
// the differential is EXACT. Trap-free (no div/mod/array-index, shift counts
// masked) because predication executes both arms, and a trapping op in an
// untaken branch would panic in the twin but not the original.
options gen2

require daslib/rtti
require daslib/fio
require daslib/debugger
require daslib/random
require daslib/clargs
require daslib/ast
require daslib/flatten
require strings
require math

// ===== generator config + state =====

struct GenCfg {
    maxDepth : int      // expression / nesting depth budget
    maxStmts : int      // max statements per block
    literalPct : int    // leaf literal probability (vs in-scope var)
    constBias : int     // 0..100 — bias toward const subtrees (--strict-fold mode)
    boolDomain : bool   // pure-bool grammar + exhaustive truth-table oracle (vs integer)
}

struct Var {
    name : string
    mutable : bool
}

struct Gen {
    seed : int4
    counter : int
    helperDefs : array<string>  // per-unit accumulator of inlinable helpers
    unitIdx : int
    noHelpers : bool            // true while generating a helper body (no nesting)
    cfg : GenCfg
}

def rnd(var g : Gen&; n : int) : int {
    return random_int(g.seed) % n
}

def chance(var g : Gen&; pct : int) : bool {
    return rnd(g, 100) < pct
}

def next_id(var g : Gen&) : int {
    let i = g.counter
    g.counter++
    return i
}

// ===== expressions / conditions (trap-free: integer or pure-bool) =====

def gen_leaf(var g : Gen&; scope : array<Var>) : string {
    if (empty(scope) || rnd(g, 100) < max(g.cfg.literalPct, g.cfg.constBias)) {
        if (g.cfg.boolDomain) {
            return rnd(g, 2) == 0 ? "true" : "false"
        }
        return "{rnd(g, 21) - 10}"
    }
    return scope[rnd(g, length(scope))].name
}

// Pure-bool value tree: &&/||/==/!= and !. &&/|| short-circuit in the original but the
// predicated twin evaluates both arms eagerly — value-equivalent for side-effect-free bools,
// so any divergence is a real flatten bug. Same operators the live/break/continue masks use.
def gen_bool_expr(var g : Gen&; scope : array<Var>; depth : int) : string {
    if (depth <= 0 || chance(g, g.cfg.constBias / 2)) {
        return gen_leaf(g, scope)
    }
    let k = rnd(g, 100)
    if (k < 60) {
        let ops = fixed_array("&&", "||", "==", "!=")
        let op = ops[rnd(g, length(ops))]
        let l = gen_bool_expr(g, scope, depth - 1)
        let r = gen_bool_expr(g, scope, depth - 1)
        return "({l} {op} {r})"
    } elif (k < 82) {
        let e = gen_bool_expr(g, scope, depth - 1)
        return "(!{e})"
    }
    return gen_leaf(g, scope)
}

def gen_expr(var g : Gen&; scope : array<Var>; depth : int) : string {
    if (g.cfg.boolDomain) {
        return gen_bool_expr(g, scope, depth)
    }
    if (depth <= 0 || chance(g, g.cfg.constBias / 2)) {
        return gen_leaf(g, scope)
    }
    let k = rnd(g, 100)
    if (k < 55) {
        let ops = fixed_array("+", "-", "*", "&", "|", "^")
        let op = ops[rnd(g, length(ops))]
        let l = gen_expr(g, scope, depth - 1)
        let r = gen_expr(g, scope, depth - 1)
        return "({l} {op} {r})"
    } elif (k < 70) {
        let op = rnd(g, 2) == 0 ? "<<" : ">>"
        let l = gen_expr(g, scope, depth - 1)
        let r = gen_expr(g, scope, depth - 1)
        return "({l} {op} ({r} & 31))"   // mask shift count: avoid UB
    } elif (k < 82) {
        let op = rnd(g, 2) == 0 ? "-" : "~"
        let e = gen_expr(g, scope, depth - 1)
        return "({op} {e})"   // space: avoid `--`/`-~` token glue on negative literals
    } elif (k < 90) {
        // exercises the mad disassemble -> re-fuse arc (int mad is exact, so the
        // differential oracle rides free; strict mode checks the fusion residual arms)
        let a = gen_expr(g, scope, depth - 1)
        let b = gen_expr(g, scope, depth - 1)
        let c = gen_expr(g, scope, depth - 1)
        return "mad({a}, {b}, {c})"
    }
    return gen_leaf(g, scope)
}

def gen_cmp(var g : Gen&; scope : array<Var>) : string {
    let cmps = fixed_array("<", "<=", ">", ">=", "==", "!=")
    let op = cmps[rnd(g, length(cmps))]
    let l = gen_expr(g, scope, 2)
    let r = gen_expr(g, scope, 2)
    return "({l} {op} {r})"
}

def gen_cond(var g : Gen&; scope : array<Var>; depth : int) : string {
    if (g.cfg.boolDomain) {
        return gen_bool_expr(g, scope, depth + 1)
    }
    if (depth <= 0) {
        return gen_cmp(g, scope)
    }
    let k = rnd(g, 100)
    if (k < 62) {
        return gen_cmp(g, scope)
    } elif (k < 86) {
        let op = rnd(g, 2) == 0 ? "&&" : "||"
        let l = gen_cond(g, scope, depth - 1)
        let r = gen_cond(g, scope, depth - 1)
        return "({l} {op} {r})"
    }
    let c = gen_cond(g, scope, depth - 1)
    return "(!{c})"
}

// ===== statements / blocks =====

def pick_mutable(var g : Gen&; scope : array<Var>) : string {
    let muts <- [for (v in scope); v.name; where v.mutable]
    if (empty(muts)) {
        return ""
    }
    return muts[rnd(g, length(muts))]
}

// A bare exit terminating an if-branch. break/continue only inside a loop.
def emit_exit(var g : Gen&; scope : array<Var>; loopDepth : int; indent : string) : string {
    if (loopDepth > 0) {
        let k = rnd(g, 3)
        if (k == 0) {
            return "{indent}break\n"
        }
        if (k == 1) {
            return "{indent}continue\n"
        }
    }
    return "{indent}return {gen_expr(g, scope, 2)}\n"
}

// Generate a branch/loop body. `allowExit` permits a single bare trailing exit
// (the branch unconditionally returns/breaks/continues when taken). Returns the
// source plus whether a bare exit was appended.
def gen_branch(var g : Gen&; var scope : array<Var>; depth, loopDepth : int; indent : string; allowExit : bool) : tuple<string; bool> {
    let mark = length(scope)
    let n = 1 + rnd(g, g.cfg.maxStmts)
    var exited = false
    let s = build_string() $(var w) {
        for (_j in range(n)) {
            write(w, gen_stmt(g, scope, depth, loopDepth, indent))
        }
        if (allowExit && chance(g, 45)) {
            write(w, emit_exit(g, scope, loopDepth, indent))
            exited = true
        }
    }
    resize(scope, mark)
    return s => exited
}

// at most ONE branch ever exits, so the if as a whole always falls through →
// the function's final return stays reachable, no unreachable-code errors.
def gen_if(var g : Gen&; var scope : array<Var>; depth, loopDepth : int; indent : string) : string {
    let inner = "{indent}    "
    var s = "{indent}if ({gen_cond(g, scope, 1)}) \{\n"
    let thenR = gen_branch(g, scope, depth - 1, loopDepth, inner, true)
    s += thenR._0
    s += "{indent}\}\n"
    let k = rnd(g, 100)
    if (k < 30) {
        s += "{indent}elif ({gen_cond(g, scope, 1)}) \{\n"
        let er = gen_branch(g, scope, depth - 1, loopDepth, inner, !thenR._1)
        s += er._0
        s += "{indent}\}\n"
    } elif (k < 55) {
        s += "{indent}else \{\n"
        let er = gen_branch(g, scope, depth - 1, loopDepth, inner, !thenR._1)
        s += er._0
        s += "{indent}\}\n"
    }
    return s
}

def gen_loop_source(var g : Gen&) : string {
    let k = rnd(g, 3)
    if (k == 0) {
        return "range({1 + rnd(g, 4)})"
    }
    if (k == 1) {
        let a = rnd(g, 3)
        let b = a + 1 + rnd(g, 3)
        return "range({a}, {b})"
    }
    return "[{rnd(g, 9) - 4}, {rnd(g, 9) - 4}, {rnd(g, 9) - 4}]"
}

// A loop source of EXACTLY length n, so parallel sources line up. Same shapes as
// gen_loop_source (range / range(a,b) / array literal) but length-controlled.
def gen_loop_source_n(var g : Gen&; n : int) : string {
    let k = rnd(g, 3)
    if (k == 0) {
        return "range({n})"
    }
    if (k == 1) {
        let a = rnd(g, 3)
        return "range({a}, {a + n})"
    }
    return build_string() $(var w) {
        write(w, "[")
        for (e in range(n)) {
            write(w, "{e == 0 ? "" : ", "}{rnd(g, 9) - 4}")
        }
        write(w, "]")
    }
}

// loop bodies never bare-exit at top level (allowExit=false) so control always
// falls through the loop; nested if-branches inside may still break/continue/return.
def gen_for(var g : Gen&; var scope : array<Var>; depth, loopDepth : int; indent : string) : string {
    let shape = rnd(g, 100)
    // urange needs the int() binding to feed the integer body, so it's int-domain only.
    if (!g.cfg.boolDomain && shape < 18) {
        return gen_for_urange(g, scope, depth, loopDepth, indent)
    }
    // parallel multi-source `for (a, b in S1, S2)` — unrolled in lockstep, both domains.
    if (shape < 40) {
        return gen_for_parallel(g, scope, depth, loopDepth, indent)
    }
    // The loop counter is always int. In bool domain it can't feed a bool expr, so it stays
    // out of the value scope (unused, hence `_`-prefixed); the body still accumulates across
    // iterations over the enclosing bool vars, exercising loop-carried (break/continue) masks.
    let iv = g.cfg.boolDomain ? "_i{next_id(g)}" : "i{next_id(g)}"
    let src = gen_loop_source(g)
    let inner = "{indent}    "
    var s = "{indent}for ({iv} in {src}) \{\n"
    let mark = length(scope)
    if (!g.cfg.boolDomain) {
        scope |> push(Var(name = iv, mutable = false))
    }
    let br = gen_branch(g, scope, depth - 1, loopDepth + 1, inner, false)
    s += br._0
    resize(scope, mark)
    s += "{indent}\}\n"
    return s
}

// urange(N) source — the loop var is uint, bound through int() into a per-copy local so the
// integer-domain body can use it. Exercises the ExprConstURange -> ExprConstUInt unroll path
// plus the int(const) fold; the per-copy `let` rides the per-iteration-local rename.
def gen_for_urange(var g : Gen&; var scope : array<Var>; depth, loopDepth : int; indent : string) : string {
    let uv = "_u{next_id(g)}"
    let iv = "i{next_id(g)}"
    let n = 1 + rnd(g, 4)
    let inner = "{indent}    "
    let mark = length(scope)
    var s = "{indent}for ({uv} in urange({n}u)) \{\n{inner}let {iv} = int({uv})\n"
    scope |> push(Var(name = iv, mutable = false))
    let br = gen_branch(g, scope, depth - 1, loopDepth + 1, inner, false)
    s += br._0
    resize(scope, mark)
    s += "{indent}\}\n"
    return s
}

// Parallel multi-source loop `for (a, b in S1, S2)`: 2-3 sources sharing one length,
// unrolled in lockstep (count = the shared length, NOT the product). In bool domain the
// vars are unused (`_`-prefixed) but the multi-iterator unroll + masks still get exercised.
def gen_for_parallel(var g : Gen&; var scope : array<Var>; depth, loopDepth : int; indent : string) : string {
    let nSrc = 2 + rnd(g, 2)
    let n = 1 + rnd(g, 3)
    let inner = "{indent}    "
    let mark = length(scope)
    // `i`/`_i` prefix (like the single-source loop) + the unique next_id keeps these clear of
    // the hardcoded params `a`/`b`/`c` and helper `p0`/`p1` — a loop var shadowing a function
    // argument is error[30708], an invalid program that never reaches flatten.
    var names <- [for (_v in range(nSrc)); g.cfg.boolDomain ? "_i{next_id(g)}" : "i{next_id(g)}"]
    let header = build_string() $(var w) {
        write(w, "{indent}for (")
        for (vi in range(nSrc)) {
            write(w, "{vi == 0 ? "" : ", "}{names[vi]}")
        }
        write(w, " in ")
        for (vi in range(nSrc)) {
            write(w, "{vi == 0 ? "" : ", "}{gen_loop_source_n(g, n)}")
        }
        write(w, ") \{\n")
    }
    if (!g.cfg.boolDomain) {
        scope |> reserve(length(scope) + nSrc)
        for (nm in names) {
            scope |> push(Var(name = nm, mutable = false))
        }
    }
    var s = header
    let br = gen_branch(g, scope, depth - 1, loopDepth + 1, inner, false)
    s += br._0
    resize(scope, mark)
    s += "{indent}\}\n"
    return s
}

// A standalone helper function (inlining surface). No nested helper calls.
def make_helper(var g : Gen&) : string {
    let hn = "helper_{g.unitIdx}_{next_id(g)}"
    let savedNo = g.noHelpers
    g.noHelpers = true
    var hscope <- [Var(name = "p0", mutable = false), Var(name = "p1", mutable = false)]   // nolint:LINT003 -- gen_branch mutates scope by-ref
    add_globals(hscope)
    let br = gen_branch(g, hscope, min(2, g.cfg.maxDepth), 0, "    ", false)
    let fin = gen_expr(g, hscope, 2)
    g.noHelpers = savedNo
    let ty = g.cfg.boolDomain ? "bool" : "int"
    g.helperDefs |> push("def {hn}(p0, p1 : {ty}) : {ty} \{\n{br._0}    return {fin}\n\}\n\n")
    return hn
}

def gen_stmt(var g : Gen&; var scope : array<Var>; depth, loopDepth : int; indent : string) : string {
    let mut = pick_mutable(g, scope)
    let canAssign = !empty(mut)
    let canNest = depth > 0
    let canBreak = loopDepth > 0
    let k = rnd(g, 100)
    if (canNest && k < 22) {
        return gen_if(g, scope, depth, loopDepth, indent)
    }
    if (canNest && k < 34) {
        return gen_for(g, scope, depth, loopDepth, indent)
    }
    if (canAssign && k < 56) {
        if (g.cfg.boolDomain) {
            return "{indent}{mut} = {gen_expr(g, scope, 2)}\n"
        }
        let kk = rnd(g, 6)
        if (kk == 4) {
            return "{indent}{mut}++\n"
        }
        if (kk == 5) {
            return "{indent}{mut}--\n"
        }
        let ops = fixed_array("=", "+=", "-=", "*=")
        return "{indent}{mut} {ops[kk]} {gen_expr(g, scope, 2)}\n"
    }
    if (k < 74) {
        return "{indent}return {gen_expr(g, scope, 2)} if ({gen_cond(g, scope, 1)})\n"
    }
    if (canBreak && k < 86) {
        let bc = rnd(g, 2) == 0 ? "break" : "continue"
        return "{indent}{bc} if ({gen_cond(g, scope, 1)})\n"
    }
    if (!g.noHelpers && chance(g, 55)) {
        let hn = make_helper(g)
        let a1 = gen_expr(g, scope, 2)
        let a2 = gen_expr(g, scope, 2)
        let nm = "v{next_id(g)}"
        scope |> push(Var(name = nm, mutable = true))
        return "{indent}var {nm} = {hn}({a1}, {a2})\n"
    }
    let nm = "v{next_id(g)}"
    let e = gen_expr(g, scope, 2)
    scope |> push(Var(name = nm, mutable = true))
    return "{indent}var {nm} = {e}\n"
}

def gen_function_body(var g : Gen&) : string {
    var scope <- [Var(name = "a", mutable = false), Var(name = "b", mutable = false), Var(name = "c", mutable = false)]
    add_globals(scope)
    let br = gen_branch(g, scope, g.cfg.maxDepth, 0, "    ", false)
    let fin = gen_expr(g, scope, 3)
    return "{br._0}    return {fin}\n"
}

// One self-contained unit: helpers + [flatten] fn (leaves the pristine original
// AND emits the _flat twin) + an [export,pinvoke] driver calling both.
def gen_unit(var g : Gen&; idx : int) : string {
    g.unitIdx = idx
    resize(g.helperDefs, 0)
    let body = gen_function_body(g)
    // Fold completeness (strict mode) is verified post-compile by walking the twin with
    // `flatten_fold_residuals` (see run_batch) — not by a compile-time annotation flag. The
    // const-density bias that makes residuals likely is driven by cfg.constBias, set in main.
    let ann = "[flatten]"
    let ty = g.cfg.boolDomain ? "bool" : "int"
    // The driver always takes/returns int so the harness (int3 inputs, int out-params, exact
    // int compare) stays domain-agnostic; a bool candidate marshals 0/1 at the boundary.
    let call = g.cfg.boolDomain ? "(a != 0, b != 0, c != 0)" : "(a, b, c)"
    let wrap = g.cfg.boolDomain ? " ? 1 : 0" : ""
    return build_string() $(var w) {
        for (h in g.helperDefs) {
            write(w, h)
        }
        write(w, "{ann}\ndef cand_{idx}(a, b, c : {ty}) : {ty} \{\n{body}\}\n\n")
        write(w, "[export, pinvoke]\ndef drive_{idx}(a, b, c : int; var o_orig : int?; var o_flat : int?) \{\n")
        write(w, "    unsafe \{\n")
        write(w, "        *o_orig = cand_{idx}{call}{wrap}\n")
        write(w, "        *o_flat = cand_{idx}_flat{call}{wrap}\n")
        write(w, "    \}\n\}\n\n")
    }
}

// ===== harness =====

// Set once in main. When true, generated files carry `options log_infer_passes`,
// which dumps the compiler's per-infer-pass AST — to catch daslang infer bugs
// (non-convergence / oscillation), not just flatten lowering bugs.
var g_log_infer = false

// Set once in main (int domain only). When true, generated units reference read-only module
// globals — UNIFORM leaves that exercise preshader extraction (the twin hoists globals-only
// subtrees to `_preshader_` lets). Read-only, so they never write shared state across the
// orig/flat differential calls. Bool domain skips them (int globals don't type in a bool expr).
var g_use_globals = false
let G_DEFS = "var G0 = 7\nvar G1 = -3\nvar G2 = 11\n\n"

def add_globals(var scope : array<Var>) {
    if (g_use_globals) {
        scope |> push(Var(name = "G0", mutable = false))
        scope |> push(Var(name = "G1", mutable = false))
        scope |> push(Var(name = "G2", mutable = false))
    }
}

// Tally of post-flatten fold residuals seen across all batches in --strict-fold mode.
// Non-fatal (see run_batch) — reported as a summary line at the end.
var g_residual_warnings = 0

// Per-process unique suffix for temp files (set once in main) so concurrent
// flatten-fuzz runs never clobber each other's generated source / read a stale one.
var g_tmp_suffix = ""

def tmp_path(kind : string) : string {
    return path_join(get_das_root(), "_flatten_fuzz_{kind}_{g_tmp_suffix}.das")
}

def build_file(units : array<string>) : string {
    return build_string() $(var w) {
        write(w, "options gen2\n")
        write(w, "options no_unused_function_arguments = false\n")
        if (g_log_infer) {
            write(w, "options log_infer_passes = true\n")
        }
        write(w, "require daslib/flatten\n")
        write(w, "require math\n\n")
        if (g_use_globals) {
            write(w, G_DEFS)
        }
        for (u in units) {
            write(w, u)
        }
    }
}

def write_source(path, src : string) : bool {
    var ok = false
    fopen(path, "wb") $(fw) {
        if (fw == null) {
            return
        }
        ok = true
        fw |> fwrite(src)
    }
    return ok
}

def gen_inputs(var g : Gen&; n : int) : array<int3> {
    var res <- [int3(0, 0, 0), int3(1, 1, 1), int3(-1, -1, -1), int3(2, -3, 5), int3(-7, 7, 0)]
    res |> reserve(length(res) + n)
    for (_i in range(n)) {
        res |> push(int3(rnd(g, 201) - 100, rnd(g, 201) - 100, rnd(g, 201) - 100))
    }
    return <- res
}

// All 2^3 bool assignments (0/1 per param). Bool mode tests the COMPLETE input space of each
// generated 3-bool-input function, so the differential is exhaustive, not sampled.
def enum_bool_inputs() : array<int3> {
    return <- [int3(0, 0, 0), int3(0, 0, 1), int3(0, 1, 0), int3(0, 1, 1),
        int3(1, 0, 0), int3(1, 0, 1), int3(1, 1, 0), int3(1, 1, 1)]
}

def try_compile(src : string; var issues : string&) : bool {
    let tmp = tmp_path("chk")
    if (!write_source(tmp, src)) {
        issues = "cannot write temp file {tmp}"
        return false
    }
    var compiled = false
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.ignore_shared_modules = true
            compile_file(tmp, access, unsafe(addr(mg)), cop) $(ok; _program; iss) {
                if (!ok) {
                    issues = string(iss)
                } else {
                    compiled = true
                }
            }
        }
    }
    remove(tmp)
    return compiled
}

def dump_reproducer(unitSrc : string; seedArg, idx : int; kind : string) {
    let fn = "flatten_fuzz_{kind}_seed{seedArg}_unit{idx}.das"
    if (write_source(fn, build_file([unitSrc]))) {
        to_log(LOG_INFO, "  dumped {kind}: {fn}\n")
    }
}

// On a batch compile failure, find and dump the offending unit(s).
def bisect_compile(units : array<string>; seedArg : int) {
    var found = false
    for (idx in range(length(units))) {
        var issues : string
        if (!try_compile(build_file([units[idx]]), issues)) {
            found = true
            to_log(LOG_ERROR, "COMPILE-FAIL unit {idx} (seed {seedArg}):\n{issues}\n")
            dump_reproducer(units[idx], seedArg, idx, "fail")
        }
    }
    if (!found) {
        to_log(LOG_ERROR, "batch failed to compile but no single unit reproduces it (cross-unit interaction?)\n")
    }
}

// Map a flatten twin name (`cand_N_flat`) back to its unit index, or -1 if it isn't one.
def twin_unit_index(name : string) : int {
    if (!(name |> starts_with("cand_")) || !(name |> ends_with("_flat"))) {
        return -1
    }
    return to_int(slice(name, length("cand_"), length(name) - length("_flat")))
}

// Returns (#value mismatches, compiled?). In strict mode, after the differential, every
// twin's final body is walked with `flatten_fold_residuals`. A survivor is reported as a
// WARNING (tallied in g_residual_warnings), not a failure: because the fuzzer emits plain
// `[flatten]`, whose fold fixpoint already converged, any residual in the FINAL compiled
// twin materialized AFTER the macro settled — the compiler's own later const-prop proved an
// operand constant (e.g. `x*k` with `k` proven 1 -> `x*1`), exposing an identity flatten can
// no longer reach. So it's a missed-fold *opportunity* surfaced for the const-fold-in-flatten
// work, not a flatten correctness bug (the differential still proves the value is right).
def run_batch(units : array<string>; inputs : array<int3>; seedArg : int; strict, keep : bool) : tuple<int; bool> {
    let nUnits = length(units)
    let src = build_file(units)
    let tmp = tmp_path("gen")
    if (!write_source(tmp, src)) {
        to_log(LOG_ERROR, "cannot write {tmp}\n")
        return 0 => false
    }
    var mismatches = 0
    var compiled = false
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.ignore_shared_modules = true
            compile_file(tmp, access, unsafe(addr(mg)), cop) $(ok; program; iss) {
                // log_infer_passes output is routed into `iss` (not stdout) by the
                // in-process compile, so surface it here when requested.
                if (g_log_infer && !empty(iss)) {
                    to_log(LOG_INFO, "{iss}\n")
                }
                if (!ok) {
                    return
                }
                compiled = true
                if (strict) {
                    program_for_each_module(program) $(mod) {
                        for_each_function(mod, "") $(func) {
                            if (func.body == null || func.flags.builtIn || !(func.name |> ends_with("_flat"))) {
                                return
                            }
                            var res <- flatten_fold_residuals(func)
                            res |> push_clone_from(flatten_opt_residuals(func))
                            if (!empty(res)) {
                                g_residual_warnings ++
                                to_log(LOG_WARNING, "RESIDUAL (post-flatten, non-fatal) {func.name}: {res}\n")
                                let idx = twin_unit_index("{func.name}")
                                if (idx >= 0 && idx < nUnits) {
                                    dump_reproducer(units[idx], seedArg, idx, "residual")
                                }
                            }
                        }
                    }
                }
                simulate(program) $(sok; var ctx; serrors) {
                    if (!sok) {
                        to_log(LOG_ERROR, "simulate failed:\n{serrors}\n")
                        return
                    }
                    for (idx in range(nUnits)) {
                        for (inp in inputs) {
                            var r_orig = 0
                            var r_flat = 0
                            unsafe {
                                invoke_in_context(*ctx, "drive_{idx}", inp.x, inp.y, inp.z, addr(r_orig), addr(r_flat))
                            }
                            if (r_orig != r_flat) {
                                mismatches++
                                to_log(LOG_ERROR, "MISMATCH unit {idx} a={inp.x} b={inp.y} c={inp.z}: orig={r_orig} flat={r_flat}\n")
                                dump_reproducer(units[idx], seedArg, idx, "fail")
                                break
                            }
                        }
                    }
                }
            }
        }
    }
    if (keep) {
        to_log(LOG_INFO, "kept generated source: {tmp}\n")
    } else {
        remove(tmp)
    }
    return mismatches => compiled
}

// One batch at a given seed: generate units, compile, run the differential (plus, in
// strict mode, the post-compile fold-residual walk). Returns true on a clean batch. A
// compile failure triggers bisect+dump.
def fuzz_one(seedArg, nUnits, nInputs : int; cfg : GenCfg; strict, keep : bool) : bool {
    var g = Gen(seed = random_seed(seedArg), cfg = cfg)
    let units <- [for (i in range(nUnits)); gen_unit(g, i)]
    var inputs : array<int3>
    if (cfg.boolDomain) {
        inputs <- enum_bool_inputs()
    } else {
        inputs <- gen_inputs(g, nInputs)
    }
    let r = run_batch(units, inputs, seedArg, strict, keep)
    if (!r._1) {
        to_log(LOG_ERROR, "seed {seedArg}: batch compile failed — isolating\n")
        bisect_compile(units, seedArg)
        return false
    }
    if (r._0 == 0) {
        to_log(LOG_INFO, "seed {seedArg}: PASS ({nUnits} units x {length(inputs)} inputs)\n")
        return true
    }
    to_log(LOG_ERROR, "seed {seedArg}: FAIL ({r._0} unit(s) diverged)\n")
    return false
}

[CommandLineArgs]
struct Config {
    @clarg_short = "s"
    @clarg_doc = "Base RNG seed (default 1)"
    seed : int

    @clarg_short = "u"
    @clarg_doc = "Units (function pairs) per batch (default 20)"
    units : int

    @clarg_short = "n"
    @clarg_doc = "Random inputs per unit (default 40)"
    inputs : int

    @clarg_short = "d"
    @clarg_doc = "Max nesting/expression depth (default 3)"
    depth : int

    @clarg_short = "b"
    @clarg_doc = "Batches to run, seeds seed..seed+b-1 (default 1)"
    batches : int

    @clarg_short = "f"
    @clarg_doc = "post-compile fold-residual oracle (flatten_fold_residuals) + const-density bias"
    strict_fold : bool

    @clarg_short = "k"
    @clarg_doc = "Keep generated temp source (don't delete)"
    keep : bool

    @clarg_short = "L"
    @clarg_doc = "Emit 'options log_infer_passes' in generated source (diagnose compiler infer)"
    log_infer : bool

    @clarg_short = "B"
    @clarg_name = "bool"
    @clarg_doc = "Pure-bool domain: bool params/locals/return, exhaustive 2^3 truth-table oracle (ignores --inputs)"
    bool_domain : bool

    @clarg_short = "?"
    @clarg_name = "show-help"
    @clarg_doc = "Show this help and exit"
    help : bool
}

[export]
def main : int {
    var ar <- parse_args(type<Config>, get_cli_arguments())
    if (ar |> is_err) {
        to_log(LOG_ERROR, "error: {ar |> unwrap_err}\n")
        print_help(get_command_info(type<Config>), "flatten-fuzz")
        return 2
    }
    let cfg_args <- ar |> move_unwrap
    if (cfg_args.help) {
        print_help(get_command_info(type<Config>), "flatten-fuzz")
        return 0
    }
    let baseSeed = cfg_args.seed != 0 ? cfg_args.seed : 1
    let nUnits = cfg_args.units > 0 ? cfg_args.units : 20
    let nInputs = cfg_args.inputs > 0 ? cfg_args.inputs : 40
    let depth = cfg_args.depth > 0 ? cfg_args.depth : 3
    let batches = cfg_args.batches > 0 ? cfg_args.batches : 1
    let strict = cfg_args.strict_fold
    g_log_infer = cfg_args.log_infer
    g_use_globals = !cfg_args.bool_domain
    g_tmp_suffix = "{ref_time_ticks()}"
    let cfg = GenCfg(maxDepth = depth, maxStmts = 4, literalPct = 40, constBias = strict ? 55 : 0, boolDomain = cfg_args.bool_domain)
    let domain = cfg_args.bool_domain ? "bool(exhaustive 2^3)" : "int(sampled ~{nInputs})"

    to_log(LOG_INFO, "flatten-fuzz: seeds {baseSeed}..{baseSeed + batches - 1} units={nUnits} domain={domain} depth={depth} strict_fold={strict}\n")
    var failed = 0
    for (b in range(batches)) {
        if (!fuzz_one(baseSeed + b, nUnits, nInputs, cfg, strict, cfg_args.keep)) {
            failed++
        }
    }
    if (g_residual_warnings > 0) {
        to_log(LOG_WARNING, "flatten-fuzz: {g_residual_warnings} post-flatten fold residual(s) — non-fatal (see 'residual' dumps)\n")
    }
    if (failed == 0) {
        to_log(LOG_INFO, "flatten-fuzz: ALL {batches} batch(es) PASS\n")
        return 0
    }
    to_log(LOG_ERROR, "flatten-fuzz: {failed}/{batches} batch(es) FAILED — see dumps\n")
    return 1
}
