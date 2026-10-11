options gen2
options indenting = 4
options strict_smart_pointers

module apply shared private

//! Compile-time field reflection for structs, tuples, and variants.
//!
//! ``apply`` runs a per-field block once for each field of the value, binding the field name (a
//! compile-time constant) and a reference to the field value (and optionally its RTTI annotations).
//! When the block has no function-escaping ``return`` it inlines the body directly into the caller's
//! scope — ``let/var <field> & = unsafe(<access>)`` — with no helper function and no per-field invoke,
//! so a hot field walk (serialization, UI binding, reflection) pays no dispatch overhead. A block that
//! uses ``return`` to skip a field falls back transparently to a generated per-field helper, where the
//! ``return`` keeps its original block-local "skip this field" meaning — so no consumer changes. Useful
//! without runtime RTTI. ``apply_imm`` is a struct-only, slightly faster sibling that always inlines
//! (and therefore cannot fall back, so its block may not use ``return``).

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost
require math

// Split a field count into argument-count-limited subranges — a struct/tuple/variant with more fields
// than DAS_MAX_FUNCTION_ARGUMENTS gets several generated helpers, one per subrange (invoke fallback only).
[macro_function]
def for_each_subrange(total : int; blk : block<(r : range) : void>) {
    let MAX_FIELD_BLOCKS_PER_HELPER = DAS_MAX_FUNCTION_ARGUMENTS - 2
    var i = 0
    while (i < total) {
        let fromI = i
        let toI = min(i + MAX_FIELD_BLOCKS_PER_HELPER, total)
        blk |> invoke(range(fromI, toI))
        i = toI
    }
}

[macro_function]
def private field_rtti_annotations(fld : FieldDeclaration) : ExpressionPtr {
    var rttiAnnValues : array<tuple<name : string; data : RttiValue>>
    rttiAnnValues |> reserve(length(fld.annotation))
    for (ann in fld.annotation) {
        var value : tuple<name : string; data : RttiValue>
        value.name = string(ann.name)
        if (ann.basicType == Type.tString) {
            value.data = RttiValue(tString = string(ann.sValue))
        } elif (ann.basicType == Type.tInt) {
            value.data = RttiValue(tInt = ann.iValue)
        } elif (ann.basicType == Type.tFloat) {
            value.data = RttiValue(tFloat = ann.fValue)
        } elif (ann.basicType == Type.tBool) {
            value.data = RttiValue(tBool = ann.bValue)
        }
        rttiAnnValues |> emplace(value)
    }
    return qmacro($v(rttiAnnValues))
}

[macro_function]
def private inline_apply_field(nameStr : string; var access : ExpressionPtr; fieldType : TypeDeclPtr;
                               isConst : bool; hasExtraArg : bool; var annExpr : ExpressionPtr;
                               blk : ExpressionPtr) : ExpressionPtr {
    var res = clone_expression(blk)
    var imb = res as ExprMakeBlock
    var rblk = imb._block as ExprBlock
    var vtype = clone_type(fieldType)
    vtype.flags.ref = true
    if (isConst) {
        vtype.flags.constant = true
    }
    access.genFlags.alwaysSafe = true
    var vlet = new ExprLet(at = blk.at, atInit = blk.at)
    vlet.genFlags.alwaysSafe = true
    vlet.variables |> emplace_new() <| new Variable(at = blk.at,
        name := rblk.arguments[1].name,
        _type <- vtype,
        init <- access,
        flags = VariableFlags.can_shadow | VariableFlags.generated | VariableFlags.capture_as_ref)
    if (hasExtraArg) {
        var aAnn = new ExprAssume(at = blk.at, alias := rblk.arguments[2].name, subexpr <- annExpr)
        rblk.list |> emplace(aAnn, 0)
    }
    var aName = new ExprAssume(at = blk.at, alias := rblk.arguments[0].name,
        subexpr <- new ExprConstString(at = blk.at, value := nameStr))
    rblk.list |> emplace(aName, 0)
    rblk.list |> emplace(vlet, 0)
    while (!empty(rblk.arguments)) {
        rblk.arguments |> erase(0)
    }
    return imb._block
}

