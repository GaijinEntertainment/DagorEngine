options gen2
// PERF001: String concatenation with += in a loop
//
// Problem:
//   Using `str += "text"` inside a loop where `str` is defined outside the loop
//   creates O(n^2) total allocations. Each += allocates a new string of increasing
//   length, copying all previous content.
//
// Bad pattern:
//   var result = ""
//   for (i in range(100)) {
//       result += "x"       // O(n^2) — new allocation every iteration
//   }
//
// Good pattern:
//   let result = build_string() <| $(var writer) {
//       for (i in range(100)) {
//           write(writer, "x")  // O(n) — appends to a single buffer
//       }
//   }

require strings

def bad_for_loop() : string {
    var result = ""
    for (i in range(100)) {
        result += "x"           // PERF001
    }
    return result
}

def bad_while_loop() : string {
    var result = ""
    var i = 0
    while (i < 100) {
        result += "x"           // PERF001
        i ++
    }
    return result
}

def bad_nested_loop() : string {
    var result = ""
    for (i in range(10)) {
        for (j in range(10)) {
            result += "x"       // PERF001
        }
    }
    return result
}

// --- Good patterns (no warnings) ---

def good_build_string() : string {
    return build_string() <| $(var writer) {
        for (i in range(100)) {
            write(writer, "x")
        }
    }
}

def good_local_string_in_loop() : string {
    var last = ""
    for (i in range(100)) {
        var local_str = ""
        local_str += "x"       // local to loop body — no warning
        last = local_str
    }
    return last
}
