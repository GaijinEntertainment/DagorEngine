// Test: async await mechanics
// Tests let/var await, copy/move/clone assign await,
// void await, nested await, and await inside loops.

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/async_boost

// ---- helpers ----

[async]
def add_one(x : int) : int {
    await_next_frame()
    yield x + 1
}

[async]
def slow_add(x : int; y : int) : int {
    await_next_frame()
    await_next_frame()
    yield x + y
}

struct Data {
    value : int
    tag : string
}

[async]
def make_data(v : int; l : string) : Data {
    await_next_frame()
    var d : Data
    d.value = v
    d.tag = l
    yield <- d
}

[async]
def void_delay(n : int) : void {
    for (i in range(n)) {
        await_next_frame()
    }
}

// ---- let await (move assign) ----

[async]
def test_let_await_impl() : void {
    let a <- await <| add_one(10)
    assert(a == 11)
}

[test]
def test_let_await(t : T?) {
    t |> run("let x <- await typed async") @(t : T?) {
        var it <- test_let_await_impl()
        async_run(it)
        t |> success(true)
    }
}

// ---- var copy assign await ----

[async]
def test_copy_await_impl() : void {
    var a = 0
    a = await <| add_one(99)
    assert(a == 100)
}

[test]
def test_copy_await(t : T?) {
    t |> run("x = await copies result") @(t : T?) {
        var it <- test_copy_await_impl()
        async_run(it)
        t |> success(true)
    }
}

// ---- var move assign await ----

[async]
def test_move_await_impl() : void {
    var a = 0
    a <- await <| add_one(49)
    assert(a == 50)
}

[test]
def test_move_await(t : T?) {
    t |> run("x <- await moves result") @(t : T?) {
        var it <- test_move_await_impl()
        async_run(it)
        t |> success(true)
    }
}

// ---- struct await (move) ----

[async]
def test_struct_await_impl() : void {
    let d <- await <| make_data(42, "hello")
    assert(d.value == 42)
    assert(d.tag == "hello")
}

[test]
def test_struct_await(t : T?) {
    t |> run("let d <- await struct async") @(t : T?) {
        var it <- test_struct_await_impl()
        async_run(it)
        t |> success(true)
    }
}

// ---- void await ----

[async]
def test_void_await_impl() : void {
    await <| void_delay(3)
}

[test]
def test_void_await(t : T?) {
    t |> run("await void async") @(t : T?) {
        var it <- test_void_await_impl()
        var frames = 0
        for (v in it) {
            frames ++
        }
        // void_delay(3) takes 3 frames, so parent should take >= 3
        t |> success(frames >= 3)
    }
}

// ---- nested await: A awaits B awaits C ----

[async]
def inner_task(x : int) : int {
    await_next_frame()
    yield x * 2
}

[async]
def middle_task(x : int) : int {
    let v <- await <| inner_task(x)
    yield v + 1
}

[async]
def outer_task() : void {
    let v <- await <| middle_task(10)
    assert(v == 21) // 10*2 + 1
}

[test]
def test_nested_await(t : T?) {
    t |> run("three-level nested await") @(t : T?) {
        var it <- outer_task()
        async_run(it)
        t |> success(true)
    }
}

// ---- await in loop ----

[async]
def sum_sequence(n : int) : void {
    var total = 0
    for (i in range(1, n + 1)) {
        let v <- await <| add_one(i - 1) // returns i
        total += v
    }
    assert(total == n * (n + 1) / 2)
}

[test]
def test_await_in_loop(t : T?) {
    t |> run("await inside a for loop") @(t : T?) {
        var it <- sum_sequence(5)
        async_run(it)
        t |> success(true)
    }
}

// ---- chained await ----

[async]
def chained_computation() : void {
    var x = 0
    x = await <| add_one(0)   // 1
    x = await <| add_one(x)   // 2
    x = await <| add_one(x)   // 3
    assert(x == 3)
}

[test]
def test_chained_await(t : T?) {
    t |> run("sequential await calls chain results") @(t : T?) {
        var it <- chained_computation()
        async_run(it)
        t |> success(true)
    }
}
