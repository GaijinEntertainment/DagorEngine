options gen2
options indenting = 4
options strict_smart_pointers = true
options _comment_hygiene = true

module flatten_opt_straightline shared private

//! Straight-line cleanup of a lowered twin body (pre-infer): copy-prop, dead-store elimination, mask const-prop, SSA-rename.

require daslib/ast
require daslib/ast_boost
require daslib/ast_match
require daslib/templates_boost
require daslib/macro_boost
require daslib/match
require daslib/rtti
require daslib/flatten_opt_common


// ===== Dead-store elimination =====
// Straight-line body → liveness is one backward scan. Touches only `__flat_*` locals (never
// user locals/globals); drops a store whose target is dead afterward and whose RHS is pure.

class private FlatScan : AstVisitor {
    reads : table<uint64>
    risky : bool
    def override preVisitExprVar(expr : ExprVar?) : void {
        if (is_flat_temp(expr.name)) reads |> insert(hash(expr.name))
    }
    def override preVisitExprCall(expr : ExprCall?) : void {
        risky = true
    }
    def override preVisitExprAt(expr : ExprAt?) : void {
        risky = true
    }
    def override preVisitExprOp2(expr : ExprOp2?) : void {
        if (expr.op == "/" || expr.op == "%") {
            risky = true
        }
    }
}

// Scan a subtree for the `__flat_*` names it reads and whether it carries a
// fault-capable / side-effecting node (call, index, division).
def private scan_flat(var e : ExpressionPtr; var reads : table<uint64>; var risky : bool&) {
    if (e == null) return
    var v = new FlatScan()
    make_visitor(*v) $(adapter) {
        visit(e, adapter)
    }
    for (nm in keys(v.reads)) {
        reads |> insert(nm)
    }
    if (v.risky) {
        risky = true
    }
    unsafe {
        delete v
    }
}

// ===== Boolean const-fold of live masks =====
// With no early return `__flat_live` stays constant `true`; copy_prop inlines the single-def
// mask and fold_stmt collapses the selects it exposes (`true ? a:b → a`), unwinding the scaffold.

def private const_bool(e : ExpressionPtr; var val : bool&) : bool {
    if (e == null) return false
    if (e is ExprConstBool) {
        val = (e as ExprConstBool).value
        return true
    }
    return false
}

def private fold_const(e : ExpressionPtr; constmap : table<uint64; bool>) : ExpressionPtr {
    if (e == null) return null
    if (e is ExprVar) {
        let h = hash((e as ExprVar).name)
        if (key_exists(constmap, h)) return new ExprConstBool(at = e.at, value = constmap?[h] ?? false)
        return clone_expression(e)
    }
    if (e is ExprOp1) {
        let o = e as ExprOp1
        var sub = fold_const(o.subexpr, constmap)
        var bv = false
        if (o.op == "!" && const_bool(sub, bv)) {
            return new ExprConstBool(at = e.at, value = !bv)
        }
        return new ExprOp1(at = e.at, atEnclosure = o.atEnclosure, genFlags = o.genFlags,
            op := o.op, subexpr = sub)
    }
    if (e is ExprOp2) {
        let o = e as ExprOp2
        var l = fold_const(o.left, constmap)
        var r = fold_const(o.right, constmap)
        var lb = false
        var rb = false
        let lc = const_bool(l, lb)
        let rc = const_bool(r, rb)
        if (o.op == "&&") {
            if (lc) {
                return r if (lb)
                return new ExprConstBool(at = e.at, value = false)
            }
            if (rc) {
                return l if (rb)
                return new ExprConstBool(at = e.at, value = false)
            }
        } elif (o.op == "||") {
            if (lc) {
                return new ExprConstBool(at = e.at, value = true) if (lb)
                return r
            }
            if (rc) {
                return new ExprConstBool(at = e.at, value = true) if (rb)
                return l
            }
        }
        return new ExprOp2(at = e.at, atEnclosure = o.atEnclosure, genFlags = o.genFlags,
            op := o.op, left = l, right = r)
    }
    if (e is ExprOp3) {
        let o = e as ExprOp3
        var c = fold_const(o.subexpr, constmap)
        var cb = false
        if (const_bool(c, cb)) {
            return cb ? fold_const(o.left, constmap) : fold_const(o.right, constmap)
        }
        return new ExprOp3(at = e.at, atEnclosure = o.atEnclosure, genFlags = o.genFlags,
            op := o.op, subexpr = c,
            left = fold_const(o.left, constmap),
            right = fold_const(o.right, constmap))
    }
    if (e is ExprCall) {
        let o = e as ExprCall
        var n = new ExprCall(at = e.at, atEnclosure = o.atEnclosure, genFlags = o.genFlags, name := o.name)
        for (ai in range(length(o.arguments))) {
            emplace_new(n.arguments) <| fold_const(o.arguments[ai], constmap)
        }
        return n
    }
    return clone_expression(e)
}

