options gen2
options indenting = 4

module dupe_detect shared private

//! Extractable-duplicate detection over the AST. Backs STYLE040; the rule wiring lives in
//! ``daslib/style_lint``.
//!
//! Hashes only PICK candidates - ``subtree_equal`` decides, so no collision can be reported.
//! Do NOT replace the ``hash_combine64`` fold with ``(h ^ v) * prime``: it annihilates to 0 when
//! the accumulator equals the child hash, and a zeroed accumulator swallows the subtree below it.

require daslib/ast_boost
require daslib/rtti
require strings
require math

// tags a payload arrival so it cannot collide with a child subtree hash, which folds in untagged
let private TAG_PAYLOAD = 0x50415900ul

//! Tunables. Defaults calibrated on ``daslib/``: the widest duplicate there is 39 nodes.
struct public DupeConfig {
    min_nodes : int = 20
    min_statements : int = 2
    max_params : int = 5
    max_bucket : int = 200
    max_findings : int = 64
}

//! One occurrence of a duplicated region, as text for the message.
struct public DupeSite {
    file : string
    line : int
    column : int
    last_line : int     //!< last physical line of the region, so callers can report the span
}

//! One duplicated, extractable region. ``anchor`` is the first statement of the first
//! occurrence - report against ``anchor.at``; ``sites`` lists every occurrence in source order.
struct public DupeFinding {
    @do_not_delete anchor : Expression?
    sites : array<DupeSite>
    statements : int
    nodes : int
    params : array<string>
    message : string        //!< ready to report against `anchor.at`; see `finding_message`
}

let private OPAQUE_CLASSES : table<string> <- {
    "ExprReader", "ExprQuote", "ExprTag", "ExprLabel", "ExprGoto", "ExprReturn", "ExprYield",
    "ExprMakeGenerator", "ExprFakeContext", "ExprFakeLineInfo", "ExprTryCatch"
}

def private needs_unsafe(expr : Expression?) : bool {
    if (expr is ExprCast) {
        let c = expr as ExprCast
        return c.castFlags.reinterpretCast || c.castFlags.upcastCast
    }
    if (expr is ExprPtr2Ref) return (expr as ExprPtr2Ref).unsafeDeref
    if (expr is ExprCall) {
        let f = (expr as ExprCall).func
        return f != null && f.flags.unsafeOperation
    }
    return expr is ExprDelete || expr is ExprRef2Ptr
}

