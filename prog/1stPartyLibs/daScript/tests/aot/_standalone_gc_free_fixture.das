options gen2

// the same program as _standalone_gc_fixture.das without options gc - a global, no collector

var g_counter = 0

[export]
def count_up : int {
    g_counter++
    return g_counter
}
