options gen2
options indenting = 4
options strict_smart_pointers = true
options _comment_hygiene = true

module flatten_opt_preshade shared private

//! Preshader extraction + CSE + const-minus regroup + pure-alias elimination, and the optimize-pass completeness oracle.

require daslib/ast
require daslib/ast_boost
require daslib/ast_match
require daslib/templates_boost
require daslib/macro_boost
require daslib/match
require daslib/rtti
require strings
require daslib/flatten_opt_common
require daslib/flatten_opt_fuse


def private psh_reserved(name : das_string) : bool {
    return name |> starts_with("__flat_") || name |> starts_with("__ssa_") || name |> starts_with("_preshader_")
}

def private name_counter_suffix(name : das_string; prefix : string) : int {
    if (!(name |> starts_with(prefix))) return -1
    let s = slice("{name}", length(prefix))
    if (empty(s)) return -1
    let v = to_int(s)
    return "{v}" == s ? v : -1
}

def private psh_has_compute(e : ExpressionPtr) : bool {
    if (e == null) return false
    if (e is ExprRef2Value) return psh_has_compute((e as ExprRef2Value).subexpr)
    if (e is ExprOp1 || e is ExprOp2 || e is ExprOp3 || e is ExprCall) return true
    if (e is ExprField) return psh_has_compute((e as ExprField).value)
    if (e is ExprSwizzle) return psh_has_compute((e as ExprSwizzle).value)
    if (e is ExprCast) return psh_has_compute((e as ExprCast).subexpr)
    if (e is ExprPtr2Ref) return psh_has_compute((e as ExprPtr2Ref).subexpr)
    if (e is ExprAt) {
        let o = e as ExprAt
        return psh_has_compute(o.subexpr) || psh_has_compute(o.index)
    }
    return false
}

struct private PEX {
    paramNames : table<string>
    localColor : table<string; bool>
    mutableVars : table<string>
    top : array<ExpressionPtr>
    dedup : table<string; string>
    rename : table<string; ExpressionPtr>
    counter : int
    changed : bool
    noFastMath : bool
}

def private hoisted_ref(refName : string; src : ExpressionPtr) : ExpressionPtr {
    var v = new ExprVar(at = src.at, name := refName)
    if (src._type != null) {
        v._type = clone_type(src._type)
    }
    return v
}

def private is_rcp_candidate(pe : PEX; barriers : table<string>; o : ExprOp2?) : bool {
    return (o.op == "/" && expr_bt(o.right) == Type.tFloat && is_float_valued(o) &&
        !(o.right |> is_expr_const()) && !pex_eligible(pe, barriers, o) &&
        pex_eligible(pe, barriers, o.right))
}

def private pex_eligible(pe : PEX; barriers : table<string>; e : ExpressionPtr) : bool {   // nolint:STYLE037 - exact-type `is` ladder, one arm per expression kind
    if (e == null) return false
    if (e |> is_expr_const()) return true
    if (e is ExprVar) {
        let nm = "{(e as ExprVar).name}"
        if (key_exists(pe.mutableVars, nm)) return false
        if (key_exists(pe.localColor, nm)) return pe.localColor?[nm] ?? false
        return !key_exists(pe.paramNames, nm)
    }
    if (e is ExprRef2Value) return pex_eligible(pe, barriers, (e as ExprRef2Value).subexpr)
    if (e is ExprField) return pex_eligible(pe, barriers, (e as ExprField).value)
    if (e is ExprSwizzle) return pex_eligible(pe, barriers, (e as ExprSwizzle).value)
    if (e is ExprCast) return pex_eligible(pe, barriers, (e as ExprCast).subexpr)
    if (e is ExprPtr2Ref) return pex_eligible(pe, barriers, (e as ExprPtr2Ref).subexpr)
    if (e is ExprAt) {
        let o = e as ExprAt
        return pex_eligible(pe, barriers, o.subexpr) && pex_eligible(pe, barriers, o.index)
    }
    if (e is ExprMakeTuple) {
        for (v in (e as ExprMakeTuple).values) {
            if (!pex_eligible(pe, barriers, v)) return false
        }
        return true
    }
    if (e is ExprOp1) return pex_eligible(pe, barriers, (e as ExprOp1).subexpr)
    if (e is ExprOp2) {
        let o = e as ExprOp2
        return pex_eligible(pe, barriers, o.left) && pex_eligible(pe, barriers, o.right)
    }
    if (e is ExprOp3) {
        let o = e as ExprOp3
        return pex_eligible(pe, barriers, o.subexpr) && pex_eligible(pe, barriers, o.left) && pex_eligible(pe, barriers, o.right)
    }
    if (e is ExprCall) {
        let c = e as ExprCall
        if (key_exists(barriers, simple_name("{c.name}"))) return false
        for (a in c.arguments) {
            if (!pex_eligible(pe, barriers, a)) return false
        }
        return true
    }
    return false
}

