options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot
options multiple_contexts
options strict_smart_pointers = true

module decs_boost shared private

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
    query ( ) <| $ ( pos:float3&; vel:float3; col:uint=0x12345678 )
        pos += vel
to:
    for_each_archetype ( ERQ_HASH, @@ => [[EcsRequest req <- [[string "pos"; "vel"]] ]] ) <| $ ( arch )
        for pos, vel in get(arch,"pos",type<float3>), get_ro(arch,"vel",type<float3>), get_default_ro(arch,"col",0x12345678)
            tag
*/

[block_macro(name="REQUIRE")]
class DecsReq : AstBlockAnnotation /*! This annotation provides list of required components for entity. */ {}

[block_macro(name="REQUIRE_NOT")]
class DecsReqN : AstBlockAnnotation /*! This annotation provides list of components, which are required to not be part of the entity. */ {}

[structure_macro(name="decs_template")]
class DecsTemplate : AstStructureAnnotation
    //! This macro creates a template for the given structure.
    //! `apply_decs_template` and `remove_decs_template` functions are generated for the structure type.
    def override apply ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        if length(st.fields)==0
            errors := "expecting at least one field in the desc_template {st.name}"
            return false
        let ppref = decs_prefix(args)
        let prefix = (ppref is yes) ? (ppref as yes) : "{st.name}_"
        var inscope blkApply : array<ExpressionPtr>
        var inscope blkErase : array<ExpressionPtr>
        for fld in st.fields
            blkApply |> emplace_new <| qmacro(decs::set(cmp,$v("{prefix}{fld.name}"),src.$f(fld.name)))
            blkErase |> emplace_new <| qmacro(decs::remove(cmp,$v("{prefix}{fld.name}")))
        var inscope fnApply <- qmacro_function("apply_decs_template") <| $ ( var cmp:ComponentMap; var src:$t(st) ) : void
            $b(blkApply)
        if !(compiling_module() |> add_function(fnApply))
            panic("failed to add apply_decs_template, can't add function {fnApply.name}")
        var inscope  fnErase <- qmacro_function("remove_decs_template`{st.name}") <| $ ( var cmp:ComponentMap; var src:$t(st) ) : void
            $b(blkErase)
        if !(compiling_module() |> add_function(fnErase))
            panic("failed to add remove_decs_template, can't add function {fnErase.name}")
        return true

variant ItCheck
    //! DECS prefix check.
    yes : string
    no  : bool

[macro_function]
def private decs_prefix ( arg:AnnotationArgumentList )
    var p = arg |> find_arg("prefix")
    if p is tString
        return [[ItCheck yes=p as tString]]
    elif p is tBool
        return [[ItCheck yes=""]]
    else
        return [[ItCheck no=true]]

[macro_function]
def private is_decs_template ( v:VariablePtr )
    if v._type.baseType != Type tStructure
        return [[ItCheck no=true]]
    var p = v.annotation |> decs_prefix
    if p is no
        for ann in v._type.structType.annotations
            if ann.annotation.name=="decs_template"
                p = ann.arguments |> decs_prefix
                return p is yes ? p : [[ItCheck yes="{v._type.structType.name}_"]]
    return p

[macro_function]
def build_req_from_args ( qblk:ExprBlock? ) : EcsRequest
    var req : EcsRequest
    for a in qblk.arguments
        if a.init==null
            let detp = a |> is_decs_template
            if detp is yes
                for f in a._type.structType.fields
                    req.req |> push("{detp as yes}{f.name}")
            else
                if !(find_arg(a.annotation,"optional") ?as tBool ?? false)
                    req.req |> push(string(a.name))
    for aa in qblk.annotations
        let isreq = aa.annotation.name=="REQUIRE"
        let isreqn = aa.annotation.name=="REQUIRE_NOT"
        if isreq || isreqn
            for ab in aa.arguments
                if ab.basicType==Type tBool && ab.bValue
                    (isreq ? req.req : req.reqn) |> push(string(ab.name))
    compile_request(req)
    return <- req

