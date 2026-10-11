options gen2

require _standalone_macro_module
require strings

// 5050 is summed at compile time; the runtime only ever sees the number.
[export]
def macro_baked_value : int {
    return bake_sum(100)
}

// reaches a C++ module beyond the builtin one, so the registration walk runs for this fixture
[export]
def macro_baked_text(prefix : string) : string {
    return to_upper(prefix) + "{macro_baked_value()}"
}
