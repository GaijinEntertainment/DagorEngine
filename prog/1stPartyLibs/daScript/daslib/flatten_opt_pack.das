options gen2
options indenting = 4
options strict_smart_pointers = true
options _comment_hygiene = true

module flatten_opt_pack shared private

//! Scalar packing: re-roll the N isomorphic scalar lanes of an unrolled min/max reduction into width-W vector ops + a horizontal reduce.

require daslib/ast
require daslib/ast_boost
require daslib/ast_match
require daslib/templates_boost
require daslib/macro_boost
require daslib/match
require daslib/rtti
require daslib/flatten_opt_common


// ===== Scalar packing — map-reduce loop re-roll (Phase 1) =====
// Re-rolls an unrolled min/max reduction's N isomorphic scalar lanes into width-W vector ops + a
// horizontal reduce (backend emits one width-W node, not N). Pre-fold.

struct private LinkInfo {
    ok      : bool
    declaredName : string
    laneOperandName : string
    accumulatorName : string
    op      : string
}

def private classify_link(s : ExpressionPtr) : LinkInfo {
    var r : LinkInfo
    if (!(s is ExprLet)) return r
    let lc = s as ExprLet
    if (length(lc.variables) != 1) return r
    let t = sel3(lc.variables[0].init)
    if (t == null || !(t.subexpr is ExprOp2)) return r
    let cond = t.subexpr as ExprOp2
    let isLess = cond.op == "<" || cond.op == "<="
    let isGreater = cond.op == ">" || cond.op == ">="
    if (!isLess && !isGreater) return r
    let ta = ref_var_name(t.left)
    let fa = ref_var_name(t.right)
    if (ta == "" || fa == "" || ta == fa) return r
    let cl = ref_var_name(cond.left)
    let cr = ref_var_name(cond.right)
    var condLeftIsNew = false
    if (cl == ta && cr == fa) {
        condLeftIsNew = true
    } elif (cr == ta && cl == fa) {
        condLeftIsNew = false
    } else {
        return r
    }
    r.op = (isLess == condLeftIsNew) ? "min" : "max"
    r.declaredName = "{lc.variables[0].name}"
    r.laneOperandName = ta
    r.accumulatorName = fa
    r.ok = true
    return r
}

class private VarNames : AstVisitor {
    found : table<uint64>
    def override preVisitExprVar(expr : ExprVar?) : void {
        found |> insert(hash(expr.name))
    }
}

def private collect_var_hashes(e : ExpressionPtr) : table<uint64> {
    var res : table<uint64>
    var v = new VarNames()
    make_visitor(*v) $(adapter) {
        visit(e, adapter)
    }
    res <- v.found
    unsafe {
        delete v
    }
    return <- res
}