[macro_function]
def getter_name ( a; const_parent:bool; can_be_optional:bool )
    var getter = "get_ro"
    static_if typeinfo(stripped_typename a)=="ast::FieldDeclaration"        // part of the desc_template
        if a.init != null && can_be_optional
            getter = "get_default_ro"
        elif !const_parent
            getter = "get"
    else
        if find_arg(a.annotation,"optional") ?as tBool ?? false
            if a.init != null
                macro_error(compiling_program(),a.at,"optional argument {a.name} can't have a default value")
                return ""
            elif !a._type.isPointer
                macro_error(compiling_program(),a.at,"optional argument {a.name} must be a pointer")
                return ""
            getter = "get_optional"
        elif a.init != null && can_be_optional
            if a._type.flags.ref || !a._type.flags.constant
                macro_error(compiling_program(),a.at,"argument {a.name} has default value, it can't be & or var")
                return ""
            getter = "get_default_ro"
        elif (a._type.isRef && !a._type.isRefType)
            if a._type.flags.constant
                macro_error(compiling_program(),a.at,"argument {a.name} is both & and constant")
                return ""
            getter = "get"
        elif a._type.isRefType && !a._type.flags.constant
            getter = "get"
    return getter

[macro_function]
def private append_iterator ( arch_name:string; var qloop:smart_ptr<ExprFor>; a; prefix,suffix:string; const_parent : bool = false; can_be_optional : bool = true )
    let qli = length(qloop.iterators)
    qloop.iterators |> resize( qli + 1 )
    qloop.iterators[qli] := "{prefix}{a.name}{suffix}"
    qloop.iteratorsAka |> resize( qli + 1 )
    if typeinfo(has_field<_aka> a)
        qloop.iteratorsAka[qli] := a._aka
    else
        qloop.iteratorsAka[qli] := ""
    qloop.iteratorsAt |> push(a.at)
    qloop.iteratorsTags |> resize ( qli + 1 )
    var getter = getter_name(a,const_parent,can_be_optional)
    if empty(getter)
        return false
    if getter=="get_default_ro"
        qloop.sources |> emplace_new <| qmacro($c(getter)($i(arch_name),$v("{prefix}{a.name}"),$e(a.init)))
    else
        var inscope ftype <- clone_type(a._type)
        ftype.flags &= ~ TypeDeclFlags constant
        ftype.flags &= ~ TypeDeclFlags ref
        qloop.sources |> emplace_new <| qmacro($c(getter)($i(arch_name),$v("{prefix}{a.name}"),type<$t(ftype)>))
    return true

[macro_function]
def private append_index_lookup ( arch_name:string; var qblock:smart_ptr<ExprBlock>; a; prefix, suffix:string; const_parent : bool = false; can_be_optional : bool = true )
    var getter = getter_name(a,const_parent,can_be_optional)
    if empty(getter)
        return false
    var inscope iget : ExpressionPtr
    if getter=="get_default_ro"
        unsafe
            iget <- clone_expression(a.init)
    else
        var inscope ftype <- clone_type(a._type)
        ftype.flags &= ~ TypeDeclFlags constant
        ftype.flags &= ~ TypeDeclFlags ref
        move(iget, get_ptr(qmacro($c(getter)($i(arch_name),$v("{prefix}{a.name}"),type<$t(ftype)>)[entity_index])))
    var inscope vlet <- new [[ExprLet() at=a.at, atInit=a.at]]
    vlet.genFlags |= ExprGenFlags alwaysSafe
    var inscope vtype <- clone_type(a._type)
    if getter=="get_default_ro"
        vtype.flags |= TypeDeclFlags constant
    elif getter=="get_ro"
        vtype.flags |= TypeDeclFlags constant
        // if length(a._type.dim)==0
        vtype.flags |= TypeDeclFlags ref
    else
        // if length(a._type.dim)==0
        vtype.flags |= TypeDeclFlags ref
        // else
        //    macro_error(compiling_program(),a.at,"[] arguments are not supported in eid queries yet")
        //    return false
    vlet.variables |> emplace_new() <| new [[Variable() at = a.at,
        name := "{prefix}{a.name}{suffix}",
        _type <- vtype,
        init <- iget,
        flags = VariableFlags can_shadow
        ]]
    qblock.list |> emplace(vlet)
    return true

enum private DecsQueryType
    query
    eid_query
    find_query

