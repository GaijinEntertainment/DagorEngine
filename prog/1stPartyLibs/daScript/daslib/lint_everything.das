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

[global_lint_macro]
class LintEverythingGlobal : AstPassMacro {
    //! Global pass macro that enables lint checking across all modules.
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        paranoid(prog, true)
        return true
    }
}
