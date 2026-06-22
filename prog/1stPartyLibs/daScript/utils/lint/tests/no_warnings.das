options gen2
// No warnings test — exercises correct patterns for all lint rules.
// Should produce zero warnings from paranoid, perf, and style lint.

require strings

// --- perf_lint: correct patterns ---

def good_build_string() : string {
    return build_string() $(var writer) {
        for (i in range(100)) {
            write(writer, "item {i}\n")
        }
    }
}

def good_peek_data_iteration(s : string) : int {
    var count = 0
    peek_data(s) $(arr) {
        for (i in range(length(arr))) {
            if (int(arr[i]) == 'o') {
                count ++
            }
        }
    }
    return count
}

def good_modify_data(s : string) : string {
    return modify_data(s) $(var arr) {
        for (i in range(length(arr))) {
            arr[i] = uint8(int(arr[i]) - 32)
        }
    }
}

def good_cached_length_while(s : string) : int {
    let slen = length(s)
    var i = 0
    while (i < slen) {
        i ++
    }
    return i
}

def good_for_range_length(s : string) : int {
    var total = 0
    for (i in range(length(s))) {
        total += i
    }
    return total
}

def good_string_concat_outside_loop() : string {
    var s = "hello"
    s += " world"      // Not in a loop — no warning
    return s
}

def good_string_builder_no_self_ref(prefix : string) : string {
    var result = ""
    for (i in range(10)) {
        result = "{prefix}_{i}"   // Does not reference `result` — no PERF004
    }
    return result
}

// --- style_lint: correct patterns ---

def good_trailing_block() {
    let s = build_string() $(var w) {
        w |> write("hello")
    }
}

def good_bare_block() {
    var x = 1
    {
        x = 2
    }
}

def good_conditional() {
    let x = 5
    if (x > 0) {
        print("positive\n")
    }
}