// Apply fold_const at the STATEMENT level: fold the sub-expression of an assignment /
// decl-init / return in place (fold_const itself only folds expression trees, not the
// ExprCopy/ExprLet/ExprReturn wrappers). Used by copy_prop after it inlines a const mask.
def private fold_stmt(var s : ExpressionPtr; constmap : table<uint64; bool>) : ExpressionPtr {
    if (s is ExprCopy) {
        var cp = s as ExprCopy
        cp.right = fold_const(cp.right, constmap)
        return s
    } elif (s is ExprLet) {
        var lc = s as ExprLet
        for (v in lc.variables) {
            if (v.init != null) {
                v.init = fold_const(v.init, constmap)
            }
        }
        return s
    } elif (s is ExprReturn) {
        var r = s as ExprReturn
        if (r.subexpr != null) {
            r.subexpr = fold_const(r.subexpr, constmap)
        }
        return s
    }
    return fold_const(s, constmap)
}

class private FlatScanAll : AstVisitor {
    reads : table<uint64>
    risky : bool
    def override preVisitExprVar(expr : ExprVar?) : void {
        reads |> insert(hash(expr.name))
    }
    def override preVisitExprCall(expr : ExprCall?) : void {
        risky = true
    }
    def override preVisitExprAt(expr : ExprAt?) : void {
        risky = true
    }
    def override preVisitExprOp2(expr : ExprOp2?) : void {
        if (expr.op == "/" || expr.op == "%") {
            risky = true
        }
    }
}

// Collect every variable name read by `e` (not just flatten temps) and whether it
// carries a fault-capable / re-evaluation-sensitive node.
def private scan_src(var e : ExpressionPtr; var reads : table<uint64>; var risky : bool&) {
    if (e == null) return
    var v = new FlatScanAll()
    make_visitor(*v) $(adapter) {
        visit(e, adapter)
    }
    for (nm in keys(v.reads)) {
        reads |> insert(nm)
    }
    if (v.risky) {
        risky = true
    }
    unsafe {
        delete v
    }
}

