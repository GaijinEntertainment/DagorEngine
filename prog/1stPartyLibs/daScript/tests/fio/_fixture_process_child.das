options gen2
options no_aot

require daslib/clargs
require daslib/fio

// Child for test_process.das: prints a few known lines (env override, working directory, two
// plain lines), signals ready, waits for the parent's release event, then exits with code 7.
// Codes come back from main - a das `exit()` unwinds as an abnormal termination and the CLI
// reports 1, so it cannot carry a code. The elapsed-time check is only a deadlock guard.
[export]
def main() : int {
    let args <- get_cli_arguments()
    if (length(args) != 2) return 2
    let ready = args[0]
    let release = args[1]
    var env_val = "<unset>"
    if (has_env_variable("DAS_PROC_TEST")) {
        env_val = get_env_variable("DAS_PROC_TEST")
    }
    print("env={env_val}\n")
    print("cwd={base_name(getcwd())}\n")
    print("hello one\n")
    print("hello two\n")
    if (!fwrite(ready, "ready")) return 3
    let started = ref_time_ticks()
    while (!fexist(release)) {
        if (get_time_usec(started) > 30000000) return 4
        sleep(5u)
    }
    return 7
}
