options gen2

// spawned by test_build_const.das with controlled argv/env; never required, never run by dastest.
// resolves every accessor at compile time and reports the folded values via the --out file.
require daslib/build_const
require daslib/fio

let PROF = build_flag("--bct-prof", "BCT_PROF")
let NAME = build_value("--bct-name", "BCT_NAME", "fallback")
let LEVEL = build_int("--bct-level", "BCT_LEVEL", 7)
let HAS = build_has_arg("--bct-has")
let AVAL = build_arg_value("--bct-aval", "adef")
let AINT = build_arg_int("--bct-aint", 1)
let EFLAG = build_env_flag("BCT_EFLAG")
let EVAL = build_env_value("BCT_EVAL", "edef")
let EINT = build_env_int("BCT_EINT", 2)
let OUT = build_arg_value("--out", "")

def gate_state() : string {
    static_if (PROF) {
        return "on"
    } else {
        return "off"
    }
}

[export]
def main() {
    if (empty(OUT)) {
        return
    }
    fopen(OUT, "wb") $(f) {
        if (f != null) {
            f |> fwrite("PROF={PROF};NAME={NAME};LEVEL={LEVEL};HAS={HAS};AVAL={AVAL};AINT={AINT};EFLAG={EFLAG};EVAL={EVAL};EINT={EINT};GATE={gate_state()}")
        }
    }
}
