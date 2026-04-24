options gen2
// PERF009: Redundant move-init variable immediately returned
//
// Problem:
//   `var x <- expr(); return <- x` introduces an unnecessary intermediate
//   variable. The value is moved into x and then immediately moved out.
//   This can be simplified to `return <- expr()`.
//
// Bad pattern:
//   var inscope result <- make_thing()
//   return <- result
//
// Good pattern:
//   return <- make_thing()

expect 40217:3

require daslib/perf_lint

struct Thing {
    data : array<int>
}

def make_thing() : Thing {
    return <- Thing()
}

// --- Bad patterns (warnings) ---

def bad_simple() : Thing {
    var inscope result <- make_thing()
    return <- result                        // PERF009
}

def bad_array() : array<int> {
    var result <- [1, 2, 3]
    return <- result                        // PERF009
}

def bad_table() : table<string; int> {
    var result <- { "a" => 1 }
    return <- result                        // PERF009
}

// --- Good patterns (no warnings) ---

def good_direct_return() : Thing {
    return <- make_thing()
}

def good_used_before_return() : Thing {
    var inscope result <- make_thing()
    print("got thing\n")
    return <- result
}

def good_copy_init() : int {
    var result = 42
    return result
}

def good_conditional_return() : Thing {
    var inscope result <- make_thing()
    if (true) {
        return <- result
    }
    return <- Thing()
}
