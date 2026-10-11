options gen2

require dastest/testing_boost public
require daslib/fio

//! the AOT build of this file is the proof: generated C++ calls builtin_exit_now, which the public
//! header must declare; the call itself never runs, since argv is never empty under the CLI

[test]
def test_exit_now_binds_under_aot(t : T?) {
    t |> run("exit_now compiles into generated C++ against the public header") @(t : T?) {
        let args <- get_command_line_arguments()
        if (empty(args)) {
            unsafe(exit_now(3))
        }
        t |> success(!empty(args), "the process is still here: argv carried the interpreter")
    }
}
