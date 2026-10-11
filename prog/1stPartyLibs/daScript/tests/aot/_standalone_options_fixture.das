options gen2
options stack = 262144
options heap_size_hint = 123456
options string_heap_size_hint = 234567

[export]
def options_probe : int {
    return 1
}

[no_aot]
def unused_interpreted_helper(x : int) : int {
    return x - 1
}
