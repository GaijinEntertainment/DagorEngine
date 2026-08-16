options gen2
options no_aot

// Fixtures for the failed_* malformed-tree tests: each macro produces one specific
// malformation that infer must report (sticky error) instead of crashing or silently
// repairing. Trigger is the source call name, so one module serves all the tests.

module _malformed_tree_macros

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost

def find_named_source(expr : ExprFor?; nm : string) : int {
    for (index, src in count(), expr.sources) {
        if (src is ExprCall) {
            if ((src as ExprCall).name == nm) {
                return index
            }
        }
    }
    return -1
}

def rewrite_to_range(var new_for : ExprFor?; expr : ExprFor?; idx : int) {
    let src_call = expr.sources[idx] as ExprCall
    var range_call = new ExprCall(at = src_call.at, name := "range")
    for (a in src_call.arguments) {
        range_call.arguments |> emplace_new <| clone_expression(a)
    }
    new_for.sources[idx] = range_call
    new_for.iteratorVariables |> clear()
}

// iteratorsAt/iteratorsAka left shorter than iterators (silent OOB before the guard)
[for_loop_macro(name=short_iters_loop)]
class ShortItersLoop : AstForLoopMacro {
    def override visitExprFor(prog : ProgramPtr; mod : Module?; expr : ExprFor?) : ExpressionPtr {
        if (is_in_completion()) return default<ExpressionPtr>
        let idx = find_named_source(expr, "short_iters_range")
        if (idx == -1) return default<ExpressionPtr>
        var new_for_e = clone_expression(expr)
        var new_for = new_for_e as ExprFor
        rewrite_to_range(new_for, expr, idx)
        new_for.iteratorsAka |> clear()
        new_for.iteratorsAt |> clear()
        return new_for_e
    }
}

// for loop with null body
[for_loop_macro(name=nobody_loop)]
class NobodyLoop : AstForLoopMacro {
    def override visitExprFor(prog : ProgramPtr; mod : Module?; expr : ExprFor?) : ExpressionPtr {
        if (is_in_completion()) return default<ExpressionPtr>
        let idx = find_named_source(expr, "nobody_range")
        if (idx == -1) return default<ExpressionPtr>
        var new_for_e = clone_expression(expr)
        var new_for = new_for_e as ExprFor
        rewrite_to_range(new_for, expr, idx)
        new_for.body = default<ExpressionPtr>
        return new_for_e
    }
}

// null entry in ExprLet.variables
[for_loop_macro(name=nullvar_loop)]
class NullVarLoop : AstForLoopMacro {
    def override visitExprFor(prog : ProgramPtr; mod : Module?; expr : ExprFor?) : ExpressionPtr {
        if (is_in_completion()) return default<ExpressionPtr>
        let idx = find_named_source(expr, "nullvar_range")
        if (idx == -1) return default<ExpressionPtr>
        var new_for_e = clone_expression(expr)
        var new_for = new_for_e as ExprFor
        rewrite_to_range(new_for, expr, idx)
        var let_expr = new ExprLet(at = expr.at)
        let_expr.variables |> push(default<VariablePtr>)
        var body = new_for.body as ExprBlock
        body.list |> push(let_expr, 0)
        return new_for_e
    }
}

// valid rewrite, but the macro reports a sticky error — the error must survive
// the repaired-tree re-infer and still fail the compile
[for_loop_macro(name=sticky_loop)]
class StickyLoop : AstForLoopMacro {
    def override visitExprFor(prog : ProgramPtr; mod : Module?; expr : ExprFor?) : ExpressionPtr {
        if (is_in_completion()) return default<ExpressionPtr>
        let idx = find_named_source(expr, "sticky_range")
        if (idx == -1) return default<ExpressionPtr>
        var new_for_e = clone_expression(expr)
        var new_for = new_for_e as ExprFor
        rewrite_to_range(new_for, expr, idx)
        macro_sticky_error(prog, expr.at, "sticky_range: reporting a sticky macro error on a valid tree")
        return new_for_e
    }
}

// generated function with null result type
[structure_macro(name=null_result_fn)]
class NullResultFn : AstStructureAnnotation {
    def override apply(var st : StructurePtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        var fn = qmacro_function("broken`result`fn") $() {
            pass
        }
        fn.result = default<TypeDeclPtr>
        add_function(st._module, fn)
        return true
    }
}
