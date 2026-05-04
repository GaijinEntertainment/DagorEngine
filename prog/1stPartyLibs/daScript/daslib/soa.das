options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module soa private

//! Structure-of-Arrays (SOA) macro.
//!
//! Annotate a struct with ``[soa]`` to generate a parallel SOA layout:
//! every field becomes an array, and ``[]`` indexing returns an ``SOA_INDEX``
//! proxy that redirects field access (``soa[i].field`` becomes ``soa.field[i]``).
//! Also generates ``length``, ``capacity``, ``push``, ``push_clone``, ``emplace``,
//! ``erase``, ``pop``, ``clear``, ``resize``, ``reserve``, ``swap``,
//! ``from_array``, and ``to_array``.

require daslib/ast_boost
require daslib/templates_boost
require daslib/strings_boost

// replacing a[index].b with a.b[index]

struct public SOA_INDEX {
    //! Proxy type returned by the ``[]`` operator on an SOA structure.
    //! Field access on this proxy is rewritten by `SoaCallMacro` to index into the correct column array.
}

[tag_function(soa_call)]
def public operator .(src : SOA_INDEX explicit; field : string) {
    //! Field access operator for SOA_INDEX; rewritten by SoaCallMacro to convert soa[index].field into soa.field[index].
    // its important that this operator returns `void`, because it would prevent the compiler from cascading infer of wrong type
    assert(false, "this code should never been instanced. soa_call macro replaces the soa[index].field with soa.field[index]")
}

[tag_function_macro(tag="soa_call")]
class SoaCallMacro : AstFunctionAnnotation {
    //! Rewrites ``soa[index].field`` into ``soa.field[index]`` at compile time.
    //! This is the core of the SOA access pattern — it transforms AOS-style element access
    //! into column-wise array indexing for better cache locality.
    def override transform(var expr : smart_ptr<ExprCallFunc>; var errors : das_string) : ExpressionPtr {
        if (is_in_completion()) {
            return <- default<ExpressionPtr>
        }
        if (expr.arguments |> length != 2) {
            errors := "expecting soa[index].field"
            return <- default<ExpressionPtr>
        }
        if (!(expr.arguments[0] is ExprCall)) {
            errors := "expecting soa[index] as first argument, got {expr.__rtti}"
            return <- default<ExpressionPtr>
        }
        let idx_call = expr.arguments[0] as ExprCall
        if (idx_call.name != "[]" || idx_call.arguments |> length != 2) {
            errors := "expecting soa[index] as first argument, got {describe(expr.arguments[1])}"
            return <- default<ExpressionPtr>
        }
        if (!(expr.arguments[1] is ExprConstString)) {
            errors := "expecting field"
            return <- default<ExpressionPtr>
        }
        let fld = expr.arguments[1] as ExprConstString
        var inscope e_field <- new ExprField(at = expr.at,
            value <- clone_expression(idx_call.arguments[0]),
            name := fld.value,
            fieldFlags = ExprFieldFieldFlags.no_promotion
        )
        var inscope e_at <- new ExprAt(at = expr.at,
            subexpr <- e_field,
            index <- clone_expression(idx_call.arguments[1]),
            atFlags = ExprAtFlags.no_promotion
        )
        return <- e_at
    }
}