def public copy_prop_pass(var out : array<ExpressionPtr>) {
    // Empty map: fold_const with no var bindings still simplifies LITERAL const-bools
    // (`true && c → c`, `true ? a : b → a`) freshly inlined below. This is how copy-prop
    // subsumes the old const_prop_pass: inline a proven-const mask, then collapse its uses.
    var emptyConstMap : table<uint64; bool>
    var changed = true
    while (changed) {
        changed = false
        let n = length(out)
        // Precompute once per pass: each store target's indices + a name -> (ref statements, count)
        // index. A source reading V is unsafe only if V is reassigned BETWEEN this temp's def and a
        // use (a reassignment fully before the def leaves the captured value intact).
        var storeIdx : table<uint64; array<int>>
        for (j in range(n)) {
            if (out[j] is ExprCopy && (out[j] as ExprCopy).left is ExprVar) {
                storeIdx[hash(((out[j] as ExprCopy).left as ExprVar).name)] |> push(j)
            }
        }
        var ub = new FlatUsesBuilder()
        make_visitor(*ub) $(adapter) {
            for (j in range(n)) {
                ub.j = j
                clear(ub.seen)
                visit(out[j], adapter)
            }
        }
        var srcReads : table<uint64>
        var useJs : array<int>   // hoisted: cleared per candidate, freed once per pass
        for (k in range(n)) {
            if (!(out[k] is ExprLet)) continue
            var lc = out[k] as ExprLet
            if (length(lc.variables) != 1 || !is_flat_temp(lc.variables[0].name)) continue
            let hk = hash(lc.variables[0].name)
            let nStores = key_exists(storeIdx, hk) ? length(storeIdx[hk]) : 0
            // Single definition: decl-init + no stores, OR no-init decl + one store. Use the ORIGINAL
            // source (no clone) — apply_template clones per use; pre-cloning would orphan a copy on
            // every non-folding candidate (re-examined each while-restart).
            var src : ExpressionPtr = null
            var si = -1
            if (lc.variables[0].init != null && nStores == 0) {
                src = lc.variables[0].init                    // decl-init is the single definition
            } elif (lc.variables[0].init == null && nStores == 1) {
                si = storeIdx[hk][0]
                var cp = out[si] as ExprCopy
                src = cp.right                                // no-init decl + single store
            } else {
                continue                                      // multi-def (e.g. early-return mask) — leave it
            }
            let defIdx = si >= 0 ? si : k
            clear(srcReads)
            var srcRisky = false
            scan_src(src, srcReads, srcRisky)
            if (key_exists(srcReads, hk)) continue            // self-referential source — never fold
            // Uses (excluding def k and store si) straight from the precomputed index.
            // nameCount includes the store's lvalue occurrence (1), which is not a use.
            let refCount = (key_exists(ub.nameCount, hk) ? ub.nameCount[hk] : 0) - (si >= 0 ? 1 : 0)
            var firstRef = n
            var maxUseIdx = -1
            clear(useJs)
            if (key_exists(ub.nameUses, hk)) {
                for (j in ub.nameUses[hk]) {
                    if (j == k || j == si) continue
                    useJs |> push(j)
                    if (j < firstRef) {
                        firstRef = j
                    }
                    if (j > maxUseIdx) {
                        maxUseIdx = j
                    }
                }
            }
            if ((refCount > 0 && firstRef < defIdx) || (srcRisky && refCount > 1)) continue
            // Position-aware guard: block only if a source var is reassigned strictly
            // between the def and the last use.
            var blocked = false
            for (rn in keys(srcReads)) {
                if (!key_exists(storeIdx, rn)) continue
                for (r in storeIdx[rn]) {
                    if (r > defIdx && r <= maxUseIdx) {
                        blocked = true
                        break
                    }
                }
                if (blocked) break
            }
            if (blocked) continue
            // Inline the source at every use, then drop the def (+ store).
            let nm = "{lc.variables[0].name}"
            var rules : Template
            rules |> replaceVariable(nm, src)   // store src directly; apply_template clones per use
            let constSrc = src is ExprConstBool
            for (j in useJs) {
                out[j] = apply_template(rules, out[j].at, out[j], false)   // only statements that reference nm
                if (constSrc) {
                    out[j] = fold_stmt(out[j], emptyConstMap)   // collapse the now-literal mask: true && c → c
                }
            }
            unsafe { delete rules }
            out <- [for (j in range(n)); out[j]; where j != k && j != si]
            changed = true
            break
        }
        delete srcReads
        delete storeIdx
        delete useJs
        unsafe {
            delete ub
        }
    }
    delete emptyConstMap
}