def private payload_key(expr : Expression?) : string {   // nolint:STYLE037 - `is` on handled types is exact-type, so one arm per payload-bearing class is irreducible; hashing and verification MUST read the same key, so this stays one function
    if (expr is ExprVar) {
        let e = expr as ExprVar
        return (e.varFlags.local || e.varFlags.argument || e.varFlags._block) ? "" : "{e.name}"
    }
    if (expr is ExprField) return "{(expr as ExprField.name)}:{(expr as ExprField.fieldFlags)}:{(expr as ExprField.derefFlags)}"
    if (expr is ExprSafeField) return "{(expr as ExprSafeField.name)}:{(expr as ExprSafeField.skipQQ)}"
    if (expr is ExprSwizzle) return "{(expr as ExprSwizzle.mask)}:{(expr as ExprSwizzle.fieldFlags)}"
    if (expr is ExprIsVariant) return "{(expr as ExprIsVariant.name)}"
    if (expr is ExprAsVariant) return "{(expr as ExprAsVariant.name)}"
    if (expr is ExprSafeAsVariant) return "{(expr as ExprSafeAsVariant.name)}:{(expr as ExprSafeAsVariant.skipQQ)}"
    if (expr is ExprOp1) return "{(expr as ExprOp1.op)}:{func_key((expr as ExprOp1.func))}"
    if (expr is ExprOp2) return "{(expr as ExprOp2.op)}:{func_key((expr as ExprOp2.func))}"
    if (expr is ExprOp3) return "{(expr as ExprOp3.op)}"
    if (expr is ExprCopy) return "{(expr as ExprCopy.copy_flags)}"
    if (expr is ExprMove) return "{(expr as ExprMove.move_flags)}"
    if (expr is ExprClone) return "{(expr as ExprClone.clone_flags)}"
    if (expr is ExprCall) return "{(expr as ExprCall.name)}:{func_key((expr as ExprCall.func))}"
    if (expr is ExprNamedCall) return "{(expr as ExprNamedCall.name)}:{(expr as ExprNamedCall.nonNamedArguments) |> length}"
    if (expr is ExprCallMacro) return "{(expr as ExprCallMacro.name)}"
    if (expr is ExprInvoke) return "{(expr as ExprInvoke.name)}:{(expr as ExprInvoke.isInvokeMethod)}"
    if (expr is ExprNew) return "{(expr as ExprNew.name)}:{func_key((expr as ExprNew.func))}:{(expr as ExprNew.initializer)}:{(expr as ExprNew.allocate_on_stack)}"
    if (expr is ExprPtr2Ref) return "{(expr as ExprPtr2Ref.unsafeDeref)}:{(expr as ExprPtr2Ref.assumeNoAlias)}"
    if (expr is ExprStringBuilder) return "{(expr as ExprStringBuilder.stringBuilderFlags)}"
    if (expr is ExprAscend) return "{(expr as ExprAscend.ascendFlags)}"
    if (expr is ExprDelete) return "{(expr as ExprDelete.native)}"
    if (expr is ExprCast) return "{(expr as ExprCast.castFlags)}"
    if (expr is ExprAt) return "{(expr as ExprAt.atFlags)}"
    if (expr is ExprSafeAt) return "{(expr as ExprSafeAt.atFlags)}"
    if (expr is ExprBlock) {
        let e = expr as ExprBlock
        return "{e.blockFlags}:{e.list |> length}:{e.finalList |> length}:{annotation_key(e.annotations)}"
    }
    if (expr is ExprMakeBlock) return "{(expr as ExprMakeBlock.mmFlags)}"
    if (expr is ExprLet) return "{(expr as ExprLet.letFlags)}:{(expr as ExprLet.variables) |> length}"
    if (expr is ExprFor) {
        let e = expr as ExprFor
        return "{e.iterators |> length}:{annotation_key(e.annotations)}"
    }
    if (expr is ExprWhile) return "{annotation_key((expr as ExprWhile.annotations))}"
    if (expr is ExprWith) return "{(expr as ExprWith.moduleName)}"
    if (expr is ExprAssume) return "{(expr as ExprAssume.alias)}"
    if (expr is ExprIfThenElse) return "{(expr as ExprIfThenElse.if_flags)}:{(expr as ExprIfThenElse.if_false) != null}"
    if (expr is ExprMakeStruct) return "{(expr as ExprMakeStruct.makeFlags)}:{(expr as ExprMakeStruct.makeStructFlags)}:{(expr as ExprMakeStruct.structs) |> length}"
    if (expr is ExprMakeVariant) return "{(expr as ExprMakeVariant.variants) |> length}"
    if (expr is ExprMakeArray) return "{(expr as ExprMakeArray.values) |> length}:{(expr as ExprMakeArray.makeFlags)}"
    if (expr is ExprMakeTuple) return "{(expr as ExprMakeTuple.values) |> length}:{(expr as ExprMakeTuple.isKeyValue)}"
    if (expr is ExprArrayComprehension) return "{(expr as ExprArrayComprehension.generatorSyntax)}:{(expr as ExprArrayComprehension.tableSyntax)}"
    if (expr is ExprTypeInfo) return "{(expr as ExprTypeInfo.trait)}:{(expr as ExprTypeInfo.subtrait)}:{(expr as ExprTypeInfo.extratrait)}"
    if (expr is ExprAddr) return "{(expr as ExprAddr.target)}:{func_key((expr as ExprAddr.func))}"
    if (expr is ExprAssert) return "{(expr as ExprAssert.name)}:{(expr as ExprAssert.isVerify)}"
    return const_key(expr)
}

def private const_key(expr : Expression?) : string {   // nolint:STYLE037 - one arm per const class; `is` on handled types is exact-type
    return "" if (!starts_with("{expr.__rtti}", "ExprConst"))
    if (expr is ExprConstInt) return "ci:{(expr as ExprConstInt).value}"
    if (expr is ExprConstUInt) return "cu:{(expr as ExprConstUInt).value}"
    if (expr is ExprConstString) return "cs:{(expr as ExprConstString).value}"
    if (expr is ExprConstBool) return "cb:{(expr as ExprConstBool).value}"
    if (expr is ExprConstFloat) return "cf:{(expr as ExprConstFloat).value}"
    if (expr is ExprConstDouble) return "cd:{(expr as ExprConstDouble).value}"
    if (expr is ExprConstInt64) return "ci64:{(expr as ExprConstInt64).value}"
    if (expr is ExprConstUInt64) return "cu64:{(expr as ExprConstUInt64).value}"
    if (expr is ExprConstInt8) return "ci8:{(expr as ExprConstInt8).value}"
    if (expr is ExprConstUInt8) return "cu8:{(expr as ExprConstUInt8).value}"
    if (expr is ExprConstInt16) return "ci16:{(expr as ExprConstInt16).value}"
    if (expr is ExprConstUInt16) return "cu16:{(expr as ExprConstUInt16).value}"
    if (expr is ExprConstFloat16) return "cf16:{(expr as ExprConstFloat16).value}"
    if (expr is ExprConstInt2) return "ci2:{(expr as ExprConstInt2).value}"
    if (expr is ExprConstInt3) return "ci3:{(expr as ExprConstInt3).value}"
    if (expr is ExprConstInt4) return "ci4:{(expr as ExprConstInt4).value}"
    if (expr is ExprConstUInt2) return "cu2:{(expr as ExprConstUInt2).value}"
    if (expr is ExprConstUInt3) return "cu3:{(expr as ExprConstUInt3).value}"
    if (expr is ExprConstUInt4) return "cu4:{(expr as ExprConstUInt4).value}"
    if (expr is ExprConstFloat2) return "cf2:{(expr as ExprConstFloat2).value}"
    if (expr is ExprConstFloat3) return "cf3:{(expr as ExprConstFloat3).value}"
    if (expr is ExprConstFloat4) return "cf4:{(expr as ExprConstFloat4).value}"
    if (expr is ExprConstRange) return "cr:{(expr as ExprConstRange).value}"
    if (expr is ExprConstURange) return "cur:{(expr as ExprConstURange).value}"
    if (expr is ExprConstRange64) return "cr64:{(expr as ExprConstRange64).value}"
    if (expr is ExprConstURange64) return "cur64:{(expr as ExprConstURange64).value}"
    if (expr is ExprConstPtr) return "cptr:{(expr as ExprConstPtr).value}"
    if (expr is ExprConstBitfield) return "cbf:{(expr as ExprConstBitfield).value}"
    if (expr is ExprConstEnumeration) return "cenum:{(expr as ExprConstEnumeration).value}"
    return ""
}