def private vectorize(exprs : array<ExpressionPtr>; nameToRole : table<string; int>;   // nolint:STYLE037,STYLE038 — one arm per lane node class, each collect-columns → recurse → rebuild
                      roleVecName : array<string>; at : LineInfo) : ExpressionPtr {
    let N = length(exprs)
    let key0 = describe(exprs[0])
    var allEq = true
    for (i in range(1, N)) {
        if (describe(exprs[i]) != key0) {
            allEq = false
            break
        }
    }
    if (allEq) return make_float_splat(N, at, clone_expression(exprs[0]))
    if (exprs[0] is ExprRef2Value) {
        var inner : array<ExpressionPtr>
        inner |> reserve(N)
        var okk = true
        for (e in exprs) {
            if (!(e is ExprRef2Value)) {
                okk = false
                break
            }
            inner |> push((e as ExprRef2Value).subexpr)
        }
        var res : ExpressionPtr = null
        if (okk) {
            res = vectorize(inner, nameToRole, roleVecName, at)
        }
        unsafe {
            delete inner
        }
        return res
    }
    if (exprs[0] is ExprConstFloat) {
        var lanes : array<ExpressionPtr>
        lanes |> reserve(N)
        var okk = true
        for (e in exprs) {
            if (!(e is ExprConstFloat)) {
                okk = false
                break
            }
            lanes |> push(clone_expression(e))
        }
        var res : ExpressionPtr = null
        if (okk) {
            res = make_float_ctor(N, at, lanes)
        }
        unsafe {
            delete lanes
        }
        return res
    }
    if (exprs[0] is ExprVar) {
        var role = -1
        for (e in exprs) {
            if (!(e is ExprVar)) return null
            let nm = "{(e as ExprVar).name}"
            let rr = nameToRole?[nm] ?? -1
            if (rr < 0) return null
            if (role < 0) {
                role = rr
            } elif (role != rr) {
                return null
            }
        }
        return new ExprVar(at = at, name := roleVecName[role])
    }
    if (exprs[0] is ExprOp2) {
        let l0 = exprs[0] as ExprOp2
        let op0 = "{l0.op}"
        var lefts : array<ExpressionPtr>
        var rights : array<ExpressionPtr>
        lefts |> reserve(N)
        rights |> reserve(N)
        var okk = true
        for (e in exprs) {
            if (!(e is ExprOp2) || "{(e as ExprOp2).op}" != op0) {
                okk = false
                break
            }
            lefts |> push((e as ExprOp2).left)
            rights |> push((e as ExprOp2).right)
        }
        var res : ExpressionPtr = null
        if (okk) {
            var vl = vectorize(lefts, nameToRole, roleVecName, at)
            var vr : ExpressionPtr = null
            if (vl != null) {
                vr = vectorize(rights, nameToRole, roleVecName, at)
            }
            if (vl != null && vr != null) {
                res = new ExprOp2(at = at, atEnclosure = l0.atEnclosure, genFlags = l0.genFlags,
                                  op := op0, left = vl, right = vr)
            }
        }
        unsafe {
            delete lefts
            delete rights
        }
        return res
    }
    if (exprs[0] is ExprOp1) {
        let l0 = exprs[0] as ExprOp1
        let op0 = "{l0.op}"
        var subs : array<ExpressionPtr>
        subs |> reserve(N)
        var okk = true
        for (e in exprs) {
            if (!(e is ExprOp1) || "{(e as ExprOp1).op}" != op0) {
                okk = false
                break
            }
            subs |> push((e as ExprOp1).subexpr)
        }
        var res : ExpressionPtr = null
        if (okk) {
            var vs = vectorize(subs, nameToRole, roleVecName, at)
            if (vs != null) {
                res = new ExprOp1(at = at, atEnclosure = l0.atEnclosure, genFlags = l0.genFlags,
                                  op := op0, subexpr = vs)
            }
        }
        unsafe {
            delete subs
        }
        return res
    }
    if (exprs[0] is ExprCall) {
        let l0 = exprs[0] as ExprCall
        let nm0 = simple_name("{l0.name}")
        let argc = length(l0.arguments)
        for (e in exprs) {
            if (!(e is ExprCall) || simple_name("{(e as ExprCall).name}") != nm0 ||
                    length((e as ExprCall).arguments) != argc) {
                return null
            }
        }
        var c = new ExprCall(at = at, atEnclosure = l0.atEnclosure, genFlags = l0.genFlags, name := nm0)
        for (ai in range(argc)) {
            var col : array<ExpressionPtr>
            col |> reserve(N)
            for (e in exprs) {
                col |> push((e as ExprCall).arguments[ai])
            }
            var v = vectorize(col, nameToRole, roleVecName, at)
            unsafe {
                delete col
            }
            if (v == null) return null
            c.arguments |> push(v)
        }
        return c
    }
    return null
}

def private make_vec_let(vecName : string; var vecExpr : ExpressionPtr) : ExpressionPtr {
    return qmacro_expr(${ let $i(vecName) = $e(vecExpr); })
}

def private hreduce(vecName : string; width : int; op : string; at : LineInfo) : ExpressionPtr {
    assert(width >= 2 && width <= 4, "hreduce expects a packed group of width 2..4")
    var call = new ExprCall(at = at, name := "h{op}")
    call.arguments |> push(new ExprVar(at = at, name := vecName))
    return call
}

def private find_chain_init(var blk : ExprBlock?; accumulatorName : string; h : int) : int {
    for (j in range(h)) {
        if (!(blk.list[j] is ExprLet)) continue
        let lc = blk.list[j] as ExprLet
        if (length(lc.variables) == 1 && "{lc.variables[0].name}" == accumulatorName &&
                lc.variables[0].init != null && is_all_const(lc.variables[0].init)) {
            return j
        }
    }
    return -1
}

def private follow_chain(links : array<LinkInfo>; var accumulatorToIdx : table<string; int>; var ub : FlatUsesBuilder?;
                         h : int; var chainIdx : array<int>; var laneOperandNames : array<string>) {
    let op = links[h].op
    var cur = h
    while (true) {
        chainIdx |> push(cur)
        laneOperandNames |> push(links[cur].laneOperandName)
        let dn = links[cur].declaredName
        if (!key_exists(accumulatorToIdx, dn) ||
                !key_exists(ub.nameUses, hash(dn)) || length(ub.nameUses[hash(dn)]) != 1) break
        let nx = accumulatorToIdx[dn]
        if (nx <= cur || !links[nx].ok || links[nx].op != op) break
        cur = nx
    }
}

