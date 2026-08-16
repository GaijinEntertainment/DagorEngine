options gen2
options auto_inline_functions = false
options multiple_contexts

// LINT016 stays silent here: multiple_contexts makes := and push_clone perform
// the string clone that their spelling promises.

require daslib/lint

var global_clone := "global"

def clone_for_context(var dst : string; var arr : array<string>; src : string) {
    var local := src
    dst := src
    print(local)
    local = dst
    arr |> push_clone(src)
    print(local)
}

[export]
def main() {
}