def private pex_hoist_name(var pe : PEX; var e : ExpressionPtr) : string {
    let key = describe(e)
    if (key_exists(pe.dedup, key)) return pe.dedup[key]
    let nm = "_preshader_{pe.counter}"
    pe.counter ++
    pe.dedup[key] = nm
    pe.localColor[nm] = true
    pe.top |> push <| qmacro_expr(${ let $i(nm) = $e(e); })
    return nm
}

def private pex_extract(var pe : PEX; barriers : table<string>; var e : ExpressionPtr) : ExpressionPtr {
    if (e == null) return null
    if (psh_has_compute(e) && pex_eligible(pe, barriers, e)) {
        let nm = pex_hoist_name(pe, e)
        pe.changed = true
        return hoisted_ref(nm, e)
    }
    if (e is ExprOp2 && !pe.noFastMath && is_rcp_candidate(pe, barriers, e as ExprOp2)) {
        var o = e as ExprOp2
        var one = new ExprConstFloat(at = o.at, value = 1.0)
        var recip = new ExprOp2(at = o.at, op := "/", left = one, right = o.right)
        let nm = pex_hoist_name(pe, recip)
        pe.changed = true
        return new ExprOp2(at = o.at, op := "*",
            left = pex_extract(pe, barriers, o.left),
            right = hoisted_ref(nm, o.right))
    }
    rewrite_children(e) $(var c : ExpressionPtr) { return pex_extract(pe, barriers, c) }
    return e
}

def private extract_preshader(var func : FunctionPtr; var blk : ExprBlock?; barriers : table<string>; var counter : int&; no_fast_math : bool) : bool {   // nolint:STYLE037,STYLE038 - one hoist pass; seeding, colouring and splice share the dedup state
    var pe = PEX(counter = counter, noFastMath = no_fast_math)
    for (a in func.arguments) {
        pe.paramNames |> insert("{a.name}")
    }
    pe.mutableVars <- collect_mutable(blk)
    var oldPrefix = 0
    for (s in blk.list) {
        if (s is ExprLet && length((s as ExprLet).variables) == 1 &&
                (s as ExprLet).variables[0].name |> starts_with("_preshader_")) {
            let lv = (s as ExprLet).variables[0]
            if (lv.init != null) {
                pe.dedup["{describe(lv.init)}"] = "{lv.name}"
            }
            pe.localColor["{lv.name}"] = true
            let cnum = name_counter_suffix(lv.name, "_preshader_")
            if (cnum >= pe.counter) {
                pe.counter = cnum + 1
            }
            oldPrefix ++
        } else {
            break
        }
    }
    var body : array<ExpressionPtr>
    for (s0 in blk.list) {
        var s = s0
        if (!empty(pe.rename) && reads_any(s, pe.rename)) {
            s = subst_consts(s, pe.rename)
        }
        if (s is ExprLet && length((s as ExprLet).variables) == 1 &&
                (s as ExprLet).variables[0].name |> starts_with("_preshader_")) {
            body |> push(s)
            continue
        }
        if (s is ExprLet && length((s as ExprLet).variables) == 1 && !psh_reserved((s as ExprLet).variables[0].name)) {
            var lc = s as ExprLet
            let vname = "{lc.variables[0].name}"
            var init = lc.variables[0].init
            if (init != null && pex_eligible(pe, barriers, init)) {
                if (psh_has_compute(init)) {
                    let nm = pex_hoist_name(pe, init)
                    pe.rename[vname] = hoisted_ref(nm, init)
                    pe.changed = true
                } else {
                    pe.rename[vname] = clone_expression(init)
                    pe.changed = true
                }
                continue
            }
        }
        if (s is ExprLet) {
            var lc = s as ExprLet
            for (lv in lc.variables) {
                if (lv.init != null) {
                    lv.init = pex_extract(pe, barriers, lv.init)
                }
                pe.localColor["{lv.name}"] = false
            }
        } elif (s is ExprCopy) {
            var cp = s as ExprCopy
            cp.right = pex_extract(pe, barriers, cp.right)
        } elif (s is ExprReturn) {
            var rt = s as ExprReturn
            rt.subexpr = pex_extract(pe, barriers, rt.subexpr)
        }
        body |> push(s)
    }
    if (pe.changed) {
        while (!empty(blk.list)) {
            blk.list |> erase(length(blk.list) - 1)
        }
        for (i in range(oldPrefix)) {
            blk.list |> emplace(body[i])
        }
        for (t in pe.top) {
            blk.list |> emplace(t)
        }
        for (i in range(oldPrefix, length(body))) {
            blk.list |> emplace(body[i])
        }
    }
    let result = pe.changed
    counter = pe.counter
    delete pe.paramNames
    delete pe.localColor
    delete pe.mutableVars
    delete pe.dedup
    unsafe {
        delete pe.rename
        delete pe.top
        delete body
    }
    return result
}