def private collect_lane_lets(var blk : ExprBlock?; chainIdx : array<int>; laneOperandNames : array<string>;
                              initIdx, R, M : int; var laneInit : array<array<ExpressionPtr>>;
                              var laneNames : array<array<string>>; var laneOwnHash : array<table<uint64>>;
                              var nameToRole : table<string; int>; var roleHash : table<uint64>) : bool {
    laneInit |> reserve(M)
    laneNames |> reserve(M)
    laneOwnHash |> reserve(M)
    for (k in range(M)) {
        let prev = k == 0 ? initIdx : chainIdx[k - 1]
        var inits : array<ExpressionPtr>
        var names : array<string>
        var own : table<uint64>
        var bail = false
        inits |> reserve(R)
        names |> reserve(R)
        for (idx in range(prev + 1, chainIdx[k])) {
            if (!(blk.list[idx] is ExprLet) || length((blk.list[idx] as ExprLet).variables) != 1) {
                bail = true
                break
            }
            var lc = blk.list[idx] as ExprLet
            inits |> push(lc.variables[0].init)
            let nm = "{lc.variables[0].name}"
            names |> push(nm)
            own |> insert(hash(nm))
            nameToRole |> insert(nm, length(names) - 1)
            roleHash |> insert(hash(nm))
        }
        if (!bail && (empty(names) || names[length(names) - 1] != laneOperandNames[k])) {
            bail = true
        }
        laneInit |> emplace(inits)
        laneNames |> emplace(names)
        laneOwnHash |> emplace(own)
        if (bail) return false
    }
    return true
}

def private lanes_independent(links : array<LinkInfo>; headAccRef : string; chainIdx : array<int>;
                              laneInit : array<array<ExpressionPtr>>; roleHash : table<uint64>;
                              laneOwnHash : array<table<uint64>>; M, R : int) : bool {
    var spineHash : table<uint64>
    spineHash |> insert(hash(headAccRef))
    for (ci in chainIdx) {
        spineHash |> insert(hash(links[ci].declaredName))
    }
    var ok = true
    for (k in range(M)) {
        if (!ok) break
        for (r in range(R)) {
            var refs <- collect_var_hashes(laneInit[k][r])
            for (rh in keys(refs)) {
                if (key_exists(spineHash, rh) ||
                        (key_exists(roleHash, rh) && !key_exists(laneOwnHash[k], rh))) {
                    ok = false
                }
            }
            delete refs
            if (!ok) break
        }
    }
    delete spineHash
    return ok
}

def private build_packed_lets(var blk : ExprBlock?; groups : array<int>; var laneInit : array<array<ExpressionPtr>>;
                              nameToRole : table<string; int>; h, R : int; var newLets : array<ExpressionPtr>;
                              var groupDName : array<string>; var groupWidth : array<int>) : bool {
    newLets |> reserve(length(groups) * R)
    groupDName |> reserve(length(groups))
    groupWidth |> reserve(length(groups))
    var laneBase = 0
    for (gid in range(length(groups))) {
        let w = groups[gid]
        var roleVecName : array<string>
        roleVecName |> reserve(R)
        for (r in range(R)) {
            roleVecName |> push("_pk{h}_{gid}_{r}")
        }
        var ok = true
        for (r in range(R)) {
            var col : array<ExpressionPtr>
            col |> reserve(w)
            for (li in range(w)) {
                col |> push(laneInit[laneBase + li][r])
            }
            var v = vectorize(col, nameToRole, roleVecName, blk.at)
            unsafe {
                delete col
            }
            if (v == null) {
                ok = false
                break
            }
            newLets |> push(make_vec_let(roleVecName[r], v))
        }
        groupDName |> push(roleVecName[R - 1])
        groupWidth |> push(w)
        delete roleVecName
        laneBase += w
        if (!ok) return false
    }
    return true
}

def private splice_packed_body(var func : FunctionPtr; var blk : ExprBlock?; groups : array<int>;
                               groupDName : array<string>; groupWidth : array<int>; op : string;
                               initIdx, lastLinkIdx, n : int; var newLets : array<ExpressionPtr>;
                               finalAcc : string) {
    var acc = clone_expression((blk.list[initIdx] as ExprLet).variables[0].init)
    for (gid in range(length(groups))) {
        var hm = hreduce(groupDName[gid], groupWidth[gid], op, blk.at)
        var call = new ExprCall(at = blk.at, name := op)
        call.arguments |> push(acc)
        call.arguments |> push(hm)
        acc = call
    }
    var finalStmt = qmacro_expr(${ var $i(finalAcc) = $e(acc); })
    var newBody = new ExprBlock(at = blk.at)
    for (i in range(initIdx)) {
        newBody.list |> emplace(blk.list[i])
    }
    for (s in newLets) {
        newBody.list |> emplace(s)
    }
    newBody.list |> emplace(finalStmt)
    for (i in range(lastLinkIdx + 1, n)) {
        newBody.list |> emplace(blk.list[i])
    }
    func.body = newBody
}

