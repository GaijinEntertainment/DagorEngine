options gen2
// PERF004: String interpolation reassignment in loop
//
// Problem:
//   `str = "{str}{more}"` inside a loop where `str` is defined outside the loop
//   has the same O(n^2) behavior as `str += "..."`. Each iteration allocates a
//   new string containing all previous content plus the new part.
//
// Bad pattern:
//   var result = ""
//   for (i in range(100)) {
//       result = "{result}x"    // O(n^2) — same as +=
//   }
//
// Good pattern:
//   let result = build_string() <| $(var writer) {
//       for (i in range(100)) {
//           write(writer, "x")
//       }
//   }

require strings

def bad_string_builder_for() : string {
    var result = ""
    for (i in range(100)) {
        result = "{result}x"            // PERF004
    }
    return result
}

def bad_string_builder_while() : string {
    var result = ""
    var i = 0
    while (i < 100) {
        result = "{result}x"            // PERF004
        i ++
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

def good_unrelated_string_builder(suffix : string) : string {
    var result = ""
    for (i in range(100)) {
        result = "{suffix}_{i}"         // Does NOT reference result itself — no PERF004
    }
    return result
}
