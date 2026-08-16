options gen2
options no_fast_call   // keep the das frames distinct so a captured stack is readable

// Deliberate crash generator — see README.md.
//
//   daslang main.das -- --kind null-write          # interpreter
//   daslang -jit main.das -- --kind null-write     # JIT
//
// The nested call chain is deliberate: it gives the crash handler several das frames to walk, so a
// captured stack shows whether frame recovery works, not just that a fault happened.

require crash_kinds
require daslib/clargs
require daslib/fio                    // sleep

[CommandLineArgs]
struct CrashArgs {
    @clarg_short = "k"
    @clarg_doc = "Failure mode to produce (default: none)"
    kind : CrashKind

    @clarg_short = "c"
    @clarg_doc = "Exit code for --kind das-exit-code / das-fio-exit (default: 1)"
    code : int = 1

    @clarg_short = "d"
    @clarg_doc = "Seconds to run before failing, to mimic a long-lived service"
    delay : int

    @clarg_short = "?"
    @clarg_name = "show-help"
    @clarg_doc = "Show this help and exit"
    help : bool
}

def private deep_function(kind : CrashKind; code : int) : int {
    return run_crash_kind(kind, code)
}

def private middle_function(kind : CrashKind; code : int) : int {
    return deep_function(kind, code)
}

def private outer_function(kind : CrashKind; code : int) : int {
    return middle_function(kind, code)
}

[export]
def main : int {
    var r <- parse_args(type<CrashArgs>)
    if (r |> is_err) {
        print("error: {r |> unwrap_err}\n\n")
        print_help(get_command_info(type<CrashArgs>), "crash")
        return 2
    }
    let cfg <- r |> move_unwrap
    if (cfg.help) {
        print_help(get_command_info(type<CrashArgs>), "crash")
        return 0
    }
    print("crash: kind={cfg.kind} native={crash_kind_is_native(cfg.kind)}\n")
    if (cfg.delay > 0) {
        print("crash: running {cfg.delay}s before failing\n")
        sleep(uint(cfg.delay) * 1000u)
    }
    print("crash: failing now\n")
    let rc = outer_function(cfg.kind, cfg.code)
    print("crash: returned {rc} without failing (kind={cfg.kind})\n")
    return rc
}