[structure_macro(name=soa)]
class SoaStructMacro : AstStructureAnnotation {
    //! Generates a Structure-of-Arrays layout from a regular struct.
    //! For a struct ``Foo`` with fields ``x : float`` and ``y : float``, generates:
    //!
    //!   * ``Foo`SOA`` — struct where every field is ``array<FieldType>``
    //!   * ``operator []`` returning ``SOA_INDEX`` proxy
    //!   * ``length``, ``capacity``, ``push``, ``push_clone``, ``emplace``, ``erase``, ``pop``, ``clear``, ``resize``, ``reserve``, ``swap``, ``from_array``, ``to_array`` functions
    def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        if (is_in_completion()) {
            var inscope tdef <- new TypeDecl(at = st.at,
                baseType = Type.tArray,
                alias := "{st.name}`SOA",
                firstType <- new TypeDecl(at = st.at,
                    baseType = Type.tStructure,
                    structType = get_ptr(st)
                )
            )
            compiling_module() |> add_alias(tdef)
            return true
        }
        if (st.fields |> length == 0) {
            errors := "soa macro expects at least one field"
            return false
        }
        // soa
        var inscope stype <- clone_structure(get_ptr(st))
        stype.name := "{stype.name}`SOA"
        for (fld in stype.fields) {
            move_new(fld._type) <| make_field_type(fld)
        }
        // [] operator
        var inscope idx_op <- make_index_op(st, stype)
        compiling_module() |> add_function(idx_op)
        // length()
        var inscope length_op <- make_length(st, stype)
        compiling_module() |> add_function(length_op)
        // push
        var inscope push_op <- make_named_call(st, stype, "push", "push_clone", true)
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
        // resize
        var inscope resize_op <- make_int_call(st, stype, "resize", "resize")
        compiling_module() |> add_function(resize_op)
        // reserve
        var inscope reserve_op <- make_int_call(st, stype, "reserve", "reserve")
        compiling_module() |> add_function(reserve_op)
        // clear
        var inscope clear_op <- make_no_arg_call(st, stype, "clear", "clear")
        compiling_module() |> add_function(clear_op)
        // pop
        var inscope pop_op <- make_no_arg_call(st, stype, "pop", "pop")
        compiling_module() |> add_function(pop_op)
        // capacity
        var inscope capacity_op <- make_capacity(st, stype)
        compiling_module() |> add_function(capacity_op)
        // swap
        var inscope swap_op <- make_swap(st, stype)
        compiling_module() |> add_function(swap_op)
        // from_array
        var inscope from_array_op <- make_from_array(st, stype)
        compiling_module() |> add_function(from_array_op)
        // to_array
        var inscope to_array_op <- make_to_array(st, stype)
        compiling_module() |> add_function(to_array_op)
        // now types
        compiling_module() |> add_structure(stype)
        return true
    }
    def make_field_type(fld : FieldDeclaration) : TypeDeclPtr {
        return <- new TypeDecl(at = fld.at,
                        baseType = Type.tArray,
                        firstType <- clone_type(fld._type)
                    )
    }
    def make_soa_type(st : StructurePtr; var stype : StructurePtr; isConst : bool = false) : TypeDeclPtr {
        return <- new TypeDecl(baseType = Type.tStructure, at = st.at,
            structType = get_ptr(stype),
            flags = isConst ? TypeDeclFlags.constant : bitfield(0)
        )
    }
    def make_index_op(st : StructurePtr; var stype : StructurePtr) : FunctionPtr {
        var inscope stypeT <- make_soa_type(st, stype, true)
        var inscope fn <- qmacro_function("[]") $(st : $t(stypeT); index : int const) {
            return SOA_INDEX()
        }
        fn.flags |= FunctionFlags.generated
        return <- fn
    }
    def make_length(st : StructurePtr; var stype : StructurePtr) : FunctionPtr {
        var inscope stypeT <- make_soa_type(st, stype, true)
        let firstFieldName = string(st.fields[0].name)
        var inscope fn <- qmacro_function("length") $(st : $t(stypeT)) : int {
            return length(st.$f(firstFieldName))
        }
        fn.flags |= FunctionFlags.generated
        return <- fn
    }
    def make_any_named_call(st : StructurePtr; var stype : StructurePtr; var argT : TypeDeclPtr&; fnName, callName : string; argField : bool) : FunctionPtr {
        var inscope stypeT <- make_soa_type(st, stype)
        var inscope bodyExprs : array<ExpressionPtr>
        for (fld in st.fields) {
            let fieldName = string(fld.name)
            if (argField) {
                bodyExprs |> emplace_new <| qmacro($c(callName)(st.$f(fieldName), arg.$f(fieldName)))
            } else {
                bodyExprs |> emplace_new <| qmacro($c(callName)(st.$f(fieldName), arg))
            }
        }
        var inscope fn <- qmacro_function(fnName) $(var st : $t(stypeT); var arg : $t(argT)) : void {
            $b(bodyExprs)
        }
        fn.flags |= FunctionFlags.generated
        return <- fn
    }
    def make_erase(st : StructurePtr; var stype : StructurePtr) {
        var inscope argT <- new TypeDecl(baseType = Type.tInt, at = st.at)
        return <- make_any_named_call(st, stype, argT, "erase", "erase", false)
    }
    def make_named_call(var st : StructurePtr; var stype : StructurePtr; fnName, callName : string; isConst : bool = false) : FunctionPtr {
        var inscope argT <- new TypeDecl(baseType = Type.tStructure, at = st.at, structType = get_ptr(st), flags = isConst ? TypeDeclFlags.constant : bitfield(0))
        return <- make_any_named_call(st, stype, argT, fnName, callName, true)
    }
    def make_int_call(st : StructurePtr; var stype : StructurePtr; fnName, callName : string) : FunctionPtr {
        var inscope argT <- new TypeDecl(baseType = Type.tInt, at = st.at)
        return <- make_any_named_call(st, stype, argT, fnName, callName, false)
    }
    def make_no_arg_call(st : StructurePtr; var stype : StructurePtr; fnName, callName : string) : FunctionPtr {
        var inscope stypeT <- make_soa_type(st, stype)
        var inscope bodyExprs : array<ExpressionPtr>
        for (fld in st.fields) {
            let fieldName = string(fld.name)
            bodyExprs |> emplace_new <| qmacro($c(callName)(st.$f(fieldName)))
        }
        var inscope fn <- qmacro_function(fnName) $(var st : $t(stypeT)) : void {
            $b(bodyExprs)
        }
        fn.flags |= FunctionFlags.generated
        return <- fn
    }
    def make_capacity(st : StructurePtr; var stype : StructurePtr) : FunctionPtr {
        var inscope stypeT <- make_soa_type(st, stype, true)
        let firstFieldName = string(st.fields[0].name)
        var inscope fn <- qmacro_function("capacity") $(st : $t(stypeT)) : int {
            return capacity(st.$f(firstFieldName))
        }
        fn.flags |= FunctionFlags.generated
        return <- fn
    }
    def make_swap(st : StructurePtr; var stype : StructurePtr) : FunctionPtr {
        var inscope stypeT <- make_soa_type(st, stype)
        var inscope bodyExprs : array<ExpressionPtr>
        for (fld in st.fields) {
            let fieldName = string(fld.name)
            bodyExprs |> emplace_new <| qmacro(swap(st.$f(fieldName)[i], st.$f(fieldName)[j]))
        }
        var inscope fn <- qmacro_function("swap") $(var st : $t(stypeT); i : int const; j : int const) : void {
            $b(bodyExprs)
        }
        fn.flags |= FunctionFlags.generated
        return <- fn
    }
    def make_from_array(var st : StructurePtr; var stype : StructurePtr) : FunctionPtr {
        var inscope stypeT <- make_soa_type(st, stype)
        var inscope srcT <- new TypeDecl(baseType = Type.tArray, at = st.at,
            firstType <- new TypeDecl(baseType = Type.tStructure, at = st.at,
                structType = get_ptr(st)
            )
        )
        var inscope reserveExprs : array<ExpressionPtr>
        var inscope bodyExprs : array<ExpressionPtr>
        for (fld in st.fields) {
            let fieldName = string(fld.name)
            reserveExprs |> emplace_new <| qmacro(reserve(st.$f(fieldName), length(st.$f(fieldName)) + length(arr)))
            bodyExprs |> emplace_new <| qmacro(push_clone(st.$f(fieldName), elem.$f(fieldName)))
        }
        var inscope fn <- qmacro_function("from_array") $(var st : $t(stypeT); arr : $t(srcT)) : void {
            $b(reserveExprs)
            for (elem in arr) {
                $b(bodyExprs)
            }
        }
        fn.flags |= FunctionFlags.generated
        return <- fn
    }
    def make_to_array(var st : StructurePtr; var stype : StructurePtr) : FunctionPtr {
        var inscope stypeT <- make_soa_type(st, stype, true)
        var inscope retT <- new TypeDecl(baseType = Type.tArray, at = st.at,
            firstType <- new TypeDecl(baseType = Type.tStructure, at = st.at,
                structType = get_ptr(st)
            )
        )
        var inscope elemT <- new TypeDecl(baseType = Type.tStructure, at = st.at,
            structType = get_ptr(st)
        )
        var inscope bodyExprs : array<ExpressionPtr>
        for (fld in st.fields) {
            let fieldName = string(fld.name)
            bodyExprs |> emplace_new <| qmacro_expr(${ soa_elem.$f(fieldName) := st.$f(fieldName)[soa_idx]; })
        }
        let firstFieldName = string(st.fields[0].name)
        var inscope retT2 <- clone_type(retT)
        var inscope fn <- qmacro_function("to_array") $(st : $t(stypeT)) : $t(retT) {
            var result : $t(retT2)
            result |> reserve(length(st.$f(firstFieldName)))
            for (soa_idx in range(length(st.$f(firstFieldName)))) {
                var soa_elem : $t(elemT)
                $b(bodyExprs)
                result |> emplace(soa_elem)
            }
            return <- result
        }
        fn.flags |= FunctionFlags.generated
        return <- fn
    }
}