def public dse_pass(var out : array<ExpressionPtr>) {
    let n = length(out)
    // Names that are ever an assignment target: their decl must stay even if the
    // init value is dead (only fully-unreferenced decls are removable).
    var storeTargets : table<uint64>
    for (s in out) {
        if (s is ExprCopy && (s as ExprCopy).left is ExprVar) {
            let lv = (s as ExprCopy).left as ExprVar
            if (is_flat_temp(lv.name)) storeTargets |> insert(hash(lv.name))
        }
    }
    var live : table<uint64>
    var dead : table<int>
    var reads : table<uint64>   // hoisted: one scratch table, cleared per statement (was n allocations)
    var i = n - 1
    while (i >= 0) {
        var stmt = out[i]
        var handled = false
        clear(reads)
        if (stmt is ExprCopy) {
            var cp = stmt as ExprCopy
            if (cp.left is ExprVar && is_flat_temp((cp.left as ExprVar).name)) {
                handled = true
                let lh = hash((cp.left as ExprVar).name)
                var risky = false
                scan_flat(cp.right, reads, risky)
                if (!key_exists(live, lh) && !risky) {
                    dead |> insert(i)   // target not live afterward, RHS pure → dead
                } else {
                    live |> erase(lh)
                    for (nm in keys(reads)) {
                        live |> insert(nm)
                    }
                }
            }
        }
        if (!handled && stmt is ExprLet) {
            var lc = stmt as ExprLet
            if (length(lc.variables) == 1 && is_flat_temp(lc.variables[0].name)) {
                handled = true
                let dh = hash(lc.variables[0].name)
                var risky = false
                scan_flat(lc.variables[0].init, reads, risky)
                if (!key_exists(live, dh) && !key_exists(storeTargets, dh) && !risky) {
                    dead |> insert(i)   // declared temp fully unreferenced, init pure → dead
                } else {
                    live |> erase(dh)
                    for (nm in keys(reads)) {
                        live |> insert(nm)
                    }
                }
            }
        }
        if (!handled) {
            // Decl / return / global write / bare expr — over-approximate: every
            // flat var it mentions becomes live (never wrongly removes a store).
            var risky = false
            scan_flat(stmt, reads, risky)
            for (nm in keys(reads)) {
                live |> insert(nm)
            }
        }
        i --
    }
    delete storeTargets
    delete live
    delete reads
    if (empty(dead)) {
        delete dead
        return
    }
    out <- [for (j in range(n)); out[j]; where !key_exists(dead, j)]
    delete dead
}

// ===== Flow-sensitive const-bool propagation of loop/iter masks =====
// Break/continue masks are multi-def, so single-def copy_prop skips them; a forward scan instead
// carries each mask's known const value to its uses, collapsing narrows and leaving spent masks dead.

// True if `e` reads any flat-mask currently in `known` (fills `reads`, cleared first). Cheap
// O(size) scan gating the cloning fold_const so mask-free statements are skipped.
def private reads_known(var e : ExpressionPtr; known : table<uint64; bool>; var reads : table<uint64>) : bool {
    clear(reads)
    var risky = false
    scan_flat(e, reads, risky)
    for (h in keys(reads)) {
        if (key_exists(known, h)) return true
    }
    return false
}

def public mask_const_prop_pass(var out : array<ExpressionPtr>) {
    var known : table<uint64; bool>   // flat-mask name-hash -> currently-known const bool value
    var reads : table<uint64>         // scratch, cleared per statement
    let n = length(out)
    for (i in range(n)) {
        var s = out[i]
        // (1) flat-mask decl `var M = <init>`: fold init under known masks, then (re)seed M.
        if (s is ExprLet && length((s as ExprLet).variables) == 1 && is_flat_temp((s as ExprLet).variables[0].name)) {
            var lc = s as ExprLet
            let h = hash(lc.variables[0].name)
            if (lc.variables[0].init != null) {
                if (!empty(known) && reads_known(lc.variables[0].init, known, reads)) {
                    lc.variables[0].init = fold_const(lc.variables[0].init, known)
                }
                var bv = false
                if (const_bool(lc.variables[0].init, bv)) {
                    known[h] = bv
                } else {
                    known |> erase(h)
                }
            } else {
                known |> erase(h)
            }
            continue
        }
        // (2) flat-mask store `M = <rhs>`: fold rhs (the `M = M && X` collapse), then update M.
        if (s is ExprCopy && (s as ExprCopy).left is ExprVar && is_flat_temp(((s as ExprCopy).left as ExprVar).name)) {
            var cp = s as ExprCopy
            let h = hash((cp.left as ExprVar).name)
            if (!empty(known) && reads_known(cp.right, known, reads)) {
                cp.right = fold_const(cp.right, known)
            }
            var bv = false
            if (const_bool(cp.right, bv)) {
                known[h] = bv
            } else {
                known |> erase(h)
            }
            continue
        }
        // (3) any other statement: fold its mask reads (select predicates) under known masks.
        if (!empty(known) && reads_known(s, known, reads)) {
            out[i] = fold_stmt(s, known)
        }
    }
    delete known
    delete reads
}