def private annotation_key(anns : AnnotationList) : string {
    return empty(anns) ? "" : describe(anns)
}

def private annotation_key(args : AnnotationArgumentList) : string {
    return describe(args)
}

def private func_key(fn : Function?) : string {
    return "" if (fn == null)
    return "{fn.getMangledNameHash}"
}

struct private NodeRec {
    payload : uint64        //!< class name folded with every non-child payload arrival
    hash : uint64           //!< payload folded with child hashes, in visit order (Merkle)
    size : int = 1          //!< self + descendants; the subtree is preorder interval [id, id + size)
    nchild : int = 0
    vid : int = -1          //!< >= 0 when the node references or declares a local / argument
    wr : bool = false       //!< that reference is in a write position
    known : bool = false    //!< class is on the allowlist
}

struct private VarRec {
    decl_node : int = -1    //!< node that declares it, -1 for a function argument or an unseen decl
    last_use : int = -1
    is_ref : bool = false   //!< propagates through a parameter (ref or ref-type)
    name : string
    type_text : string      //!< full describe, for the "is this spellable" gate
    type_show : string      //!< how a suggested parameter list spells it
}

struct private StmtFrame {
    blk_id : int
    node : int = -1
    escape : int
    freejump : int
    defer_calls : int
    scoped : int
    binds : int
    unsafe_ops : int
    loops : int
}

class private DupeIndexer : AstVisitor {
    nodes : array<NodeRec>
    @do_not_delete node_expr : array<Expression?>
    vars : array<VarRec>
    decls_ascending_by_node : array<int>
    @do_not_delete var_id : table<Variable?; int>
    @do_not_delete type_cache : table<TypeDecl?; string>
    stmt_block : array<int>
    stmt_node : array<int>
    stmt_ok : array<bool>
    stmt_pos_in_block : array<int>
    by_block : array<array<int>>
    n_blocks : int = 0
    stack : array<int>
    block_stack : array<int>
    stmt_frames : array<StmtFrame>
    n_escape : int = 0
    n_freejump : int = 0
    n_defer : int = 0
    n_scoped : int = 0
    n_bind : int = 0
    n_unsafe_ops : int = 0
    loop_depth : int = 0

    def type_text(td : TypeDecl?) : string {
        return "" if (td == null)
        if (key_exists(type_cache, td)) return type_cache[td]
        let t = describe(td)
        type_cache |> insert(td, t)
        return t
    }

    def type_hash(td : TypeDecl?) : uint64 {
        return 0ul if (td == null)
        return hash(type_text(td))
    }

    def type_display(td : TypeDecl?) : string {
        return "" if (td == null)
        let t = describe(td, false, false, false)
        return ends_with(t, " const") ? slice(t, 0, length(t) - 6) : t
    }

    def open_node() : int {
        var i = length(stack)
        while (i > 0) {
            return stack[i - 1] if (stack[i - 1] >= 0)
            i--
        }
        return -1
    }

    def add_payload(v : uint64) : void {
        let top = open_node()
        return if (top < 0)
        nodes[top].payload = hash_combine64(nodes[top].payload, v)
        nodes[top].hash = hash_combine64(nodes[top].hash, v)
    }

