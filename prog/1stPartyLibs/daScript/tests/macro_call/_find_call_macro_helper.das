// Helper module for tests/macro_call/test_find_call_macro_null.das.
//
// Pre-fix, find_call_macro(mod, "absent_name") null-deref'd in the C++
// binding (findModuleCallMacro in src/builtin/module_builtin_ast_adapters.cpp)
// because Module::findCall returns nullptr on miss. Now it returns null and
// callers can use it as a soft "is this name registered?" probe.
//
// This call_macro performs the compile-time probe and folds to a bool literal
// that the [test] in the sibling file asserts on.
options gen2
options indenting = 4

module _find_call_macro_helper public

require daslib/ast public
require daslib/ast_boost
require daslib/templates_boost

[call_macro(name = "probe_find_call_macro_missing")]
class private ProbeFindCallMacroMissing : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        let result = find_call_macro(mod, "definitely_not_a_registered_call_macro_xyz_42")
        let is_null = result == null
        return new ExprConstBool(at = call.at, value = is_null)
    }
}
