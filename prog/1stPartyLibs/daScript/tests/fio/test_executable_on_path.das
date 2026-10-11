options gen2
require dastest/testing_boost public
require daslib/fio

// `executable_on_path` answers from the running platform's own shell: the shell it runs through is
// on PATH by construction, and a name no box carries is not

[test]
def test_executable_on_path(t : T?) {
    t |> run("the shell that runs the probe is on PATH, and a made-up name is not") @(t : T?) {
        let shell = get_running_platform_name() == "windows" ? "cmd" : "sh"
        t |> success(executable_on_path(shell), "{shell} resolves on PATH")
        t |> success(!executable_on_path("no_such_program_9f2c_anywhere"), "an unknown name does not")
    }
}