class private CseTally : AstVisitor {
    collectSample : bool
    counts : table<string; int>
    sample : table<string; ExpressionPtr>
    mutableVars : table<string>
    def override preVisitExpression(expr : ExpressionPtr) : void {
        if (expr is ExprRef2Value) {
            return
        }
        if (psh_has_compute(expr) && !reads_mutable(expr, mutableVars)) {
            let k = "{describe(expr)}"
            counts[k] = (counts?[k] ?? 0) + 1
            if (collectSample && !key_exists(sample, k)) {
                sample[k] = clone_expression(expr)
            }
        }
    }
}

def private cse_replace_expr(var e : ExpressionPtr; key, refName : string) : ExpressionPtr {
    if (e == null) return null
    if (psh_has_compute(e) && "{describe(e)}" == key) {
        return hoisted_ref(refName, e)
    }
    rewrite_children(e) $(var c : ExpressionPtr) { return cse_replace_expr(c, key, refName) }
    return e
}

def private cse_replace_stmt(var s : ExpressionPtr; key, refName : string) {
    if (s is ExprLet) {
        var lc = s as ExprLet
        for (lv in lc.variables) {
            if (lv.init != null) {
                lv.init = cse_replace_expr(lv.init, key, refName)
            }
        }
    } elif (s is ExprCopy) {
        var cp = s as ExprCopy
        cp.right = cse_replace_expr(cp.right, key, refName)
    } elif (s is ExprReturn) {
        var rt = s as ExprReturn
        rt.subexpr = cse_replace_expr(rt.subexpr, key, refName)
    }
}

def private cse_tally_stmt(s : ExpressionPtr; a) {
    if (s is ExprLet) {
        for (lv in (s as ExprLet).variables) {
            if (lv.init != null) {
                visit(lv.init, a)
            }
        }
    } elif (s is ExprCopy) {
        visit((s as ExprCopy).right, a)
    } elif (s is ExprReturn) {
        let rt = s as ExprReturn
        if (rt.subexpr != null) {
            visit(rt.subexpr, a)
        }
    }
}

def private preshader_prefix_len(blk : ExprBlock?) : int {
    var k = 0
    for (s in blk.list) {
        if (s is ExprLet && length((s as ExprLet).variables) == 1 &&
                (s as ExprLet).variables[0].name |> starts_with("_preshader_")) {
            k ++
        } else {
            break
        }
    }
    return k
}