[macro_function]
def generateApplyVisitStruct(stype : TypeDeclPtr; frange : range; fnname : string; at : LineInfo; var names : array<string>; hasExtraArg : bool) {
    assert(stype.baseType == Type.tStructure)
    let nfields = frange.y - frange.x
    var selfT = clone_type(stype)
    selfT.flags |= TypeDeclFlags.isExplicit | TypeDeclFlags.explicitConst
    var blkList : array<ExpressionPtr>
    var func_args : array<VariablePtr>
    names |> reserve(nfields)
    blkList |> reserve(nfields)
    for (fldi in frange) {
        {
            let fld & = unsafe(stype.structType.fields[fldi])
            names |> push(string(fld.name))
            if (hasExtraArg) {
                var annExpr = field_rtti_annotations(fld)
                blkList |> push(qmacro(invoke($i("__arg_{fld.name}"), _`_self.$f(fld.name), $e(annExpr))))
            } else {
                blkList |> push(qmacro(invoke($i("__arg_{fld.name}"), _`_self.$f(fld.name))))
            }
            var argT = new TypeDecl(baseType = Type.tBlock, at = at)
            var fldT = clone_type(fld._type)
            if (stype.flags.constant) {
                fldT.flags.constant = true
            }
            if (!fldT.isRefType) {
                fldT.flags.ref = true
            }
            emplace(argT.argTypes, fldT)
            if (hasExtraArg) {
                argT.argTypes |> emplace_new <| qmacro_type(type<array<tuple<name : string; data : RttiValue>>>)
            }
            argT.firstType = new TypeDecl(baseType = Type.tVoid, at = at)
            argT.flags = TypeDeclFlags.constant
            func_args |> emplace_new <| new Variable(name := "__arg_{fld.name}", _type <- argT)
        }
    }
    var fn = qmacro_function(fnname) $ [unused_argument(_`_self)] (_`_self : $t(selfT); $a(func_args)) {
        $b(blkList)
    }
    fn.flags.privateFunction = true
    compiling_module() |> add_function(fn)
}

[macro_function]
def generateApplyVisitStruct(stype : TypeDeclPtr; fnname : string; at : LineInfo; hasExtraArg : bool) : array<string> {
    var names : array<string>
    for_each_subrange(length(stype.structType.fields)) $(frange) {
        generateApplyVisitStruct(stype, frange, "{fnname}`{frange.x}`{frange.y}", at, names, hasExtraArg)
    }
    return <- names
}

[macro_function]
def generateApplyVisitTuple(stype : TypeDeclPtr; frange : range; fnname : string; at : LineInfo; var names : array<string>) {
    assert(stype.baseType == Type.tTuple)
    let nfields = frange.y - frange.x
    var selfT = clone_type(stype)
    selfT.flags |= TypeDeclFlags.isExplicit | TypeDeclFlags.explicitConst
    var blkList : array<ExpressionPtr>
    var func_args : array<VariablePtr>
    names |> reserve(nfields)
    blkList |> reserve(nfields)
    for (fldi in frange) {
        {
            assume flda = stype.argTypes[fldi]
            let fldname = length(stype.argNames) == length(stype.argTypes) ? string(stype.argNames[fldi]) : "_{fldi}"
            names |> push(fldname)
            blkList |> push(qmacro(invoke($i("__arg_{fldname}"), _`_self.$f(fldname))))
            var argT = new TypeDecl(baseType = Type.tBlock, at = at)
            var fldT = clone_type(flda)
            if (stype.flags.constant) {
                fldT.flags.constant = true
            }
            if (!fldT.isRefType) {
                fldT.flags.ref = true
            }
            emplace(argT.argTypes, fldT)
            argT.firstType = new TypeDecl(baseType = Type.tVoid, at = at)
            argT.flags = TypeDeclFlags.constant
            func_args |> emplace_new <| new Variable(name := "__arg_{fldname}", _type <- argT)
        }
    }
    var fn = qmacro_function(fnname) $ [unused_argument(_`_self)] (_`_self : $t(selfT); $a(func_args)) {
        $b(blkList)
    }
    fn.flags.privateFunction = true
    compiling_module() |> add_function(fn)
}

[macro_function]
def generateApplyVisitTuple(stype : TypeDeclPtr; fnname : string; at : LineInfo) : array<string> {
    var names : array<string>
    for_each_subrange(length(stype.argTypes)) $(frange) {
        generateApplyVisitTuple(stype, frange, "{fnname}`{frange.x}`{frange.y}", at, names)
    }
    return <- names
}

[macro_function]
def generateApplyVisitVariant(stype : TypeDeclPtr; frange : range; fnname : string; at : LineInfo; var names : array<string>) {
    assert(stype.baseType == Type.tVariant)
    let nfields = frange.y - frange.x
    var selfT = clone_type(stype)
    selfT.flags |= TypeDeclFlags.isExplicit | TypeDeclFlags.explicitConst
    var blkList : array<ExpressionPtr>
    var func_args : array<VariablePtr>
    names |> reserve(nfields)
    blkList |> reserve(nfields)
    for (fldi in frange) {
        {
            assume flda = stype.argTypes[fldi]
            let fldname = length(stype.argNames) == length(stype.argTypes) ? string(stype.argNames[fldi]) : "_{fldi}"
            names |> push(fldname)
            var vexpr = qmacro_block() {
                if (variant_index(_`_self) == $v(fldi)) {
                    invoke($i("__arg_{fldname}"), _`_self as $f(fldname))
                    return
                }
            }
            blkList |> emplace(vexpr)
            var argT = new TypeDecl(baseType = Type.tBlock, at = at)
            var fldT = clone_type(flda)
            if (stype.flags.constant) {
                fldT.flags.constant = true
            }
            if (!fldT.isRefType) {
                fldT.flags.ref = true
            }
            emplace(argT.argTypes, fldT)
            argT.firstType = new TypeDecl(baseType = Type.tVoid, at = at)
            argT.flags = TypeDeclFlags.constant
            func_args |> emplace_new <| new Variable(name := "__arg_{fldname}", _type <- argT)
        }
    }
    var fn = qmacro_function(fnname) $ [unused_argument(_`_self)] (_`_self : $t(selfT); $a(func_args)) {
        $b(blkList)
    }
    fn.flags.privateFunction = true
    compiling_module() |> add_function(fn)
}

[macro_function]
def generateApplyVisitVariant(stype : TypeDeclPtr; fnname : string; at : LineInfo) : array<string> {
    var names : array<string>
    for_each_subrange(length(stype.argTypes)) $(frange) {
        generateApplyVisitVariant(stype, frange, "{fnname}`{frange.x}`{frange.y}", at, names)
    }
    return <- names
}

[macro_function]
def clone_block_with_name(name : string; blk : ExpressionPtr; hasExtraArg : bool) {
    macro_verify(blk is ExprMakeBlock, this_program(), blk.at, "expecting make block, i.e. $(..)")
    var res = clone_expression(blk)
    var imb = res as ExprMakeBlock
    var rblk = imb._block as ExprBlock
    macro_verify(rblk != null, this_program(), blk.at, "expecting block")
    if (hasExtraArg) {
        macro_verify(length(rblk.arguments) == 3, this_program(), blk.at, "expecting block with 3 arguments")
    } else {
        macro_verify(length(rblk.arguments) == 2, this_program(), blk.at, "expecting block with 2 arguments")
    }
    var eassume = new ExprAssume(at = blk.at,
        alias := rblk.arguments[0].name,
        subexpr <- new ExprConstString(at = blk.at, value := name)
    )
    rblk.arguments |> erase(0)
    rblk.list |> emplace(eassume, 0)
    return <- res
}

[macro]
class FindEscapingReturn : AstVisitor {
    found : bool
    def override preVisitExprReturn(expr : ExprReturn?) : void {
        found = true
    }
    def override canVisitMakeBlockBody(expr : ExprMakeBlock?) : bool {
        return false
    }
}

[macro_function]
def private apply_block_inline_safe(blk : ExpressionPtr) : bool {
    if (!(blk is ExprMakeBlock)) {
        return true
    }
    var astVisitor = new FindEscapingReturn()
    make_visitor(*astVisitor) $(adapter) {
        visit((blk as ExprMakeBlock)._block, adapter)
    }
    let escaping = astVisitor.found
    unsafe {
        delete astVisitor
    }
    return !escaping
}

[call_macro(name = "apply")]
class ApplyMacro : AstCallMacro {
    //! Visits every field of a struct, tuple, or variant with a per-field block. The block is ``$(name, field)`` — ``name`` is the field-name string (a compile-time constant, so ``static_if (name == "...")`` folds) and ``field`` is a reference to the field value (read or mutate; writes propagate to the source). A struct may use a third ``annotations`` param receiving ``array<tuple<name:string; data:RttiValue>>`` per field. A variant runs only its active arm.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 2, prog, expr.at, "expecting apply(value, block)")
        if (expr.arguments[0]._type == null) {
            return null
        }
        var argT = clone_type(expr.arguments[0]._type)
        macro_verify(argT.baseType == Type.tStructure || argT.baseType == Type.tTuple || argT.baseType == Type.tVariant,
            prog, expr.at, "can only apply to {describe(expr.arguments[0]._type)}")
        macro_verify(expr.arguments[1] is ExprMakeBlock, prog, expr.at, "expecting make block, i.e. $(..)")
        var hasExtraArg = false
        let emb = expr.arguments[1] as ExprMakeBlock
        if (emb._block is ExprBlock) {
            let eb = emb._block as ExprBlock
            macro_verify(length(eb.arguments) == 2 || length(eb.arguments) == 3, prog, expr.at,
                "expecting block with 2 or 3 arguments, i.e. $(name, field) or $(name, field, annotations)")
            hasExtraArg = length(eb.arguments) == 3
        }
        macro_verify(!hasExtraArg || argT.baseType == Type.tStructure, prog, expr.at,
            "the annotations block param is supported for structs only")
        if (apply_block_inline_safe(expr.arguments[1])) {
            return inline_apply(expr, argT, hasExtraArg)
        } else {
            return invoke_apply(expr, argT, hasExtraArg)
        }
    }
}

[macro_function]
def private inline_apply(var expr : ExprCallMacro?; argT : TypeDeclPtr; hasExtraArg : bool) : ExpressionPtr {
    let selfIsConst = argT.flags.constant
    var selfPeeled = expr.arguments[0]
    while (selfPeeled != null && selfPeeled is ExprRef2Value) {
        selfPeeled = (selfPeeled as ExprRef2Value).subexpr
    }
    let selfIsRef = selfPeeled != null && (selfPeeled is ExprVar || selfPeeled is ExprField
        || selfPeeled is ExprSafeField || selfPeeled is ExprAt || selfPeeled is ExprSafeAt || selfPeeled is ExprSwizzle)
    let selfName = "__apply_self`{expr.at.line}`{expr.at.column}"
    var selfType = clone_type(expr.arguments[0]._type)
    selfType.flags.constant = selfIsConst
    var selfAccess = clone_expression(selfPeeled)
    var selfLet = new ExprLet(at = expr.at, atInit = expr.at)
    if (selfIsRef) {
        selfType.flags.ref = true
        selfAccess.genFlags.alwaysSafe = true
        selfLet.genFlags.alwaysSafe = true
    } else {
        selfType.flags.ref = false
    }
    selfLet.variables |> emplace_new() <| new Variable(at = expr.at,
        name := selfName,
        _type <- selfType,
        init <- selfAccess,
        flags = VariableFlags.can_shadow | VariableFlags.generated | VariableFlags.capture_as_ref)
    var outer = new ExprBlock(at = expr.at)
    outer.list |> emplace(selfLet)
    if (argT.baseType == Type.tStructure) {
        let st = expr.arguments[0]._type.structType
        for (fld in st.fields) {
            let fname = string(fld.name)
            var access = qmacro($i(selfName).$f(fname))
            let isConst = selfIsConst || fld._type.flags.constant
            var annExpr : ExpressionPtr
            if (hasExtraArg) {
                annExpr = field_rtti_annotations(fld)
            }
            var scope = inline_apply_field(fname, access, fld._type, isConst, hasExtraArg, annExpr, expr.arguments[1])
            outer.list |> emplace(scope)
        }
    } elif (argT.baseType == Type.tTuple) {
        let tt = expr.arguments[0]._type
        for (fldi in range(length(tt.argTypes))) {
            let fname = length(tt.argNames) == length(tt.argTypes) ? string(tt.argNames[fldi]) : "_{fldi}"
            var access = qmacro($i(selfName).$f(fname))
            let isConst = selfIsConst || tt.argTypes[fldi].flags.constant
            var noAnn : ExpressionPtr
            var scope = inline_apply_field(fname, access, tt.argTypes[fldi], isConst, false, noAnn, expr.arguments[1])
            outer.list |> emplace(scope)
        }
    } else {
        let vt = expr.arguments[0]._type
        let nfields = length(vt.argTypes)
        var chain : ExpressionPtr
        for (ri in range(nfields)) {
            let fldi = nfields - 1 - ri
            let fname = length(vt.argNames) == length(vt.argTypes) ? string(vt.argNames[fldi]) : "_{fldi}"
            var access = qmacro($i(selfName) as $f(fname))
            let isConst = selfIsConst || vt.argTypes[fldi].flags.constant
            var noAnn : ExpressionPtr
            var armScope = inline_apply_field(fname, access, vt.argTypes[fldi], isConst, false, noAnn, expr.arguments[1])
            var cond = qmacro(variant_index($i(selfName)) == $v(fldi))
            chain = new ExprIfThenElse(at = expr.at, cond <- cond, if_true <- armScope, if_false <- chain)
        }
        if (chain != null) {
            outer.list |> emplace(chain)
        }
    }
    return outer
}

[macro_function]
def private invoke_apply(var expr : ExprCallMacro?; argT : TypeDeclPtr; hasExtraArg : bool) : ExpressionPtr {
    var callName : string
    var nfields : int
    var names : array<string>
    if (argT.baseType == Type.tStructure) {
        callName = "apply`struct`{argT.structType.name}`{expr.at.line}`{expr.at.column}"
        nfields = argT.structType.fields |> length
        names <- generateApplyVisitStruct(expr.arguments[0]._type, callName, expr.at, hasExtraArg)  // nolint:PERF030 - target is the fresh empty decl above; the three fills are mutually exclusive branches
    } elif (argT.baseType == Type.tTuple) {
        callName = "apply`tuple`{expr.arguments[0]._type.get_mnh}`{expr.at.line}`{expr.at.column}"
        nfields = length(argT.argTypes)
        names <- generateApplyVisitTuple(expr.arguments[0]._type, callName, expr.at)  // nolint:PERF030 - target is the fresh empty decl above; the three fills are mutually exclusive branches
    } else {
        callName = "apply`variant`{expr.arguments[0]._type.get_mnh}`{expr.at.line}`{expr.at.column}"
        nfields = length(argT.argTypes)
        names <- generateApplyVisitVariant(expr.arguments[0]._type, callName, expr.at)  // nolint:PERF030 - target is the fresh empty decl above; the three fills are mutually exclusive branches
    }
    var calls : array<ExpressionPtr>
    for_each_subrange(nfields) $(frange) {
        var call = new ExprCall(name := "_::{callName}`{frange.x}`{frange.y}", at = expr.at)
        emplace_new(call.arguments, clone_expression(expr.arguments[0]))
        for (idx in frange) {
            emplace_new(call.arguments, clone_block_with_name(names[idx], expr.arguments[1], hasExtraArg))
        }
        calls |> emplace(call)
    }
    if (length(calls) == 1) {
        return <- calls[0]
    }
    var callblock = new ExprBlock(at = expr.at)
    unsafe {
        callblock.list := calls
    }
    return <- callblock
}

[macro_function]
def private inline_apply_imm_field(fldname : string; var fieldAccess : ExpressionPtr; hasExtraArg : bool;
                                   var annExpr : ExpressionPtr; blk : ExpressionPtr) : ExpressionPtr {
    var res = clone_expression(blk)
    var imb = res as ExprMakeBlock
    var rblk = imb._block as ExprBlock
    if (hasExtraArg) {
        var aAnn = new ExprAssume(at = blk.at, alias := rblk.arguments[2].name, subexpr <- annExpr)
        rblk.list |> emplace(aAnn, 0)
    }
    var aField = new ExprAssume(at = blk.at, alias := rblk.arguments[1].name, subexpr <- fieldAccess)
    rblk.list |> emplace(aField, 0)
    var aName = new ExprAssume(at = blk.at, alias := rblk.arguments[0].name,
        subexpr <- new ExprConstString(at = blk.at, value := fldname))
    rblk.list |> emplace(aName, 0)
    while (!empty(rblk.arguments)) {
        rblk.arguments |> erase(0)
    }
    return imb._block
}

[call_macro(name = "apply_imm")]
class ApplyImmMacro : AstCallMacro {
    //! Struct-only, faster sibling of ``apply`` for a hot field walk (e.g. a serialization materializer).
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 2, prog, expr.at, "expecting apply_imm(value, block)")
        if (expr.arguments[0]._type == null) {
            return null
        }
        var argT = clone_type(expr.arguments[0]._type)
        macro_verify(argT.baseType == Type.tStructure, prog, expr.at, "apply_imm supports struct only")
        macro_verify(expr.arguments[1] is ExprMakeBlock, prog, expr.at, "expecting make block, i.e. $(..)")
        macro_verify(apply_block_inline_safe(expr.arguments[1]), prog, expr.at,
            "apply_imm can't inline a block that uses 'return' to skip a field — use apply instead")
        let emb = expr.arguments[1] as ExprMakeBlock
        var hasExtraArg = false
        if (emb._block is ExprBlock) {
            let eb = emb._block as ExprBlock
            hasExtraArg = length(eb.arguments) == 3
        }
        let st = expr.arguments[0]._type.structType
        var scopes : array<ExpressionPtr>
        scopes |> reserve(length(st.fields))
        for (fld in st.fields) {
            let fname = string(fld.name)
            var fieldAccess = qmacro($e(expr.arguments[0]).$f(fname))
            var annExpr : ExpressionPtr
            if (hasExtraArg) {
                annExpr = field_rtti_annotations(fld)
            }
            scopes |> push(inline_apply_imm_field(fname, fieldAccess, hasExtraArg, annExpr, expr.arguments[1]))
        }
        var outer = new ExprBlock(at = expr.at)
        unsafe {
            outer.list := scopes
        }
        return outer
    }
}
