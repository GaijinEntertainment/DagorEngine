options gen2
options indenting = 4

// Helper module for test_collect_deps_closure.das: the [probe_deps] function macro lives in
// its own module so it is registered before the test file annotates a function with it.

module _test_collect_deps_closure_helper public

require daslib/ast public
require daslib/ast_boost
require daslib/strings_boost

//! [probe_deps(functions = "a,b", variables = "g")]: patch() collects the annotated function's
//! dependencies and fails the compile unless the sorted names equal the annotation's lists
[function_macro(name = "probe_deps")]
class DepsProbe : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; var errors : das_string) : bool {
        return true
    }
    def override patch(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; progArgs : AnnotationArgumentList; var errors : das_string;
                       var astChanged : bool&) : bool {
        let want_fn = args |> find_arg("functions") ?as tString ?? ""
        let want_var = args |> find_arg("variables") ?as tString ?? ""
        var fns : array<string>
        var vars : array<string>
        collect_dependencies(func) $(vfun, vvar) {
            for (f in vfun) {
                fns |> push("{f.name}")
            }
            for (v in vvar) {
                vars |> push("{v.name}")
            }
        }
        sort(fns)
        sort(vars)
        let got_fn = join(fns, ",")
        let got_var = join(vars, ",")
        if (got_fn != want_fn || got_var != want_var) {
            errors := "probe_deps on {func.name}: functions [{got_fn}] expected [{want_fn}], variables [{got_var}] expected [{want_var}]"
            return false
        }
        return true
    }
}
