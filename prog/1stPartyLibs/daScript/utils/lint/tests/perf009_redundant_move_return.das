options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
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
//
// Clone variant (`var x := src; return <- x`): the move-out of a clone collapses
// to `return clone_to_move(src)` — NOT `return <- src`, which would move/destroy
// the source. Suggested separately.

expect 31208:4

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

def bad_clone_return() : array<int> {
    var src <- [1, 2, 3]
    var result := src                       // PERF009 (clone-init then move-return)
    return <- result
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

def good_clone_used_before_return() : array<int> {
    // Clone-init, but used before return — not a redundant pass-through.
    var src <- [1, 2, 3]
    var result := src
    result |> push(4)
    return <- result
}
