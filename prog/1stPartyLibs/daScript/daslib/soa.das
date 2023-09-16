options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module soa private

require daslib/ast_boost
require daslib/templates_boost
require daslib/strings_boost

// replacing a[index].b with a.b[index]

struct public SOA_INDEX {}

[tag_function(soa_call)]
def public operator . ( src : SOA_INDEX explicit; field:string )
    // its important that this operator returns `void`, because it would prevent the compiler from cascading infer of wrong type
    assert(false,"this code should never been instanced. soa_call macro replaces the soa[index].field with soa.field[index]")

[tag_function_macro(tag="soa_call")]
class SoaCallMacro : AstFunctionAnnotation
    //! This macro is used to promote a[index].b to a.b[index] for SOA operations
    def override transform ( var expr : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        if is_in_completion()
            return <- [[ExpressionPtr]]
        if expr.arguments |> length != 2
            errors := "expecting soa[index].field"
            return <- [[ExpressionPtr]]
        if !(expr.arguments[0] is ExprCall)
            errors := "expecting soa[index] as first argument, got {expr.__rtti}"
            return <- [[ExpressionPtr]]
        let idx_call = expr.arguments[0] as ExprCall
        if idx_call.name!="[]" || idx_call.arguments |> length!=2
            errors := "expecting soa[index] as first argument, got {describe(expr.arguments[1])}"
            return <- [[ExpressionPtr]]
        if !(expr.arguments[1] is ExprConstString)
            errors := "expecting field"
            return <- [[ExpressionPtr]]
        let fld = expr.arguments[1] as ExprConstString
        var inscope e_field <- new [[ExprField() at=expr.at,
            value <- clone_expression(idx_call.arguments[0]),
            name := fld.value,
            fieldFlags = ExprFieldFieldFlags no_promotion
        ]]
        var inscope e_at <- new [[ExprAt() at=expr.at,
            subexpr <- e_field,
            index <- clone_expression(idx_call.arguments[1]),
            atFlags = ExprAtFlags no_promotion
        ]]
        return <- e_at

[structure_macro(name=soa)]
class SoaStructMacro : AstStructureAnnotation
    //! This macro generates the following::
    //! * structure, where every field is replaced by an array
    //! * operator [] to access the structure, which returns [[SOA_INDEX]]
    //! * length, push, emplace, push_clone, erase
    def override apply ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        if is_in_completion()
            var inscope tdef <- new [[TypeDecl() at=st.at,
                baseType = Type tArray,
                alias := "{st.name}`SOA",
                firstType <- new [[TypeDecl() at=st.at,
                    baseType = Type tStructure,
                    structType = get_ptr(st)
                ]]
            ]]
            compiling_module() |> add_alias(tdef)
            return true
        if st.fields |> length == 0
            errors := "soa macro expects at least one field"
            return false
        // soa
        var inscope stype <- clone_structure(get_ptr(st))
        stype.name := "{stype.name}`SOA"
        for fld in stype.fields
            move_new(fld._type) <| make_field_type(fld)
        // [] operator
        var inscope idx_op <- make_index_op(st, stype)
        compiling_module() |> add_function(idx_op)
        // length()
        var inscope length_op <- make_length(st, stype)
        compiling_module() |> add_function(length_op)
        // push
        var inscope push_op <- make_named_call(st, stype, "push", "push", true)
        compiling_module() |> add_generic(push_op)
        // push_clone
        var inscope push_clone_op <- make_named_call(st, stype, "push_clone", "push_clone", true)
        compiling_module() |> add_function(push_clone_op)
        // emplace
        var inscope emplace_op <- make_named_call(st, stype, "emplace", "emplace", false)
        compiling_module() |> add_function(emplace_op)
        // erase
        var inscope erase_op <- make_erase(st, stype)
        compiling_module() |> add_function(erase_op)
        // now types
        compiling_module() |> add_structure(stype)
        return true
    def make_field_type ( fld:FieldDeclaration ) : TypeDeclPtr
        return <- new [[TypeDecl() at=fld.at,
                        baseType = Type tArray,
                        firstType <- clone_type(fld._type)
                    ]]
    def make_index_op ( st, stype : StructurePtr ) : FunctionPtr
        var inscope idx_op <- new [[Function() at = st.at, atDecl = st.at, name := "[]"]]
        idx_op.flags |= FunctionFlags generated
        move_new(idx_op.result) <| new [[TypeDecl() baseType=Type autoinfer, at=st.at]]
        var inscope idx_body <- quote() <|
            return [[SOA_INDEX]]
        var inscope qres <- move_unquote_block(idx_body)
        qres.blockFlags &= ~ ExprBlockFlags isClosure
        move(idx_op.body) <| qres
        idx_op.arguments |> emplace_new <| new [[Variable()
            name:="st",
            at=st.at,
            _type <- new [[TypeDecl() baseType=Type tStructure, at=st.at, structType=get_ptr(stype), flags=TypeDeclFlags constant]]
        ]]
        idx_op.arguments |> emplace_new <| new [[Variable()
            name:="index",
            at=st.at,
            _type <- new [[TypeDecl() baseType=Type tInt, at=st.at, flags=TypeDeclFlags constant]]
        ]]
        return <- idx_op
    def make_length ( st, stype : StructurePtr  ) : FunctionPtr
        var inscope length_op <- new [[Function() at = st.at, atDecl = st.at, name := "length"]]
        length_op.flags |= FunctionFlags generated
        move_new(length_op.result) <| new [[TypeDecl() baseType=Type tInt, at=st.at]]
        var inscope length_body <- new [[ExprBlock() at=st.at]]
        var inscope length_call <- new [[ExprCall() at=st.at, name:="length"]]
        length_call.arguments |> emplace_new <| new [[ExprField() at=st.at,
            value <- new [[ExprVar() at=st.at, name:="st"]],
            name := st.fields[0].name,
            fieldFlags = ExprFieldFieldFlags no_promotion
        ]]
        length_body.list |> emplace_new <| new [[ExprReturn() at=st.at,
            subexpr <- length_call
        ]]
        length_op.arguments |> emplace_new <| new [[Variable()
            name:="st",
            at=st.at,
            _type <- new [[TypeDecl() baseType=Type tStructure, at=st.at, structType=get_ptr(stype), flags=TypeDeclFlags constant]]
        ]]
        move(length_op.body) <| length_body
        return <- length_op
    def make_any_named_call ( st,stype:StructurePtr; var argT:TypeDeclPtr&; fnName, callName:string; argField:bool ) : FunctionPtr
        var inscope push_op <- new [[Function() at = st.at, atDecl = st.at, name := fnName]]
        push_op.flags |= FunctionFlags generated
        move_new(push_op.result) <| new [[TypeDecl() baseType=Type tVoid, at=st.at]]
        var inscope push_body <- new [[ExprBlock() at=st.at]]
        for fld in st.fields
            if true
                var inscope push_call <- new [[ExprCall() at=st.at, name:=callName]]
                push_call.arguments |> emplace_new <| new [[ExprField() at=st.at,
                    value <- new [[ExprVar() at=st.at, name:="st"]],
                    name := fld.name,
                    fieldFlags = ExprFieldFieldFlags no_promotion
                ]]
                if argField
                    push_call.arguments |> emplace_new <| new [[ExprField() at=st.at,
                        value <- new [[ExprVar() at=st.at, name:="arg"]],
                        name := fld.name,
                        fieldFlags = ExprFieldFieldFlags no_promotion
                    ]]
                else
                    push_call.arguments |> emplace_new <| new [[ExprVar() at=st.at, name:="arg"]]
                push_body.list |> emplace(push_call)
        push_op.arguments |> emplace_new <| new [[Variable()
            name:="st",
            at=st.at,
            _type <- new [[TypeDecl() baseType=Type tStructure, at=st.at, structType=get_ptr(stype)]]
        ]]
        push_op.arguments |> emplace_new <| new [[Variable()
            name:="arg",
            at=st.at,
            _type <- argT
        ]]
        move(push_op.body) <| push_body
        return <- push_op
    def make_erase ( st, stype:StructurePtr )
        var inscope argT <- new [[TypeDecl() baseType=Type tInt, at=st.at]]
        return <- make_any_named_call(st,stype,argT,"erase","erase",false)
    def make_named_call ( st,stype:StructurePtr; fnName, callName:string; isConst:bool=false ) : FunctionPtr
        var inscope argT <- new [[TypeDecl() baseType=Type tStructure, at=st.at, structType=get_ptr(st), flags= isConst ? TypeDeclFlags constant : bitfield(0)]]
        return <- make_any_named_call(st,stype,argT,fnName,callName,true)

class CollectAndReplaceIteratorFields : AstVisitor
    it_prefix : string
    names : table<string;bool>
    def CollectAndReplaceIteratorFields ( prefix:string )
        it_prefix = prefix
    def override visitExprField(var expr:smart_ptr<ExprField>) : ExpressionPtr
        if expr.value is ExprVar
            if (expr.value as ExprVar).name==it_prefix
                let ename = string(expr.name)
                names[ename] = true
                return <- new [[ExprVar() at=expr.at, name:=it_prefix+"`"+ename]]
        return <- expr

def collect_and_replace_iterator_fields ( prefix:string; blk:ExpressionPtr ) : array<string>
    var names : array<string>
    var astVisitor = new CollectAndReplaceIteratorFields(prefix)
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    visit(blk, astVisitorAdapter)
    for n in keys(astVisitor.names)
        names |> push(n)
    unsafe
        delete astVisitor
    return <- names

[for_loop_macro(name=soa_for_loop)]
class SoaForLoop : AstForLoopMacro
    def override visitExprFor ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprFor> ) : ExpressionPtr
        if is_in_completion()
            return <- [[ExpressionPtr]]
        var soa_index = -1
        for index,its in count(),expr.sources
            if its._type!=null && its._type.isStructure
                var is_soa = false
                peek(its._type.structType.name) <| $(sn)
                    if sn |> ends_with("`SOA")
                        is_soa = true
                if is_soa
                    soa_index = index
                    break
        if soa_index != -1
            let it_prefix = string(expr.iterators[soa_index])
            var inscope new_for_e <- clone_expression(expr)
            var new_for = new_for_e as ExprFor
            new_for.sources |> erase(soa_index)
            new_for.iterators |> erase(soa_index)
            new_for.iteratorsAt |> erase(soa_index)
            new_for.iteratorsAka |> erase(soa_index)
            new_for.iteratorsTags |> erase(soa_index)
            var names <- collect_and_replace_iterator_fields(it_prefix,new_for.body)
            if names |> length == 0
                names |> push<| string(expr.sources[soa_index]._type.structType.fields[0].name)
            for itn in names
                let si = new_for.iterators |> length            // todo: add 'push'
                new_for.iterators |> resize(si+1)
                new_for.iterators[si] := "{it_prefix}`{itn}"
                new_for.iteratorsAka |> resize(si+1)
                new_for.iteratorsAka[si] := ""
                new_for.iteratorsAt |> push(expr.iteratorsAt[soa_index])
                new_for.iteratorsTags |> emplace_new <| clone_expression(expr.iteratorsTags[soa_index])
                new_for.sources |> emplace_new <| new [[ExprField() at=expr.sources[soa_index].at,
                    value <- clone_expression(expr.sources[soa_index]),
                    name := itn,
                    fieldFlags = ExprFieldFieldFlags no_promotion
                ]]
            new_for.iteratorVariables |> clear()
            return <- new_for_e
        return <- [[ExpressionPtr]]