    def var_slot(v : Variable?) : int {
        return -1 if (v == null)
        if (key_exists(var_id, v)) return var_id[v]
        let id = length(vars)
        let td = v._type
        var rec = VarRec(name = "{v.name}", type_text = type_text(td), type_show = type_display(td))
        rec.is_ref = td != null && (td.isRef || td.isRefType)
        vars |> push(rec)
        var_id |> insert(v, id)
        return id
    }

    def note_use(vid, node : int) : void {
        return if (vid < 0)
        vars[vid].last_use = node
    }


    def override preVisitExpression(var expr : ExpressionPtr) {
        let top = open_node()
        if (top >= 0 && node_expr[top] == expr) {
            stack |> push(-1)
            return
        }
        if (needs_unsafe(expr)) {
            n_unsafe_ops++
        }
        let id = length(nodes)
        let cls = "{expr.__rtti}"
        let seed = hash(cls)
        var rec = NodeRec(payload = seed, hash = seed)
        rec.known = !key_exists(OPAQUE_CLASSES, cls)
        nodes |> push(rec)
        node_expr |> push(expr)
        stack |> push(id)
        add_payload(hash_combine64(TAG_PAYLOAD, type_hash(expr._type)))
        let key = payload_key(expr)
        if (!empty(key)) {
            add_payload(hash_combine64(TAG_PAYLOAD, hash(key)))
        }
        if (!empty(stmt_frames)) {
            let last = length(stmt_frames) - 1
            if (stmt_frames[last].node < 0) {
                stmt_frames[last].node = id
            }
        }
    }

    def override visitExpression(var expr : ExpressionPtr) : ExpressionPtr {
        let id = stack[length(stack) - 1]
        stack |> pop
        return expr if (id < 0)
        nodes[id].size = length(nodes) - id
        let parent = open_node()
        if (parent >= 0) {
            nodes[parent].hash = hash_combine64(nodes[parent].hash, nodes[id].hash)
            nodes[parent].nchild++
        }
        return expr
    }


    def override preVisitExprVar(expr : ExprVar?) {
        let id = open_node()
        return if (id < 0 || !(expr.varFlags.local || expr.varFlags.argument
            || expr.varFlags._block))
        let vid = var_slot(expr.variable)
        nodes[id].vid = vid
        nodes[id].wr = expr.varFlags.write
        note_use(vid, id)
    }


    def override preVisitFunctionArgument(fun : FunctionPtr; arg : VariablePtr; lastArg : bool) {
        var_slot(arg)
    }

    def override preVisitExprLetVariable(expr : ExprLet?; arg : VariablePtr; lastArg : bool) {
        declare_var(arg)
        if (arg.flags.inScope || (arg._type != null && arg._type.needDelete)) {
            n_scoped++
        }
    }

    def override preVisitExprForVariable(expr : ExprFor?; svar : VariablePtr; last : bool) {
        declare_var(svar)
    }

    def override preVisitExprBlockArgument(blk : ExprBlock?; arg : VariablePtr; lastArg : bool) {
        declare_var(arg)
    }

    def declare_var(v : VariablePtr) : void {
        let vid = var_slot(v)
        let id = open_node()
        return if (vid < 0 || id < 0)
        vars[vid].decl_node = id
        decls_ascending_by_node |> push(vid)
        note_use(vid, id)
        add_payload(hash_combine64(TAG_PAYLOAD, type_hash(v._type)))
    }


    def override preVisitTypeDecl(typ : TypeDeclPtr) {
        add_payload(hash_combine64(TAG_PAYLOAD, type_hash(typ)))
    }

    def override preVisitExprMakeStructField(expr : ExprMakeStruct?; index : int; decl : MakeFieldDeclPtr; last : bool) {
        add_payload(hash_combine64(TAG_PAYLOAD, hash("{index}:{decl.name}:{decl.flags}")))
    }

    def override preVisitExprMakeVariantField(expr : ExprMakeVariant?; index : int; decl : MakeFieldDeclPtr; last : bool) {
        add_payload(hash_combine64(TAG_PAYLOAD, hash("{index}:{decl.name}:{decl.flags}")))
    }

    def override preVisitExprNamedCallArgument(expr : ExprNamedCall?; arg : MakeFieldDeclPtr; last : bool) {
        add_payload(hash_combine64(TAG_PAYLOAD, hash("{arg.name}:{arg.flags}")))
    }


    def override preVisitExprReturn(expr : ExprReturn?) {
        n_escape++
    }

    def override preVisitExprYield(expr : ExprYield?) {
        n_escape++
    }

    def override preVisitExprGoto(expr : ExprGoto?) {
        n_escape++
    }

    def override preVisitExprLabel(expr : ExprLabel?) {
        n_escape++
    }

    def stmt_loop_base() : int {
        return empty(stmt_frames) ? 0 : stmt_frames[length(stmt_frames) - 1].loops
    }

