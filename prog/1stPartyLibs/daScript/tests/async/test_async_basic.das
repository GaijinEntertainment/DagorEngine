// Test: async_boost basic functionality
// Tests [async] annotation, void and typed async functions,
// await_next_frame, yield, early return, and frame counting.

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/async_boost

// ---- void async ----

[async]
def void_task() : void {
    await_next_frame()
    await_next_frame()
}

[test]
def test_void_async(t : T?) {
    t |> run("void async produces iterator<bool>") @(t : T?) {
        var it <- void_task()
        var count = 0
        for (v in it) {
            count ++
        }
        // 2 await_next_frame + implicit yield on entry = at least 2 frames
        t |> success(count >= 2)
    }
    t |> run("void async drives with next()") @(t : T?) {
        var it <- void_task()
        var v : bool
        var count = 0
        while (next(it, v)) {
            count ++
        }
        t |> success(count >= 2)
    }
}

// ---- typed async (int) ----

[async]
def get_value(x : int) : int {
    await_next_frame()
    yield x * 2
}

[test]
def test_typed_async(t : T?) {
    t |> run("typed async yields variant<res:int;wait:bool>") @(t : T?) {
        var found_value = false
        var result = 0
        for (v in get_value(21)) {
            if (v is res) {
                result = v as res
                found_value = true
            }
        }
        t |> success(found_value)
        t |> equal(result, 42)
    }
}

// ---- typed async (struct) ----

struct Payload {
    x : int
    name : string
}

[async]
def get_payload(n : int) : Payload {
    await_next_frame()
    var p : Payload
    p.x = n
    p.name = "hello"
    yield <- p
}

[test]
def test_struct_async(t : T?) {
    t |> run("struct async yields and moves result") @(t : T?) {
        var found = false
        var px = 0
        var pname = ""
        for (v in get_payload(7)) {
            if (v is res) {
                px = (v as res).x
                pname = (v as res).name
                found = true
            }
        }
        t |> success(found)
        t |> equal(px, 7)
        t |> equal(pname, "hello")
    }
}

// ---- await_next_frame frame counting ----

[async]
def frame_counter(n : int) : void {
    for (i in range(n)) {
        await_next_frame()
    }
}

[test]
def test_frame_counting(t : T?) {
    t |> run("await_next_frame suspends for exactly N frames") @(t : T?) {
        var it <- frame_counter(5)
        var frames = 0
        for (v in it) {
            frames ++
        }
        t |> equal(frames, 5)
    }
}

// ---- early return ----

[async]
def early_return(do_return : bool) : void {
    await_next_frame()
    if (do_return) {
        return false
    }
    await_next_frame()
    await_next_frame()
}

[test]
def test_early_return(t : T?) {
    t |> run("return false exits early") @(t : T?) {
        var it <- early_return(true)
        var frames = 0
        for (v in it) {
            frames ++
        }
        // Should exit after 1 frame (the first await_next_frame before if)
        t |> equal(frames, 1)
    }
    t |> run("no early return completes normally") @(t : T?) {
        var it <- early_return(false)
        var frames = 0
        for (v in it) {
            frames ++
        }
        t |> equal(frames, 3)
    }
}

// ---- empty async ----

[async]
def empty_async() : void {
    pass
}

[test]
def test_empty_async(t : T?) {
    t |> run("empty async completes immediately") @(t : T?) {
        var it <- empty_async()
        var frames = 0
        for (v in it) {
            frames ++
        }
        t |> equal(frames, 0)
    }
}

// ---- typed async with multiple yields ----

[async]
def multi_yield() : int {
    yield 10
    yield 20
    yield 30
}

[test]
def test_multi_yield(t : T?) {
    t |> run("typed async yields multiple values") @(t : T?) {
        var vals : array<int>
        for (v in multi_yield()) {
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

// ---- typed async returning negative result ----

[async]
def get_or_default(x : int) : int {
    if (x < 0) {
        return false
    }
    await_next_frame()
    yield x
}

[test]
def test_typed_early_return(t : T?) {
    t |> run("typed async with return false yields nothing") @(t : T?) {
        var vals : array<int>
        for (v in get_or_default(-1)) {
            if (v is res) {
                vals |> push(v as res)
            }
        }
        t |> equal(length(vals), 0)
    }
    t |> run("typed async with valid input yields value") @(t : T?) {
        var vals : array<int>
        for (v in get_or_default(42)) {
            if (v is res) {
                vals |> push(v as res)
            }
        }
        t |> equal(length(vals), 1)
        t |> equal(vals[0], 42)
    }
}
