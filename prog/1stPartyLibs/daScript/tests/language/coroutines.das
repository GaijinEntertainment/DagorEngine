options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/coroutines

// Typed coroutine that yields int values
[coroutine]
def count_up(n : int) : int {
    for (i in range(1, n + 1)) {
        yield i
    }
    return false
}

// Typed coroutine that counts down
[coroutine]
def count_down(n : int) : int {
    var i = n
    while (i > 0) {
        yield i
        i --
    }
    return false
}

[test]
def test_typed_coroutine(t : T?) {
    t |> run("count_up via for loop") @(t : T?) {
        var vals : array<int>
        for (v in count_up(3)) {
            vals |> push(v)
        }
        t |> equal(length(vals), 3)
        t |> equal(vals[0], 1)
        t |> equal(vals[1], 2)
        t |> equal(vals[2], 3)
    }
    t |> run("count_down via for loop") @(t : T?) {
        var vals : array<int>
        for (v in count_down(3)) {
            vals |> push(v)
        }
        t |> equal(length(vals), 3)
        t |> equal(vals[0], 3)
        t |> equal(vals[1], 2)
        t |> equal(vals[2], 1)
    }
    t |> run("empty coroutine") @(t : T?) {
        var vals : array<int>
        for (v in count_up(0)) {
            vals |> push(v)
        }
        t |> equal(length(vals), 0)
    }
}

[coroutine]
def yielder() : int {
    yield 10
    yield 20
    yield 30
    return false
}

[test]
def test_yielder(t : T?) {
    t |> run("yielder produces exact values") @(t : T?) {
        var vals : array<int>
        for (v in yielder()) {
            vals |> push(v)
        }
        t |> equal(length(vals), 3)
        t |> equal(vals[0], 10)
        t |> equal(vals[1], 20)
        t |> equal(vals[2], 30)
    }
}

// Void coroutine for cr_run test
var g_step_count : int = 0

[coroutine]
def stepper(n : int) {
    for (i in range(n)) {
        g_step_count ++
        co_continue()
    }
}

[test]
def test_cr_run(t : T?) {
    t |> run("cr_run drives void coroutine to completion") @(t : T?) {
        g_step_count = 0
        var c <- stepper(5)
        cr_run(c)
        t |> equal(g_step_count, 5)
    }
}

// cr_run_all test
var g_a_count : int = 0
var g_b_count : int = 0

[coroutine]
def worker_a() {
    g_a_count ++
    co_continue()
    g_a_count ++
}

[coroutine]
def worker_b() {
    g_b_count ++
    co_continue()
    g_b_count ++
    co_continue()
    g_b_count ++
}

[test]
def test_cr_run_all(t : T?) {
    t |> run("cr_run_all runs multiple coroutines") @(t : T?) {
        g_a_count = 0
        g_b_count = 0
        var tasks : Coroutines
        tasks |> emplace <| worker_a()
        tasks |> emplace <| worker_b()
        cr_run_all(tasks)
        t |> equal(g_a_count, 2)
        t |> equal(g_b_count, 3)
    }
}

// Sub-generator for yeild_from
[coroutine]
def sub_gen() : int {
    yield 100
    yield 200
    return false
}

[coroutine]
def parent_gen() : int {
    yield 1
    yeild_from <| sub_gen()
    yield 2
    return false
}

[test]
def test_yeild_from(t : T?) {
    t |> run("yeild_from delegates to sub-generator") @(t : T?) {
        var vals : array<int>
        for (v in parent_gen()) {
            vals |> push(v)
        }
        t |> equal(length(vals), 4)
        t |> equal(vals[0], 1)
        t |> equal(vals[1], 100)
        t |> equal(vals[2], 200)
        t |> equal(vals[3], 2)
    }
}

// co_await test using void coroutines and globals
var g_main_log : array<string>

[coroutine]
def sub_task() {
    g_main_log |> push("sub1")
    co_continue()
    g_main_log |> push("sub2")
}

[coroutine]
def main_task() {
    g_main_log |> push("main_start")
    co_await <| sub_task()
    g_main_log |> push("main_end")
}

[test]
def test_co_await(t : T?) {
    t |> run("co_await waits for sub-coroutine") @(t : T?) {
        g_main_log |> clear()
        var c <- main_task()
        cr_run(c)
        t |> equal(length(g_main_log), 4)
        t |> equal(g_main_log[0], "main_start")
        t |> equal(g_main_log[1], "sub1")
        t |> equal(g_main_log[2], "sub2")
        t |> equal(g_main_log[3], "main_end")
    }
}