    def override preVisitExprBreak(expr : ExprBreak?) {
        if (loop_depth <= stmt_loop_base()) {
            n_freejump++
        }
    }

    def override preVisitExprContinue(expr : ExprContinue?) {
        if (loop_depth <= stmt_loop_base()) {
            n_freejump++
        }
    }

    def override preVisitExprFor(expr : ExprFor?) {
        loop_depth++
    }

    def override visitExprFor(var expr : ExprFor?) : ExpressionPtr {
        loop_depth--
        return null
    }

    def override preVisitExprWhile(expr : ExprWhile?) {
        loop_depth++
    }

    def override visitExprWhile(var expr : ExprWhile?) : ExpressionPtr {
        loop_depth--
        return null
    }

    def override preVisitExprAssume(expr : ExprAssume?) {
        n_bind++
    }

    def override preVisitExprCall(expr : ExprCall?) {
        if (expr.name == "nada" || expr.name == "defer") {
            n_defer++
        }
    }


    def override preVisitExprBlock(blk : ExprBlock?) {
        block_stack |> push(n_blocks)
        n_blocks++
    }

    def override visitExprBlock(var blk : ExprBlock?) : ExpressionPtr {
        block_stack |> pop
        return null
    }

    def override preVisitExprBlockExpression(blk : ExprBlock?; expr : ExpressionPtr) {
        return if (empty(block_stack))
        stmt_frames |> push(StmtFrame(blk_id = block_stack[length(block_stack) - 1],
            escape = n_escape, freejump = n_freejump, defer_calls = n_defer,
            scoped = n_scoped, binds = n_bind, unsafe_ops = n_unsafe_ops, loops = loop_depth))
    }

    def override visitExprBlockExpression(blk : ExprBlock?; var expr : ExpressionPtr) : ExpressionPtr {
        return expr if (empty(stmt_frames))
        let f = stmt_frames[length(stmt_frames) - 1]
        stmt_frames |> pop
        if (f.node >= 0) {
            let clean = (n_escape == f.escape && n_freejump == f.freejump
                && n_defer == f.defer_calls && n_scoped == f.scoped && n_bind == f.binds
                && n_unsafe_ops == f.unsafe_ops)
            while (length(by_block) <= f.blk_id) {
                var fresh : array<int>
                by_block |> emplace(fresh)
            }
            stmt_pos_in_block |> push(length(by_block[f.blk_id]))
            by_block[f.blk_id] |> push(length(stmt_node))
            stmt_block |> push(f.blk_id)
            stmt_node |> push(f.node)
            stmt_ok |> push(clean && expr.at.fileInfo != null && !expr.genFlags.generated)
        }
        return expr
    }


}


def private subtree_equal(var ix : DupeIndexer?; a, b : int;
                          var m_ab : table<int; int>; var m_ba : table<int; int>) : bool {
    let na = ix.nodes[a]
    let nb = ix.nodes[b]
    return false if (na.size != nb.size || na.nchild != nb.nchild
        || !na.known || !nb.known || na.wr != nb.wr || na.payload != nb.payload)
    let ea = ix.node_expr[a]
    let eb = ix.node_expr[b]
    return false if (ea == null || eb == null || ea.__rtti != eb.__rtti
        || payload_key(ea) != payload_key(eb)
        || ix->type_text(ea._type) != ix->type_text(eb._type))
    if (na.vid >= 0 || nb.vid >= 0) {
        return false if (na.vid < 0 || nb.vid < 0)
        let mapped = m_ab?[na.vid] ?? -1
        let back = m_ba?[nb.vid] ?? -1
        if (mapped < 0 && back < 0) {
            m_ab |> insert(na.vid, nb.vid)
            m_ba |> insert(nb.vid, na.vid)
        } elif (mapped != nb.vid || back != na.vid) {
            return false
        }
    }
    var ca = a + 1
    var cb = b + 1
    let enda = a + na.size
    let endb = b + nb.size
    while (ca < enda && cb < endb) {
        return false if (!subtree_equal(ix, ca, cb, m_ab, m_ba))
        ca += ix.nodes[ca].size
        cb += ix.nodes[cb].size
    }
    return ca == enda && cb == endb
}

def private run_equal(var ix : DupeIndexer?; a_blk, a_pos, b_blk, b_pos, len : int) : bool {
    var m_ab : table<int; int>
    var m_ba : table<int; int>
    for (k in range(len)) {
        let sa = ix.stmt_node[ix.by_block[a_blk][a_pos + k]]
        let sb = ix.stmt_node[ix.by_block[b_blk][b_pos + k]]
        return false if (!subtree_equal(ix, sa, sb, m_ab, m_ba))
    }
    return true
}

struct private Region {
    blk : int           //!< the block the run lives in
    pos : int           //!< index of the first statement within that block
    len : int
    lo : int            //!< first node id
    hi : int            //!< one past the last node id (hi - lo is the node count)
}