// ===== SSA-rename of reassigned user locals =====
// Straight-line body → a reassigned local is a linear def-chain (no phi); renaming each write to a
// fresh single-def `__ssa_` version lets post-infer const-prop fold the chain (declared-with-init only).

def private rename_reads(var e : ExpressionPtr; current : table<string; string>) : ExpressionPtr {
    if (e == null) return null
    var tmpl : Template
    var any = false
    for (orig in keys(current)) {
        let vn = current?[orig] ?? ""
        tmpl |> replaceVariable(orig, qmacro($i(vn)))
        any = true
    }
    if (!any) return e
    var res = apply_template(tmpl, e.at, e, false)
    unsafe { delete tmpl }
    return res
}

def public ssa_rename_pass(var out : array<ExpressionPtr>) {
    // (1) user locals declared (single-var) with an initializer
    var declaredInit : table<uint64>
    for (s in out) {
        if (s is ExprLet && length((s as ExprLet).variables) == 1) {
            let v = (s as ExprLet).variables[0]
            // User locals, plus inline-renamed `__flat_loc_` accumulators (NOT the masks).
            if ((!is_flat_temp(v.name) || is_flat_inlined_local(v.name)) && v.init != null) declaredInit |> insert(hash(v.name))
        }
    }
    if (empty(declaredInit)) {
        delete declaredInit
        return
    }
    // (2) of those, the ones reassigned via a plain `var = ...` (multi-def)
    var ssaNames : table<uint64>
    for (s in out) {
        if (s is ExprCopy && (s as ExprCopy).left is ExprVar) {
            let h = hash(((s as ExprCopy).left as ExprVar).name)
            if (key_exists(declaredInit, h)) ssaNames |> insert(h)
        }
    }
    delete declaredInit
    if (empty(ssaNames)) {
        delete ssaNames
        return
    }
    // (3) forward rename: each read maps to the current version; each write mints a new
    // version (after resolving the RHS reads, so a self-ref reads the prior version).
    var current : table<string; string>
    var counter = 0
    var i = 0
    while (i < length(out)) {
        var s = out[i]
        if (s is ExprLet && length((s as ExprLet).variables) == 1 &&
                key_exists(ssaNames, hash((s as ExprLet).variables[0].name))) {
            let orig = "{(s as ExprLet).variables[0].name}"
            let newInit = rename_reads((s as ExprLet).variables[0].init, current)
            counter ++
            let vn = "__ssa_{orig}_{counter}"
            out[i] = qmacro_expr(${ var $i(vn) = $e(newInit); })
            current[orig] = vn
        } elif (s is ExprCopy && (s as ExprCopy).left is ExprVar &&
                key_exists(ssaNames, hash(((s as ExprCopy).left as ExprVar).name))) {
            let orig = "{((s as ExprCopy).left as ExprVar).name}"
            let newRhs = rename_reads((s as ExprCopy).right, current)
            counter ++
            let vn = "__ssa_{orig}_{counter}"
            out[i] = qmacro_expr(${ var $i(vn) = $e(newRhs); })
            current[orig] = vn
        } else {
            out[i] = rename_reads(s, current)
        }
        i ++
    }
    delete ssaNames
    delete current
}
