options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT012: a function argument the body never uses. An argument or local a DISCARDED `static_if`
// arm names is used in the source the author wrote - the module-gated test shape (`static_if
// (typeinfo builtin_module_exists(x)) { ...the arm that uses every argument... } else { t |>
// skip(...) }`) compiles without the module on a box that lacks it, and the skip arm must not
// read as forgetting the arguments. The fold marks those variables used (LINT012, and LINT003 on
// a `var` whose only write sits in the dropped arm); an argument neither arm names still reports.

expect 50503:2

require daslib/lint

def gated_uses_in_dropped_arm(t : int; rows, cols : int) : int {
    static_if (typeinfo builtin_module_exists(no_such_module_anywhere)) {
        return t + rows * cols          // the arm this build drops still names every argument
    } else {
        return t                        // rows, cols: silent
    }
}

def gated_var_written_in_dropped_arm(var acc : int; n : int) : int {
    var tripped = false                 // written only in the dropped arm: LINT003 stays silent
    static_if (typeinfo builtin_module_exists(no_such_module_anywhere)) {
        acc += n                        // the write the dropped arm holds keeps LINT003 off `acc`
        tripped = true
        return acc
    } else {
        return tripped ? n : n + 1
    }
}

def gated_never_named(t : int; unused_in_both : int) : int {   // LINT012 - neither arm names it
    static_if (typeinfo builtin_module_exists(no_such_module_anywhere)) {
        return t + 1
    } else {
        return t
    }
}

def plain_unused(t : int; unused_plain : int) : int {   // LINT012 - the control with no static_if
    return t
}

[export]
def main() {
    var acc = 1
    acc = gated_var_written_in_dropped_arm(acc, 2)
    print("{gated_uses_in_dropped_arm(1, 2, 3)} {acc} {gated_never_named(1, 2)} {plain_unused(1, 2)}\n")
}