def private make_region(ix : DupeIndexer?; blk, pos, len : int) : Region {
    let first = ix.stmt_node[ix.by_block[blk][pos]]
    let last = ix.stmt_node[ix.by_block[blk][pos + len - 1]]
    let hi = last + ix.nodes[last].size
    return Region(blk = blk, pos = pos, len = len, lo = first, hi = hi)
}

struct private Gate {
    ok : bool
    params : array<string>
}

def private region_gates(ix : DupeIndexer?; reg : Region; cfg : DupeConfig) : Gate {
    var params : array<string>
    for (vid in ix.decls_ascending_by_node) {
        let d = ix.vars[vid].decl_node
        continue if (d < reg.lo)
        break if (d >= reg.hi)
        return <- Gate() if (ix.vars[vid].last_use >= reg.hi)
    }
    var vid_to_index : table<int; int>
    var vids : array<int>
    var written : array<bool>
    for (n in range(reg.lo, reg.hi)) {
        let vid = ix.nodes[n].vid
        continue if (vid < 0 || (ix.vars[vid].decl_node >= reg.lo
            && ix.vars[vid].decl_node < reg.hi))
        if (!key_exists(vid_to_index, vid)) {
            vid_to_index |> insert(vid, length(vids))
            vids |> push(vid)
            written |> push(false)
        }
        if (ix.nodes[n].wr) {
            written[vid_to_index[vid]] = true
        }
    }
    var nparams = 0
    params |> reserve(length(vids))
    for (vid, wr in vids, written) {
        let v = ix.vars[vid]
        nparams++
        return <- Gate() if (empty(v.type_text) || find(v.type_text, "auto") >= 0
            || empty(v.name) || starts_with(v.name, "__")
            || (wr && !v.is_ref) || nparams > cfg.max_params)
        params |> push("{wr ? "var " : ""}{v.name} : {v.type_show}")
    }
    return <- Gate(ok = true, params <- params)
}

struct private Candidate {
    a_blk : int
    a_pos : int
    b_blk : int
    b_pos : int
    a_lo : int      //!< first preorder id of each occurrence - a source-order key, unlike *_pos
    b_lo : int
    len : int
    nodes : int
    key : uint64
}

def private run_key(ix : DupeIndexer?; blk, pos, len : int) : uint64 {
    var h = 0ul
    for (k in range(len)) {
        h = hash_combine64(h, ix.nodes[ix.stmt_node[ix.by_block[blk][pos + k]]].hash)
    }
    return h
}

def private stmt_hash(ix : DupeIndexer?; si : int) : uint64 {
    return ix.nodes[ix.stmt_node[si]].hash
}

def private extend_run(ix : DupeIndexer?; a_blk, a_pos, b_blk, b_pos : int) : int {
    let a_stop = length(ix.by_block[a_blk])
    let b_stop = length(ix.by_block[b_blk])
    var len = 1
    while (a_pos + len < a_stop && b_pos + len < b_stop) {
        break if (a_blk == b_blk && a_pos + len == b_pos)
        let sa = ix.by_block[a_blk][a_pos + len]
        let sb = ix.by_block[b_blk][b_pos + len]
        break if (!ix.stmt_ok[sa] || !ix.stmt_ok[sb]
            || stmt_hash(ix, sa) != stmt_hash(ix, sb))
        len++
    }
    return len
}

def private maximal_start(ix : DupeIndexer?; a_blk, a_pos, b_blk, b_pos : int) : bool {
    return true if (a_pos == 0 || b_pos == 0)
    let pa = ix.by_block[a_blk][a_pos - 1]
    let pb = ix.by_block[b_blk][b_pos - 1]
    return true if (!ix.stmt_ok[pa] || !ix.stmt_ok[pb])
    return stmt_hash(ix, pa) != stmt_hash(ix, pb)
}

def private site_of(ix : DupeIndexer?; reg : Region) : DupeSite {
    let at = ix.node_expr[reg.lo].at
    var last = int(at.line)
    for (n in range(reg.lo, reg.hi)) {
        let a = ix.node_expr[n].at
        continue if (a.fileInfo != at.fileInfo)
        last = max(last, max(int(a.line), int(a.last_line)))
    }
    return DupeSite(file = at.fileInfo != null ? "{at.fileInfo.name}" : "",
        line = int(at.line), column = int(at.column), last_line = last)
}