def private cse_body(var blk : ExprBlock?; var counter : int&) : bool {   // nolint:STYLE037,STYLE038 - one CSE pass; scan, mint and rewrite share the name counter
    var anyChange = false
    var guard = 0
    for (s in blk.list) {
        if (s is ExprLet && length((s as ExprLet).variables) == 1) {
            var cnum = name_counter_suffix((s as ExprLet).variables[0].name, "_preshader_cse_")
            if (cnum < 0) {
                cnum = name_counter_suffix((s as ExprLet).variables[0].name, "_cse_")
            }
            if (cnum >= counter) {
                counter = cnum + 1
            }
        }
    }
    var mutableVars <- collect_mutable(blk)
    while (guard < 4096) {
        guard ++
        var counts : table<string; int>
        var sample : table<string; ExpressionPtr>
        var firstStmt : table<string; int>
        for (i in range(length(blk.list))) {
            var ct = new CseTally()
            ct.collectSample = true
            ct.mutableVars := mutableVars
            make_visitor(*ct) $(a) {
                cse_tally_stmt(blk.list[i], a)
            }
            for (k in keys(ct.counts)) {
                counts[k] = (counts?[k] ?? 0) + (ct.counts?[k] ?? 0)
                if (!key_exists(firstStmt, k)) {
                    firstStmt[k] = i
                    sample[k] = ct.sample?[k] ?? null
                }
            }
            unsafe {
                delete ct
            }
        }
        var bestKey = ""
        var bestLen = 0
        for (k in keys(counts)) {
            if ((counts?[k] ?? 0) >= 2 && length(k) > bestLen) {
                bestKey = k
                bestLen = length(k)
            }
        }
        if (bestKey == "") {
            delete counts
            delete firstStmt
            unsafe {
                delete sample
            }
            break
        }
        let fi = firstStmt?[bestKey] ?? 0
        let uniform = fi < preshader_prefix_len(blk)
        let nm = uniform ? "_preshader_cse_{counter}" : "_cse_{counter}"
        counter ++
        var src = sample?[bestKey] ?? null
        for (s in blk.list) {
            cse_replace_stmt(s, bestKey, nm)
        }
        var theLet = qmacro_expr(${ let $i(nm) = $e(src); })
        var newlist : array<ExpressionPtr>
        newlist |> reserve(length(blk.list) + 1)
        for (i in range(length(blk.list))) {
            if (i == fi) {
                newlist |> push(theLet)
            }
            newlist |> push(blk.list[i])
        }
        while (!empty(blk.list)) {
            blk.list |> erase(length(blk.list) - 1)
        }
        for (m in newlist) {
            blk.list |> emplace(m)
        }
        delete counts
        delete firstStmt
        unsafe {
            delete sample
            delete newlist
        }
        anyChange = true
    }
    delete mutableVars
    return anyChange
}

def private cm_key(l, r : ExpressionPtr) : string {
    return "{describe(l)}\t-\t{describe(r)}"
}

class private CmTally : AstVisitor {
    counts : table<string; int>
    mutableVars : table<string>
    def override preVisitExprOp2(var expr : ExprOp2?) : void {
        if (expr.op == "-" && is_all_const(expr.left) && !is_all_const(expr.right) &&
                !reads_mutable(expr.right, mutableVars)) {
            let k = cm_key(expr.left, expr.right)
            counts[k] = (counts?[k] ?? 0) + 1
        }
    }
}

def private chain_has_grouped(e : ExpressionPtr; key : string) : bool {
    if (e == null) return false
    if (e is ExprOp2) {
        let o = e as ExprOp2
        if (o.op == "+" || o.op == "-") {
            if (o.op == "-" && is_all_const(o.left) && !is_all_const(o.right) && cm_key(o.left, o.right) == key) {
                return true
            }
            return chain_has_grouped(o.left, key) || chain_has_grouped(o.right, key)
        }
    } elif (e is ExprOp1) {
        let o = e as ExprOp1
        if (o.op == "-") return chain_has_grouped(o.subexpr, key)
    }
    return false
}

def private regroup_candidate(var that : ExprOp2?; cm : table<string; int>; mutableVars : table<string>;
                              no_fast_math : bool; var terms : array<ReTerm>; var ci : int&; var xi : int&) : bool {
    if (that == null || !(that.op == "+" || that.op == "-") || (no_fast_math && is_float_valued(that))) {
        return false
    }
    collect_add(that, false, terms)
    ci = -1
    for (i in range(length(terms))) {
        if (is_all_const(terms[i].e)) {
            if (ci >= 0) return false
            ci = i
        }
    }
    if (ci < 0 || terms[ci].neg) return false
    for (i in range(length(terms))) {
        if (i == ci || !terms[i].neg || reads_mutable(terms[i].e, mutableVars)) {
            continue
        }
        let k = cm_key(terms[ci].e, terms[i].e)
        if ((cm?[k] ?? 0) >= 1 && !chain_has_grouped(that, k)) {
            xi = i
            return true
        }
    }
    return false
}

