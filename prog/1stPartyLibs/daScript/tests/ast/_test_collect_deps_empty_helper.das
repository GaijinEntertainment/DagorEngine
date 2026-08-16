// Helper module: declares the [function_macro] that exercises
// collect_dependencies on its annotated function. Lives in a separate module
// so the macro is registered BEFORE the test file annotates a function with it
// (within a single file the registration and the annotation happen in the same
// inference pass and the typer rejects the annotation as "not a function macro").

options gen2
options indenting = 4

module _test_collect_deps_empty_helper public

require daslib/ast public
require daslib/ast_boost

// Counters captured by the function macro. Bumped on patch(). The test reads them after
// compilation and asserts both are zero.
var public g_empty_fn_count : int = -1
var public g_empty_var_count : int = -1

[function_macro(name="probe_empty_deps")]
class EmptyDepsProbe : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        return true
    }
    def override patch(var func : FunctionPtr; var group : ModuleGroup;
                       args, progArgs : AnnotationArgumentList; var errors : das_string;
                       var astChanged : bool&) : bool {
        // Drive collect_dependencies on a function whose body is genuinely empty. Before the
        // src/simulate/simulate_fn_hash.cpp fix, the C++ side leaves Array.magic uninitialised
        // when the dep list is empty, and `for (... in ...)` panics with
        // "array magic mismatch on lock" on the FIRST iteration line.
        var fn_count = 0
        var var_count = 0
        collect_dependencies(func) $(vfun, vvar) {
            for (_ in vfun) { fn_count++ }
            for (_ in vvar) { var_count++ }
        }
        g_empty_fn_count = fn_count
        g_empty_var_count = var_count
        return true
    }
}
