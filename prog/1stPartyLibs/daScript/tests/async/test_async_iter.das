// Test: iterating async generators
// Tests for-loop over typed async generators, zip with count(),
// bool-typed async, and await_next_frame inside iteration.

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/async_boost

// ---- basic async range ----

[async]
def int_range() : int {
    yield 10
    yield 20
    yield 30
}

[test]
def test_async_int_range(t : T?) {
    t |> run("iterate typed async with for loop") @(t : T?) {
        var vals : array<int>
        for (v in int_range()) {
            if (v is res) {
                vals |> push(v as res)
            }
        }
        t |> equal(length(vals), 3)
        t |> equal(vals[0], 10)
        t |> equal(vals[1], 20)
        t |> equal(vals[2], 30)
    }
}

// ---- bool async range ----

[async]
def bool_range() : bool {
    yield true
    yield false
    yield true
}

[test]
def test_async_bool_range(t : T?) {
    t |> run("iterate bool async") @(t : T?) {
        var vals : array<bool>
        for (v in bool_range()) {
            if (v is res) {
                vals |> push(v as res)
            }
        }
        t |> equal(length(vals), 3)
        t |> equal(vals[0], true)
        t |> equal(vals[1], false)
        t |> equal(vals[2], true)
    }
}

// ---- zip with count ----

[async]
def indexed_range() : int {
    yield 100
    yield 200
    yield 300
}

[test]
def test_zip_with_count(t : T?) {
    t |> run("async generator zipped with count()") @(t : T?) {
        var indices : array<int>
        var values : array<int>
        for (idx, v in count(), indexed_range()) {
            indices |> push(idx)
            if (v is res) {
                values |> push(v as res)
            }
        }
        t |> equal(length(indices), 3)
        // indices include wait frames too, so check values
        t |> equal(length(values), 3)
        t |> equal(values[0], 100)
        t |> equal(values[1], 200)
        t |> equal(values[2], 300)
    }
}

// ---- async range with frame pauses ----

[async]
def spaced_range() : int {
    yield 1
    await_next_frame()
    yield 2
    await_next_frame()
    yield 3
}

[test]
def test_spaced_range(t : T?) {
    t |> run("async range with await_next_frame between yields") @(t : T?) {
        var vals : array<int>
        var total_frames = 0
        for (v in spaced_range()) {
            total_frames ++
            if (v is res) {
                vals |> push(v as res)
            }
        }
        t |> equal(length(vals), 3)
        t |> equal(vals[0], 1)
        t |> equal(vals[1], 2)
        t |> equal(vals[2], 3)
        // 3 values + 2 waits = 5 total iterations
        t |> equal(total_frames, 5)
    }
}

// ---- empty async range ----

[async]
def empty_range() : int {
    return false
}

[test]
def test_empty_range(t : T?) {
    t |> run("empty async range produces no values") @(t : T?) {
        var count = 0
        for (v in empty_range()) {
            count ++
        }
        t |> equal(count, 0)
    }
}

// ---- single yield ----

[async]
def single_value() : int {
    yield 42
}

[test]
def test_single_value(t : T?) {
    t |> run("async with single yield") @(t : T?) {
        var vals : array<int>
        for (v in single_value()) {
            if (v is res) {
                vals |> push(v as res)
            }
        }
        t |> equal(length(vals), 1)
        t |> equal(vals[0], 42)
    }
}

// ---- string async range ----

[async]
def string_range() : string {
    yield "alpha"
    yield "beta"
    yield "gamma"
}

[test]
def test_string_range(t : T?) {
    t |> run("string-typed async range") @(t : T?) {
        var vals : array<string>
        for (v in string_range()) {
            if (v is res) {
                vals |> push(v as res)
            }
        }
        t |> equal(length(vals), 3)
        t |> equal(vals[0], "alpha")
        t |> equal(vals[1], "beta")
        t |> equal(vals[2], "gamma")
    }
}