class private RegroupShareVisitor : AstVisitor {
    changed : bool
    noFastMath : bool
    cm : table<string; int>
    mutableVars : table<string>
    def override visitExprOp2(var that : ExprOp2?) : ExpressionPtr {
        var terms : array<ReTerm>
        var ci = -1
        var xi = -1
        if (!regroup_candidate(that, cm, mutableVars, noFastMath, terms, ci, xi)) {
            unsafe {
                delete terms
            }
            return that
        }
        var newTerms : array<ReTerm>
        newTerms |> push(ReTerm(neg = false,
            e = new ExprOp2(at = that.at, op := "-", left = terms[ci].e, right = terms[xi].e)))
        for (i in range(length(terms))) {
            if (i != ci && i != xi) {
                newTerms |> push(terms[i])
            }
        }
        changed = true
        var r = build_chain(newTerms, that.at, "+", "-")
        unsafe {
            delete terms
            delete newTerms
        }
        return r
    }
}

class private RegroupResidualVisitor : AstVisitor {
    found : bool
    noFastMath : bool
    cm : table<string; int>
    mutableVars : table<string>
    def override preVisitExprOp2(var expr : ExprOp2?) : void {
        var terms : array<ReTerm>
        var ci = -1
        var xi = -1
        if (regroup_candidate(expr, cm, mutableVars, noFastMath, terms, ci, xi)) {
            found = true
        }
        unsafe {
            delete terms
        }
    }
}

def private regroup_to_share(var blk : ExprBlock?; no_fast_math : bool) : bool {
    var mutableVars <- collect_mutable(blk)
    var ct = new CmTally()
    ct.mutableVars := mutableVars
    make_visitor(*ct) $(a) {
        visit(blk, a)
    }
    var ch = false
    if (!empty(ct.counts)) {
        var rv = new RegroupShareVisitor(noFastMath = no_fast_math)
        rv.cm := ct.counts
        rv.mutableVars := mutableVars
        make_visitor(*rv) $(a) {
            visit(blk, a)
        }
        ch = rv.changed
        unsafe {
            delete rv
        }
    }
    unsafe {
        delete ct
    }
    delete mutableVars
    return ch
}

def private alias_source(e : Expression const?) : ExpressionPtr {
    if (e == null) return null
    if (e is ExprRef2Value) return alias_source((e as ExprRef2Value).subexpr)
    return e is ExprVar ? unsafe(reinterpret<Expression?>(e)) : null
}

def private eliminate_aliases(var blk : ExprBlock?) : bool {
    var mutableVars <- collect_mutable(blk)
    var rename : table<string; ExpressionPtr>
    var body : array<ExpressionPtr>
    var changed = false
    for (s0 in blk.list) {
        var s = !empty(rename) && reads_any(s0, rename) ? subst_consts(s0, rename) : s0
        if (s is ExprLet && length((s as ExprLet).variables) == 1) {
            let lvName = "{(s as ExprLet).variables[0].name}"
            let src = alias_source((s as ExprLet).variables[0].init)
            if (src != null && !key_exists(mutableVars, lvName) &&
                    !key_exists(mutableVars, "{(src as ExprVar).name}")) {
                rename[lvName] = clone_expression(src)
                changed = true
                continue
            }
        }
        body |> push(s)
    }
    if (changed) {
        while (!empty(blk.list)) {
            blk.list |> erase(length(blk.list) - 1)
        }
        for (b in body) {
            blk.list |> emplace(b)
        }
    }
    delete mutableVars
    unsafe {
        delete rename
        delete body
    }
    return changed
}

def private pex_residual_expr(pe : PEX; barriers : table<string>; e : ExpressionPtr) : bool {
    if (e == null) return false
    if (psh_has_compute(e) && pex_eligible(pe, barriers, e)) return true
    return fold_children_or(e) $(c) => pex_residual_expr(pe, barriers, c)
}

def private rcp_residual_expr(pe : PEX; barriers : table<string>; e : ExpressionPtr) : bool {
    if (e == null) return false
    if (e is ExprOp2 && is_rcp_candidate(pe, barriers, e as ExprOp2)) return true
    return fold_children_or(e) $(c) => rcp_residual_expr(pe, barriers, c)
}

