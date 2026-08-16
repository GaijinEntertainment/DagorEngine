options gen2
options persistent_heap
require dastest/testing_boost
require strings
require daslib/strings_boost
require daslib/md_boost
require daslib/fio

// Sweep-batch coverage for [temp_string_result]: flatness rows per flagged family, and
// row-(b) passthrough traps - identity-shaped inputs whose freshness rides an argument
// precondition; a future `return input` shortcut in the impl fails these loudly.
//
// Rows measure RESULT reclaim - the arc's contract. Two shapes deliberately NOT asserted
// flat: a call with TWO builder args queues only the rightmost (one site per call, arg
// order is unspecified for externs), and a builder passed to a string-returning das
// function is never temp-marked (such functions are conservatively captureString).

[test]
def test_sweep_flag_flatness(t : T?) {
    t |> run("fio path helpers reclaim in a loop") @@(t) {
        let before = string_heap_bytes_allocated()
        var summ = 0
        for (i in range(20000)) {
            summ += length(base_name("/tmp/dir{i}/file.txt"))
            summ += length(dir_name("/tmp/dir{i}/file.txt"))
            summ += length(path_join("root", "leaf{i}.das"))
        }
        let after = string_heap_bytes_allocated()
        t |> success(summ > 0)
        t |> success(int64(after) - int64(before) < 512l, "path helper temps should be reclaimed")
    }

    t |> run("sprint and fmt reclaim in a loop") @@(t) {
        let before = string_heap_bytes_allocated()
        var summ = 0
        for (i in range(20000)) {
            summ += length(sprint(i, print_flags.singleLine))
            summ += length(fmt(":d", i)) // fmt wraps its arg in {} - fmtlib syntax, not printf
        }
        let after = string_heap_bytes_allocated()
        t |> success(summ > 0)
        t |> success(int64(after) - int64(before) < 512l, "sprint/fmt temps should be reclaimed")
    }

    t |> run("propagated daslib producers reclaim in a loop") @@(t) {
        let parts <- [ "alpha", "beta", "gamma" ]
        let before = string_heap_bytes_allocated()
        var summ = 0
        for (_i in range(5000)) {
            summ += length(join(parts, ","))
            summ += length(md_escape("plain [text] with no pipes"))
            summ += length(to_generic_path("a\\b\\c.txt"))
        }
        let after = string_heap_bytes_allocated()
        t |> success(summ > 0)
        t |> success(int64(after) - int64(before) < 512l, "daslib producer temps should be reclaimed")
    }
}

[test]
def test_sweep_passthrough_traps(t : T?) {
    t |> run("identity inputs in temp position leave the source intact") @@(t) {
        // md_escape/to_generic_path freshness rides replace()'s non-empty search
        // argument; join's rides its builder. If any impl grows a `return input`
        // shortcut, the source below gets queued and the churn reuses its cell.
        let keep_md = clone_string("no pipes here")
        let keep_path = clone_string("already/generic.txt")
        let parts <- [ clone_string("solo") ]
        var summ = 0
        for (_i in range(500)) {
            summ += length(md_escape(keep_md))
            summ += length(to_generic_path(keep_path))
            summ += length(join(parts, ","))
        }
        for (i in range(2000)) {
            summ += length(string(7000000 + i))
        }
        t |> success(summ > 0)
        t |> equal(keep_md, "no pipes here")
        t |> equal(keep_path, "already/generic.txt")
        t |> equal(parts[0], "solo")
    }

    t |> run("base_name identity shape leaves the source intact") @@(t) {
        // base_name of a bare name (no separators) must still be a fresh copy
        let keep = clone_string("bare_name.txt")
        var summ = 0
        for (_i in range(500)) {
            summ += length(base_name(keep))
        }
        for (i in range(2000)) {
            summ += length(string(8000000 + i))
        }
        t |> success(summ > 0)
        t |> equal(keep, "bare_name.txt")
    }
}


