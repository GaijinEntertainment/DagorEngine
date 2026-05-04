options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
options persistent_heap

require dastest/testing_boost public
require daslib/defer
require daslib/static_let
require strings

// ============================================================
// defer tests
// ============================================================

[test]
def test_defer_basic(t : T?) {
    t |> run("defer executes on scope exit") @(t : T?) {
        var log = ""
        invoke() {
            defer() {
                log += "cleanup"
            }
            log += "work,"
        }
        t |> equal(log, "work,cleanup")
    }
    t |> run("defer LIFO ordering") @(t : T?) {
        var log = ""
        invoke() {
            defer() { log += "1"; }
            defer() { log += "2"; }
            defer() { log += "3"; }
        }
        t |> equal(log, "321")
    }
    t |> run("defer runs on early return") @(t : T?) {
        var log = ""
        invoke() {
            defer() { log += "cleaned"; }
            log += "before,"
            return
        }
        t |> equal(log, "before,cleaned")
    }
}

// ============================================================
// static_let tests
// ============================================================

def call_counter() : int {
    var count = 0
    static_let() {
        var counter = 0
    }
    counter ++
    count = counter
    return count
}

[test]
def test_static_let(t : T?) {
    t |> run("static_let persists across calls") @(t : T?) {
        // Reset by calling multiple times and verifying increment
        let a = call_counter()
        let b = call_counter()
        let c = call_counter()
        // Each call increments, so b > a and c > b
        t |> equal(b, a + 1)
        t |> equal(c, b + 1)
    }
}