def public opt_residuals(var func : FunctionPtr; barriers : table<string>;   // nolint:STYLE037,STYLE038 - one arm per residual peephole; the ladder IS the optimizer
                         no_fast_math : bool = false; hadd_min_width : int = 3) : array<string> {
    var res : array<string>
    if (func == null || func.body == null || !(func.body is ExprBlock)) return <- res
    var blk = func.body as ExprBlock
    var pe : PEX
    for (a in func.arguments) {
        pe.paramNames |> insert("{a.name}")
    }
    pe.mutableVars <- collect_mutable(blk)
    for (s in blk.list) {
        if (s is ExprLet) {
            for (lv in (s as ExprLet).variables) {
                pe.localColor["{lv.name}"] = lv.name |> starts_with("_preshader_")
            }
        }
    }
    let prefix = preshader_prefix_len(blk)
    for (i in range(length(blk.list))) {
        if (i < prefix) {
            continue
        }
        let s = blk.list[i]
        var hit = false
        if (s is ExprLet) {
            for (lv in (s as ExprLet).variables) {
                if (lv.init != null && pex_residual_expr(pe, barriers, lv.init)) {
                    hit = true
                }
            }
        } elif (s is ExprCopy) {
            hit = pex_residual_expr(pe, barriers, (s as ExprCopy).right)
        } elif (s is ExprReturn) {
            hit = pex_residual_expr(pe, barriers, (s as ExprReturn).subexpr)
        }
        if (hit) {
            res |> push("an un-extracted uniform subtree (preshader miss)")
            break
        }
    }
    if (!no_fast_math) {
        for (i in range(length(blk.list))) {
            if (i < prefix) {
                continue
            }
            let s = blk.list[i]
            var hit = false
            if (s is ExprLet) {
                for (lv in (s as ExprLet).variables) {
                    if (lv.init != null && rcp_residual_expr(pe, barriers, lv.init)) {
                        hit = true
                    }
                }
            } elif (s is ExprCopy) {
                hit = rcp_residual_expr(pe, barriers, (s as ExprCopy).right)
            } elif (s is ExprReturn) {
                hit = rcp_residual_expr(pe, barriers, (s as ExprReturn).subexpr)
            }
            if (hit) {
                res |> push("an un-rewritten division by a uniform ('x / U' should be 'x * _preshader(1/U)')")
                break
            }
        }
    }
    var counts : table<string; int>
    for (i in range(length(blk.list))) {
        var ct = new CseTally()
        ct.mutableVars := pe.mutableVars
        make_visitor(*ct) $(a) {
            cse_tally_stmt(blk.list[i], a)
        }
        for (k in keys(ct.counts)) {
            counts[k] = (counts?[k] ?? 0) + (ct.counts?[k] ?? 0)
        }
        unsafe {
            delete ct
        }
    }
    for (k in keys(counts)) {
        if ((counts?[k] ?? 0) >= 2) {
            res |> push("a repeated subtree '{k}' (>=2x) not CSE'd")
            break
        }
    }
    var cmt = new CmTally()
    cmt.mutableVars := pe.mutableVars
    make_visitor(*cmt) $(a) {
        visit(blk, a)
    }
    if (!empty(cmt.counts)) {
        var rr = new RegroupResidualVisitor(noFastMath = no_fast_math)
        rr.cm := cmt.counts
        rr.mutableVars := pe.mutableVars
        make_visitor(*rr) $(a) {
            visit(blk, a)
        }
        if (rr.found) {
            res |> push("a sum splitting a 'C - x' apart from its grouped twin (regroup-to-share miss)")
        }
        unsafe {
            delete rr
        }
    }
    unsafe {
        delete cmt
    }
    for (s in blk.list) {
        if (s is ExprLet && length((s as ExprLet).variables) == 1) {
            let lvName = "{(s as ExprLet).variables[0].name}"
            let src = alias_source((s as ExprLet).variables[0].init)
            if (src != null && !key_exists(pe.mutableVars, lvName) &&
                    !key_exists(pe.mutableVars, "{(src as ExprVar).name}")) {
                res |> push("a pure alias let '{lvName}' = '{(src as ExprVar).name}'")
                break
            }
        }
    }
    let cm = fuse_callable(func._module, "mad")
    var fr = new FuseResidualVisitor(checkMad = cm, checkLerp = cm && fuse_callable(func._module, "lerp"),
        checkDot = !no_fast_math && fuse_callable(func._module, "dot", 2),
        checkHadd = !no_fast_math && fuse_callable(func._module, "hadd", 1),
        haddMinWidth = hadd_min_width, noFastMath = no_fast_math)
    make_visitor(*fr) $(a) {
        visit(func.body, a)
    }
    if (fr.foundMad) {
        res |> push("an un-fused mul-add ('a*b +/- c' should be mad)")
    }
    if (fr.foundLerp) {
        res |> push("an un-refused lerp shape (mad(b-a,t,a))")
    }
    if (fr.foundNegAdd) {
        res |> push("an un-packed neg-add ('-a + b' should be 'b - a')")
    }
    if (fr.foundCtor) {
        res |> push("an un-packed ctor lane pattern (should re-vectorize / extract the shared factor)")
    }
    if (fr.foundDot) {
        res |> push("an un-fused lane sum ('v.x + v.y + ...' should be dot(v, mask))")
    }
    if (fr.foundHadd) {
        res |> push("an un-fused scalar sum of >= {hadd_min_width} lanes (should be hadd(floatN(...)))")
    }
    unsafe {
        delete fr
    }
    delete pe.paramNames
    delete pe.localColor
    delete pe.mutableVars
    delete counts
    return <- res
}

