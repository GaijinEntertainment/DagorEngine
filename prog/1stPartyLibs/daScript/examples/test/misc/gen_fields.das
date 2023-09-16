module gen_fields

require UnitTest

require daslib/strings_boost
require daslib/templates
require daslib/templates_boost
require daslib/ast_boost

def public create_component ( name:string; blk:block<(cmp:int):void> )
    print("create_component\n")
    invoke(blk, 13)

def public set_component ( cmp:int; name:string; value:int )
    print("\tset {cmp}.{name} = {value}\n")

def public set_component ( cmp:int; name:string; value:float )
    print("\tset {cmp}.{name} = {value}\n")

def public set_component ( cmp:int; name:string; value:EntityId )
    print("\tset {cmp}.{name} = {value}\n")

[sideeffects]
def public set_component ( cmp:int; name:string; value )
    concept_assert(false,"unsupported component type {typeinfo(typename value)}")

[structure_macro(name=dump)]
class DumpFields : AstStructureAnnotation
    def override apply ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        let fnname = "create`ecs`template`{st.name}"
        var fn <- new [[Function() at=st.at, atDecl=st.at, name:=fnname]]
        fn.flags |= FunctionFlags generated
        fn.flags |= FunctionFlags privateFunction
        fn.flags |= FunctionFlags init
        fn.result <- new [[TypeDecl() baseType=Type tVoid, at=st.at]]
        var blk <- new [[ExprBlock() at=st.at]]
        var inner_block <- new [[ExprBlock() at=st.at]]
        inner_block.blockFlags |= ExprBlockFlags isClosure
        inner_block.arguments |> emplace_new <| new [[Variable() at=st.at, _type<-new [[TypeDecl() baseType=Type tInt]], name:="cmp"]]
        inner_block.returnType <- new [[TypeDecl() baseType=Type tVoid]]
        for field in st.fields
            var setComp <- new [[ExprCall() at=field.at, name:="set_component"]]
            setComp.arguments |> emplace_new <| new [[ExprVar() at=field.at, name:="cmp"]]
            setComp.arguments |> emplace_new <| new [[ExprConstString() at=field.at, value:=field.name]]
            if field.init != null
                setComp.arguments |> emplace_new <| clone_expression(field.init)
            else
                setComp.arguments |> emplace_new <| new [[ExprMakeStruct() at=field.at, makeType<-clone_type(field._type)]]
            inner_block.list |> emplace(setComp)
        var createComp <- new [[ExprCall() at=st.at, name:="create_component"]]
        createComp.arguments |> emplace_new <| new [[ExprConstString() at=st.at, value:=st.name]]
        createComp.arguments |> emplace_new <| new [[ExprMakeBlock() _block <- inner_block]]
        blk.list |> emplace(createComp)
        fn.body <- blk
        compiling_module() |> add_function(fn)
        return true

def follow_the_yellow_brick_road ( expr:ExpressionPtr ) : ExpressionPtr
    if expr is ExprRef2Value
        return (expr as ExprRef2Value).subexpr |> follow_the_yellow_brick_road
    elif expr is ExprVar
        let ev = expr as ExprVar
        if !ev.varFlags.local && !ev.varFlags.argument && !ev.varFlags._block
            if ev.variable._type.flags.constant && ev.variable.init!=null
                return ev.variable.init |> follow_the_yellow_brick_road
    return expr

[template(result)]
def get_eval_any ( econst:ExpressionPtr; result:auto(TT); var errors:das_string; var any_errors:bool& ) : TT
    if econst is ExprCall
        if econst.flags.noSideEffects
            var ok = true
            var res : variant<dummy:float4;id:TT - &>
            unsafe
                res.dummy = eval_single_expression(econst,ok)
            if !ok
                errors := "{errors}\nexpression failed to evaluate {describe(econst)}"
                any_errors = true
            unsafe
                return res.id
    errors := "{errors}\nunsupported initializer {describe(econst)}"
    any_errors = true
    return [[TT]]

def get_const_expr ( expr:ExpressionPtr ) : ExpressionPtr
    if expr == null
        return [[ExpressionPtr]]
    let econst <- expr |> follow_the_yellow_brick_road
    return econst |> is_expr_const ? econst : [[ExpressionPtr]]

def get_entity_id_const ( expr:ExpressionPtr; var errors:das_string; var any_errors:bool& )
    if expr == null
        return make_invalid_id()
    let econst <- expr |> follow_the_yellow_brick_road
    return econst |> get_eval_any(type<EntityId>, errors, any_errors)

[structure_macro(name=dump2)]
class Dump2Fields : AstStructureAnnotation
    def override finish ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        var any_errors = false
        create_component(string(st.name)) <| $ ( cmp )
            for field in st.fields
                if length(field._type.dim)==0
                    let econst <- field.init |> get_const_expr
                    if field._type.baseType==Type tInt
                        set_component(cmp, string(field.name), (econst ?as ExprConstInt)?.value ?? 0)
                    elif field._type.baseType==Type tFloat
                        set_component(cmp, string(field.name), (econst ?as ExprConstFloat)?.value ?? 0.)
                    elif field._type.baseType==Type tHandle
                        if field._type.annotation.name=="EntityId"
                            set_component(cmp, string(field.name), get_entity_id_const(field.init,errors,any_errors))
        return !any_errors




