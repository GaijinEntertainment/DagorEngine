options gen2
options indenting = 4
options no_aot
options multiple_contexts
options strict_smart_pointers = true

module decs_boost shared private

//! DECS query macro and compile-time helpers.
//!
//! Implements the ``decs_query`` block macro that generates
//! type-safe, zero-overhead iteration over ECS archetypes.
//! Also provides ``[decs_event]`` and ``[decs_template]``.
//! Requires the ``decs`` module.

require daslib/decs public
require daslib/ast_boost
require daslib/templates_boost
require daslib/strings_boost
require daslib/ast_block_to_loop
require daslib/defer
require daslib/decs_state
require daslib/macro_boost

/*
from:
    query ( ) $ ( pos:float3&; vel:float3; col:uint=0x12345678 ) {
        pos += vel
    }
to:
    for_each_archetype ( ERQ_HASH, @@ => [[EcsRequest req <- [[string "pos"; "vel"]] ]] ) $ ( arch ) {
        for ( pos, vel in get(arch,"pos",type<float3>), get_ro(arch,"vel",type<float3>), get_default_ro(arch,"col",0x12345678) ) {
            tag
        }
    }
*/

[block_macro(name="REQUIRE")]
class DecsReq : AstBlockAnnotation {
    //! This annotation provides list of required components for entity.
}

[block_macro(name="REQUIRE_NOT")]
class DecsReqN : AstBlockAnnotation {
    //! This annotation provides list of components, which are required to not be part of the entity.
}

[structure_macro(name="decs_template")]
class DecsTemplate : AstStructureAnnotation {
    //! This macro creates a template for the given structure.
    def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        if (empty(st.fields)) {
            errors := "expecting at least one field in the desc_template {st.name}"
            return false
        }
        let ppref = decs_prefix(args)
        let prefix = (ppref is yes) ? (ppref as yes) : "{st.name}_"
        var blkApply : array<ExpressionPtr>
        var blkErase : array<ExpressionPtr>
        for (fld in st.fields) {
            blkApply |> push(qmacro(decs::set(cmp, $v("{prefix}{fld.name}"), src.$f(fld.name))))
            blkErase |> push(qmacro(decs::remove(cmp, $v("{prefix}{fld.name}"))))
        }
        var fnApply = qmacro_function("apply_decs_template") $(var cmp : ComponentMap; var src : $t(st)) : void {
            $b(blkApply)
        }
        if (!(compiling_module() |> add_function(fnApply))) {
            panic("failed to add apply_decs_template, can't add function {fnApply.name}")
        }
        var fnErase = qmacro_function("remove_decs_template`{st.name}") $(var cmp : ComponentMap; var src : $t(st)) : void {
            $b(blkErase)
        }
        if (!(compiling_module() |> add_function(fnErase))) {
            panic("failed to add remove_decs_template, can't add function {fnErase.name}")
        }
        // Generate apply_decs_template_arch — writes into archetype storage with cached indices.
        var blkArchSetup : array<ExpressionPtr>
        var blkArchWrite : array<ExpressionPtr>
        // eid at cache[0], fields at cache[1..N]
        blkArchSetup |> push(qmacro(cache |> push(decs::find_component_index(arch, "eid"))))
        blkArchWrite |> push(qmacro(decs::set_direct_at(arch, cache[0], eidx, eid)))
        for (fld_idx, fld in iter_range(st.fields), st.fields) {
            let ci = fld_idx + 1
            blkArchSetup |> push(qmacro(cache |> push(decs::find_component_index(arch, $v("{prefix}{fld.name}")))))
            blkArchWrite |> push(qmacro(decs::set_direct_at(arch, cache[$v(ci)], eidx, src.$f(fld.name))))
        }
        var fnArch = qmacro_function("apply_decs_template_arch") $(var arch : Archetype; eidx : int; eid : EntityId; src : $t(st); var cache : array<int>) : void {
            pass
        }
        var blkA = unsafe(reinterpret<ExprBlock?>(fnArch.body))
        blkA.list |> clear()
        blkA.list |> emplace_new <| qmacro_block() {
            if (length(cache) == 0) {
                $b(blkArchSetup)
            }
            $b(blkArchWrite)
        }
        if (!(compiling_module() |> add_function(fnArch))) {
            panic("failed to add apply_decs_template_arch, can't add function {fnArch.name}")
        }
        // Generate create_entities`T — bulk creation with cached index resolution
        var fnBulk = qmacro_function("create_entities`{st.name}") $(count : int; user_blk : block<(eid : decs::EntityId; i : int; var entity : $t(st)) : void>) : void {
            pass
        }
        var blk = unsafe(reinterpret<ExprBlock?>(fnBulk.body))
        blk.list |> clear()
        blk.list |> emplace_new <| qmacro_block() {
            var defval := default<$t(st)>
            var cmp : decs::ComponentMap
            apply_decs_template(cmp, defval)
            var cache : array<int>
            decs::create_entities_from_cmp(count, cmp) $(var arch : decs::Archetype; eidx : int; eid : decs::EntityId; i : int) {
                var entity := default<$t(st)>
                invoke(user_blk, eid, i, entity)
                apply_decs_template_arch(arch, eidx, eid, entity, cache)
            }
            delete cmp
        }
        if (!(compiling_module() |> add_function(fnBulk))) {
            panic("failed to add create_entities`{st.name}, can't add function {fnBulk.name}")
        }
        return true
    }
}

