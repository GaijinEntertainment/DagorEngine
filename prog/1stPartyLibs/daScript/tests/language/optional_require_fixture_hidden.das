options gen2
// the module optional_require_fixture_private requires PRIVATELY: loaded into the program, so the
// process registry (builtin_module_exists) knows it, yet not visible from the program root
module optional_require_fixture_hidden shared public

def hidden_marker() : int {
    return 45
}