[call_macro(name="query")]
class DecsQueryMacro : AstCallMacro
    //! This macro implmenets 'query` functionality. There are 2 types of queries:
    //!     * query(...) - returns a list of entities matching the query
    //!     * query(eid) - returns a single entity matching the eid
    //! For example::
    //!
    //!     query() <| $ ( eid:EntityId; pos, vel : float3 )
    //!         print("[{eid}] pos={pos} vel={vel}\n")
    //!
    //! The query above will print all entities with position and velocity.
    //! Here is another example::
    //!
    //!     query(kaboom) <| $ ( var pos:float3&; vel:float3; col:uint=13u )
    //!         pos += vel
    //! The query above will add the velocity to the position of an entity with eid kaboom.
    //!
    //! Query can have `REQUIRE` and `REQUIRE_NOT` clauses::
    //!
    //!     var average : float3
    //!     query <| $ [REQUIRE(tank)] ( pos:float3 )
    //!         average += pos
    //!
    //! The query above will add `pos` components of all entities, which also have a `tank` component.
    //!
    //! Additionally queries can atuomaticall expand components of entities. For example::
    //!
    //!     [decs_template(prefix="particle")]
    //!     struct Particle
    //!         pos, vel : float3
    //!     ...
    //!     query <| $ ( var q : Particle )
    //!         q.pos += q.vel                  // this is actually particlepos += particlevel
    //!
    //!
    //! In the example above structure q : Particle does not exist as a variable. Instead it is expanded into accessing individual componentes of the entity.
    //! REQURE section of the query is automatically filled with all components of the template.
    //! If template prefix is not specified, prefix is taken from the name of the template (would be "Particle_").
    //! Specifying empty prefix `[decs_template(prefix)]` will result in no prefix beeing added.
    //!
    //! Note: apart from tagging structure as a template, the macro also generates `apply_decs_template` and `remove_decs_template` functions.
    //! `apply_decs_template` is used to add template to an entity, and `remove_decs_template` is used to remove all components of the template from the entity::
    //!
    //!     for i in range(3)
    //!         create_entity <| @ ( eid, cmp )
    //!             apply_decs_template(cmp, [[Particle pos=float3(i), vel=float3(i+1)]])
    def override preVisit ( prog:ProgramPtr; mod:Module?; var expr:smart_ptr<ExprCallMacro> ) : void
        let totalArgs = length(expr.arguments)
        if totalArgs!=1 && totalArgs!=2
            return
        let qt = totalArgs==2 ? DecsQueryType eid_query : DecsQueryType query
        let block_arg_index = totalArgs-1
        if !(expr.arguments[block_arg_index] is ExprMakeBlock)
            return
        var mblk = expr.arguments[block_arg_index] as ExprMakeBlock
        var qblk = mblk._block as ExprBlock
        for arg in qblk.arguments
            if arg._type != null
                if !arg._type.flags.constant
                    arg._type.flags |= TypeDeclFlags ref
    def override visit ( prog:ProgramPtr; mod:Module?; var expr:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        let totalArgs = length(expr.arguments)
        macro_verify(totalArgs==1 || totalArgs==2,prog,expr.at,"expecting query($(block_with_arguments)) or query(eid,$(block_with_arguments))")
        let qt = totalArgs==2 ? DecsQueryType eid_query : DecsQueryType query
        let block_arg_index = totalArgs-1
        return <- self->implement(expr, block_arg_index, qt)
    def implement ( var expr:smart_ptr<ExprCallMacro>; block_arg_index:int; qt:DecsQueryType ) : ExpressionPtr
        for arg in expr.arguments
            macro_verify(!arg._type.isAutoOrAlias,compiling_program(),expr.at,"argument types are not fully inferred")
        macro_verify(expr.arguments[block_arg_index] is ExprMakeBlock,compiling_program(),expr.at,"expecting $(block_with_arguments)")
        let mblk = expr.arguments[block_arg_index] as ExprMakeBlock
        let qblk = mblk._block as ExprBlock
        macro_verify(length(qblk.arguments)!=0,compiling_program(),expr.at,"expecting query($(block_with_arguments)), arguments are missing")
        let prefix = "__{expr.at.line}_desc"
        let arch_name = "{prefix}_arch"
        var req <- build_req_from_args(qblk)
        req.at = EcsRequestPos(expr.at)
        var vreq = verify_request(req)
        macro_verify(vreq.ok,compiling_program(),expr.at,"incorrect query, {vreq.error}")
        // @@ => [[EcsQuery ...]]
        var inscope erq_fun <- qmacro <| @@
            return <- $v(req)
        var kaboom : array<tuple<string;string;string>>
        var inscope qtop : ExpressionPtr
        if qt==DecsQueryType eid_query
            var inscope qlbody <- new [[ExprBlock() at=qblk.at]]
            for a in qblk.arguments
                let detp = a |> is_decs_template
                if detp is yes
                    kaboom |> push <| [[auto string(a.name),detp as yes,"_{a.name}"]]
                    for f in a._type.structType.fields
                        if !append_index_lookup(arch_name, qlbody, f, detp as yes, "_{a.name}", a._type.flags.constant, false)
                            return <- [[ExpressionPtr]]
                else
                    if !append_index_lookup(arch_name, qlbody, a, "", "")
                        return <- [[ExpressionPtr]]
            for l in qblk.list
                qlbody.list |> emplace_new <| clone_expression(l)
            for fl in qblk.finalList
                qlbody.finalList |> emplace_new <| clone_expression(fl)
            convert_block_to_loop(qlbody, true, false, false)
            move(qtop) <| qlbody
        else
            // for s1, s2 ...
            var inscope qloop <- new [[ExprFor() at=qblk.at, visibility=qblk.at]]
            qloop.allowIteratorOptimization = true
            qloop.canShadow = true
            for a in qblk.arguments
                let detp = a |> is_decs_template
                if detp is yes
                    kaboom |> push <| [[auto string(a.name),detp as yes,"_{a.name}"]]
                    for f in a._type.structType.fields
                        if !append_iterator(arch_name, qloop, f, detp as yes, "_{a.name}", a._type.flags.constant, false)
                            return <- [[ExpressionPtr]]
                else
                    if !append_iterator(arch_name, qloop, a, "", "")
                        return <- [[ExpressionPtr]]
            var inscope qlbody <- new [[ExprBlock() at=qblk.at]]
            for l in qblk.list
                qlbody.list |> emplace_new <| clone_expression(l)
            for fl in qblk.finalList
                qlbody.finalList |> emplace_new <| clone_expression(fl)
            if qt==DecsQueryType query
                convert_block_to_loop(qlbody, false, true, false )
            else
                convert_block_to_loop(qlbody, false, true, true )
            move(qloop.body) <| qlbody
            move(qtop) <| qloop
        apply_template(qtop) <| $ ( rules )
            for kb in kaboom
                rules |> kaboomVarField(kb._0,kb._1,kb._2)
        if length(kaboom) > 0
            apply_template(qtop) <| $ ( rules )
                for kb in kaboom
                    rules |> replaceVariable(kb._0) <| make_static_assert_false("decs_template variables can only be accessed by fields", expr.at)
        var inscope qblock : ExpressionPtr
        unsafe
            if qt==DecsQueryType eid_query
                qblock <- quote() <|
                    for_eid_archetype (tag_eid, tag_req, tag_erq) <| $ ( tag_arch, entity_index )
                        tag_loop
            elif qt==DecsQueryType find_query
                qblock <- quote() <|
                    for_each_archetype_find (tag_req, tag_erq) <| $ ( tag_arch )
                        tag_loop
                        return false
            elif qt==DecsQueryType query
                qblock <- quote() <|
                    for_each_archetype (tag_req, tag_erq) <| $ ( tag_arch )
                        tag_loop
            else
                macro_error(compiling_program(),expr.at,"internal error. unsupported query type")
                return [[ExpressionPtr]]
        qblock |> force_at(expr.at)
        apply_template(qblock) <| $ ( rules )
            if qt==DecsQueryType eid_query
                rules |> replaceVariable("tag_eid") <| clone_expression(expr.arguments[0])
            rules |> replaceVariable("tag_erq") <| add_ptr_ref(erq_fun)
            rules |> replaceBlockArgument("tag_arch") <| arch_name
            rules |> replaceVariable("tag_req") <| new [[ExprConstUInt64() at=expr.at, value=req.hash]]
            rules |> replaceVariable("tag_loop") <| add_ptr_ref(qtop)
        var inscope qres <- move_unquote_block(qblock)
        assert(length(qres.list)==1 && length(qres.finalList)==0)
        var inscope rqres <- qres.list[0]
        qres := null
        return <- rqres

[call_macro(name="find_query")]
class DecsFindQueryMacro : DecsQueryMacro
    //! This macro implmenets 'find_query` functionality.
    //! It is similar to `query` in most ways, with the main differences being:
    //!     * there is no eid-based find query
    //!     * the find_query stops once the first match is found
    //! For example::
    //!
    //!     let found = find_query <| $ ( pos,dim:float3; obstacle:Obstacle )
    //!     if !obstacle.wall
    //!         return false
    //!     let aabb = [[AABB min=pos-dim*0.5, max=pos+dim*0.5 ]]
    //!     if is_intersecting(ray, aabb, 0.1, dist)
    //!         return true
    //!
    //! In the example above the find_query will return `true` once the first intesection is found.
    //! Note: if return is missing, or end of find_query block is reached - its assumed that find_query did not find anything, and will return false.
    def override visit ( prog:ProgramPtr; mod:Module?; var expr:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        macro_verify(length(expr.arguments)==1,prog,expr.at,"expecting find_query($(block_with_arguments))")
        return <- self->implement(expr, 0, DecsQueryType find_query)

[function_macro(name="decs")]
class DecsEcsMacro : AstFunctionAnnotation
    //! This macro converts a function into a DECS pass stage query. Possible arguments are `stage`, 'REQUIRE', and `REQUIRE_NOT`.
    //! It has all other properties of a `query` (like ability to operate on templates). For example::
    //!
    //!     [decs(stage=update_ai, REQUIRE=ai_turret)]
    //!         def update_ai ( eid:EntityId; var turret:Turret; pos:float3 )
    //!             ...
    //!
    //! In the example above a query is added to the `update_ai` stage. The query also requires that each entity passed to it has an `ai_turret` property.
    def override apply ( var func:FunctionPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        let argPass = find_arg(args,"stage")
        if !(argPass is tString)
            errors := "need to specify stage"
            return false
        let passName = argPass as tString
        let passFuncName = "decs`pass`{passName}"
        var blk <- setup_call_list(passFuncName, func.at, false, true)
        if length(blk.list)==0
            var reg <- setup_call_list("register`decs`passes", func.at, true, true)
            reg.list |> emplace_new <| qmacro(decs::register_decs_stage_call($v(passName),@@$c(passFuncName)))
        func.flags |= FunctionFlags privateFunction
        blk.list |> emplace_new <| qmacro($c("_::{func.name}")())
        var inscope fblk <- new [[ExprBlock() at=func.body.at]]                 // new function block
        var inscope cqq <- make_call(func.at,"query")
        var cquery = cqq as ExprCallMacro
        var inscope qblk <- new [[ExprBlock() at=func.body.at]]                 // inside the query block
        qblk.blockFlags |= ExprBlockFlags isClosure
        move_new(qblk.returnType) <| new [[TypeDecl() baseType=Type tVoid, at=func.at]]
        var req, nreq : array<string>
        for arg in args
            if arg.basicType==Type tString
                if arg.name=="REQUIRE"
                    req |> push("{arg.sValue}")
                if arg.name=="REQUIRE_NOT"
                    nreq |> push("{arg.sValue}")
        if !req |> empty
            var inscope decl <- new [[AnnotationDeclaration() ]]
            var ann = find_annotation("decs_boost", "REQUIRE")
            unsafe
                decl.annotation := reinterpret<smart_ptr<Annotation>> ann
            for rq in req
                decl.arguments |> add_annotation_argument(rq, true)
            qblk.annotations |> emplace(decl)
        if !nreq |> empty
            var inscope decl <- new [[AnnotationDeclaration() ]]
            var ann = find_annotation("decs_boost", "REQUIRE_NOT")
            unsafe
                decl.annotation := reinterpret<smart_ptr<Annotation>> ann
            for rq in nreq
                decl.arguments |> add_annotation_argument(rq, true)
            qblk.annotations |> emplace(decl)
        var fnbody = func.body as ExprBlock
        for el in fnbody.list                                           // list goes to inside query
            qblk.list |> emplace_new <| clone_expression(el)
        for ef in fnbody.finalList                                      // finally goes to new finally
            fblk.finalList |> emplace_new <| clone_expression(ef)
        for arg in func.arguments                                       // add function arguments to query arguments
            qblk.arguments |> emplace_new <| new [[Variable() at=arg.at,
                name := arg.name,
                _type <- clone_type(arg._type),
                init <- clone_expression(arg.init)
            ]]
        func.arguments |> clear
        cquery.arguments |> emplace_new <| new [[ExprMakeBlock() _block <- qblk, at=func.at]]
        fblk.list |> emplace(cqq)
        move(func.body, fblk)
        return true