variant ItCheck {
    //! DECS prefix check.
    yes : string
    no  : bool
}

[macro_function]
def private decs_prefix(arg : AnnotationArgumentList) {
    let p = arg |> find_arg("prefix")
    if (p is tString) {
        return ItCheck(yes = p as tString)
    } elif (p is tBool) {
        return ItCheck(yes = "")
    } else {
        return ItCheck(no = true)
    }
}

[macro_function]
def private is_decs_template(v : VariablePtr) {
    if (v._type.baseType != Type.tStructure) return ItCheck(no = true)
    var p = v.annotation |> decs_prefix
    if (p is no) {
        for (ann in v._type.structType.annotations) {
            if (ann.annotation.name == "decs_template") {
                p = ann.arguments |> decs_prefix
                return p is yes ? p : ItCheck(yes = "{v._type.structType.name}_")
            }
        }
    }
    return p
}

[macro_function]
def build_req_from_args(qblk : ExprBlock?) : EcsRequest {
    var req : EcsRequest
    for (a in qblk.arguments) {
        if (a.init == null) {
            let detp = a |> is_decs_template
            if (detp is yes) {
                for (f in a._type.structType.fields) {
                    req.req |> push("{detp as yes}{f.name}")
                }
            } else {
                if (!(find_arg(a.annotation, "optional") ?as tBool ?? false)) {
                    req.req |> push(string(a.name))
                }
            }
        }
    }
    for (aa in qblk.annotations) {
        let isreq = aa.annotation.name == "REQUIRE"
        let isreqn = aa.annotation.name == "REQUIRE_NOT"
        if (isreq || isreqn) {
            for (ab in aa.arguments) {
                if (ab.basicType == Type.tBool && ab.bValue) {
                    (isreq ? req.req : req.reqn) |> push(string(ab.name))
                }
            }
        }
    }
    compile_request(req)
    return <- req
}

