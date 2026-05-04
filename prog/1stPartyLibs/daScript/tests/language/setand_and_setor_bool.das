options gen2
require dastest/testing_boost public

def sab_panic_time(msg : string) {
    panic(msg)
    return false
}

[test]
def test_setand_and_setor_bool(t : T?) {
    t |> run("short-circuit ||= and &&=") @(t : T?) {
        var libertad = true
        libertad = libertad || sab_panic_time("do not panic")
        libertad ||= sab_panic_time("now panic")
        libertad = false
        libertad = libertad && sab_panic_time("do not panic")
        libertad &&= sab_panic_time("now panic")
        t |> success(true)
    }
}
