options gen2
options no_aot

// Tutorial macro module: pass macro (AstPassMacro).
//
// AstPassMacro is a whole-program macro invoked during compilation.
// It has a single method:
//   apply(prog : ProgramPtr; mod : Module?) : bool
// Five annotations control WHEN the macro runs:
//   [infer_macro]        — after clean inference (returning true re-infers)
//   [dirty_infer_macro]  — during each dirty inference pass
//   [lint_macro]         — after each module is compiled (per-module)
//   [global_lint_macro]  — once after the entire program is compiled
//   [optimization_macro] — during optimization passes
//
// Section 1 — [lint_macro]: invoked for each module compiled after this
//   macro module.  Uses `compiling_module()` to inspect the current
//   module.  Read-only — cannot modify the AST.
//
// Section 2 — [infer_macro]: instruments every function body with a
//   _trace_enter() call, following the heartbeat.das pattern.
//   Returns true when changes are made so the compiler re-infers.

module pass_macro_mod

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost

// ---- Section 1: lint_macro — per-module analysis ---------------------------

[lint_macro]
class CodeStatsLint : AstPassMacro {
    //! Lint pass invoked for each module compiled after this one.
    //! Uses compiling_module() to inspect the current module only.
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        // compiling_module() returns the module being compiled right now.
        // This is NOT the same as `mod`, which is the module that owns
        // this macro (pass_macro_mod).
        let cm = compiling_module()
        let WARN_THRESHOLD = 4
        cm |> for_each_function("") <| $(var func : FunctionPtr) {
            if (func.body == null || !(func.body is ExprBlock)) {
                return
            }
            let body = func.body as ExprBlock
            let nStmts = length(body.list)
            if (nStmts > WARN_THRESHOLD) {
                print("[lint] '{func.name}' has {nStmts} top-level statements (>{WARN_THRESHOLD})\n")
            }
        }
        return false  // lint macros don't modify the AST
    }
}

// ---- Section 2: infer_macro — AST transformation ---------------------------

def public _trace_enter(name : string) {
    //! Prints a trace message when entering a function.
    print(">>> {name}\n")
}

class TraceCallsVisitor : AstVisitor {
    //! AST visitor that inserts _trace_enter() at the start of every function.
    astChanged : bool = false
    @do_not_delete func : Function?
    def override preVisitFunction(var fun : FunctionPtr) {
        func = get_ptr(fun)
    }
    def override visitFunction(var fun : FunctionPtr) : FunctionPtr {
        // Skip our own helper to avoid infinite recursion at runtime.
        if (string(fun.name) == "_trace_enter") {
            func = null
            return <- fun
        }
        if (fun.body == null || !(fun.body is ExprBlock)) {
            func = null
            return <- fun
        }
        var body = fun.body as ExprBlock
        if (length(body.list) == 0) {
            func = null
            return <- fun
        }
        // Idempotency: skip if already instrumented.
        if ((body.list[0] is ExprCall) && (body.list[0] as ExprCall).name == "_trace_enter") {
            func = null
            return <- fun
        }
        // Insert _trace_enter("function_name") at the beginning of the body.
        let fname = string(fun.name)
        var inscope expr <- qmacro(_trace_enter($v(fname)))
        body.list |> emplace(expr, 0)
        astChanged = true
        func.not_inferred()
        func = null
        return <- fun
    }
}

[infer_macro]
class TraceCallsPass : AstPassMacro {
    //! Pass macro that inserts trace calls at every function entry.
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        var astVisitor = new TraceCallsVisitor()
        var inscope astVisitorAdapter <- make_visitor(*astVisitor)
        visit(prog, astVisitorAdapter)
        var result = astVisitor.astChanged
        unsafe {
            delete astVisitor
        }
        return result
    }
}
