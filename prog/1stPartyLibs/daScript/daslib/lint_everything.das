options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module lint_everything shared private

require daslib/ast_boost
require daslib/lint

[global_lint_macro]
class LintEverythingGlobal : AstPassMacro
    def override apply ( prog:ProgramPtr; mod:Module? ) : bool
        paranoid(prog, true)
        return true