class CollectAndReplaceIteratorFields : AstVisitor {
    //! AST visitor that collects and replaces SOA iterator field access.
    it_prefix : string
    names : table<string; bool>
    def CollectAndReplaceIteratorFields(prefix : string) {
        it_prefix = prefix
    }
    def override visitExprField(var expr : smart_ptr<ExprField>) : ExpressionPtr {
        if (expr.value is ExprVar) {
            if ((expr.value as ExprVar).name == it_prefix) {
                let ename = string(expr.name)
                names |> insert(ename, true)
                return <- new ExprVar(at = expr.at, name := it_prefix + "`" + ename)
            }
        }
        return <- expr
    }
}

def collect_and_replace_iterator_fields(prefix : string; blk : ExpressionPtr) : array<string> {
    var names : array<string>
    var astVisitor = new CollectAndReplaceIteratorFields(prefix)
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    visit(blk, astVisitorAdapter)
    for (n in keys(astVisitor.names)) {
        names |> push(n)
    }
    unsafe {
        delete astVisitor
    }
    return <- names
}

[for_loop_macro(name=soa_for_loop)]
class SoaForLoop : AstForLoopMacro {
    //! For-loop macro that transforms SOA array iteration.
    def override visitExprFor(prog : ProgramPtr; mod : Module?; expr : smart_ptr<ExprFor>) : ExpressionPtr {
        if (is_in_completion()) {
            return <- default<ExpressionPtr>
        }
        var soa_index = -1
        for (index, its in count(), expr.sources) {
            if (its._type != null && its._type.isStructure) {
                var is_soa = false
                peek(its._type.structType.name) $(sn) {
                    if (sn |> ends_with("`SOA")) {
                        is_soa = true
                    }
                }
                if (is_soa) {
                    soa_index = index
                    break
                }
            }
        }
        if (soa_index != -1) {
            let it_prefix = string(expr.iterators[soa_index])
            var inscope new_for_e <- clone_expression(expr)
            var new_for = new_for_e as ExprFor
            new_for.sources |> erase(soa_index)
            new_for.iterators |> erase(soa_index)
            new_for.iteratorsAt |> erase(soa_index)
            new_for.iteratorsAka |> erase(soa_index)
            new_for.iteratorsTags |> erase(soa_index)
            var names <- collect_and_replace_iterator_fields(it_prefix, new_for.body)
            if (names |> length == 0) {
                names |> push <| string(expr.sources[soa_index]._type.structType.fields[0].name)
            }
            for (itn in names) {
                let si = new_for.iterators |> length            // das_string arrays use resize+assign
                new_for.iterators |> resize(si + 1)
                new_for.iterators[si] := "{it_prefix}`{itn}"
                new_for.iteratorsAka |> resize(si + 1)
                new_for.iteratorsAka[si] := ""
                new_for.iteratorsAt |> push(expr.iteratorsAt[soa_index])
                new_for.iteratorsTags |> emplace_new <| clone_expression(expr.iteratorsTags[soa_index])
                new_for.sources |> emplace_new <| new ExprField(at = expr.sources[soa_index].at,
                    value <- clone_expression(expr.sources[soa_index]),
                    name := itn,
                    fieldFlags = ExprFieldFieldFlags.no_promotion
                )
            }
            new_for.iteratorVariables |> clear()
            return <- new_for_e
        }
        return <- default<ExpressionPtr>
    }
}