[macro_function]
def getter_name(a; const_parent : bool; can_be_optional : bool) {
    var getter = "get_ro"
    static_if (typeinfo stripped_typename(a) == "ast_core::FieldDeclaration") {// part of the desc_template
        if (a.init != null && can_be_optional) {
            getter = "get_default_ro"
        } elif (!const_parent) {
            getter = "get"
        }
    } else {
        if (find_arg(a.annotation, "optional") ?as tBool ?? false) {
            if (a.init != null) {
                macro_error(compiling_program(), a.at, "optional argument {a.name} can't have a default value")
                return ""
            } elif (!a._type.isPointer) {
                macro_error(compiling_program(), a.at, "optional argument {a.name} must be a pointer")
                return ""
            }
            getter = "get_optional"
        } elif (a.init != null && can_be_optional) {
            if (a._type.flags.ref || !a._type.flags.constant) {
                macro_error(compiling_program(), a.at, "argument {a.name} has default value, it can't be & or var")
                return ""
            }
            getter = "get_default_ro"
        } elif (a._type.isRef && !a._type.isRefType) {
            if (a._type.flags.constant) {
                macro_error(compiling_program(), a.at, "argument {a.name} is both & and constant")
                return ""
            }
            getter = "get"
        } elif (a._type.isRefType && !a._type.flags.constant) {
            getter = "get"
        }
    }
    return getter
}

[macro_function]
def private append_iterator(arch_name : string; var qloop : ExprFor?; a; prefix, suffix : string; const_parent : bool = false; can_be_optional : bool = true) {
    let qli = length(qloop.iterators)
    qloop.iterators |> resize(qli + 1)
    qloop.iterators[qli] := "{prefix}{a.name}{suffix}"
    qloop.iteratorsAka |> resize(qli + 1)
    static_if (typeinfo has_field<_aka>(a)) {
        qloop.iteratorsAka[qli] := a._aka
    } else {
        qloop.iteratorsAka[qli] := ""
    }
    qloop.iteratorsAt |> push(a.at)
    qloop.iteratorsTags |> resize(qli + 1)
    let getter = getter_name(a, const_parent, can_be_optional)
    if (empty(getter)) return false
    if (getter == "get_default_ro") {
        qloop.sources |> emplace_new <| qmacro($c(getter)($i(arch_name), $v("{prefix}{a.name}"), $e(a.init)))
    } else {
        var ftype = clone_type(a._type)
        ftype.flags.constant = false
        ftype.flags.ref = false
        var it = qmacro($c(getter)($i(arch_name), $v("{prefix}{a.name}"), unsafe(type<$t(ftype)>)))
        force_generated(it, true)
        qloop.sources |> emplace <| it
    }
    return true
}

[macro_function]
def private append_index_lookup(arch_name : string; var qblock : ExprBlock?; a; prefix, suffix : string; const_parent : bool = false; can_be_optional : bool = true) {
    let getter = getter_name(a, const_parent, can_be_optional)
    if (empty(getter)) return false
    var iget : ExpressionPtr
    if (getter == "get_default_ro") {
        iget = clone_expression(a.init)
    } else {
        var ftype = clone_type(a._type)
        ftype.flags.constant = false
        ftype.flags.ref = false
        var getFn = qmacro($c(getter)($i(arch_name), $v("{prefix}{a.name}"), unsafe(type<$t(ftype)>)))
        getFn.genFlags.alwaysSafe = true
        iget = qmacro($e(getFn)[entity_index])
        force_generated(iget, true)
    }
    var vlet = new ExprLet(at = a.at, atInit = a.at)
    vlet.genFlags.alwaysSafe = true
    var vtype = clone_type(a._type)
    if (getter == "get_default_ro") {
        vtype.flags.constant = true
    } elif (getter == "get_ro") {
        vtype.flags.constant = true
        // if a._type.baseType != Type.tFixedArray
        vtype.flags.ref = true
    } else {
        // if a._type.baseType != Type.tFixedArray
        vtype.flags.ref = true
    }
        // else
        //    macro_error(compiling_program(),a.at,"[] arguments are not supported in eid queries yet")
        //    return false
    vlet.variables |> emplace_new() <| new Variable(at = a.at,
        name := "{prefix}{a.name}{suffix}",
        _type <- vtype,
        init <- iget,
        flags = VariableFlags.can_shadow | VariableFlags.generated
        )
    qblock.list |> emplace(vlet)
    return true
}

enum private DecsQueryType {
    //! Enum for ECS query types.
    query
    eid_query
    find_query
}

