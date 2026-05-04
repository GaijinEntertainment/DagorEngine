options gen2
options no_aot

// Tutorial macro module: for-loop macro that enables
//   for ((k,v) in tab)
// syntax on tables.  Rewrites to
//   for (k, v in keys(tab), values(tab))

module for_loop_macro_mod

require daslib/ast
require daslib/ast_boost
require strings

[for_loop_macro(name=table_kv)]
class TableKVForLoop : AstForLoopMacro {
    //! Transforms  for ((k,v) in tab)  into  for (k, v in keys(tab), values(tab)).
    def override visitExprFor(prog : ProgramPtr; mod : Module?; expr : smart_ptr<ExprFor>) : ExpressionPtr {
        if (is_in_completion()) {
            return <- default<ExpressionPtr>
        }
        // Find a tuple-expanded iterator whose source is a table
        var tab_index = -1
        for (index, src in count(), expr.sources) {
            if (index < int(expr.iteratorsTupleExpansion |> length)) {
                if (int(expr.iteratorsTupleExpansion[index]) != 0 && src._type != null && src._type.isGoodTableType) {
                    tab_index = index
                    break
                }
            }
        }
        if (tab_index == -1) {
            return <- default<ExpressionPtr>
        }
        // Split the backtick-joined name "k`v" into key_name and val_name
        let joined_name = string(expr.iterators[tab_index])
        let bt = find(joined_name, "`")
        if (bt < 0 || find(joined_name, "`", bt + 1) >= 0) {
            return <- default<ExpressionPtr>  // need exactly 2 parts
        }
        let key_name = slice(joined_name, 0, bt)
        let val_name = slice(joined_name, bt + 1)
        // Clone the for expression
        var inscope new_for_e <- clone_expression(expr)
        var new_for = new_for_e as ExprFor
        // Erase the table entry from all parallel vectors
        let source_at = expr.sources[tab_index].at
        new_for.sources |> erase(tab_index)
        new_for.iterators |> erase(tab_index)
        new_for.iteratorsAt |> erase(tab_index)
        new_for.iteratorsAka |> erase(tab_index)
        new_for.iteratorsTags |> erase(tab_index)
        new_for.iteratorsTupleExpansion |> erase(tab_index)
        // Add keys(tab) and values(tab) as new sources
        new_for.sources |> emplace_new <| make_kv_call("keys", expr.sources[tab_index], source_at)
        new_for.sources |> emplace_new <| make_kv_call("values", expr.sources[tab_index], source_at)
        // Add key and value iterator names
        let si = new_for.iterators |> length
        new_for.iterators |> resize(si + 2)
        new_for.iterators[si] := key_name
        new_for.iterators[si + 1] := val_name
        new_for.iteratorsAka |> resize(si + 2)
        new_for.iteratorsAka[si] := ""
        new_for.iteratorsAka[si + 1] := ""
        new_for.iteratorsAt |> push(expr.iteratorsAt[tab_index])
        new_for.iteratorsAt |> push(expr.iteratorsAt[tab_index])
        new_for.iteratorsTags |> emplace_new <| clone_expression(expr.iteratorsTags[tab_index])
        new_for.iteratorsTags |> emplace_new <| clone_expression(expr.iteratorsTags[tab_index])
        new_for.iteratorsTupleExpansion |> push(0u8)
        new_for.iteratorsTupleExpansion |> push(0u8)
        // Clear iterator variables for re-inference
        new_for.iteratorVariables |> clear()
        return <- new_for_e
    }
}

def make_kv_call(fn_name : string; src_expr : ExpressionPtr; at : LineInfo) : ExpressionPtr {
    var inscope call <- new ExprCall(at = at, name := fn_name)
    call.arguments |> emplace_new <| clone_expression(src_expr)
    return <- call
}