def private try_pack_chain_at(var func : FunctionPtr; var blk : ExprBlock?; links : array<LinkInfo>;
                              var accumulatorToIdx : table<string; int>; isDef : table<string>;
                              var ub : FlatUsesBuilder?; h, n : int) : bool {
    if (!links[h].ok || key_exists(isDef, links[h].accumulatorName)) return false
    let initIdx = find_chain_init(blk, links[h].accumulatorName, h)
    if (initIdx < 0 ||
            (key_exists(ub.nameUses, hash(links[h].accumulatorName)) &&
             length(ub.nameUses[hash(links[h].accumulatorName)]) != 1)) return false
    let op = links[h].op
    var chainIdx : array<int>
    var laneOperandNames : array<string>
    chainIdx |> reserve(n)
    laneOperandNames |> reserve(n)
    follow_chain(links, accumulatorToIdx, ub, h, chainIdx, laneOperandNames)
    let M = length(chainIdx)
    let lastLinkIdx = chainIdx[M - 1]
    let R = chainIdx[0] - initIdx - 1
    if (M < 3 || R < 1) {
        delete chainIdx
        delete laneOperandNames
        return false
    }
    var bail = false
    for (k in range(1, M)) {
        if (chainIdx[k] - chainIdx[k - 1] - 1 != R) {
            bail = true
            break
        }
    }
    var laneInit : array<array<ExpressionPtr>>
    var laneNames : array<array<string>>
    var laneOwnHash : array<table<uint64>>
    var nameToRole : table<string; int>
    var roleHash : table<uint64>
    if (!bail) {
        bail = !collect_lane_lets(blk, chainIdx, laneOperandNames, initIdx, R, M,
            laneInit, laneNames, laneOwnHash, nameToRole, roleHash)
    }
    if (!bail) {
        bail = !lanes_independent(links, links[h].accumulatorName, chainIdx, laneInit, roleHash, laneOwnHash, M, R)
    }
    if (bail) {
        delete chainIdx; delete laneOperandNames; delete laneNames; delete laneOwnHash; delete nameToRole; delete roleHash
        unsafe {
            delete laneInit
        }
        return false
    }
    var groups <- plan_groups(M)
    var newLets : array<ExpressionPtr>
    var groupDName : array<string>
    var groupWidth : array<int>
    let packed = build_packed_lets(blk, groups, laneInit, nameToRole, h, R, newLets, groupDName, groupWidth)
    if (packed) {
        splice_packed_body(func, blk, groups, groupDName, groupWidth, op, initIdx, lastLinkIdx, n,
            newLets, "{links[lastLinkIdx].declaredName}")
    }
    delete chainIdx; delete laneOperandNames; delete laneNames
    delete laneOwnHash; delete nameToRole; delete roleHash; delete groups
    delete groupDName; delete groupWidth
    unsafe {
        delete laneInit
        delete newLets
    }
    return packed
}

def public pack_map_reduce(var func : FunctionPtr; no_fast_math : bool = false) : bool {
    //! Map-reduce loop re-roll: pack an unrolled min/max reduction's N isomorphic scalar lanes into
    //! width-W vector ops + a horizontal reduce. Returns true if it rewrote a chain. Run PRE-fold;
    //! caller re-infers after. Skipped under no_fast_math (the reduce-tree reassociation opt-out).
    if (no_fast_math || !(func.body is ExprBlock)) return false
    var blk = func.body as ExprBlock
    let n = length(blk.list)
    if (n < 6) return false
    var links : array<LinkInfo>
    links |> reserve(n)
    for (i in range(n)) {
        links |> push(classify_link(blk.list[i]))
    }
    var accumulatorToIdx : table<string; int>
    var isDef : table<string>
    for (i in range(n)) {
        if (links[i].ok) {
            if (!key_exists(accumulatorToIdx, links[i].accumulatorName)) {
                accumulatorToIdx |> insert(links[i].accumulatorName, i)
            }
            isDef |> insert(links[i].declaredName)
        }
    }
    var ub = new FlatUsesBuilder()
    make_visitor(*ub) $(adapter) {
        for (j in range(n)) {
            ub.j = j
            clear(ub.seen)
            visit(blk.list[j], adapter)
        }
    }
    var did = false
    for (h in range(n)) {
        if (try_pack_chain_at(func, blk, links, accumulatorToIdx, isDef, ub, h, n)) {
            did = true
            break
        }
    }
    unsafe {
        delete ub
    }
    return did
}