[call_macro(name="query")]
class DecsQueryMacro : AstCallMacro {
    //! This macro implements `query` functionality. There are 2 types of queries:
    def override preVisit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : void {
        let totalArgs = length(expr.arguments)
        if (totalArgs != 1 && totalArgs != 2) return
        let block_arg_index = totalArgs - 1
        if (!(expr.arguments[block_arg_index] is ExprMakeBlock)) return
        var mblk = expr.arguments[block_arg_index] as ExprMakeBlock
        var qblk = mblk._block as ExprBlock
        for (arg in qblk.arguments) {
            if (arg._type != null) {
                if (!arg._type.flags.constant) {
                    arg._type.flags.ref = true
                }
            }
        }
    }
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        let totalArgs = length(expr.arguments)
        macro_verify(totalArgs == 1 || totalArgs == 2, prog, expr.at, "expecting query($(block_with_arguments)) or query(eid,$(block_with_arguments))")
        let qt = totalArgs == 2 ? DecsQueryType.eid_query : DecsQueryType.query
        let block_arg_index = totalArgs - 1
        return <- implement(expr, block_arg_index, qt)
    }
    def implement(var expr : ExprCallMacro?; block_arg_index : int; qt : DecsQueryType) : ExpressionPtr {
        for (arg in expr.arguments) {
            macro_verify(!arg._type.isAutoOrAlias, compiling_program(), expr.at, "argument types are not fully inferred")
        }
        macro_verify(expr.arguments[block_arg_index] is ExprMakeBlock, compiling_program(), expr.at, "expecting $(block_with_arguments)")
        let mblk = expr.arguments[block_arg_index] as ExprMakeBlock
        let qblk = mblk._block as ExprBlock
        macro_verify(!empty(qblk.arguments), compiling_program(), expr.at, "expecting query($(block_with_arguments)), arguments are missing")
        let prefix = "__{expr.at.line}_desc"
        let arch_name = "{prefix}_arch"
        var req <- build_req_from_args(qblk)
        req.at = EcsRequestPos(expr.at)
        let vreq = verify_request(req)
        macro_verify(vreq.ok, compiling_program(), expr.at, "incorrect query, {vreq.error}")
        // @@ => [[EcsQuery ...]]
        var erq_fun = qmacro() @@ {
            return <- $v(req)
        }
        var kaboom : array<tuple<string; string; string>>
        var qtop : ExpressionPtr
        if (qt == DecsQueryType.eid_query) {
            var qlbody = new ExprBlock(at = qblk.at)
            for (a in qblk.arguments) {
                let detp = a |> is_decs_template
                if (detp is yes) {
                    kaboom |> push <| (string(a.name), detp as yes, "_{a.name}")
                    for (f in a._type.structType.fields) {
                        if (!append_index_lookup(arch_name, qlbody, f, detp as yes, "_{a.name}", a._type.flags.constant, false)) return null
                    }
                } else {
                    if (!append_index_lookup(arch_name, qlbody, a, "", "")) return null
                }
            }
            for (l in qblk.list) {
                qlbody.list |> push(clone_expression(l))
            }
            for (fl in qblk.finalList) {
                qlbody.finalList |> push(clone_expression(fl))
            }
            convert_block_to_loop(qlbody, true, false, false)
            qtop = qlbody
        } else {
            // for s1, s2 ...
            var qloop = new ExprFor(at = qblk.at, visibility = qblk.at)
            qloop.allowIteratorOptimization = true
            qloop.canShadow = true
            for (a in qblk.arguments) {
                let detp = a |> is_decs_template
                if (detp is yes) {
                    kaboom |> push <| (string(a.name), detp as yes, "_{a.name}")
                    for (f in a._type.structType.fields) {
                        if (!append_iterator(arch_name, qloop, f, detp as yes, "_{a.name}", a._type.flags.constant, false)) return null
                    }
                } else {
                    if (!append_iterator(arch_name, qloop, a, "", "")) return null
                }
            }
            var qlbody = new ExprBlock(at = qblk.at)
            for (l in qblk.list) {
                qlbody.list |> push(clone_expression(l))
            }
            for (fl in qblk.finalList) {
                qlbody.finalList |> push(clone_expression(fl))
            }
            if (qt == DecsQueryType.query) {
                convert_block_to_loop(qlbody, false, true, false)
            } else {
                convert_block_to_loop(qlbody, false, true, true)
            }
            qloop.body = qlbody
            qtop = qloop
        }
        apply_template(qtop) $(rules) {
            for (kb in kaboom) {
                rules |> kaboomVarField(kb._0, kb._1, kb._2)
            }
        }
        if (!empty(kaboom)) {
            apply_template(qtop) $(rules) {
                for (kb in kaboom) {
                    rules |> replaceVariable(kb._0) <| make_static_assert_false("decs_template variables can only be accessed by fields", expr.at)
                }
            }
        }
        var qblock : ExpressionPtr
        unsafe {
            if (qt == DecsQueryType.eid_query) {
                qblock = quote() {
                    for_eid_archetype(tag_eid, tag_req, tag_erq) $(tag_arch, entity_index) {
                        tag_loop
                    }
                }
            } elif (qt == DecsQueryType.find_query) {
                qblock = quote() {
                    for_each_archetype_find(tag_req, tag_erq) $(tag_arch) {
                        tag_loop
                        return false
                    }
                }
            } elif (qt == DecsQueryType.query) {
                qblock = quote() {
                    for_each_archetype(tag_req, tag_erq) $(tag_arch) {
                        tag_loop
                    }
                }
            } else {
                macro_error(compiling_program(), expr.at, "internal error. unsupported query type")
                return null
            }
        }
        qblock |> force_at(expr.at)
        apply_template(qblock) $(rules) {
            if (qt == DecsQueryType.eid_query) {
                rules |> replaceVariable("tag_eid") <| clone_expression(expr.arguments[0])
            }
            rules |> replaceVariable("tag_erq") <| add_ptr_ref(erq_fun)
            rules |> replaceBlockArgument("tag_arch") <| arch_name
            rules |> replaceVariable("tag_req") <| new ExprConstUInt64(at = expr.at, value = req.hash)
            rules |> replaceVariable("tag_loop") <| add_ptr_ref(qtop)
        }
        var qres = move_unquote_block(qblock)
        assert(length(qres.list) == 1 && empty(qres.finalList))
        var rqres = qres.list[0]
        rqres.genFlags.alwaysSafe = true
        return rqres
    }
}