def private finding_message(f : DupeFinding) : string {
    let home = f.sites[0].file
    let span = f.sites[0].last_line - f.sites[0].line + 1
    let where_text = build_string() $(var sb) {
        for (i in range(1, length(f.sites))) {
            let s = f.sites[i]
            write(sb, i > 1 ? ", " : "")
            write(sb, s.file == home ? "line {s.line}" : "{s.file}:{s.line}")
        }
    }
    let param_text = build_string() $(var sb) {
        for (i in range(length(f.params))) {
            write(sb, i > 0 ? "; " : "")
            write(sb, f.params[i])
        }
    }
    let sig = empty(param_text) ? "no parameters" : "parameters: {param_text}"
    return "STYLE040: these {f.statements} statement(s) / {span} line(s) ({f.nodes} AST nodes) are duplicated at {where_text}; extract them into a helper ({sig}), or '// nolint:STYLE040' when the repetition is intended"
}


def private statements_by_hash(ix : DupeIndexer?) : array<int> {
    var order : array<int>
    order |> resize(length(ix.stmt_node))
    for (i in range(length(order))) {
        order[i] = i
    }
    order |> sort() $(x, y) {
        let hx = stmt_hash(ix, x)
        let hy = stmt_hash(ix, y)
        return hx != hy ? hx < hy : x < y
    }
    return <- order
}

def private verified_run_len(var ix : DupeIndexer?; cfg : DupeConfig;
                             a_blk, a_pos, b_blk, b_pos : int) : int {
    var len = extend_run(ix, a_blk, a_pos, b_blk, b_pos)
    while (len >= cfg.min_statements) {
        let ra = make_region(ix, a_blk, a_pos, len)
        let rb = make_region(ix, b_blk, b_pos, len)
        if ((ra.hi <= rb.lo || rb.hi <= ra.lo) && ra.hi - ra.lo >= cfg.min_nodes
            && run_equal(ix, a_blk, a_pos, b_blk, b_pos, len)) {
            return len
        }
        len--
    }
    return 0
}

def private group_end(ix : DupeIndexer?; order : array<int>; i : int) : int {
    var j = i + 1
    while (j < length(order) && stmt_hash(ix, order[j]) == stmt_hash(ix, order[i])) {
        j++
    }
    return j
}

def private collect_candidates(var ix : DupeIndexer?; cfg : DupeConfig) : array<Candidate> {
    var cands : array<Candidate>
    return <- cands if (length(ix.stmt_node) < 2)
    let order <- statements_by_hash(ix)
    var i = 0
    while (i < length(order)) {
        let j = group_end(ix, order, i)
        let group = j - i
        if (group >= 2 && group <= cfg.max_bucket) {
            for (p in range(i, j)) {
                continue if (!ix.stmt_ok[order[p]])
                for (q in range(p + 1, j)) {
                    continue if (!ix.stmt_ok[order[q]])
                    let a_blk = ix.stmt_block[order[p]]
                    let b_blk = ix.stmt_block[order[q]]
                    let a_pos = ix.stmt_pos_in_block[order[p]]
                    let b_pos = ix.stmt_pos_in_block[order[q]]
                    continue if (!maximal_start(ix, a_blk, a_pos, b_blk, b_pos))
                    let len = verified_run_len(ix, cfg, a_blk, a_pos, b_blk, b_pos)
                    continue if (len == 0)
                    let reg = make_region(ix, a_blk, a_pos, len)
                    cands |> push(Candidate(a_blk = a_blk, a_pos = a_pos, b_blk = b_blk,
                        b_pos = b_pos, a_lo = reg.lo, b_lo = make_region(ix, b_blk, b_pos, len).lo,
                        len = len, nodes = reg.hi - reg.lo,
                        key = run_key(ix, a_blk, a_pos, len)))
                }
            }
        }
        i = j
    }
    return <- cands
}

def private order_candidates(cands : array<Candidate>) : array<int> {
    var order : array<int>
    order |> resize(length(cands))
    for (i in range(length(cands))) {
        order[i] = i
    }
    order |> sort() $(x, y) {
        let cx = cands[x]
        let cy = cands[y]
        return cx.nodes > cy.nodes if (cx.nodes != cy.nodes)
        return cx.a_lo < cy.a_lo if (cx.a_lo != cy.a_lo)
        return cx.b_lo < cy.b_lo
    }
    return <- order
}

def private is_claimed(ix : DupeIndexer?; claimed : array<bool>; reg : Region) : bool {
    for (k in range(reg.len)) {
        return true if (claimed[ix.by_block[reg.blk][reg.pos + k]])
    }
    return false
}

def private nested_in_accepted(reg : Region; acc_lo, acc_hi : array<int>) : bool {
    for (lo, hi in acc_lo, acc_hi) {
        return true if (reg.lo >= lo && reg.hi <= hi && (reg.lo != lo || reg.hi != hi))
    }
    return false
}

struct private Ledger {
    claimed : array<bool>
    cluster : table<uint64; int>    //!< run key -> index into the findings
    rep_blk : array<int>            //!< per finding: the run to re-verify later occurrences against
    rep_pos : array<int>
    rep_len : array<int>
    acc_lo : array<int>             //!< reported node intervals
    acc_hi : array<int>
}

