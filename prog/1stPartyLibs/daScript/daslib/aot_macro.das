options gen2
module aot_macro

options no_aot
require daslib/aot_cpp
require daslib/macro_boost
require daslib/templates_boost
require daslib/ast_boost
require strings
require daslib/fio


//! Simulate macro that performs AOT compilation and writes output to the policy-specified file.
[simulate_macro(name="ast_aot")]
class AstAot : AstSimulateMacro {
    //! Simulate macro that triggers AOT C++ code generation.
    def override simulate(prog : Program?; var ctx : Context?) : bool {
        //! Runs AOT compilation of the program and writes the result to the output file specified in policies.
        return true if (is_in_completion() || is_compiling_macros() || is_folding())
        let res = run_aot(prog, ctx, prog.policies)
        var is_ok = res |> length() > 0
        if (is_ok) {
            let fileName = "{prog.policies.aot_result}"
            print("Aot to {fileName}\n")
            fopen(fileName, "wb") $(fw) {
                if (fw != null) {
                    fwrite(fw, res)
                } else {
                    is_ok = false
                    print("Couldn't create output file {fileName}\n")
                }
            }
        } else {
            print("Somehow aot result is empty.\n")
        }
        return is_ok
    }
}

