options gen2
options indenting = 4
options strict_smart_pointers = true

module lint_everything shared private

//! Global lint pass for paranoid checks on all modules.
//!
//! Like ``lint``, but applies paranoid diagnostics to every module in
//! the program, not just the one that requires it.

require daslib/ast_boost
require daslib/lint
require daslib/lint_config

[global_lint_macro]
class LintEverythingGlobal : AstPassMacro {
    //! Global pass macro that enables lint checking across all modules.
    // Same rule set as the per-module [lint_macro] path: default-off rules stay off
    // unless armed, and the repo config, environment and module overrides all apply.
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        paranoid(prog, true, build_lint_macro_disabled(prog))
        return true
    }
}