def private record(ix : DupeIndexer?; var led : Ledger; reg : Region) : void {
    for (k in range(reg.len)) {
        led.claimed[ix.by_block[reg.blk][reg.pos + k]] = true
    }
    led.acc_lo |> push(reg.lo)
    led.acc_hi |> push(reg.hi)
}

def private already_listed(f : DupeFinding; s : DupeSite) : bool {
    for (o in f.sites) {
        return true if (o.line == s.line && o.column == s.column && o.file == s.file)
    }
    return false
}

def private merge_occurrence(var ix : DupeIndexer?; cfg : DupeConfig; var led : Ledger;
                             var out : array<DupeFinding>; fi : int; fresh : Region) : void {
    return if (fresh.len != led.rep_len[fi]
               || !run_equal(ix, led.rep_blk[fi], led.rep_pos[fi], fresh.blk, fresh.pos, fresh.len)
               || !region_gates(ix, fresh, cfg).ok)
    let s = site_of(ix, fresh)
    if (!already_listed(out[fi], s)) {
        out[fi].sites |> push(s)
        out[fi].message = finding_message(out[fi])
    }
    record(ix, led, fresh)
}

def private open_finding(var ix : DupeIndexer?; cfg : DupeConfig; var led : Ledger;
                         var out : array<DupeFinding>; key : uint64; ra, rb : Region) : void {
    var ga <- region_gates(ix, ra, cfg)
    let gb <- region_gates(ix, rb, cfg)
    return if (!ga.ok || !gb.ok || length(ga.params) != length(gb.params))
    var f = DupeFinding(anchor = ix.node_expr[ra.lo], statements = ra.len, nodes = ra.hi - ra.lo,
        params <- ga.params, sites <- [site_of(ix, ra), site_of(ix, rb)])
    f.message = finding_message(f)
    led.cluster |> insert(key, length(out))
    led.rep_blk |> push(ra.blk)
    led.rep_pos |> push(ra.pos)
    led.rep_len |> push(ra.len)
    out |> emplace(f)
    record(ix, led, ra)
    record(ix, led, rb)
}

def private detect(var ix : DupeIndexer?; cfg : DupeConfig) : array<DupeFinding> {
    var out : array<DupeFinding>
    let cands <- collect_candidates(ix, cfg)
    let order <- order_candidates(cands)
    var led : Ledger
    led.claimed |> resize(length(ix.stmt_node))
    for (oi in order) {
        break if (length(out) >= cfg.max_findings)
        let c = cands[oi]
        let ra = make_region(ix, c.a_blk, c.a_pos, c.len)
        let rb = make_region(ix, c.b_blk, c.b_pos, c.len)
        continue if (nested_in_accepted(ra, led.acc_lo, led.acc_hi)
                     || nested_in_accepted(rb, led.acc_lo, led.acc_hi))
        let ca = is_claimed(ix, led.claimed, ra)
        let cb = is_claimed(ix, led.claimed, rb)
        continue if (ca && cb)
        let fi = led.cluster?[c.key] ?? -1
        if (fi >= 0) {
            merge_occurrence(ix, cfg, led, out, fi, ca ? rb : ra)
        } elif (!ca && !cb) {
            open_finding(ix, cfg, led, out, c.key, ra, rb)
        }
    }
    return <- out
}

//! Finds every duplicated, extractable statement region in the program's own module.
//! Findings come back in a deterministic order, widest first.
def public find_extractable_dupes(prog : ProgramPtr; cfg : DupeConfig) : array<DupeFinding> {
    var out : array<DupeFinding>
    let mod = prog.getThisModule
    return <- out if (mod == null)
    var ix = new DupeIndexer()
    make_visitor(*ix) $(adapter) {
        for_each_function(mod, "") $(var fn) {
            return if (fn == null || fn.body == null || fn._module != mod || fn.at.fileInfo == null
                || fn.flags.generated || fn.fromGeneric != null || fn.isGeneric
                || fn.moreFlags.isTemplate)
            visit(fn, adapter)
        }
    }
    out <- detect(ix, cfg)
    unsafe { delete ix; }
    return <- out
}

//! Entry point for the lint rule: reads the module's `options _dupe_min_nodes` /
//! `_dupe_min_statements` overrides, falling back to the defaults.
def public find_extractable_dupes(prog : ProgramPtr) : array<DupeFinding> {
    var cfg = DupeConfig()
    let nodes = prog._options |> find_arg("_dupe_min_nodes") ?as tInt ?? 0
    let stmts = prog._options |> find_arg("_dupe_min_statements") ?as tInt ?? 0
    if (nodes > 0) {
        cfg.min_nodes = nodes
    }
    if (stmts > 0) {
        cfg.min_statements = stmts
    }
    return <- find_extractable_dupes(prog, cfg)
}