[call_macro(name="find_query")]
class DecsFindQueryMacro : DecsQueryMacro {
    //! This macro implements `find_query` functionality.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(length(expr.arguments) == 1, prog, expr.at, "expecting find_query($(block_with_arguments))")
        return <- implement(expr, 0, DecsQueryType.find_query)
    }
}

[function_macro(name="decs")]
class DecsEcsMacro : AstFunctionAnnotation {
    //! This macro converts a function into a DECS pass stage query. Possible arguments are `stage`, `REQUIRE`, and `REQUIRE_NOT`.
    def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        let argPass = find_arg(args, "stage")
        if (!(argPass is tString)) {
            errors := "need to specify stage"
            return false
        }
        let passName = argPass as tString
        let passFuncName = "decs`pass`{passName}"
        var blk <- setup_call_list(passFuncName, func.at, false, true)
        if (empty(blk.list)) {
            var reg <- setup_call_list("register`decs`passes", func.at, true, true)
            reg.list |> push(qmacro(decs::register_decs_stage_call($v(passName), @@$c(passFuncName))))
        }
        func.flags.privateFunction = true
        blk.list |> push(qmacro($c("_::{func.name}")()))
        var fblk = new ExprBlock(at = func.body.at)                // new function block
        var cqq = make_call(func.at, "query")
        var cquery = cqq as ExprCallMacro
        var qblk = new ExprBlock(at = func.body.at)                // inside the query block
        qblk.blockFlags.isClosure = true
        qblk.returnType = new TypeDecl(baseType = Type.tVoid, at = func.at)
        var req, nreq : array<string>
        for (arg in args) {
            if (arg.basicType == Type.tString) {
                if (arg.name == "REQUIRE") {
                    req |> push("{arg.sValue}")
                }
                if (arg.name == "REQUIRE_NOT") {
                    nreq |> push("{arg.sValue}")
                }
            }
        }
        if (!req |> empty) {
            var decl = new AnnotationDeclaration(uninitialized)
            var ann = find_annotation("decs_boost", "REQUIRE")
            unsafe {
                decl.annotation = reinterpret<Annotation?>(ann)
            }
            for (rq in req) {
                decl.arguments |> add_annotation_argument(rq, true)
            }
            qblk.annotations |> emplace(decl)
        }
        if (!nreq |> empty) {
            var decl = new AnnotationDeclaration(uninitialized)
            var ann = find_annotation("decs_boost", "REQUIRE_NOT")
            unsafe {
                decl.annotation = reinterpret<Annotation?>(ann)
            }
            for (rq in nreq) {
                decl.arguments |> add_annotation_argument(rq, true)
            }
            qblk.annotations |> emplace(decl)
        }
        var fnbody = func.body as ExprBlock
        for (el in fnbody.list) {// list goes to inside query
            qblk.list |> push(clone_expression(el))
        }
        for (ef in fnbody.finalList) {// finally goes to new finally
            fblk.finalList |> push(clone_expression(ef))
        }
        for (arg in func.arguments) {// add function arguments to query arguments
            qblk.arguments |> emplace_new <| new Variable(at = arg.at,
                name := arg.name,
                _type = clone_type(arg._type),
                init = clone_expression(arg.init)
            )
        }
        func.arguments |> clear
        cquery.arguments |> emplace_new <| new ExprMakeBlock(_block <- qblk, at = func.at)
        fblk.list |> emplace(cqq)
        func.body = fblk
        return true
    }
}

