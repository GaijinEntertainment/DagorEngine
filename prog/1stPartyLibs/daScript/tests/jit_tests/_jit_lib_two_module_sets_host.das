options gen2

require _standalone_modules_fixture_c
require _jit_lib_guest_c

[export]
def main() : int {
    var mods = standalone_modules_fixture_create()
    if (mods == null) {
        print("the fio library refused to create: {standalone_modules_fixture_last_error(null)}\n")
        return 1
    }
    var guest = jit_lib_guest_create()
    if (guest == null) {
        print("the ast library refused to create: {jit_lib_guest_last_error(null)}\n")
        standalone_modules_fixture_destroy(mods)
        return 1
    }
    var bad = 0
    if (!standalone_modules_fixture_has_path_variable(mods)) {
        print("the fio library cannot read the environment\n")
        bad++
    }
    let answered = jit_lib_guest_probe(guest, 35)
    if (answered != 42) {
        print("the ast library answered {answered}\n")
        bad++
    }
    jit_lib_guest_destroy(guest)
    standalone_modules_fixture_destroy(mods)
    return bad
}
