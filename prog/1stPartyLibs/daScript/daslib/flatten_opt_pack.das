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
// horizontal reduce (backend emits one width-W node, not N). Pre-fold. Design: history/examples/scalar_packing/PLAN.md.

struct private LinkInfo {
    ok      : bool
    defName : string   // var this link declares (acc_{k+1})
    dName   : string   // per-lane operand (ternary true arm)
    accRef  : string   // running accumulator (ternary false arm)
    op      : string   // "min" / "max"
}

// A reduction link `var acc' = (d cmp acc) ? d : acc`, classifying min/max. Fail-closed: both compare
// operands must be exactly the lane operand and the accumulator, so a threshold/shifted select does
// NOT classify (left untouched rather than mis-packed).
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
    r.defName = "{lc.variables[0].name}"
    r.dName = ta
    r.accRef = fa
    r.ok = true
    return r
}

// Name-HASHES of every var read in `e` (independence check) — hash reads the das_string directly, no
// per-ExprVar temp alloc. A 64-bit collision among one function's locals is effectively impossible
// (worst case: skip a valid pack). Practically safe, not provably so.
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

// Vectorize N parallel scalar exprs into one width-N (2..4) vector expr (CLONE-built): invariants
// splat to floatN, differing float consts → a floatN const, lane-local vars → the role-vector
// temp, op/call nodes rebuild over vectorized children. null on any unpackable shape (bails clean).
def private vectorize(exprs : array<ExpressionPtr>; nameToRole : table<string; int>;
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
    // invariant subtree → splat to floatN (daslang has no scalar→vector broadcast for +/-/call-args)
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
        // simple name (post-infer .name is the mangled scalar overload) so re-infer picks by arg width
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

// Horizontal reduce over `vecName`'s W (2..4) components as a single hmin/hmax call.
def private hreduce(vecName : string; width : int; op : string; at : LineInfo) : ExpressionPtr {
    // groups are width 2..4 (caller bails below M=3, plan_groups avoids width-1 tails), so one horizontal-reduce call (hmin/hmax) replaces the lane-extract + scalar tree.
    assert(width >= 2 && width <= 4, "hreduce expects a packed group of width 2..4")
    var call = new ExprCall(at = at, name := "h{op}")
    call.arguments |> push(new ExprVar(at = at, name := vecName))
    return call
}

def public pack_map_reduce(var func : FunctionPtr; no_fast_math : bool = false) : bool {
    //! Map-reduce loop re-roll: pack an unrolled min/max reduction's N isomorphic scalar lanes into
    //! width-W vector ops + a horizontal reduce. Returns true if it rewrote a chain. Run PRE-fold;
    //! caller re-infers after. Skipped under no_fast_math (the reduce-tree reassociation opt-out).
    if (no_fast_math || !(func.body is ExprBlock)) return false
    var blk = func.body as ExprBlock
    let n = length(blk.list)
    if (n < 6) return false   // init + >=3 lanes (each >=1 let + link) is the smallest worth packing
    var links : array<LinkInfo>
    links |> reserve(n)
    for (i in range(n)) {
        links |> push(classify_link(blk.list[i]))
    }
    var accRefToIdx : table<string; int>
    var isDef : table<string>
    for (i in range(n)) {
        if (links[i].ok) {
            if (!key_exists(accRefToIdx, links[i].accRef)) {
                accRefToIdx |> insert(links[i].accRef, i)
            }
            isDef |> insert(links[i].defName)
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
        if (did) break
        if (!links[h].ok || key_exists(isDef, links[h].accRef)) continue   // not a chain head
        // head accumulator must be a const-init let, used only by this head link
        var initIdx = -1
        for (j in range(h)) {
            if (!(blk.list[j] is ExprLet)) continue
            let lc = blk.list[j] as ExprLet
            if (length(lc.variables) == 1 && "{lc.variables[0].name}" == links[h].accRef &&
                    lc.variables[0].init != null && is_all_const(lc.variables[0].init)) {
                initIdx = j
                break
            }
        }
        if (initIdx < 0 ||
                (key_exists(ub.nameUses, hash(links[h].accRef)) &&
                 length(ub.nameUses[hash(links[h].accRef)]) != 1)) continue   // init absent or escapes
        // follow the chain; each intermediate accumulator must be single-use (its successor link)
        let op = links[h].op
        var chainIdx : array<int>
        var dNames : array<string>
        chainIdx |> reserve(n)
        dNames |> reserve(n)
        var cur = h
        var bail = false
        while (true) {
            chainIdx |> push(cur)
            dNames |> push(links[cur].dName)
            let dn = links[cur].defName
            // stop: no successor link consumes dn (chain end), or dn escapes (multi-use accumulator)
            if (!key_exists(accRefToIdx, dn) ||
                    !key_exists(ub.nameUses, hash(dn)) || length(ub.nameUses[hash(dn)]) != 1) break
            let nx = accRefToIdx[dn]
            if (nx <= cur || !links[nx].ok || links[nx].op != op) break
            cur = nx
        }
        let M = length(chainIdx)
        let lastLinkIdx = chainIdx[M - 1]
        // R = lane-lets between each pair of consecutive links (and after the init); must be uniform
        let R = chainIdx[0] - initIdx - 1
        if (M < 3 || R < 1) {
            delete chainIdx
            delete dNames
            continue
        }
        for (k in range(1, M)) {
            if (chainIdx[k] - chainIdx[k - 1] - 1 != R) {
                bail = true
                break
            }
        }
        // collect per-lane let inits (role order) + own-name sets; last let must define d_k
        var laneInit : array<array<ExpressionPtr>>
        var laneNames : array<array<string>>
        var laneOwnHash : array<table<uint64>>   // per-lane owned name-hashes (independence check)
        var nameToRole : table<string; int>
        var roleHash : table<uint64>             // all lane-local name-hashes (the role-name set)
        laneInit |> reserve(M)
        laneNames |> reserve(M)
        laneOwnHash |> reserve(M)
        if (!bail) {
            for (k in range(M)) {
                let prev = k == 0 ? initIdx : chainIdx[k - 1]
                var inits : array<ExpressionPtr>
                var names : array<string>
                var own : table<uint64>
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
                if (!bail && (empty(names) || names[length(names) - 1] != dNames[k])) {
                    bail = true
                }
                laneInit |> emplace(inits)
                laneNames |> emplace(names)
                laneOwnHash |> emplace(own)
                if (bail) break
            }
        }
        // independence: no lane let may read a spine accumulator or another lane's temp
        if (!bail) {
            var spineHash : table<uint64>
            spineHash |> insert(hash(links[h].accRef))
            for (ci in chainIdx) {
                spineHash |> insert(hash(links[ci].defName))
            }
            for (k in range(M)) {
                if (bail) break
                for (r in range(R)) {
                    var refs <- collect_var_hashes(laneInit[k][r])
                    for (rh in keys(refs)) {
                        if (key_exists(spineHash, rh) ||
                                (key_exists(roleHash, rh) && !key_exists(laneOwnHash[k], rh))) {
                            bail = true
                        }
                    }
                    delete refs
                    if (bail) break
                }
            }
            delete spineHash
        }
        if (bail) {
            delete chainIdx; delete dNames; delete laneNames; delete laneOwnHash; delete nameToRole; delete roleHash
            unsafe {
                delete laneInit
            }
            continue
        }
        // build the packed lets + per-group d-vector names
        var groups <- plan_groups(M)
        var newLets : array<ExpressionPtr>
        var groupDName : array<string>
        var groupWidth : array<int>
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
                    bail = true
                    break
                }
                newLets |> push(make_vec_let(roleVecName[r], v))
            }
            groupDName |> push(roleVecName[R - 1])
            groupWidth |> push(w)
            delete roleVecName
            laneBase += w
            if (bail) break
        }
        if (bail) {
            delete chainIdx; delete dNames; delete laneNames
            delete laneOwnHash; delete nameToRole; delete roleHash; delete groups
            delete groupDName; delete groupWidth
            unsafe {
                delete laneInit
                delete newLets
            }
            continue
        }
        // reduce: op(init, op(g0, op(g1, ...))) — preserves the original reduction's value set
        var acc = clone_expression((blk.list[initIdx] as ExprLet).variables[0].init)
        for (gid in range(length(groups))) {
            var hm = hreduce(groupDName[gid], groupWidth[gid], op, blk.at)
            var call = new ExprCall(at = blk.at, name := op)
            call.arguments |> push(acc)
            call.arguments |> push(hm)
            acc = call
        }
        let finalAcc = "{links[lastLinkIdx].defName}"
        var finalStmt = qmacro_expr(${ var $i(finalAcc) = $e(acc); })
        // splice: list is a dasvector — rebuild a fresh block (preamble drop-init, packed lets,
        // final reduce, tail), moving the surviving original statements across.
        var newBody = new ExprBlock(at = blk.at)
        for (i in range(initIdx)) {   // nolint:PERF018 — preamble is a [0,initIdx) sub-range
            newBody.list |> emplace(blk.list[i])
        }
        for (s in newLets) {
            newBody.list |> emplace(s)
        }
        newBody.list |> emplace(finalStmt)
        for (i in range(lastLinkIdx + 1, n)) {   // nolint:PERF018 — tail is a (lastLinkIdx,n) sub-range
            newBody.list |> emplace(blk.list[i])
        }
        func.body = newBody
        did = true
        delete chainIdx; delete dNames; delete laneNames
        delete laneOwnHash; delete nameToRole; delete roleHash; delete groups
        delete groupDName; delete groupWidth
        unsafe {
            delete laneInit
            delete newLets
        }
    }
    unsafe {
        delete ub
    }
    return did
}