[call_macro(name="from_decs")]
class FromDecsMacro : AstCallMacro {
    //! This macro converts a DECS query into an ``iterator<tuple<...>>``.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(length(expr.arguments) == 1, prog, expr.at, "expecting from_decs($(block_with_arguments))")
        macro_verify(expr.arguments[0] is ExprMakeBlock, prog, expr.at, "expecting from_decs($(block_with_arguments))")
        let mblk = expr.arguments[0] as ExprMakeBlock
        let qblk = mblk._block as ExprBlock
        macro_verify(!empty(qblk.arguments), prog, expr.at, "expecting from_decs($(block_with_arguments)), arguments are missing")
        for (arg in qblk.arguments) {
            macro_verify(!arg._type.isAutoOrAlias, prog, expr.at, "argument types are not fully inferred")
        }
        // tuple type
        var tupleType = new TypeDecl(baseType = Type.tTuple, at = expr.at)
        tupleType.argNames.resize(qblk.arguments.length())
        for (arg, idx in qblk.arguments, count()) {
            tupleType.argTypes |> emplace_new(clone_type(arg._type))
            tupleType.argTypes[idx].flags.ref = false
            tupleType.argTypes[idx].flags.constant = false
            tupleType.argNames[idx] := arg.name

        }
        // tuple to push
        var mkTuple = new ExprMakeTuple(at = expr.at)
        mkTuple.recordNames.resize(qblk.arguments.length())
        for (arg, idx in qblk.arguments, count()) {
            mkTuple.values |> push(new ExprVar(at = arg.at, name := arg.name))
            mkTuple.recordNames[idx] := arg.name
        }
        // block with arguments
        var qblock = qmacro(${
            res |> push($e(mkTuple))
        })
        var qba = qblock as ExprMakeBlock
        var qbb = qba._block as ExprBlock
        qbb.blockFlags.isClosure = true
        for (arg, idx in qblk.arguments, count()) {
            qbb.arguments |> emplace_new <| new Variable(at = arg.at,
                name := arg.name,
                _type = clone_type(arg._type),
                init = clone_expression(arg.init)
            )
            qbb.arguments[idx]._type.flags.ref = false
            qbb.arguments[idx]._type.flags.constant = true
        }
        // call macro query
        var callquery = make_call(expr.at, "query")
        (callquery as ExprCallMacro).arguments |> push(clone_expression(qblock))
        // final output
        return qmacro(
            invoke(${
                var res : array<$t(tupleType)>
                $e(callquery)
                return res.to_sequence()
            })
        )
    }
}