def private log_cse_cache(func : FunctionPtr; var blk : ExprBlock?) {
    var mutableVars <- collect_mutable(blk)
    var counts : table<string; int>
    for (i in range(length(blk.list))) {
        var ct = new CseTally()
        ct.mutableVars := mutableVars
        make_visitor(*ct) $(a) {
            cse_tally_stmt(blk.list[i], a)
        }
        for (k in keys(ct.counts)) {
            counts[k] = (counts?[k] ?? 0) + (ct.counts?[k] ?? 0)
        }
        unsafe {
            delete ct
        }
    }
    var cm = new CmTally()
    cm.mutableVars := mutableVars
    make_visitor(*cm) $(a) {
        visit(blk, a)
    }
    var ks <- [for (k in keys(counts)); k]
    sort(ks) $(a, b : string) : bool {
        let ca = counts?[a] ?? 0
        let cb = counts?[b] ?? 0
        if (ca != cb) {
            return ca > cb
        }
        return a < b
    }
    to_log(LOG_INFO, "flatten cse cache [{func.name}]: {length(ks)} pure subtrees\n")
    for (k in ks) {
        to_log(LOG_INFO, "    {counts?[k] ?? 0}x  {k}\n")
    }
    var cks <- [for (k in keys(cm.counts)); k]
    sort(cks)
    to_log(LOG_INFO, "  const-minus shapes: {length(cks)}\n")
    for (k in cks) {
        to_log(LOG_INFO, "    {cm.counts?[k] ?? 0}x  {k}\n")
    }
    unsafe {
        delete cm
    }
    delete counts
    delete ks
    delete cks
    delete mutableVars
}

def public flatten_preshade_cse(var func : FunctionPtr; barriers : table<string>; no_fast_math : bool = false; log_cse : bool = false) : bool {
    //! Post-fold optimize pass: hoist uniform (globals+literals) subtrees to top-of-body
    //! `_preshader_N` lets, regroup const-minus sums toward live `C - x` twins, CSE-dedup repeats,
    //! collapse pure aliases — a joint fixpoint. `log_cse` dumps the final value-number cache.
    if (!(func.body is ExprBlock)) return false
    var blk = func.body as ExprBlock
    var changed = false
    var guard = 0
    var cseCounter = 0
    var preCounter = 0
    while (guard < 16) {
        guard ++
        var round = false
        if (extract_preshader(func, blk, barriers, preCounter, no_fast_math)) {
            round = true
        }
        if (regroup_to_share(blk, no_fast_math)) {
            round = true
        }
        if (cse_body(blk, cseCounter)) {
            round = true
        }
        if (eliminate_aliases(blk)) {
            round = true
        }
        if (!round) {
            break
        }
        changed = true
    }
    if (guard >= 16) {
        to_log(LOG_WARNING, "flatten: cse/alias loop guard exhausted in {func.name} (oscillation?)\n")
    }
    if (log_cse) {
        log_cse_cache(func, blk)
    }
    return changed
}
