options gen2
// STYLE002: <| pipe before defer block
//
// Problem: defer <| $() { ... } uses unnecessary pipe.
//
// Bad pattern:
//   defer <| $() { cleanup() }
//
// Good pattern:
//   defer() { cleanup() }

require daslib/defer

// bad: pipe before defer block (2 warnings)
def bad_defer_pipe() {
    var x = 1
    defer <| $() {
        x = 0
    }
    defer <| $() {
        print("")
    }
}

// good: direct trailing block
def good_defer() {
    var x = 1
    defer() {
        x = 0
    }
}