[call_macro(name="from_decs_template")]
class FromDecsTemplateMacro : AstCallMacro {
    //! This macro converts a DECS query over a ``[decs_template]`` struct into an ``iterator<tuple<...>>``.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(length(expr.arguments) == 1, prog, expr.at, "expecting from_decs_template(type<Foo>)")
        macro_verify(expr.arguments[0] is ExprTypeDecl, prog, expr.at, "expecting from_decs_template(type<Foo>), got {expr.arguments[0].__rtti}")
        let typeExpr = expr.arguments[0] as ExprTypeDecl
        let td = typeExpr.typeexpr
        macro_verify(td != null && td.baseType == Type.tStructure, prog, expr.at, "from_decs_template expects type<Struct>, got non-struct type")
        let st = td.structType
        macro_verify(st != null, prog, expr.at, "from_decs_template: struct type is not resolved")
        // Resolve [decs_template] annotation + prefix
        var prefix = ""
        var hasTemplate = false
        for (ann in st.annotations) {
            if (ann.annotation.name == "decs_template") {
                hasTemplate = true
                let ppref = ann.arguments |> decs_prefix
                prefix = (ppref is yes) ? (ppref as yes) : "{st.name}_"
                break
            }
        }
        macro_verify(hasTemplate, prog, expr.at, "from_decs_template: struct {st.name} is not annotated with [decs_template]")
        macro_verify(!empty(st.fields), prog, expr.at, "from_decs_template: struct {st.name} has no fields")
        // Tuple type (field names as record names, by-value types stripped of ref/const)
        var tupleType = new TypeDecl(baseType = Type.tTuple, at = expr.at)
        tupleType.argNames.resize(length(st.fields))
        for (fld, idx in st.fields, count()) {
            var ft = clone_type(fld._type)
            ft.flags.ref = false
            ft.flags.constant = false
            tupleType.argTypes |> emplace_new(ft)
            tupleType.argNames[idx] := fld.name
        }
        // Tuple constructor: pull each prefixed component into a record-named field
        var mkTuple = new ExprMakeTuple(at = expr.at)
        mkTuple.recordNames.resize(length(st.fields))
        for (fld, idx in st.fields, count()) {
            mkTuple.values |> push(new ExprVar(at = fld.at, name := "{prefix}{fld.name}"))
            mkTuple.recordNames[idx] := fld.name
        }
        // Query block: one arg per field, name = prefixed component, type = const non-ref fld.init (if set) clones in so the query macro picks get_default_ro
        var qblock = qmacro(${
            res |> push($e(mkTuple))
        })
        var qba = qblock as ExprMakeBlock
        var qbb = qba._block as ExprBlock
        qbb.blockFlags.isClosure = true
        for (fld in st.fields) {
            var argType = clone_type(fld._type)
            argType.flags.ref = false
            argType.flags.constant = true
            qbb.arguments |> emplace_new <| new Variable(at = fld.at,
                name := "{prefix}{fld.name}",
                _type = argType,
                init = clone_expression(fld.init)
            )
        }
        // Call query macro and wrap in invoke
        var callquery = make_call(expr.at, "query")
        (callquery as ExprCallMacro).arguments |> push(clone_expression(qblock))
        return qmacro(
            invoke(${
                var res : array<$t(tupleType)>
                $e(callquery)
                return res.to_sequence()
            })
        )
    }
}
